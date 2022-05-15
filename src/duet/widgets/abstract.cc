#include "duet/widgets/abstract.hh"

namespace gem5 {
namespace duet {

using namespace widget_types;

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::parse_req_header (
              const mem_req_header_t &  unparsed
            ,       mem_req_header_t &  parsed
            )
{
    parsed = unparsed;
}

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::unparse_req_header (
                    mem_req_header_t &  unparsed
            , const mem_req_header_t &  parsed
            )
{
    unparsed = parsed;
}

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::set_blocker (
            const stage_id_t &          stage
            , const latency_t &         latency
            , const stage_blocker_t &   blocker
            )
{
    _stage      = stage;
    _latency    = latency;
    _blocker    = blocker;
}

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::enqueue_req_header (
            std::list<mem_req_header_t> &   chan_req_header
            , const mem_req_header_t &      header
            )
{
    chan_req_header.push_back ( header );

    // transfer control back to the main thread
    _yield_lock.unlock ();
    _yield_cv->notify_one ();

    // and wait until the main thread let us resume
    _wakeup_lock.lock ();
    _wakeup_cv->wait ( _wakeup_lock ); 
}

template <unsigned wlg2>
using bytepack = typename std::array<uint8_t, (1<<wlg2)>;

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::enqueue_req_data (
            std::list<bytepack<Q>> &    chan_req_data
            , const bytepack<Q> &       data
{
    chan_req_data.push_back ( data );

    // transfer control back to the main thread
    _yield_lock.unlock ();
    _yield_cv->notify_one ();

    // and wait until the main thread let us resume
    _wakeup_lock.lock ();
    _wakeup_cv->wait ( _wakeup_lock ); 
}

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::dequeue_resp_data (
            std::list<bytepack<R>> &    chan_resp_data
            , bytepack<R> &             data
            )
{
    resp = chan_resp_data.front ();
    chan_resp_data.pop_front ();

    // transfer control back to the main thread
    _yield_lock.unlock ();
    _yield_cv->notify_one ();

    // and wait until the main thread let us resume
    _wakeup_lock.lock ();
    _wakeup_cv->wait ( _wakeup_lock ); 
}

template <unsigned R, unsigned Q>
void AbstractDuetWidget <R, Q>
    ::spawn (
            const addr_t &                      ptr_arg
            , std::list <mem_req_header_t> &    chan_req_header
            , std::list <bytepack<Q>> &         chan_req_data
            , std::list <bytepack<R>> &         chan_resp_data
            )
{
    // wait until awaken
    _wakeup_lock.lock ();
    _wakeup_cv->wait ( _wakeup_lock );

    // start kernel
    kernel ( ptr_arg, chan_req_header, chan_req_data, chan_resp_data );

    // set finished, then notify the main thread
    _finished = true;
    _yield_lock.unlock ();
    _yield_cv->notify_one ();
}

template <unsigned R, unsigned Q>
bool AbstractDuetWidget <R, Q>
    ::step ()
{
    // transfer control to the execution thread
    _wakeup_lock.unlock ();
    _wakeup_cv->notify_one ();

    // wait until the execution thread let us continue
    _yield_lock.lock ();
    _yield_cv->wait ( _yield_lock );

    // return status
    return _finished;
}

}   // namespace duet
}   // namespace gem5
