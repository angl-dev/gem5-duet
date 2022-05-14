#include "duet/DuetWidgetManager.hh"
#include <pthread.h>

namespace gem5 {
namespace duet {

AddrRangeList DuetWidgetManager::SRIPort::getAddrRanges () const {
    AddrRangeList ret;
    ret.push_back ( _owner->_range );
    return ret;
}

bool DuetWidgetManager::SRIPort::recvTimingReq ( PacketPtr pkt ) {
    // wake up owner if it is sleeping
    if ( _owner->_is_sleeping ) {
        _owner->_sri_req_buf = pkt;
        _owner->_wakeup ();
        return true;
    }

    // block if we are in pre-do_cycle phase, or if we cannot latch more packets
    else if ( _owner->_is_pre_do_cycle ()
            || nullptr != _owner->_sri_req_buf )
    {
        _owner->_is_sri_peer_waiting_for_retry = true;
        return false;
    } else {
        _owner->_sri_req_buf = pkt;
        return true;
    }
}

void DuetWidgetManager::SRIPort::recvRespRetry () {
    // postpone retry after `do_cycle` in curCycle()
    if ( _owner->_is_pre_do_cycle () )
        _owner->_is_sri_this_waiting_for_retry = false;

    else
        _owner->_try_send_sri_resp ();
}

bool DuetWidgetManager::MemoryPort::recvTimingResp ( PacketPtr pkt ) {
    // block if we are in pre-do_cycle phase, or if we cannot latch more packets
    if ( _owner->_is_pre_do_cycle ()
            || _owner->_mem_resp_fifo.size() == _owner->_fifo_capacity )
    {
        _owner->_is_mem_peer_waiting_for_retry = true;
        return false;
    } else {
        _owner->_mem_resp_fifo.push_back ( pkt );
        return true;
    }
}

void DuetWidgetManager::MemoryPort::recvReqRetry () {
    // postpone retry after `do_cycle` in curCycle()
    if ( _owner->_is_pre_do_cycle () )
        _owner->_is_mem_this_waiting_for_retry = false;

    else
        _owner->_try_send_mem_req ();
}

DuetWidgetManager::DuetWidgetManager (
        const DuetWidgetManagerParams & p
        )
    : ClockedObject                     ( p )
    , _fifo_capacity                    ( p.fifo_capacity )
    , _range                            ( p.range )
    , _sri_port                         ( p.name + ".sri_port", this )
    , _mem_port                         ( p.name + ".mem_port", this )
    , _widget                           ( p.widget )
    , _latest_cycle_plus1               ( 0 )
    , _is_sleeping                      ( true )
    , _e_do_cycle                       ( [this]{ _do_cycle(); }, name() )
    , _sri_req_buf                      ( nullptr )
    , _sri_resp_buf                     ( nullptr )
    , _mem_req_buf                      ( nullptr )
    , _is_sri_peer_waiting_for_retry    ( false )
    , _is_mem_peer_waiting_for_retry    ( false )
    , _is_sri_this_waiting_for_retry    ( false )
    , _is_mem_this_waiting_for_retry    ( false )
{
    // validate "range"
    if ( _range.interleaved() ) {
        panic ( "DuetWidgetManager does not support interleaved address range" );
    } else if ( _range.start() & 0x7 || _range.end() & 0x7 ) {
        panic ( "DuetWidgetManager address range is not aligned to and/or multiples of 8B" );
    }
}

void DuetWidgetManager::_do_cycle () {
    /* b. if SRI request buffer contains a load (join), and SRI response buffer
    *       is unused: based on the status of the retcode FIFO, return retcode
    *       or BUSY
    */
    if ( nullptr != _sri_req_buf
            && nullptr == _sri_resp_buf
            && _sri_req_buf->isRead() )
    {
        tid_t tid = _get_tid_from_addr ( _sri_req_buf->getAddr () );
        uint64_t retcode = _try_pop_retcode ( tid );

        _sri_resp_buf = _sri_req_buf;
        _sri_req_buf = nullptr;
        _sri_resp_buf->setLE ( retcode );
        _sri_resp_buf->makeResponse ();
    }

    /* c. if memory request buffer is unused, and memory request FIFO is not
    *       empty, pop one request from FIFO into buffer.
    */
    if ( nullptr == _mem_req_buf )
        _mem_req_buf = _try_pop_mem_req ();

    /* d. advance as many execution stages as possible. access request/response
    *       FIFOs normally
    */

    /* e. if invocation FIFO is not empty, and new executions can be initiated,
    *       pop one invocation from FIFO and start a new execution
    */

    /* f. if SRI request buffer contains a store (invocation), and invocation
    *       FIFO is not full, and SRI response buffer is unused: process the
    *       request, push invocation FIFO, store response in buffer
    */
    if ( nullptr != _sri_req_buf
            && nullptr == _sri_resp_buf
            && _sri_req_buf->isWrite() )
    {
        tid_t tid = _get_tid_from_addr ( _sri_req_buf->getAddr () );
        uintptr_t arg = _sri_req_buf->getLE<uintptr_t> ();

        if ( _try_push_invocation ( tid, arg ) ) {
            _sri_resp_buf = _sri_req_buf;
            _sri_req_buf = nullptr;
            _sri_resp_buf->makeResponse();
        }
    }

    // g. increment _latest_cycle_plus1
    _latest_cycle_plus1++;

    // h. send out memory request / SRI response if possible
    if ( nullptr != _mem_req_buf
            && !_is_mem_this_waiting_for_retry )
        _try_send_mem_req ();

    if ( nullptr != _sri_resp_buf
            && !_is_sri_this_waiting_for_retry )
        _try_send_sri_resp ();

    // i. send out retry notifications if necessary
    if ( nullptr == _sri_req_buf
            && _is_sri_peer_waiting_for_retry )
        _sri_port->sendRetryReq ();

    if ( _mem_resp_fifo.size() < _fifo_capacity
            && _is_mem_peer_waiting_for_retry )
        _mem_port->sendRetryResp ();

    // XXX. if there are things to do, schedule do_cycle for next cycle;
    // otherwise, go to sleep
    if ( _has_work () )
        schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    else
        _is_sleeping = true;
}

void DuetWidgetManager::_try_send_sri_resp () {
    panic_if ( nullptr == _sri_resp_buf, "No SRI response to send!" );
    if ( _sri_port.sendTimingResp ( _sri_resp_buf ) ) {
        // success!
        _sri_resp_buf = nullptr;
        _is_sri_this_waiting_for_retry = false;
    } else {
        _is_sri_this_waiting_for_retry = true;
    }
}

void DuetWidgetManager::_try_send_mem_req () {
    panic_if ( nullptr == _mem_req_buf, "No memory request to send!" );
    if ( _mem_port.sendTimingReq ( _mem_req_buf ) ) {
        // success!
        _mem_req_buf = nullptr;
        _is_mem_this_waiting_for_retry = false;
    } else {
        _is_mem_this_waiting_for_retry = true;
    }
}

void DuetWidgetManager::_wakeup () {
    // schedule do_cycle for the next cycle
    schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );

