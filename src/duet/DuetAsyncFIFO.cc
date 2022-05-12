#include "duet/DuetAsyncFIFO.hh"
#include "duet/DuetAsyncFIFOCtrl.hh"

namespace gem5 {
namespace duet {

bool DuetAsyncFIFO::UpstreamPort::recvTimingReq ( PacketPtr pkt ) {
    return _owner->_upstream_ctrl->recv ( pkt );
}

void DuetAsyncFIFO::UpstreamPort::recvRespRetry () {
    _owner->_upstream_ctrl->try_send ();
}

void DuetAsyncFIFO::UpstreamPort::recvFunctional ( PacketPtr pkt ) {
    _owner->_downstream_port.sendFunctional ( pkt );
}

bool DuetAsyncFIFO::DownstreamPort::recvTimingResp ( PacketPtr pkt ) {
    return _owner->_downstream_ctrl->recv ( pkt );
}

void DuetAsyncFIFO::DownstreamPort::recvReqRetry () {
    _owner->_downstream_ctrl->try_send ();
}

DuetAsyncFIFO::DuetAsyncFIFO ( const DuetAsyncFIFOParams & p )
    : SimObject         ( p )
    , _stage            ( p.stage )
    , _capacity         ( p.capacity )
    , _upstream_port    ( p.name + ".upstream_port", this )
    , _downstream_port  ( p.name + ".downstream_port", this )
    , _upstream_ctrl    ( p.upstream_ctrl )
    , _downstream_ctrl  ( p.downstream_ctrl )
    , _downward_fifo    ( p.capacity, nullptr )
    , _upward_fifo      ( p.capacity, nullptr )
{
    _upstream_ctrl->_owner      = this;
    _downstream_ctrl->_owner    = this;
}

Port & DuetAsyncFIFO::getPort ( const std::string & if_name
        , PortID idx ) {
    panic_if ( idx != InvalidPortID,
            "DuetAsyncFIFO does not support vector ports." );

    if ( if_name == "upstream_port" )
        return _upstream_port;
    else if ( if_name == "downstream_port" )
        return _downstream_port;
    else
        return SimObject::getPort ( if_name, idx );
}

}   // namespace duet
}   // namespace gem5
