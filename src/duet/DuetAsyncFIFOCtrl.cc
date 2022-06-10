#include "duet/DuetAsyncFIFOCtrl.hh"
#include "duet/DuetAsyncFIFO.hh"
#include "debug/DuetAsyncFIFO.hh"
#include "base/trace.hh"

namespace gem5 {
namespace duet {

void DuetAsyncFIFOCtrl::SyncEvent::process () {
    _ctrl->_sync ( push, pop );
}

void DuetAsyncFIFOCtrl::_sync (
        unsigned    push
        , unsigned  pop
        )
{
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );
    _sync_events.erase ( curTick () );

    DPRINTF ( DuetAsyncFIFO, "SYNC: push (%d -> %d), pop (%d -> %d)\n",
            _push, _push + push, _pop, _pop + pop );
    _push += push;
    _pop += pop;

    if ( _push > 0 && curCycle () >= _can_recv_pkt_on_and_after_cycle ) {
        if ( _is_upstream && _is_waiting_for_snoop_retry ) {
            DPRINTF ( DuetAsyncFIFO, " .. Sent retrySnoopResp\n" );

            _is_waiting_for_snoop_retry = false;
            _owner->_upstream_port.sendRetrySnoopResp ();
        } else if ( _is_peer_waiting_for_retry ) {
            _is_peer_waiting_for_retry = false;

            if ( _is_upstream ) {
                DPRINTF ( DuetAsyncFIFO, " .. Sent retryReq\n" );
                _owner->_upstream_port.sendRetryReq ();
            } else {
                DPRINTF ( DuetAsyncFIFO, " .. Sent retryResp\n" );
                _owner->_downstream_port.sendRetryResp ();
            }
        }
    }

    if ( _pop > 0 )
        _try_try_send ();
}

void DuetAsyncFIFOCtrl::_schedule_sync (
        Tick        t
        , unsigned  push
        , unsigned  pop
        )
{
    auto it = _sync_events.find ( t );
    if ( _sync_events.end () == it ) {
        auto event = new SyncEvent ( this, push, pop );
        schedule ( event, t );
        _sync_events.emplace ( t, event );
    } else {
        it->second->push += push;
        it->second->pop += pop;
    }
}

void DuetAsyncFIFOCtrl::_try_try_send () {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );

    // 1. check if we are waiting for retry
    if ( (!_is_upstream && _is_waiting_for_snoop_retry)
            || _is_this_waiting_for_retry )
    {
        DPRINTF ( DuetAsyncFIFO, "Not trying to send b/c: waiting for retry\n" );
        return;
    }

    // 2. check if we can send in this cycle
    if ( curCycle () < _can_send_pkt_on_and_after_cycle ) {
        DPRINTF ( DuetAsyncFIFO, "Not trying to send b/c: once per cycle\n" );
        if ( !_e_try_try_send.scheduled () )
            schedule ( _e_try_try_send, clockEdge ( Cycles(1) ) );
        return;
    }

    // 3. make sure we do not underflow
    panic_if ( _pop <= 0, "DuetAsyncFIFOCtrl (%s) underflow", name() );

    // fall through to try_send
    try_send ( !_is_upstream, true );
}

DuetAsyncFIFOCtrl::DuetAsyncFIFOCtrl ( const DuetAsyncFIFOCtrlParams & p )
    : ClockedObject                     ( p )
    , _is_upstream                      ( p.is_upstream )
    , _is_snooping                      ( false )
    , _owner                            ( nullptr )
    , _push                             ( 0 )
    , _pop                              ( 0 )
    , _is_peer_waiting_for_retry        ( false )
    , _is_this_waiting_for_retry        ( false )
    , _is_waiting_for_snoop_retry       ( false )
    , _can_recv_pkt_on_and_after_cycle  ( 0 )
    , _can_send_pkt_on_and_after_cycle  ( 0 )
    , _e_try_try_send                   ( [this]{ _try_try_send(); }, name() )
{}

