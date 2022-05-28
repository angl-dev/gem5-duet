#include "duet/DuetClockedObject.hh"

namespace gem5 {
namespace duet {

bool DuetClockedObject::UpstreamPort::recvTimingReq ( PacketPtr pkt ) {
    // wake up owner if it is sleeping
    if ( _owner->_is_sleeping ) {
        panic_if ( nullptr == _req_buf, "Owner sleeping with buffered request!\n" );

        _req_buf = pkt;
        _owner->_wakeup ();
        return true;
    }

    // block until process phase is done
    else if ( _owner->_is_process_phase ()
            || nullptr != _req_buf )
    {
        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        _req_buf = pkt;
        return true;
    }
}

void DuetClockedObject::UpstreamPort::recvRespRetry () {
    panic_if ( !_is_this_waiting_for_retry, "Not waiting for retry!\n" );

    // postpone until the process phase is done
    if ( _owner->_is_process_phase () )
        _is_this_waiting_for_retry = false;

    else
        _try_send_resp ();
}

void DuetClockedObject::UpstreamPort::_try_send_resp () {
    panic_if ( nullptr == _resp_buf, "No response to send!\n" );

    if ( sendTimingResp ( _resp_buf ) ) {
        // success!
        _resp_buf                   = nullptr;
        _is_this_waiting_for_retry  = false;
    } else {
        _is_this_waiting_for_retry  = true;
    }
}

bool DuetClockedObject::DownstreamPort::recvTimingResp ( PacketPtr pkt ) {
    // block until the process phase is done
    if ( _owner->_is_process_phase ()
            || nullptr != _resp_buf )
    {
        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        _resp_buf = pkt;
        return true;
    }
}

void DuetClockedObject::DownstreamPort::recvReqRetry () {
    panic_if ( !_is_this_waiting_for_retry, "Not waiting for retry!\n" );

    // postpone until the process phase is done
    if ( _owner->_is_process_phase () )
        _is_this_waiting_for_retry = false;

    else
        _try_send_req ();
}

void DuetClockedObject::DownstreamPort::_try_send_req () {
    panic_if ( nullptr == _req_buf, "No request to send!\n" );

    if ( sendTimingReq ( _req_buf ) ) {
        // success!
        _req_buf                    = nullptr;
        _is_this_waiting_for_retry  = false;
    } else {
        _is_this_waiting_for_retry  = true;
    }
}

void DuetClockedObject::_do_cycle () {
    // process phase
    _process ();

    // increment cycle
    ++_latest_cycle_plus1;

    // exchange phase 
    _exchange ();

    // schedule the next cycle if there is work to do
    if ( _has_work () )
        schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    else
        _is_sleeping = true;
}

void DuetClockedObject::_wakeup () {
    assert ( _is_sleeping );
    schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    _latest_cycle_plus1 = curCycle() + Cycles(1);
    _is_sleeping = false;
}

}   // namespace duet
}   // namespace gem5
