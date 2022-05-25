#ifndef __DUET_PIPELINE_HH
#define __DUET_PIPELINE_HH

#include "params/DuetPipeline.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include "mem/packet.hh"

namespace gem5 {
namespace duet {

class DuetPipeline : public ClockedObject {
private:

    /* Port to upstream */
    class UpstreamPort : public ResponsePort {
    public:
        DuetPipeline  * _owner;
        bool            _is_peer_waiting_for_retry;
        bool            _is_this_waiting_for_retry;
        PacketPtr       _req_buf;
        PacketPtr       _resp_buf;

    public:
        UpstreamPort ( const std::string & name
                , DuetPipeline * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort                  ( name, owner, id )
            , _owner                        ( owner )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
            , _req_buf                      ( nullptr )
            , _resp_buf                     ( nullptr )
        {}

        AddrRangeList getAddrRanges () const override {
            return _owner->_downstream_port.getAddrRanges ();
        }

        Tick recvAtomic ( PacketPtr pkt ) override {
            panic ( "recvAtomic unimpl." );
        }

        void recvFunctional ( PacketPtr pkt ) override {
            _owner->_downstream_port.sendFunctional ( pkt );
        }

    public:
        void _try_send_resp ();
        bool recvTimingReq ( PacketPtr pkt ) override;
        void recvRespRetry () override;
    };

    /* Port to downstream */
    class DownstreamPort : public RequestPort {
    public:
        DuetPipeline * _owner;
        bool            _is_peer_waiting_for_retry;
        bool            _is_this_waiting_for_retry;
        PacketPtr       _req_buf;
        PacketPtr       _resp_buf;

    public:
        DownstreamPort ( const std::string & name
                , DuetPipeline * owner
                , PortID id = InvalidPortID
                )
            : RequestPort                   ( name, owner, id )
            , _owner                        ( owner )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
            , _req_buf                      ( nullptr )
            , _resp_buf                     ( nullptr )
        {}

        void recvRangeChange () override {
            _owner->_upstream_port.sendRangeChange ();
        }

    public:
        void _try_send_req ();

    public:
        bool recvTimingResp ( PacketPtr pkt ) override;
        void recvReqRetry () override;
    };

private:
    DownstreamPort                              _downstream_port;
    std::list <std::pair <Cycles, PacketPtr>>   _downward_stages;
    Cycles                                      _downward_latency;
    Cycles                                      _downward_interval;

    UpstreamPort                                _upstream_port;
    std::list <std::pair <Cycles, PacketPtr>>   _upward_stages;
    Cycles                                      _upward_latency;
    Cycles                                      _upward_interval;

    // sleep when there is nothing to do
    EventFunctionWrapper                        _e_do_cycle;
    Cycles                                      _latest_cycle_plus1;
    bool                                        _is_sleeping;

private:
    bool _is_pre_do_cycle () const { return curCycle() >= _latest_cycle_plus1; }

private:
    void _do_cycle ();
    void _wakeup ();
    bool _has_work ();

public:
    DuetPipeline ( const DuetPipelineParams & p );

    Port & getPort ( const std::string & if_name
            , PortID id = InvalidPortID ) override
    {
        panic_if ( id != InvalidPortID,
                "DuetPipeline does not support vector ports." );

        if ( if_name == "upstream" )
            return _upstream_port;
        else if ( if_name == "downstream" )
            return _downstream_port;
        else
            return ClockedObject::getPort ( if_name, id );
    }
}; 

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_PIPELINE_HH */
