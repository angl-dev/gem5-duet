#ifndef __DUET_ASYNC_FIFO_CTRL_HH
#define __DUET_ASYNC_FIFO_CTRL_HH

#include <map>

#include "params/DuetAsyncFIFOCtrl.hh"
#include "sim/eventq.hh"
#include "sim/clocked_object.hh"
#include "mem/packet.hh"

namespace gem5 {
namespace duet {

class DuetAsyncFIFO;
class DuetAsyncFIFOCtrl : public ClockedObject {
private:

    // befriend my intended parent class
    friend class DuetAsyncFIFO;

    // custom events
    class SyncEvent : public Event {
    private:
        DuetAsyncFIFOCtrl * _ctrl;
        bool                _is_rptr;
        unsigned            _incr;

    public:
        SyncEvent (
                DuetAsyncFIFOCtrl * ctrl
                , bool              is_rptr
                , unsigned          incr = 1
                )
            : Event     ( Default_Pri, AutoDelete )
            , _ctrl     ( ctrl )
            , _is_rptr  ( is_rptr )
            , _incr     ( incr )
        {}

        void process () override;
        void incr ();
    };

    bool            _is_upstream;
    DuetAsyncFIFO * _owner;

    unsigned        _downward_wptr;
    unsigned        _downward_rptr;
    unsigned        _upward_wptr;
    unsigned        _upward_rptr;

    bool            _is_peer_waiting_for_retry;
    Cycles          _can_recv_pkt_on_and_after_cycle;

    bool            _is_this_waiting_for_retry;
    Cycles          _can_send_pkt_on_and_after_cycle;

    EventFunctionWrapper            _e_try_try_send;
    std::map <Tick, SyncEvent *>    _rsync_events;
    std::map <Tick, SyncEvent *>    _wsync_events;

private:
    void _sync_rptr ( unsigned incr );
    void _sync_wptr ( unsigned incr );
    void _try_try_send ();  // before we know if we are waiting for retry
    void _schedule_incr_rptr ( Tick t );
    void _schedule_incr_wptr ( Tick T );

public:
    DuetAsyncFIFOCtrl ( const DuetAsyncFIFOCtrlParams & p );
    bool recv ( PacketPtr pkt );
    void try_send ();       // only if we are not waiting for retry, or we are asked to retry
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_ASYNC_FIFO_CTRL_HH */