bool DuetAsyncFIFOCtrl::recv (
        PacketPtr   pkt
        , bool      is_snoop
        )
{
    panic_if ( nullptr == _owner,
            "DuetAsyncFIFOCtrl (%s) has no owner", name() );
    panic_if ( is_snoop && !_is_snooping,
            "DuetAsyncFIFOCtrl (%s) is not expecting snoop req/resp",
            name() );
    if ( is_snoop ) {
        assert ( _is_upstream );
        assert ( !_is_waiting_for_snoop_retry );
    } else {
        assert ( !_is_peer_waiting_for_retry );
    }

    // packet type
    std::string type = !_is_upstream ? "RESP" : (is_snoop ? "Snoop RESP" : "REQ");

    // 1. check if we can accept a packet in this cycle
    std::string blocked_reason;
    if ( curCycle() < _can_recv_pkt_on_and_after_cycle )
        blocked_reason = "once per cycle";
    else if ( _push <= 0 )
        blocked_reason = "FIFO full";

    if ( !blocked_reason.empty() ) {
        DPRINTF ( DuetAsyncFIFO, "%s (%s) blocked b/c: %s\n",
                type, pkt->print(), blocked_reason
                );

        if ( is_snoop )
            _is_waiting_for_snoop_retry = true;
        else
            _is_peer_waiting_for_retry = true;

        return false;
    }

    // 3. accept packet
    DPRINTF ( DuetAsyncFIFO, "%s (%s) accepted\n", type, pkt->print() );

    // 3.1 push into FIFO
    auto & fifo = _is_upstream ? _owner->_downward_fifo : _owner->_upward_fifo;
    fifo.push_back ( pkt );

    // 3.2 update metadata inside myself
    --_push;
    _can_recv_pkt_on_and_after_cycle = curCycle() + Cycles(1);

    // 3.3 send credit to the otherside
    auto other = _is_upstream ? _owner->_downstream_ctrl : _owner->_upstream_ctrl;
    other->_schedule_sync (
            other->clockEdge ( Cycles( _owner->_stage + 1 ) ),
            0, 1    // plus one for "pop" on the other side
            );

    return true;
}

bool DuetAsyncFIFOCtrl::recv_snoop (
        PacketPtr pkt
        )
{
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );
    panic_if ( !_is_snooping, "DuetAsyncFIFOCtrl (%s) is not snooping!", name() );

    if ( _is_upstream ) {
        // we can reject SnoopResp
        assert ( pkt->isResponse () );
        return recv ( pkt, true );
    } else {
        // we must accept any incoming snoop request immediately
        assert ( pkt->isRequest () );

        // if it is an express snoop, jump over flow control
        if ( pkt->isExpressSnoop () ) {
            DPRINTF ( DuetAsyncFIFO, "Express Snoop REQ (%s) forwarded\n", pkt->print () );
            _owner->_upstream_port.sendTimingSnoopReq ( pkt );
        } else {
            DPRINTF ( DuetAsyncFIFO, "Snoop REQ (%s) forwarded\n", pkt->print () );
            pkt->headerDelay += clockEdge () - curTick ()   //  latch until the next clock edge
                + clockPeriod ()                            //  one cycle before we push into FIFO
                + (_owner->_upstream_ctrl->clockPeriod ()
                        * (_owner->_stage + 1) );           //  sync to the other side
            _owner->_upstream_port.sendTimingSnoopReq ( pkt );
        }

        return true;
    }
}

void DuetAsyncFIFOCtrl::try_send ( bool snoop, bool or_else ) {
    panic_if ( nullptr == _owner, "DuetAsyncFIFOCtrl (%s) has no owner", name() );

    assert ( !snoop || !_is_upstream );

    // 1. peek pkt
    auto & fifo = _is_upstream ? _owner->_upward_fifo : _owner->_downward_fifo;
    auto pkt = fifo.front ();
    auto str = pkt->print ();   // it might be deleted before we want to print!

    // 2. try to send it
    if ( _is_upstream ) {
        assert ( pkt->isResponse () );
        // if ( pkt->isRequest () ) {  // snoop
        //     DPRINTF ( DuetAsyncFIFO, "Snoop REQ (%s) sent\n", str );
        //     _owner->_upstream_port.sendTimingSnoopReq ( pkt );
        // } else
        if ( _owner->_upstream_port.sendTimingResp ( pkt ) ) {
            DPRINTF ( DuetAsyncFIFO, "RESP (%s) sent\n", str );
            _is_this_waiting_for_retry = false;
        } else {
            _is_this_waiting_for_retry = true;
            return;
        }
    } else if ( pkt->isResponse () ) {
        // snoop RESP
        if ( snoop
                && _owner->_downstream_port.sendTimingSnoopResp ( pkt ) )
        {
            DPRINTF ( DuetAsyncFIFO, "Snoop RESP (%s) sent\n", str );
            _is_waiting_for_snoop_retry = false;
        } else {
            _is_waiting_for_snoop_retry = true;
            return;
        }
    } else {
        // normal request
        if ( (!snoop || or_else)
                && _owner->_downstream_port.sendTimingReq ( pkt ) )
        {
            DPRINTF ( DuetAsyncFIFO, "REQ (%s) sent\n", str );
            _is_this_waiting_for_retry = false;
        } else {
            _is_this_waiting_for_retry = true;
            return;
        }
    }

    // 3. successfully sent. update metadata
    _can_send_pkt_on_and_after_cycle = curCycle() + Cycles(1);
    --_pop; fifo.pop_front ();

    if ( _pop > 0 )
        schedule ( _e_try_try_send, clockEdge ( Cycles(1) ) );

    auto other = _is_upstream ? _owner->_downstream_ctrl : _owner->_upstream_ctrl;
    other->_schedule_sync (
            other->clockEdge ( Cycles( _owner->_stage + 1 ) ),
            1, 0    // plus one for "push" on the other side
            );
}

}   // namespace duet
}   // namespace gem5
