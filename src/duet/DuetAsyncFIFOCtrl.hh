#ifndef __DUET_ASYNC_FIFO_CTRL_HH
#define __DUET_ASYNC_FIFO_CTRL_HH

#include "params/DuetAsyncFIFOCtrl.hh"
#include "sim/clocked_object.hh"
#include "mem/packet.hh"

namespace gem5 {
namespace duet {

class DuetAsyncFIFO;
class DuetAsyncFIFOCtrl : public ClockedObject {
private:
    bool            _is_upstream;
    DuetAsyncFIFO * _owner;

    unsigned        _downward_wptr;
    unsigned        _downward_rptr;
    unsigned        _upward_wptr;
    unsigned        _upward_rptr;

    bool            _is_peer_waiting_for_retry;
    Cycles          _last_pkt_recvd_on_cycle;

    bool            _is_this_waiting_for_retry;
    Cycles          _last_pkt_sent_on_cycle;

    EventFunctionWrapper    _e_try_try_send;

public:
    EventFunctionWrapper    e_sync_rptr;
    EventFunctionWrapper    e_sync_wptr;

private:
    void _sync_rptr ();
    void _sync_wptr ();
    void _try_try_send ();  // before we know if we are waiting for retry

public:
    DuetAsyncFIFOCtrl ( const DuetAsyncFIFOCtrlParams & p );
    bool recv ( PacketPtr pkt );
    void try_send ();       // only if we are not waiting for retry, or we are asked to retry
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_ASYNC_FIFO_CTRL_HH */
