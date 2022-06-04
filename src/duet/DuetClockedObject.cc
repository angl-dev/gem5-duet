#include "duet/DuetClockedObject.hh"

namespace gem5 {
namespace duet {

bool DuetClockedObject::UpstreamPort::recvTimingReq ( PacketPtr pkt ) {
    // wake up owner if it is sleeping
    if ( owner->is_sleeping () ) {
        panic_if ( nullptr != req_buf, "Owner sleeping with buffered request!\n" );

        req_buf = pkt;
        owner->wakeup ();
        return true;
    }

    // block until update phase is done
    else if ( owner->is_update_phase ()
            || nullptr != req_buf )
    {
        is_peer_waiting_for_retry = true;
        return false;
    } else {
        req_buf = pkt;
        return true;
    }
}

void DuetClockedObject::UpstreamPort::recvRespRetry () {
    panic_if ( !is_this_waiting_for_retry, "Not waiting for retry!\n" );

    // postpone until the update phase is done
    if ( owner->is_update_phase () )
        is_this_waiting_for_retry = false;

    else
        try_send_resp ();
}

void DuetClockedObject::UpstreamPort::try_send_resp () {
    panic_if ( nullptr == resp_buf, "No response to send!\n" );

    if ( sendTimingResp ( resp_buf ) ) {
        // success!
        resp_buf                    = nullptr;
        is_this_waiting_for_retry   = false;
    } else {
        is_this_waiting_for_retry   = true;
    }
}

void DuetClockedObject::UpstreamPort::exchange () {
    if ( nullptr != resp_buf
            && !is_this_waiting_for_retry )
        try_send_resp ();

    if ( nullptr == req_buf
            && is_peer_waiting_for_retry )
    {
        is_peer_waiting_for_retry = false;
        sendRetryReq ();
    }
}

bool DuetClockedObject::DownstreamPort::recvTimingResp ( PacketPtr pkt ) {
    // block until the update phase is done
    if ( owner->is_update_phase ()
            || nullptr != resp_buf )
    {
        is_peer_waiting_for_retry = true;
        return false;
    } else {
        resp_buf = pkt;
        return true;
    }
}

void DuetClockedObject::DownstreamPort::recvReqRetry () {
    panic_if ( !is_this_waiting_for_retry, "Not waiting for retry!\n" );

    // postpone until the update phase is done
    if ( owner->is_update_phase () )
        is_this_waiting_for_retry = false;

    else
        try_send_req ();
}

void DuetClockedObject::DownstreamPort::try_send_req () {
    panic_if ( nullptr == req_buf, "No request to send!\n" );

    if ( sendTimingReq ( req_buf ) ) {
        // success!
        req_buf                     = nullptr;
        is_this_waiting_for_retry   = false;
    } else {
        is_this_waiting_for_retry   = true;
    }
}

void DuetClockedObject::DownstreamPort::exchange () {
    if ( nullptr != req_buf
            && !is_this_waiting_for_retry )
        try_send_req ();

    if ( nullptr == resp_buf
            && is_peer_waiting_for_retry )
    {
        is_peer_waiting_for_retry = false;
        sendRetryResp ();
    }
}

void DuetClockedObject::_do_cycle () {
    // update phase
    update ();

    // increment cycle
    ++_latest_cycle_plus1;

    // exchange phase 
    exchange ();

    // schedule the next cycle if there is work to do
    if ( has_work () )
        schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    else
        _is_sleeping = true;
}

void DuetClockedObject::wakeup () {
    assert ( _is_sleeping );
    schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    _latest_cycle_plus1 = curCycle() + Cycles(1);
    _is_sleeping = false;
}

}   // namespace duet
}   // namespace gem5
