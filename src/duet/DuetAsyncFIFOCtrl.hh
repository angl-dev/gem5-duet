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

    public:
        unsigned            push;   // number of credits returned to the push side
        unsigned            pop;    // number of credits returned to the pop side

    public:
        SyncEvent (
                DuetAsyncFIFOCtrl * ctrl
                , unsigned          push = 0
                , unsigned          pop = 0
                )
            : Event     ( Default_Pri, AutoDelete )
            , _ctrl     ( ctrl )
            , push      ( push )
            , pop       ( pop )
        {}

        void process () override;
    };

    bool            _is_upstream;
    bool            _is_snooping;
    DuetAsyncFIFO * _owner;

    int             _push;  // push credit
    int             _pop;   // pop credit

    bool            _is_peer_waiting_for_retry;
    bool            _is_this_waiting_for_retry;

    //  for upstream, the peer is waiting for a snoop resp retry
    //  for downstream, this object is waiting for a snoop resp retry
    bool            _is_waiting_for_snoop_retry;

    Cycles          _can_recv_pkt_on_and_after_cycle;
    Cycles          _can_send_pkt_on_and_after_cycle;


    EventFunctionWrapper            _e_try_try_send;
    std::map <Tick, SyncEvent *>    _sync_events;

private:
    void _sync ( unsigned push, unsigned pop );
    void _try_try_send ();  // before we know if we are waiting for retry
    void _schedule_sync ( Tick t, unsigned push = 0, unsigned pop = 0 );

public:
    DuetAsyncFIFOCtrl ( const DuetAsyncFIFOCtrlParams & p );
    bool recv ( PacketPtr pkt, bool is_snoop = false );
    bool recv_snoop ( PacketPtr pkt );
    // only if we are not waiting for retry, or we are asked to retry
    void try_send (
            bool    snoop   = false
            , bool  or_else = false
            );

};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_ASYNC_FIFO_CTRL_HH */
