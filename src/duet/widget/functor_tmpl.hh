#ifndef __DUET_WIDGET_FUNCTOR_TMPL_HH
#define __DUET_WIDGET_FUNCTOR_TMPL_HH

// ===========================================================================
// == Included Header Files ==================================================
// ===========================================================================

#ifdef __DUET_HLS

    #include <ac_int.h>
    #include <ac_channel.h>
    #include <mc_scverify.h>
    #include "functor_abstract.hh"

#else /* #ifdef __DUET_HLS */

    #include <thread>
    #include <mutex>
    #include <condition_variable>
    #include "duet/widget/functor_abstract.hh"
    
    namespace gem5 {
    namespace duet {

#endif /* #ifdef __DUET_HLS */

// ===========================================================================
// == Fuctor Class Template ==================================================
// ===========================================================================
template <
    unsigned req_bytes_lg2      = 3
    , unsigned resp_bytes_lg2   = 3
#ifdef __DUET_HLS
    , size_t chan_count       = 1
#endif /* #ifdef __DUET_HLS */
    >
class DuetWidgetFunctorTmpl : public AbstractDuetWidgetFunctor {
public:

    static constexpr unsigned const req_bytes  = 1 << req_bytes_lg2;
    static constexpr unsigned const resp_bytes = 1 << resp_bytes_lg2;

#ifndef __DUET_HLS
    // GEM5 version
    // interface types
    typedef uintptr_t               addr_t;
    typedef retcode_enum_t          retcode_t;

    // kernel
    virtual void kernel (
              addr_t                arg
            , chan_req_header_t *   chan_req_header
            , chan_req_data_t *     chan_req_data
            , chan_resp_data_t *    chan_resp_data
            , retcode_t *           retcode
            ) = 0;

#else /* #ifndef __DUET_HLS */
    // interface types
    typedef ac_int <80, false>              mem_req_header_t;
    typedef ac_int <64, false>              addr_t;
    typedef ac_int <64, false>              retcode_t;

    // data types
    typedef ac_int <resp_bytes, false>      resp_data_t;
    typedef ac_int <req_bytes,  false>      req_data_t;

    // channel types
    typedef ac_channel <mem_req_header_t>   chan_req_header_t;
    typedef ac_channel <resp_data_t>        chan_resp_data_t;
    typedef ac_channel <req_data_t>         chan_req_data_t;

    #pragma hls_design top
    virtual void kernel (
              addr_t            arg
            , chan_req_header_t chan_req_header [chan_count]
            , chan_req_data_t   chan_req_data   [chan_count]
            , chan_resp_data_t  chan_resp_data  [chan_count]
            , retcode_t *       retcode
            ) = 0;
#endif /* #ifndef __DUET_HLS */

protected:
    // Kernel API
    //  GEM5: called in the widget execution thread
    void enqueue_req_header (
                    int                 stage
            ,       chan_req_header_t & chan_req_header
            , const mem_req_header_t &  header
            );

    void enqueue_req_data (
                    int                 stage
            ,       chan_req_data_t &   chan_req_data
            , const req_data_t &        data
            );

    void dequeue_resp_data (
                    int                 stage
            ,       chan_resp_data_t &  chan_resp_data
            ,       resp_data_t &       data
            );

#ifndef __DUET_HLS
    // GEM5-specific impl
private:
    int                     _stage;
    retcode_enum_t          _retcode;
    chan_req_header_t *     _blocking_chan_req_header;
    chan_req_data_t *       _blocking_chan_req_data;
    chan_resp_data_t *      _blocking_chan_resp_data;

    std::mutex              _mutex;
    std::condition_variable _cv;
    std::thread             _thread;

public:
    DuetWidgetFunctorTmpl ();

    // called by the main thread to invoke `kernel` in a separate thread
    void invoke (
              addr_t                arg
            , chan_req_header_t *   chan_req_header
            , chan_req_data_t *     chan_req_data
            , chan_resp_data_t *    chan_resp_data
            ) override final;

    // called by the main thread to advance `kernel`
    retcode_enum_t advance () override final;

    // accessors
    int             get_stage ()     const override final { return _stage; }
    chan_req_header_t * get_blocking_chan_req_header () const override final;
    chan_req_data_t *   get_blocking_chan_req_data   () const override final;
    chan_resp_data_t *  get_blocking_chan_resp_data  () const override final;

#endif /* #ifndef __DUET_HLS */
};

#ifndef __DUET_HLS

    #include "duet/widget/functor_impl.hh"

    }   // namespace gem5 
    }   // namespace duet 
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_WIDGET_FUNCTOR_TMPL_HH */
