#include "duet/DuetSRIFE.hh"
#include "debug/DuetSRI.hh"
#include "base/trace.hh"

namespace gem5
{

AddrRangeList DuetSRIFE::InPort::getAddrRanges () const
{
    AddrRangeList retList;
    DuetSRIFE * _owner = dynamic_cast<DuetSRIFE *> (&owner);
    retList.push_back ( _owner->_range );
    return retList;
}

bool DuetSRIFE::InPort::recvTimingReq ( PacketPtr pkt )
{
    DuetSRIFE * _owner = dynamic_cast<DuetSRIFE *> (&owner);

    if ( !_owner->handleTiming ( pkt ) ) {
        _is_upstream_waiting_for_retry = true;
        return false;
    } else {
        return true;
    }
}

void DuetSRIFE::InPort::recvRespRetry ()
{
    DuetSRIFE * _owner = dynamic_cast<DuetSRIFE *> (&owner);
    _owner->trySendResp ();
}

DuetSRIFE::DuetSRIFE ( const DuetSRIFEParams & p )
    : ClockedObject     ( p )
      , _range          ( p.range )
      , _inport         ( p.name + ".inport", this )
      , _raw_data       ( new uint8_t [p.range.size()] )
      , _pending_resp   ( nullptr )
      , _event          ( [this]{ trySendResp(); }, name() )
{
}

DuetSRIFE::~DuetSRIFE ()
{
    delete _raw_data;
}

void DuetSRIFE::handleFunctional ( PacketPtr pkt )
{
    Addr addr       = pkt->getAddr ();

    panic_if ( !pkt->getAddrRange().isSubset(_range),
            "Access %s out of bound %s",
            pkt->getAddrRange().to_string(), _range.to_string() );

    if ( pkt->isRead() )
    {
        pkt->setData ( _raw_data + (addr - _range.start()) );
    }
    else if ( pkt->isWrite() )
    {
        pkt->writeData ( _raw_data + (addr - _range.start()) );
    }

    pkt->makeResponse ();
}

bool DuetSRIFE::handleTiming ( PacketPtr pkt )
{
    if ( nullptr != _pending_resp ) {
        DPRINTF ( DuetSRI, "Req (%s) blocked due to pending Resp (%s)",
                pkt->print(), _pending_resp->print() );
        return false;
    }

    handleFunctional ( pkt );
    _pending_resp = pkt;

    schedule ( _event, clockEdge ( Cycles(1) ) );
    DPRINTF ( DuetSRI, "Resp (%s) scheduled",
            pkt->print() );

    return true;
}

void DuetSRIFE::trySendResp ()
{
    assert ( nullptr != _pending_resp );

    if ( _inport.sendTimingResp (_pending_resp) ) {
        DPRINTF ( DuetSRI, "Resp (%s) sent",
                _pending_resp->print() );
        _pending_resp = nullptr;
    } else {
        DPRINTF ( DuetSRI, "Resp (%s) blocked",
                _pending_resp->print() );
    }
}

Port & DuetSRIFE::getPort ( const std::string & if_name
        , PortID idx )
{
    panic_if ( idx != InvalidPortID,
            "DuetSRIFE does not support vector ports." );

    if ( if_name == "inport" )
        return _inport;
    else
        return ClockedObject::getPort ( if_name, idx );
}

void DuetSRIFE::init ()
{
    ClockedObject::init ();

    if ( _inport.isConnected() )
    {
        _inport.sendRangeChange ();
    }
}

}
