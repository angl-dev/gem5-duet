#ifndef __DUET_WIDGET_FUNCTOR_ABSTRACT_HH
#define __DUET_WIDGET_FUNCTOR_ABSTRACT_HH

#ifndef __DUET_HLS
    #include <stdint.h>
    #include <list>
    #include <memory>

    namespace gem5 {
    namespace duet {
#endif /* #ifndef __DUET_HLS */

class AbstractDuetWidgetFunctor {
public:
    // types that are visible to both GEM5 and HLS:
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
        , BLOCKER_REQ_HEADER
        , BLOCKER_REQ_DATA
        , BLOCKER_RESP_DATA
    } stage_blocker_t;

    AbstractDuetWidgetFunctor () = delete;
    
#ifndef __DUET_HLS
    // types that are only visible to GEM5
    typedef uintptr_t      typedef struct _mem_req_header_t {
        mem_req_type_t  type;
        uint8_t         size_lg2;   // log ( number of bytes, 2 )
        uintptr_t       addr;
    } mem_req_header_t;

    typedef std::shared_ptr <uint8_t>       resp_data_t;
    typedef std::shared_ptr <uint8_t>       req_data_t;

    typedef std::list <mem_req_header_t>    chan_req_header_t;
    typedef std::list <req_data_t>          chan_req_data_t;
    typedef std::list <resp_data_t>         chan_resp_data_t;

    // == API ==============================================================
    // called by the main thread to invoke `kernel` in a separate thread
    virtual void invoke ( const uintptr_t & arg )   = 0;

    // called by the main thread to advance `kernel`
    //  return true if finished
    virtual bool advance () = 0;

    // accessors
    virtual int                 get_stage ()            const = 0;
    virtual stage_blocker_t     get_blocker ()          const = 0;
    virtual chan_req_header_t & get_blocking_chan_req_header () = 0;
    virtual chan_req_data_t &   get_blocking_chan_req_data   () = 0;
    virtual chan_resp_data_t &  get_blocking_chan_resp_data  () = 0;

#endif /* #ifndef __DUET_HLS */
};

#ifndef __DUET_HLS
    }   // namespace duet
    }   // namespace gem5
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_WIDGET_FUNCTOR_ABSTRACT_HH */
