#include "duet/engine/DuetReorderBuffer.hh"

namespace gem5 {
namespace duet {

void DuetReorderBuffer::update () {
    // 1. if the downstream req buffer is empty, try to send one request
    if ( nullptr == downstream.req_buf ) {
        for ( auto it = _buffer.begin (); _buffer.end () != it; ++it ) {
            if ( Entry::UNSENT == it->status ) {
                downstream.req_buf = it->pkt;

                if ( MemCmd::WriteClean == it->pkt->cmd ) {
                    it = _buffer.erase ( it );
                    break;
                } else {
                    it->status = Entry::SENT;
                    break;
                }
            }
        }
    }

    // 2. if upstream req buffer has a valid req, and the reorder buffer is
    //      not full: accept request
    if ( nullptr != upstream.req_buf
            && ( 0 == _capacity || _buffer.size() < _capacity ) )
    {
        auto pkt = upstream.req_buf;
        _buffer.emplace_back ( Entry::UNSENT, pkt );
        upstream.req_buf = nullptr;
    }

    // 3. if the upstream resp buffer is empty, try to send one response
    if ( nullptr == upstream.resp_buf
            && !_buffer.empty () )
    {
        auto & entry = _buffer.front ();
        if ( Entry::RESPONDED == entry.status
                && curTick () >= entry.readyAfter )
        {
            upstream.resp_buf = entry.pkt;
            _buffer.pop_front ();
        }
    }

    // 4. if the downstream resp buffer is not empty, update our list
    if ( nullptr != downstream.resp_buf ) {

        auto pkt = downstream.resp_buf;
        downstream.resp_buf = nullptr;

        for ( auto & entry : _buffer ) {
            if ( pkt->req == entry.req ) {

                assert ( Entry::SENT == entry.status );
                entry.status = Entry::RESPONDED;
                entry.pkt = pkt;
                entry.readyAfter = clockEdge ( Cycles(1) )
                    + pkt->headerDelay + pkt->payloadDelay;
                return;
            }
        }

        assert ( false );
    }
}

void DuetReorderBuffer::exchange () {
    upstream.exchange ();
    downstream.exchange ();
}

bool DuetReorderBuffer::has_work () {
    if ( upstream.req_buf || upstream.resp_buf )
        return true;

    if ( downstream.req_buf || downstream.resp_buf )
        return true;

    return !_buffer.empty ();
}

DuetReorderBuffer::DuetReorderBuffer ( const DuetReorderBufferParams & p )
    : DuetClockedObject     ( p )
    , _capacity             ( p.capacity )
    , upstream              ( p.name + ".upstream", this )
    , downstream            ( p.name + ".downstream", this )
{}

Port & DuetReorderBuffer::getPort ( const std::string & if_name , PortID idx )
{
    if ( if_name == "upstream" )
        return upstream;

    else if ( if_name == "downstream" )
        return downstream;

    else
        return SimObject::getPort ( if_name, idx );
}

}   // namespace duet
}   // namespace gem5
