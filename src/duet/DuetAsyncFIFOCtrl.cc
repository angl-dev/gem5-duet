#include "duet/DuetAsyncFIFOCtrl.hh"
#include "duet/DuetAsyncFIFO.hh"
#include "debug/DuetAsyncFIFO.hh"
#include "base/trace.hh"

namespace gem5 {
namespace duet {

void DuetAsyncFIFOCtrl::SyncEvent::process () {
    if ( _is_rptr )
        _ctrl->_sync_rptr ( _incr );
    else
        _ctrl->_sync_wptr ( _incr );
}

void DuetAsyncFIFOCtrl::SyncEvent::incr () {
    _incr += 1;

    panic_if ( _incr > _ctrl->_owner->_capacity,
            "DuetAsyncFIFOCtrl (%s) %s incr > FIFO capacity",
            _ctrl->name(), _is_rptr ? "rptr" : "wptr" );
}

void DuetAsyncFIFOCtrl::_sync_rptr ( unsigned incr ) {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );
    _rsync_events.erase ( curTick() );

    auto & ptr = _is_upstream ? _downward_rptr : _upward_rptr;
    const auto next = (ptr + incr) % (_owner->_capacity << 1);

    DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) %s rptr %u->%u\n",
            _owner->name(),
            _is_upstream ? "upside/downward" : "downside/upward",
            ptr,
            next
            );

    ptr = next;

    if ( curCycle() >= _can_recv_pkt_on_and_after_cycle
            && _is_peer_waiting_for_retry ) {
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) sent %s retry\n",
                _owner->name(),
                _is_upstream ? "upstream" : "downstream"
                );

        _is_peer_waiting_for_retry = false;

        if ( _is_upstream ) {
            _owner->_upstream_port.sendRetryReq ();
        } else {
            _owner->_downstream_port.sendRetryResp ();
        }
    }
}

void DuetAsyncFIFOCtrl::_sync_wptr ( unsigned incr ) {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );
    _wsync_events.erase ( curTick() );

    auto & ptr = _is_upstream ? _upward_wptr : _downward_wptr;
    const auto next = (ptr + incr) % (_owner->_capacity << 1);

    DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) %s wptr %u->%u\n",
            _owner->name(),
            _is_upstream ? "upside/upward" : "downside/downward",
            ptr,
            next
            );

    ptr = next;
    _try_try_send ();
}

void DuetAsyncFIFOCtrl::_try_try_send () {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );

    // 1. check if we are waiting to retry
    if ( _is_this_waiting_for_retry ) {
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) not trying to send %s b/c: waiting to retry\n",
                _owner->name(),
                _is_upstream ? "upstream" : "downstream"
                );
        return;
    }

    // 2. check if we can send a pkt in this cycle
    if ( curCycle() < _can_send_pkt_on_and_after_cycle ) {
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) not trying to send %s b/c: one per cycle\n",
                _owner->name(),
                _is_upstream ? "upstream" : "downstream"
                );
        return;
    }

    // 3. make sure we do not underflow
    bool underflow = _is_upstream ? ( _upward_rptr == _upward_wptr )
        : ( _downward_rptr == _downward_wptr );
    panic_if ( underflow,
            "DuetAsyncFIFO (%s) %s underflow",
            _owner->name(),
            _is_upstream ? "upside" : "downside"
            );

    // 4. fall through to try_send
    try_send ();
}

void DuetAsyncFIFOCtrl::_schedule_incr_rptr ( Tick t ) {
    auto it = _rsync_events.find ( t );
    if ( _rsync_events.end() == it ) {
        auto event = new SyncEvent ( this, true );
        schedule ( event, t );
        _rsync_events.emplace ( t, event );
    } else {
        it->second->incr ();
    }
}

void DuetAsyncFIFOCtrl::_schedule_incr_wptr ( Tick t ) {
    auto it = _wsync_events.find ( t );
    if ( _wsync_events.end() == it ) {
        auto event = new SyncEvent ( this, false );
        schedule ( event, t );
        _wsync_events.emplace ( t, event );
    } else {
        it->second->incr ();
    }
}

DuetAsyncFIFOCtrl::DuetAsyncFIFOCtrl ( const DuetAsyncFIFOCtrlParams & p )
    : ClockedObject ( p )
    , _is_upstream ( p.is_upstream )
    , _owner ( nullptr )
    , _downward_wptr ( 0 )
    , _downward_rptr ( 0 )
    , _upward_wptr ( 0 )
    , _upward_rptr ( 0 )
    , _is_peer_waiting_for_retry ( false )
    , _can_recv_pkt_on_and_after_cycle ( 0 )
    , _is_this_waiting_for_retry ( false )
    , _can_send_pkt_on_and_after_cycle ( 0 )
    , _e_try_try_send ( [this]{ _try_try_send(); }, name() )
{}