    // update _latest_cycle_plus1
    _latest_cycle_plus1 = curTick() + Cycles(1);

    // update my own state
    _is_sleeping = false;
}

bool DuetWidgetManager::_has_work () {

    if ( nullptr != _sri_req_buf
            || nullptr != _mem_req_buf )
        return true;

    else if ( _is_sri_peer_waiting_for_retry
            || _is_mem_peer_waiting_for_retry )
        return true;

    else if ( !_mem_req_fifo.empty()
            || !_mem_resp_fifo.empty() )
        return true;

    for ( const auto & fifo : _invocation_fifo_vec ) {
        if ( !fifo.empty() )
            return true;
    }

    for ( const auto & fifo : _retcode_fifo_vec ) {
        if ( !fifo.empty() )
            return true;
    }

    panic ( "[%s] partial impl. check executions!" );

    return false;
}

bool DuetWidgetManager::_try_push_invocation (
        DuetWidgetManager::tid_t tid,
        uintptr_t arg
        )
{
    if ( tid >= _invocation_fifo_vec.size() ) {
        _invocation_fifo_vec.resize ( tid + 1 );
    }

    if ( _invocation_fifo_vec[tid].size() < _fifo_capacity ) {
        _invocation_fifo_vec[tid].push_back ( arg );
        return true;
    } else {
        return false;
    }
}

PacketPtr DuetWidgetManager::_try_pop_mem_req () {
    if ( !_mem_req_fifo.empty() ) {
        PacketPtr pkt = _mem_resp_fifo.front ();
        _mem_resp_fifo.pop_front ();
        return pkt;
    } else {
        return nullptr;
    }
}

uint64_t DuetWidgetManager::_try_pop_retcode (
        DuetWidgetManager::tid_t tid
        )
{
    if ( tid >= _retcode_fifo_vec.size() ) {
        return 0;   // NO-RETURN-VALUE
    } else if ( _retcode_fifo_vec[tid].empty() ) {
        return 0;   // NO-RETURN-VALUE
    } else {
        uint64_t retcode = _retcode_fifo_vec[tid].front ();
        _retcode_fifo_vec[tid].pop_front ();
        return retcode;
    }
}

DuetWidgetManager::tid_t DuetWidgetManager::_get_tid_from_addr ( Addr addr ) {
    return (addr - _range.start ()) >> 3;
}

void DuetWidgetManager::init () {
    ClockedObject::init ();

    if ( _sri_port.isConnected() ) {
        _sri_port.sendRangeChange ();
    }
}

}   // namespace duet
}   // namespace gem5
