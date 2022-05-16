template <unsigned Q, unsigned R, unsigned N>
DuetWidgetFunctorTmpl <Q, R, N>
    ::DuetWidgetFunctorTmpl ()
        : _stage            ( -1 )
        , _blocker          ( BLOCKER_INV )
        , _blocking_chan_id ( N )
        , _finished         ( false )
{}

template <unsigned Q, unsigned R, unsigned N>
void DuetWidgetFunctorTmpl <Q, R, N>
    ::enqueue_req_header (
            const int &                                             stage
            , AbstractDuetWidgetFunctor::chan_req_header_t &        chan_req_header
            , const AbstractDuetWidgetFunctor::mem_req_header_t &   header
            )
{
    // update our state
    _stage      = stage;
    _blocker    = BLOCKER_REQ_HEADER;

    for ( _blocking_chan_id = 0; _blocking_chan_id < N; _blocking_chan_id++ )
        if ( &_chan_req_header[_blocking_chan_id] == &chan_req_header )
            break;

    panic_if ( _blocking_chan_id >= N,
            "chan_req_header is not in our array" );

    // transfer control back to the main thread
    _cv.notify_one ();

    // wait until the main thread let us resume
    std::unique_lock lock ( _mutex );
    _cv.wait ( lock );

    // resume execution
    chan_req_header.push_back ( header );
}

template <unsigned Q, unsigned R, unsigned N>
void DuetWidgetFunctorTmpl <Q, R, N>
    ::enqueue_req_data (
            const int &                                     stage
            , AbstractDuetWidgetFunctor::chan_req_data_t &  chan_req_data
            , const AbstractDuetWidgetFunctor::req_data_t & data
{
    // update our state
    _stage      = stage;
    _blocker    = BLOCKER_REQ_DATA;

    for ( _blocking_chan_id = 0; _blocking_chan_id < N; _blocking_chan_id++ )
        if ( &_chan_req_data[_blocking_chan_id] == &chan_req_data )
            break;

    panic_if ( _blocking_chan_id >= N,
            "chan_req_data is not in our array" );

    // transfer control back to the main thread
    _cv.notify_one ();

    // wait until the main thread let us resume
    std::unique_lock lock ( _mutex );
    _cv.wait ( lock );

    // resume execution
    chan_req_data.push_back ( data );
}

template <unsigned Q, unsigned R, unsigned N>
void DuetWidgetFunctorTmpl <Q, R, N>
    ::dequeue_resp_data (
            const int &                                     stage
            , AbstractDuetWidgetFunctor::chan_resp_data_t * chan_resp_data
            , AbstractDuetWidgetFunctor::resp_data_t &      data
            )
{
    // update our state
    _stage      = stage;
    _blocker    = BLOCKER_RESP_DATA;

    for ( _blocking_chan_id = 0; _blocking_chan_id < N; _blocking_chan_id++ )
        if ( &_chan_resp_data[_blocking_chan_id] == &chan_resp_data )
            break;

    panic_if ( _blocking_chan_id >= N,
            "chan_resp_data is not in our array" );

    // transfer control back to the main thread
    _cv.notify_one ();

    // wait until the main thread let us resume
    std::unique_lock lock ( _mutex );
    _cv.wait ( lock );

    // resume execution
    data = chan_resp_data.front ();
    chan_resp_data.pop_front ();
}

template <unsigned Q, unsigned R, unsigned N>
void DuetWidgetFunctorTmpl <Q, R, N>
    ::invoke ( const uintptr_t & arg )
{
    _blocker    = BLOCKER_INV;
    _finished   = false;

    // call kernel in a separate thread
    std::thread kt ( [this, arg] {

            // call kernel
            kernel ( arg, _chan_req_header, _chan_req_data, _chan_resp_data );

            // set finished, then notify the main thread
            _finished = true;
            _cv.notify_one ();
            } );

    // wait until the first stage is complete
    std::unique_lock lock ( _mutex );
    _cv.wait ( lock );

    // keep the thread handle
    std::swap ( _thread, kt );
}

template <unsigned Q, unsigned R, unsigned N>
bool DuetWidgetFunctorTmpl <Q, R, N>
    ::advance ()
{
    // transfer control to the execution thread
    _cv.notify_one ();

    // wait until the execution thread let us resume
    std::unique_lock lock ( _mutex );
    _cv.wait ( lock );

    if ( _finished ) {
        _thread.join ();
        return true;
    } else {
        return false;
    }
}

template <unsigned Q, unsigned R, unsigned N>
AbstractDuetWidgetFunctor::chan_req_header_t &
DuetWidgetFunctorTmpl <Q, R, N>
    ::get_blocking_chan_req_header ()
{
    return _chan_req_header[_blocking_chan_id];
}

template <unsigned Q, unsigned R, unsigned N>
AbstractDuetWidgetFunctor::chan_req_data_t &
DuetWidgetFunctorTmpl <Q, R, N>
    ::get_blocking_chan_req_data ()
{
    return _chan_req_data[_blocking_chan_id];
}

template <unsigned Q, unsigned R, unsigned N>
AbstractDuetWidgetFunctor::chan_resp_data_t &
DuetWidgetFunctorTmpl <Q, R, N>
    ::get_blocking_chan_resp_data ()
{
    return _chan_resp_data[_blocking_chan_id];
}