bool DuetAsyncFIFOCtrl::recv (
        PacketPtr pkt
        ) {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );

    // 1. check if we can accept a packet in this cycle
    if ( curCycle() < _can_recv_pkt_on_and_after_cycle ) {
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) blocked %s (%s) b/c: one per cycle\n",
                _owner->name(),
                _is_upstream ? "upstream REQ" : "downstream RESP",
                pkt->print() );
        _is_peer_waiting_for_retry = true;
        return false;
    }

    // 2. check if FIFO is full
    auto & wptr = _is_upstream ? _downward_wptr : _upward_wptr;
    const auto & rptr = _is_upstream ? _downward_rptr : _upward_rptr;

    if ( (rptr + _owner->_capacity) % (_owner->_capacity << 1) == wptr ) {
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) blocked %s (%s) b/c: FIFO full\n",
                _owner->name(),
                _is_upstream ? "upstream REQ" : "downstream RESP",
                pkt->print()
                );
        _is_peer_waiting_for_retry = true;
        return false;
    }

    // 3. accept this packet
    DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) enqueued %s (%s)\n",
            _owner->name(),
            _is_upstream ? "upstream REQ" : "downstream RESP",
            pkt->print()
            );

    // 3.1 push into FIFO
    auto & fifo = _is_upstream ? _owner->_downward_fifo : _owner->_upward_fifo;
    fifo [wptr % _owner->_capacity] = pkt;

    // 3.2 update metadata inside myself
    _can_recv_pkt_on_and_after_cycle = curCycle() + Cycles(1);
    const auto next = (wptr + 1) % (_owner->_capacity << 1);
    DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) %s wptr %u->%u\n",
            _owner->name(),
            _is_upstream ? "upside/downward" : "downside/upward",
            wptr,
            next
            );
    wptr = next;

    // 3.3 sync wptr to the other side of the FIFO
    auto other = _is_upstream ? _owner->_downstream_ctrl : _owner->_upstream_ctrl;
    other->_schedule_incr_wptr ( other->clockEdge ( Cycles( _owner->_stage + 1 ) ) );

    return true;
}

void DuetAsyncFIFOCtrl::try_send () {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );

    // 1. peek pkt
    const auto & fifo = _is_upstream ? _owner->_upward_fifo : _owner->_downward_fifo;
    const auto idx = ( _is_upstream ? _upward_rptr : _downward_rptr ) % _owner->_capacity;
    auto pkt = fifo[idx];

    // 2. try to send it
    bool success = false;
    if ( _is_upstream ) {
        success = _owner->_upstream_port.sendTimingResp ( pkt );
    } else {
        success = _owner->_downstream_port.sendTimingReq ( pkt );
    }

    // 3. check result
    if ( !success ) {
        // 3A. fail
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) is blocked: %s (%s)\n",
                _owner->name(),
                _is_upstream ? "upstream RESP" : "downstream REQ",
                pkt->print()
                );
        _is_this_waiting_for_retry = true;
    } else {
        // 3B. success
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) sent %s (%s)\n",
                _owner->name(),
                _is_upstream ? "upstream RESP" : "downstream REQ",
                pkt->print()
                );

        // 2B.1 update metadata on myself
        _is_this_waiting_for_retry = false;
        _can_send_pkt_on_and_after_cycle = curCycle() + Cycles(1);
        auto & rptr = _is_upstream ? _upward_rptr : _downward_rptr;
        const auto next = (rptr + 1) % (_owner->_capacity << 1);
        DPRINTF ( DuetAsyncFIFO, "DuetAsyncFIFO (%s) %s rptr %u->%u\n",
                _owner->name(),
                _is_upstream ? "upside/upward" : "downside/downward",
                rptr,
                next
                );
        rptr = next;

        // 2B.2 if there are more packet to send, schedule one for the next cycle
        const auto & wptr = _is_upstream ? _upward_wptr : _downward_wptr;
        if ( rptr != wptr )
            schedule ( _e_try_try_send, clockEdge ( Cycles(1) ) );

        // 2B.3 sync rptr to the other side of the FIFO
        auto other = _is_upstream ? _owner->_downstream_ctrl : _owner->_upstream_ctrl;
        other->_schedule_incr_rptr ( other->clockEdge ( Cycles( _owner->_stage + 1 ) ) );
    }
}

}   // namespace duet
}   // namespace gem5
