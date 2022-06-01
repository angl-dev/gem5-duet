#ifndef __DUET_FUNCTOR_HH
#define __DUET_FUNCTOR_HH

/*
 * Use ` #define __DUET_HLS ' in HLS environments
 */

#ifdef __DUET_HLS
    #include <stdint.h>
    #include <ac_int.h>
    #include <ac_channel.h>

#else /* #ifdef __DUET_HLS */
    #include <stdint.h>
    #include <string.h>
    #include <list>
    #include <map>
    #include <utility>
    #include <memory>
    #include <thread>
    #include <mutex>
    #include <condition_variable>

    namespace gem5 {
    namespace duet {

class DuetLane;

#endif /* #ifdef __DUET_HLS */

class DuetFunctor {
public:
// ===========================================================================
// == Types that are visible to both GEM5 and HLS ============================
// ===========================================================================
    typedef enum _mem_req_type_t : uint8_t {
        REQTYPE_INV = 0
        , REQTYPE_LD
        , REQTYPE_ST
        , REQTYPE_LR
        , REQTYPE_SC
        , REQTYPE_SWAP
        , REQTYPE_ADD
        , REQTYPE_AND
        , REQTYPE_OR
        , REQTYPE_XOR
        , REQTYPE_MAX
        , REQTYPE_MAXU
        , REQTYPE_MIN
        , REQTYPE_MINU
        , REQTYPE_FENCE
    } mem_req_type_t;

    typedef struct _mem_req_t {
        mem_req_type_t  type;
        uint8_t         size;       // number of bytes
        uintptr_t       addr;       // up to 48bit!
    } mem_req_t;

    typedef enum _retcode_t : uint64_t {
        RETCODE_DEFAULT = uint64_t(-1)  // functor returned without explicitly
                                        // setting the return code. Simlar to
                                        // returning "void"
        , RETCODE_RUNNING = 0           // functor still running
    } retcode_t;

// ===========================================================================
// == API for subclasses (GEM5 & HLS) ========================================
// ===========================================================================
protected:
    template <typename T_data>
    static mem_req_t make_load_req ( uintptr_t addr ) {
        mem_req_t req = { REQTYPE_LD, sizeof (T_data), addr };
        return req;
    }

    template <typename T_data>
    static mem_req_t make_store_req ( uintptr_t addr ) {
        mem_req_t req = { REQTYPE_ST, sizeof (T_data), addr };
        return req;
    }

#ifdef __DUET_HLS
// ===========================================================================
// == Types that are only visible to HLS =====================================
// ===========================================================================
protected:
    typedef ac_int<64, false>               packed_mem_req_t;
    typedef ac_channel<packed_mem_req_t>    chan_req_t;

// ===========================================================================
// == API for subclasses (HLS) ===============================================
// ===========================================================================
protected:
    /*
     * enqueue_req:
     *
     *  Enqueue a memory request to the specified channel
     */
    void enqueue_req (
            chan_req_t &            chan
            , const mem_req_t &     req
            )
    {
        packed_mem_req_t data;
        ac_int <8, false> byte;

        byte = req.type;
        data.set_slc <8> (0, byte);

        byte = req.size;
        data.set_slc <8> (8, byte);

        ac_int <48, false> addr = req.addr;
        data.set_slc <48> (16, addr);

        chan.write ( req );
    }

    /*
     * enqueue_data:
     *
     *  Enqueue a data element to the specified channel
     */
    template <typename T_chan, typename T_data>
    void enqueue_data (
            ac_channel<T_chan> &    chan
            , const T_data &        data
            )
    {
        static constexpr size_t const bitwidth = sizeof(T_data) * 8;
        ac_int <bitwidth, false> raw = data;

        T_chan packed;

        // replicate data
        for ( int i = 0; i < T_chan::width; i += bitwidth ) {
            packed.set_slc <bitwidth> ( i, raw );
        }

        chan.write ( packed );
    }

    /*
     * dequeue_data:
     *
     *  Dequeue a data element from the specified channel
     */
    template <typename T_chan, typename T_data>
    void dequeue_data (
            ac_channel<T_chan> &    chan
            , T_data &              data
            )
    {
        static constexpr size_t const bitwidth = sizeof(T_data) * 8;
        T_chan packed = chan.read ();
        ac_int <bitwidth, false> raw = packed.slc<bitwidth>(0);
        uint64_t plain = raw.to_uint64();
        data = *( reinterpret_cast <T_data*> (&plain) );
    }

#else /* ifdef __DUET_HLS */
public:
// ===========================================================================
// == Types that are only visible to GEM5 ====================================
// ===========================================================================
    typedef std::shared_ptr<uint8_t[]>  raw_data_t;
    typedef uint16_t                    caller_id_t;

    typedef struct _chan_id_t {
        enum : uint8_t {
            INVALID = 0,            // invalid tag
            REQ, WDATA, RDATA,      // memory channels
            ARG, RET,               // ABI channels
            PULL, PUSH              // inter-lane channels
        }                           tag;

        // caller ID for ARG/RET channels, or channel ID for other channels
        caller_id_t                 id;

        // define comparator so this struct can be used as map key
        bool operator< ( const _chan_id_t & other ) const {
            return tag < other.tag || id < other.id;
        }
    } chan_id_t;

    typedef std::list <mem_req_t>   chan_req_t;
    typedef std::list <raw_data_t>  chan_data_t;

    typedef uint32_t    stage_t;

// ===========================================================================
// == Member Variables (GEM5) ================================================
// ===========================================================================
private:
    DuetLane                              * _lane;
    caller_id_t                             _caller_id;
    chan_id_t                               _blocking_chan_id;
    stage_t                                 _stage;
    bool                                    _is_functors_turn;
    bool                                    _is_done;

    std::map <void *, chan_id_t>            _id_by_chan;

    std::mutex                              _mutex;
    std::unique_lock <std::mutex>           _main_lock;
    std::unique_lock <std::mutex>           _functor_lock;
    std::condition_variable                 _cv;
    std::thread                             _thread;

// ===========================================================================
// == API for DuetEngine (GEM5) ==============================================
// ===========================================================================
public:
    /*
     * (Constructor):
     *
     *  Construct and start the functor
     */
    DuetFunctor ( DuetLane * lane, caller_id_t caller_id );

    /*
     * (Destructor):
     *
     *  Virtual destructor!
     */
    virtual ~DuetFunctor() {};

    /*
     * advance:
     *
     *  DuetLane calls this method to resume execution of the functor until the
     *  next blocking operation or finish.
     *
     *  return true if finished
     */
    bool advance ();

    /* [Getter] get_caller_id: */
    caller_id_t get_caller_id () const { return _caller_id; }

    /* [Getter] get_stage: */
    stage_t get_stage () const { return _stage; }

    /* [Getter] get_blocking_chan_id: */
    chan_id_t get_blocking_chan_id () const { return _blocking_chan_id; }

    /* [Getter] is_done */
    bool is_done () const { return _is_done; }

// ===========================================================================
// == API for subclasses (GEM5) ==============================================
// ===========================================================================
protected:
    /*
     * get_chan_req & get_chan_data:
     *
     *  Get or create a channel, remember the channel->id mapping as well
     */
    chan_req_t &  get_chan_req  ( chan_id_t id );
    chan_data_t & get_chan_data ( chan_id_t id );

    // Use preprocessor tricks to automate stage annotation
    #define enqueue_req(chan, req) _enqueue_req    ( __COUNTER__, (chan), (req) )
    #define enqueue_data(chan, data) _enqueue_data ( __COUNTER__, (chan), (data) )
    #define dequeue_data(chan, data) _dequeue_data ( __COUNTER__, (chan), (data) )

    /*
     * enqueue_req:
     *
     *  Enqueue a memory request to the specified channel
     */
    void _enqueue_req (
            stage_t                 stage
            , chan_req_t &          chan
            , const mem_req_t &     req
            )
    {
        // update state
        _stage              = stage;
        _blocking_chan_id   = _id_by_chan [
            reinterpret_cast <void *> (&chan) ];

        // transfer control back to the main thread
        _yield ();

        // resume execution
        chan.push_back ( req );
    }

    /*
     * enqueue_data:
     *
     *  Enqueue a data element to the specified channel
     */
    template <typename T>
    void _enqueue_data (
            stage_t                 stage
            , chan_data_t &         chan
            , const T &             data
            )
    {
        // update state
        _stage              = stage;
        _blocking_chan_id   = _id_by_chan [
            reinterpret_cast <void *> (&chan) ];

        // transfer control back to the main thread
        _yield ();

        // resume execution
        raw_data_t packed ( new uint8_t [sizeof(data)] );
        memcpy ( packed.get(), &data, sizeof(data) );
        chan.push_back ( packed );
    }

    /*
     * dequeue_data:
     *
     *  Dequeue a data element from the specified channel
     */
    template <typename T>
    void _dequeue_data (
            stage_t                 stage
            , chan_data_t &         chan
            , T &                   data
            )
    {
        // update state
        _stage              = stage;
        _blocking_chan_id   = _id_by_chan [
            reinterpret_cast <void *> (&chan) ];

        // transfer control back to the main thread
        _yield ();

        // resume execution
        raw_data_t data_ = chan.front ();
        chan.pop_front ();
        memcpy ( &data, data_.get(), sizeof(T) );
    }

// ===========================================================================
// == Internal API (GEM5) ====================================================
// ===========================================================================
private:
    /*
     * _yield:
     *
     *  Serialize execution between the main and functor thread
     */
    void _yield ( bool wait = true );

// ===========================================================================
// == Virtual Methods (GEM5) =================================================
// ===========================================================================
public:
    /*
     * setup:
     *
     *  Should make use of ` get_chan ' to get or create necessary
     *  channels
     */
    virtual void setup () = 0;

    /*
     * use_default_retcode:
     */
    virtual bool use_default_retcode () const { return false; }

protected:
    /*
     * run:
     *
     *  A wrapper for the HLS synthesizable kernel, which may have drastically
     *  different signatures.
     */
    virtual void run () = 0;

#endif /* #ifdef __DUET_HLS */
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_FUNCTOR_HH */
