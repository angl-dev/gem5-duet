#ifndef __DUET_WIDGET_ABSTRACT_HH
#define __DUET_WIDGET_ABSTRACT_HH

#include <stdint.h>

#ifdef __DUET_HLS

    #include <ac_int.h>
    #include <ac_channel.h>
    #include <mc_scverify.h>

#else /* #ifdef __DUET_HLS */

    #include <array>
    #include <list>
    #include <mutex>
    #include <condition_variable>
    
    namespace gem5 {
    namespace duet {

#endif /* #ifdef __DUET_HLS */

#ifndef __DUET_HLS
    namespace widget_types {
#endif /* #ifndef __DUET_HLS */

typedef uintptr_t   addr_t;
typedef uint64_t    stage_id_t;
typedef uint64_t    latency_t;

typedef enum _mem_req_type_t {
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

typedef enum _stage_blocker_t {
    BLOCKER_INV     = 0
    , BLOCKER_INIT
    , BLOCKER_REQ_HEADER
    , BLOCKER_REQ_DATA
    , BLOCKER_RESP
} stage_blocker_t;

typedef struct _mem_req_header_t {
    mem_req_type_t  type;
    uint8_t         size_lg2;   // log ( number of bytes, 2 )
    addr_t          addr;
} mem_req_header_t;

#ifndef __DUET_HLS
    }   // namespace widget_types
#endif /* #ifndef __DUET_HLS */

template <unsigned resp_bytes_lg2, unsigned req_bytes_lg2 = 3>
class AbstractDuetWidget {
public:

    /* Type definitions */
#ifndef __DUET_HLS
    // interface types
    typedef mem_req_header_t    raw_req_t;
    typedef addr_t              raw_addr_t;

    // data types
    typedef std::array <uint8_t, (1 << resp_bytes_lg2)> resp_data_t;
    typedef std::array <uint8_t, (1 << req_bytes_lg2)>  req_data_t;

    // channel types
    typedef std::list <raw_req_t>       chan_req_header_t;
    typedef std::list <req_data_t>      chan_req_data_t;
    typedef std::list <resp_data_t>     chan_resp_data_t;

    // bring useful types into our namespace
    using namespace widget_types;

#else /* #ifndef __DUET_HLS */
    // interface types
    typedef ac_int <80, false>  raw_req_t;
    typedef ac_int <64, false>  raw_addr_t;

    // data types
    typedef ac_int <(1 << resp_bytes_lg2), false>       resp_data_t;
    typedef ac_int <(1 << req_bytes_lg2),  false>       req_data_t;

    // channel types
    typedef ac_channel <raw_req_t>      chan_req_header_t;
    typedef ac_channel <req_data_t>     chan_req_data_t;
    typedef ac_channel <resp_data_t>    chan_resp_data_t;
#endif /* #ifndef __DUET_HLS */

public:
    // Methods to be implemented per widget
    // called in the widget execution thread
    #pragma hls_design top
    virtual void kernel (
              const raw_addr_t &        ptr_arg
            ,       chan_req_header_t & chan_req_header
            ,       chan_req_data_t &   chan_req_data
            ,       chan_resp_data_t &  chan_resp_data
            ) = 0;

protected:
    // Kernel API (called in the widget execution thread)
    void parse_req_header (
              const raw_req_t &         unparsed
            ,       mem_req_header_t &  parsed
            ) final;

    void unparse_req_header (
                    raw_req_t &         unparsed
            , const mem_req_header_t &  parsed
            ) final;

    void set_blocker (
              const stage_id_t &        stage
            , const latency_t &         latency
            , const stage_blocker_t &   blocker
            ) final;

    void enqueue_req_header (
                    chan_req_header_t & chan_req_header
            , const raw_req_t &         header
            ) final;

    void enqueue_req_data (
                    chan_req_data_t &   chan_req_data
            , const req_data_t &        data
            ) final;

    void dequeue_resp_data (
                    chan_resp_data_t &  chan_resp_data
            ,       resp_data_t &       data
            ) final;

#ifndef __DUET_HLS
    // GEM5-specific impl
private:
    std::unique_lock <std::mutex>   _wakeup_lock;
    std::condition_variable *       _wakeup_cv;

    std::unique_lock <std::mutex>   _yield_lock;
    std::condition_variable *       _yield_cv;

    stage_id_t          _stage;
    latency_t           _latency;
    stage_blocker_t     _blocker;
    bool                _finished;

public:
    AbstractDuetWidget (
            std::mutex &                wakeup_mutex
            , std::condition_variable & wakeup_cv
            , std::mutex &              yield_mutex
            , std::condition_variable & yield_cv
            )
        : _wakeup_lock  ( wakeup_mutex, std::defer_lock )
        , _wakeup_cv    ( &wakeup_cv )
        , _yield_lock   ( yield_mutex, std::defer_lock )
        , _yield_cv     ( &yield_cv )
        , _stage        ( 0 )
        , _latency      ( 0 )
        , _blocker      ( BLOCKER_INIT )
        , _finished     ( false )
    {}

    // this method is supposed to be run as:
    //
    //      std::thread ( &Widget::spawn, &widget, ... )
    void spawn (
              const addr_t &            ptr_arg
            ,       chan_req_header_t & chan_req_header
            ,       chan_req_data_t &   chan_req_data
            ,       chan_resp_data_t &  chan_resp_data
            ) final;

    // step execution of "kernel" -- called by the main thread!
    //  return true if done
    bool step () final;

    // other utilities
    const stage_id_t &      get_stage ()   const final { return _stage; }
    const latency_t &       get_latency () const final { return _latency; }
    const stage_blocker_t & get_blocker () const final { return _blocker; }

#endif /* #ifndef __DUET_HLS */
};

#ifndef __DUET_HLS
    }   // namespace gem5 
    }   // namespace duet 
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_WIDGET_ABSTRACT_HH */
