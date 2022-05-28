#ifndef __DUET_WIDGET_HH
#define __DUET_WIDGET_HH

#include <vector>
#include <list>
#include <utility>

#include "duet/widget/functor_abstract.hh"
#include "params/DuetWidget.hh"
#include "sim/clocked_object.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "mem/port.hh"

namespace gem5 {

class System;
class Process;

namespace duet {

/*
 * Each implemented functor must subclass this class.
 *
 * Key design notes:
 *  1. For consistency, we simulate each cycle in a rigorous order:
 *     -- got port traffic before `do_cycle` in curCycle()
 *      a. block any incoming requests/responses (and remember that they are
 *         blocked); remember any retry notifications, but do not retry
 *         immediately
 *
 *     -- running `do_cycle`
 *      b. if SRI request buffer contains a load (join), and SRI response buffer
 *         is unused: make response and store in SRI response buffer
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
class DuetWidget : public ClockedObject {
// ===========================================================================
// == Type Definitions =======================================================
// ===========================================================================
private:

    typedef uint32_t        tid_t;

    /* Keeping track of the execution of a functor. */ 
    typedef struct _Execution {
        tid_t                                       caller;
        Cycles                                      remaining;
        std::unique_ptr<AbstractDuetWidgetFunctor>  functor;

        _Execution (
                tid_t       caller
                , Cycles    remaining
                , AbstractDuetWidgetFunctor * functor = nullptr
                )
            : caller    ( caller )
            , remaining ( remaining )
            , functor   ( functor )
        {}
    } Execution;

    /* Non-coherent, device-slave port */
    class SRIPort : public ResponsePort {
    public:
        DuetWidget    * _owner;
        PacketPtr       _req_buf;
        PacketPtr       _resp_buf;
        bool            _is_peer_waiting_for_retry;
        bool            _is_this_waiting_for_retry;

        /* Method definitions */
    public:
        SRIPort ( const std::string & name
                , DuetWidget * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort  ( name, owner, id )
            , _owner        ( owner )
            , _req_buf      ( nullptr )
            , _resp_buf     ( nullptr )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
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

        void _try_send_resp ();
    };

    /* Memory port */
    class MemoryPort : public RequestPort {
    public:
        DuetWidget    * _owner;
        PacketPtr       _req_buf;
        PacketPtr       _resp_buf;
        bool            _is_peer_waiting_for_retry;
        bool            _is_this_waiting_for_retry;

    /* Method definitions */
    public:
        MemoryPort ( const std::string & name
                , DuetWidget * owner
                , PortID id = InvalidPortID
                )
            : RequestPort   ( name, owner, id )
            , _owner        ( owner )
            , _req_buf      ( nullptr )
            , _resp_buf     ( nullptr )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
        {}

        void recvRangeChange () override {};

    /* Method declarations */
    public:
        bool recvTimingResp ( PacketPtr pkt ) override;
        void recvReqRetry () override;

        void _try_send_req ();
    };

// ===========================================================================
// == Parameterized Member Variables =========================================
// ===========================================================================
private:
    System                * _system;
    Process               * _process;   // to access the page table
    SRIPort                 _sri_port;
    MemoryPort              _mem_port;

    unsigned                _fifo_capacity;
    AddrRange               _range;
    std::vector <Cycles>    _latency_per_stage;
    std::vector <Cycles>    _interval_per_stage;

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
private:
    // we need a requestor ID to be able to send out memory requests
    RequestorID             _requestorId;

    // event scheduling and handling
    //      use _latest_cycle_plus1 to differentiate pre- and post- "do_cycle" phases
    //      - pre-"do_cycle" phase:   curTick() >= _latest_cycle_plus1
    //      - post-"do_cycle" phase:  curTick() < _latest_cycle_plus1
    Cycles                  _latest_cycle_plus1;
    bool                    _is_sleeping;   // sleep when there is nothing to do
    EventFunctionWrapper    _e_do_cycle;

    // FIFOs
    std::list <std::pair <tid_t, uintptr_t>>    _invocation_fifo;
    std::vector <std::list <uint64_t>>          _retcode_fifo_vec;

    std::unique_ptr <AbstractDuetWidgetFunctor::chan_req_header_t[]> _chan_req_header;
    std::unique_ptr <AbstractDuetWidgetFunctor::chan_req_data_t[]>   _chan_req_data;
    std::unique_ptr <AbstractDuetWidgetFunctor::chan_resp_data_t[]>  _chan_resp_data;
    std::vector <std::unique_ptr< std::list <Execution>>> _exec_list_per_stage;

private:
    Cycles _get_latency  ( int stage ) const;
    Cycles _get_interval ( int stage ) const;

    // move e to stage
    //  - return true if success
    //  - e.functor will be reset to nullptr if success (functor ownership
    //    transferred to the newly-constructed execution)
    bool _move_to_stage ( Execution & e );

    // do our stuff in this cycle
    void _do_cycle ();
    bool _is_pre_do_cycle () const { return curCycle() >= _latest_cycle_plus1; }

    // components of (and may be called by the ports) `_do_cycle`
    tid_t _get_tid_from_addr ( Addr addr ) const;
    PacketPtr _pop_mem_req ();
    void _advance ();
    bool _invoke ();

    // methods related to event scheduling
    void _wakeup ();
    bool _has_work () const;

protected:
    virtual AbstractDuetWidgetFunctor * _new_functor () { return nullptr; }
    virtual size_t _get_chan_count () const { return 1; }

public:
    DuetWidget ( const DuetWidgetParams & p );
    Port & getPort ( const std::string & if_name
            , PortID idx = InvalidPortID ) override;
    void init () override;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_WIDGET_HH */
