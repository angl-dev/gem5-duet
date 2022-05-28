#ifndef __DUET_FUNCTOR_HH
#define __DUET_FUNCTOR_HH

/*
 * Use ` #define __DUET_HLS ' in HLS environments
 */

#ifdef __DUET_HLS
    #include <stdint.h>
    #include <ac_channel.h>

#else /* #ifdef __DUET_HLS */
    #include <stdint.h>
    #include <list>
    #include <map>
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

    typedef uint32_t    stage_t;

#ifdef __DUET_HLS
// ===========================================================================
// == API for subclasses (HLS) ===============================================
// ===========================================================================
protected:
    /*
     * enqueue_req:
     *
     *  Enqueue a memory request to the specified channel
     */
    template <typename T>
    void enqueue_req (
            stage_t             stage   // ignored
            , ac_channel<T> &   chan
            , const T &         req
            )
    { chan.write ( req ); }

    /*
     * enqueue_data:
     *
     *  Enqueue a data element to the specified channel
     */
    template <typename T>
    void enqueue_data (
            stage_t             stage   // ignored
            , ac_channel<T> &   chan
            , const T &         data
            )
    { chan.write ( data ); }

    /*
     * dequeue_data:
     *
     *  Dequeue a data element from the specified channel
     */
    template <typename T>
    T dequeue_data (
            stage_t             stage   // ignored
            , ac_channel<T> &   chan
            )
    { return chan.read (); }

#else /* ifdef __DUET_HLS */
public:
// ===========================================================================
// == Types that are only visible to GEM5 ====================================
// ===========================================================================
    typedef shared_ptr<uint8_t[]>   raw_data_t;

    typedef struct _mem_req_t {
        mem_req_type_t  type;
        uint8_t         size_lg2;   // log ( number of bytes, 2 )
        uintptr_t       addr;
    } mem_req_t;

    typedef struct _chan_id_t {
        enum _tag_t : uint8_t {
            REQ = 0, INPUT, OUTPUT,
            INVALID
        };

        uint32_t        id;
        _tag_t          tag;

        _chan_id_t ( uint32_t id, _tag_t tag = REQ )
            : id ( id ), tag ( tag )
        {}
    } chan_id_t;

    typedef uint32_t                caller_id_t;
    typedef std::list <mem_req_t>   chan_req_t;
    typedef std::list <raw_data_t>  chan_data_t;

// ===========================================================================
// == Member Variables (GEM5) ================================================
// ===========================================================================
private:
    DuetLane                              * _lane;
    caller_id_t                             _caller_id;
    stage_t                                 _stage;
    chan_id_t                               _blocking_chan_id;
    bool                                    _is_functors_turn;
    bool                                    _is_done;

    std::map <chan_id_t, void *>            _chan_by_id;
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

    /*
     * enqueue_req:
     *
     *  Enqueue a memory request to the specified channel
     */
    void enqueue_req (
            stage_t                 stage
            , chan_req_t &          chan
            , const mem_req_t &     req
            );

    /*
     * enqueue_data:
     *
     *  Enqueue a data element to the specified channel
     */
    void enqueue_data (
            stage_t                 stage
            , chan_data_t &         chan
            , const raw_data_t &    data
            );

    /*
     * dequeue_data:
     *
     *  Dequeue a data element from the specified channel
     */
    raw_data_t dequeue_data (
            stage_t                 stage
            , chan_data_t &         chan
            );

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

protected:
// ===========================================================================
// == Virtual Methods (GEM5) =================================================
// ===========================================================================
    /*
     * setup:
     *
     *  Should make use of ` get_chan ' to get or create necessary
     *  channels
     */
    virtual void setup () = 0;

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
