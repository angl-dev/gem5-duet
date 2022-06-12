#ifndef __DUET_FUNCTOR_HH
#define __DUET_FUNCTOR_HH

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

    typedef enum _retcode_t : uint64_t {
        RETCODE_DEFAULT = uint64_t(-1)  // functor returned without explicitly
                                        // setting the return code. Simlar to
                                        // returning "void"
        , RETCODE_RUNNING = 0           // functor still running
    } retcode_t;

// ===========================================================================
// == GEM5-specific types ====================================================
// ===========================================================================
protected:
    typedef bool                Bool;
    typedef uint8_t             U8;
    typedef int8_t              S8;
    typedef uint16_t            U16;
    typedef int16_t             S16;
    typedef uint32_t            U32;
    typedef int32_t             S32;
    typedef uint64_t            U64;
    typedef int64_t             S64;
    typedef unsigned int        UInt;
    typedef int                 Int;
    typedef float               Float;
    typedef double              Double;
    typedef uintptr_t           addr_t;

public:
    typedef std::shared_ptr<uint8_t[]>  raw_data_t;
    typedef uint16_t                    caller_id_t;
    typedef uint32_t                    stage_t;

    typedef struct _mem_req_t {
        mem_req_type_t  type;
        size_t          size;       // number of bytes
        addr_t          addr;
    } mem_req_t;

    typedef std::list <mem_req_t>       chan_req_t;
    typedef std::list <raw_data_t>      chan_data_t;

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
        // bool operator< ( const _chan_id_t & other ) const {
        //     return tag < other.tag || id < other.id;
        // }
    } chan_id_t;

// ===========================================================================
// == API for subclasses =====================================================
// ===========================================================================
protected:
    // Use preprocessor tricks to automate stage annotation
    #define enqueue_req(chan, type, size, addr) _enqueue_req  ( __COUNTER__, (chan), (type), (size), (addr) )
    #define enqueue_data(chan, data)            _enqueue_data ( __COUNTER__, (chan), (data) )
    #define dequeue_data(chan, data)            _dequeue_data ( __COUNTER__, (chan), (data) )
    #define dequeue_token(chan)                 _dequeue_token( __COUNTER__, (chan) )

    /* -----------------------------------------------------------------------
     * enqueue_req:
     *  Enqueue a memory request to the specified channel
     * -------------------------------------------------------------------- */
    void _enqueue_req (
            stage_t             stage
            , chan_req_t &      chan
            , mem_req_type_t    type
            , size_t            size
            , addr_t            addr
            )
    {
        // update state
        _stage              = stage;
        _blocking_chan_id   = _id_by_chan [
            reinterpret_cast <void *> (&chan) ];

        // transfer control back to the main thread
        _yield ();

        // resume execution
        mem_req_t req = { type, size, addr };
        chan.push_back ( req );
    }

    /* -----------------------------------------------------------------------
     * enqueue_data:
     *  Enqueue a data element to the specified channel
     * -------------------------------------------------------------------- */
    template <typename T>
    void _enqueue_data (
            stage_t             stage
            , chan_data_t &     chan
            , const T &         data
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

    /* -----------------------------------------------------------------------
     * dequeue_data:
     *  Dequeue a data element from the specified channel
     * -------------------------------------------------------------------- */
    template <typename T>
    void _dequeue_data (
            stage_t             stage
            , chan_data_t &     chan
            , T &               data
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

    /* -----------------------------------------------------------------------
     * dequeue_token:
     *  Dequeue a store ACK from the specified channel
     * -------------------------------------------------------------------- */
    void _dequeue_token (
            stage_t             stage
            , chan_data_t &     chan
            )
    {
        // update state
        _stage              = stage;
        _blocking_chan_id   = _id_by_chan [
            reinterpret_cast <void *> (&chan) ];

        // transfer control back to the main thread
        _yield ();

        // resume execution
        chan.pop_front ();
    }

// ===========================================================================
// == Member Variables (GEM5) ================================================
// ===========================================================================
protected:
    DuetLane                              * lane;
    caller_id_t                             caller_id;

private:
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
    caller_id_t get_caller_id () const { return caller_id; }

    /* [Getter] get_stage: */
    stage_t get_stage () const { return _stage; }

    /* [Getter] get_blocking_chan_id: */
    chan_id_t get_blocking_chan_id () const { return _blocking_chan_id; }

    /* [Getter] is_done */
    bool is_done () const { return _is_done; }

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
    /*
     * get_chan_req & get_chan_data:
     *
     *  Get or create a channel, remember the channel->id mapping as well
     */
    chan_req_t &  get_chan_req  ( chan_id_t id );
    chan_data_t & get_chan_data ( chan_id_t id );

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
    virtual void setup () {};

    /*
     * use_default_retcode:
     */
    virtual bool use_default_retcode () const { return false; }

    /*
     * finishup
     *
     *  set_constant etc.
     */
    virtual void finishup () {};

protected:
    /*
     * run:
     *
     *  A wrapper for the HLS synthesizable kernel, which may have drastically
     *  different signatures.
     */
    virtual void run () = 0;
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_FUNCTOR_HH */
