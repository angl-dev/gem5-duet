template <unsigned Q, unsigned R>
DuetWidgetFunctorTmpl <Q, R>
    ::DuetWidgetFunctorTmpl ()
        : _stage                    ( -1 )
        , _retcode                  ( AbstractDuetWidgetFunctor::RETCODE_RUNNING )
        , _blocking_chan_req_header ( nullptr )
        , _blocking_chan_req_data   ( nullptr )
        , _blocking_chan_resp_data  ( nullptr )
{
    std::unique_lock main_lock ( _mutex, std::defer_lock );
    std::swap ( _main_lock, main_lock );

    std::unique_lock thread_lock ( _mutex, std::defer_lock );
    std::swap ( _thread_lock, thread_lock );
}

template <unsigned Q, unsigned R>
void DuetWidgetFunctorTmpl <Q, R>
    ::enqueue_req_header (
            int                                                     stage
            , AbstractDuetWidgetFunctor::chan_req_header_t &        chan_req_header
            , const AbstractDuetWidgetFunctor::mem_req_header_t &   header
            )
{
    // update our state
    _stage                      = stage;
    _blocking_chan_req_header   = &chan_req_header;
    _blocking_chan_req_data     = nullptr;
    _blocking_chan_resp_data    = nullptr;

    // transfer control back to the main thread
    _is_threads_turn = false;
    _thread_lock.unlock ();
    _cv.notify_one ();

    // wait until the main thread let us resume
    _thread_lock.lock ();
    _cv.wait ( _thread_lock, [&]{ return _is_threads_turn; } );

    // resume execution
    chan_req_header.push_back ( header );
}

template <unsigned Q, unsigned R>
void DuetWidgetFunctorTmpl <Q, R>
    ::enqueue_req_data (
            int                                             stage
            , AbstractDuetWidgetFunctor::chan_req_data_t &  chan_req_data
            , const AbstractDuetWidgetFunctor::req_data_t & data
            )
{
    // update our state
    _stage                      = stage;
    _blocking_chan_req_header   = nullptr;
    _blocking_chan_req_data     = &chan_req_data;
    _blocking_chan_resp_data    = nullptr;

    // transfer control back to the main thread
    _is_threads_turn = false;
    _thread_lock.unlock ();
    _cv.notify_one ();

    // wait until the main thread let us resume
    _thread_lock.lock ();
    _cv.wait ( _thread_lock, [&]{ return _is_threads_turn; } );

    // resume execution
    chan_req_data.push_back ( data );
}

template <unsigned Q, unsigned R>
void DuetWidgetFunctorTmpl <Q, R>
    ::dequeue_resp_data (
            int                                             stage
            , AbstractDuetWidgetFunctor::chan_resp_data_t & chan_resp_data
            , AbstractDuetWidgetFunctor::resp_data_t &      data
            )
{
    // update our state
    _stage      = stage;
    _blocking_chan_req_header   = nullptr;
    _blocking_chan_req_data     = nullptr;
    _blocking_chan_resp_data    = &chan_resp_data;

    // transfer control back to the main thread
    _is_threads_turn = false;
    _thread_lock.unlock ();
    _cv.notify_one ();

    // wait until the main thread let us resume
    _thread_lock.lock ();
    _cv.wait ( _thread_lock, [&]{ return _is_threads_turn; } );

    // resume execution
    data = chan_resp_data.front ();
    chan_resp_data.pop_front ();
}

template <unsigned Q, unsigned R>
void DuetWidgetFunctorTmpl <Q, R>
    ::invoke (
              uintptr_t                                         arg
            , AbstractDuetWidgetFunctor::chan_req_header_t *    chan_req_header
            , AbstractDuetWidgetFunctor::chan_req_data_t *      chan_req_data
            , AbstractDuetWidgetFunctor::chan_resp_data_t *     chan_resp_data
            )
{
    // initialize member variables
    _main_lock.lock ();
    _stage                      = -1;
    _retcode                    = RETCODE_RUNNING;
    _blocking_chan_req_header   = nullptr;
    _blocking_chan_req_data     = nullptr;
    _blocking_chan_resp_data    = nullptr;
    _is_threads_turn            = false;

    // call kernel in a separate thread
    std::thread kt ( [&] {
            // wait until awaken by the main thread
            _thread_lock.lock ();
            _cv.wait ( _thread_lock, [&]{ return _is_threads_turn; } );

            // call kernel
            kernel (
                    arg
                    , chan_req_header
                    , chan_req_data
                    , chan_resp_data
                    , &_retcode
                   );

            if ( RETCODE_RUNNING == _retcode ) {
            _retcode = RETCODE_DEFAULT;
            }

            // notify the main thread
            _is_threads_turn = false;
            _thread_lock.unlock ();
            _cv.notify_one ();
            } );

    // keep the thread handle
    std::swap ( _thread, kt );

    // wake up the thread
    _is_threads_turn = true;
    _main_lock.unlock ();
    _cv.notify_one ();

    // wait until the first stage is complete
    _main_lock.lock ();
    _cv.wait ( _main_lock, [&]{ return !_is_threads_turn; } );
}

template <unsigned Q, unsigned R>
AbstractDuetWidgetFunctor::retcode_enum_t
DuetWidgetFunctorTmpl <Q, R>
    ::advance ()
{
    // transfer control to the execution thread
    _is_threads_turn = true;
    _main_lock.unlock ();
    _cv.notify_one ();

    // wait until the execution thread let us resume
    _main_lock.lock ();
    _cv.wait ( _main_lock, [&]{ return !_is_threads_turn; } );

    if ( RETCODE_RUNNING != _retcode ) {
        _thread.join ();
        return _retcode;
    } else {
        return RETCODE_RUNNING;
    }
}

template <unsigned Q, unsigned R>
AbstractDuetWidgetFunctor::chan_req_header_t *
DuetWidgetFunctorTmpl <Q, R>
    ::get_blocking_chan_req_header () const
{
    return _blocking_chan_req_header;
}

template <unsigned Q, unsigned R>
AbstractDuetWidgetFunctor::chan_req_data_t *
DuetWidgetFunctorTmpl <Q, R>
    ::get_blocking_chan_req_data () const
{
    return _blocking_chan_req_data;
}

template <unsigned Q, unsigned R>
AbstractDuetWidgetFunctor::chan_resp_data_t *
DuetWidgetFunctorTmpl <Q, R>
    ::get_blocking_chan_resp_data () const
{
    return _blocking_chan_resp_data;
}
