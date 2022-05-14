#ifndef __DUET_WIDGET_MANAGER_HH
#define __DUET_WIDGET_MANAGER_HH

#include "params/DuetWidgetManager.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"

namespace gem5 {
namespace duet {

class DuetWidget;
class DuetWidgetExecution;

/*
 * DuetWidgetManager
 *
 * Key design notes:
 *  1. For consistency, we simulate each cycle in a rigorous order:
 *     -- got port traffic before `do_cycle` in curCycle()
 *      a. block any incoming requests (and remember that they are blocked);
 *         remember any retry notifications, but do not retry immediately
 *
 *     -- running `do_cycle`
 *      b. if SRI request buffer contains a load (join), and SRI response buffer
 *         is unused: based on the status of the retcode FIFO, return retcode or
 *         BUSY
 *      c. if memory request buffer is unused, and memory request FIFO is not
 *         empty, pop one request from FIFO into buffer.
 *      d. advance as many execution stages as possible. access request/response
 *         FIFOs normally
 *      e. if invocation FIFO is not empty, and new executions can be initiated,
 *         pop one invocation from FIFO and start a new execution
 *      f. if SRI request buffer contains a store (invocation), and invocation
 *         FIFO is not full, and SRI response buffer is unused: process the
 *         request, push invocation FIFO, store response in buffer
 *      g. send out memory request / SRI response if possible
 *      h. send out retry notifications if necessary
 *
 *    -- got port traffic after `do_cycle` in curCycle()
 *      i. retry according to retry notifications
 *      j. buffer SRI request; push memory response into response FIFO
 */
class DuetWidgetManager : public ClockedObject {
// ===========================================================================
// == Type Definitions =======================================================
// ===========================================================================
private:
    /* Non-coherent, device-slave port */
    class SRIPort : public ResponsePort {
    private:
        DuetWidgetManager * _owner;

    /* Method definitions */
    public:
        SRIPort ( const std::string & name
                , DuetWidgetManager * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort ( name, owner, id )
            , _owner ( owner )
        {}

        Tick recvAtomic ( PacketPtr pkt ) override {
            panic ( "recvAtomic unimpl." );
        }

        void recvFunctional ( PacketPtr pkt ) override {
            panic ( "recvFunctional unimpl." );
        }

    /* Method declarations */
    public:
        AddrRangeList getAddrRanges () const override;
        bool recvTimingReq ( PacketPtr pkt ) override;
        void recvRespRetry () override;
    };

    /* Memory port */
    class MemoryPort : public RequestPort {
    private:
        DuetWidgetManager * _owner;

    /* Method definitions */
    public:
        MemoryPort ( const std::string & name
                , DuetWidgetManager * owner
                , PortID id = InvalidPortID
                )
            : RequestPort ( name, owner, id )
            , _owner ( owner )
        {}

    protected:
        void recvRangeChange () override {};

    /* Method declarations */
    public:
        bool recvTiminResp ( PacketPtr pkt ) override;
        void recvReqRetry () override;
    };

    typedef uint64_t                tid_t;
    typedef std::list<uintptr_t>    sri_fifo_t;
    typedef std::vector<sri_fifo_t> sri_fifo_vec_t;
    typedef std::list<PacketPtr>    pkt_fifo_t;

// ===========================================================================
// == Parameterized Member Variables =========================================
// ===========================================================================
private:
    unsigned        _fifo_capacity;
    AddrRange       _range;     // address range of this manager
    SRIPort         _sri_port;  // SRI port
    MemoryPort      _mem_port;  // memory port
    DuetWidget    * _widget;    // duet widget

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
private:
    // use _latest_cycle_plus1 to differentiate pre- and post- "do_cycle" phases
    //  pre-"do_cycle" phase:   curTick() >= _latest_cycle_plus1
    //  post-"do_cycle" phase:  curTick() < _latest_cycle_plus1
    Cycles          _latest_cycle_plus1;
    bool            _is_sleeping;   // in sleeping mode since there is nothing to do

    // key event: we run this every cycle
    EventFunctionWrapper    _e_do_cycle;

    // buffer SRI request, response and memory request
    PacketPtr       _sri_req_buf;
    PacketPtr       _sri_resp_buf;
    PacketPtr       _mem_req_buf;

    // remember if our peer ports are waiting for retries
    bool            _is_sri_peer_waiting_for_retry;
    bool            _is_mem_peer_waiting_for_retry;

    // remember if we are waiting for retries
    bool            _is_sri_this_waiting_for_retry;
    bool            _is_mem_this_waiting_for_retry;

    // FIFOs
    sri_fifo_vec_t  _invocation_fifo_vec;   // TID -> [ARG]
    sri_fifo_vec_t  _retcode_fifo_vec;
    pkt_fifo_t      _mem_req_fifo;
    pkt_fifo_t      _mem_resp_fifo;

    // executions
    std::vector < uint64_t, std::list <
        std::unique_ptr <DuetWidgetExecution>
        > >         _executions;    // stage -> [executions]

// ===========================================================================
// == Private Method Declarations  ===========================================
// ===========================================================================
private:
    // key method: "process" this cycle
    void _do_cycle ();

    // components of (and may be called outside) `do_cycle`
    void _try_send_sri_resp ();
    void _try_send_mem_req ();

    // methods related to event scheduling
    void _wakeup ();
    bool _has_work ();

    // methods related to FIFOs
    bool        _try_push_invocation ( tid_t tid, uintptr_t arg );
    PacketPtr   _try_pop_mem_req ( PacketPtr pkt );
    uint64_t    _try_pop_retcode ( tid_t tid );

    // methods related to thread IDs
    tid_t       _get_tid_from_addr ( Addr addr );

// ===========================================================================
// == Private Method Definitions  ============================================
// ===========================================================================
private:
    // helper functions about `do_cycle`
    bool _is_pre_do_cycle () const { return curTick() >= _latest_cycle_plus1; }

// ===========================================================================
// == Public Method Declarations =============================================
// ===========================================================================
public:
    DuetWidgetManager ( const DuetWidgetManagerParams & p );
    Port & getPort ( const std::string & if_name
            , PortID idx = InvalidPortID ) override;
    void init () override;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_WIDGET_MANAGER_HH */
