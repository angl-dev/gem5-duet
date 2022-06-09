#include "duet/engine/DuetReorderBuffer.hh"

namespace gem5 {
namespace duet {

bool DuetReorderBuffer::Entry::match ( PacketPtr resp ) {

    // trivial match
    if ( resp == pkt || resp->req == req )
        return true;

    assert (  req->hasPaddr () );
    assert ( !req->hasInstCount () );
    assert (  req->hasSize () );
    assert ( !req->isMasked () );
    assert ( !req->hasAtomicOpFunctor () );
    assert ( !req->hasVaddr () );
    assert ( !req->hasPC () );

    assert (  resp->req->hasPaddr () );
    assert ( !resp->req->hasInstCount () );
    assert (  resp->req->hasSize () );
    assert ( !resp->req->isMasked () );
    assert ( !resp->req->hasAtomicOpFunctor () );
    assert ( !resp->req->hasVaddr () );
    assert ( !resp->req->hasPC () );

    // check a bunch of meta data
    if ( resp->req->getArchFlags ()  != req->getArchFlags ()
        || resp->req->getPaddr ()    != req->getPaddr ()
        || resp->req->requestorId () != req->requestorId () )
        return false;

    // match command
    return cmd.responseCommand () == resp->cmd.responseCommand ();
}

void DuetReorderBuffer::update () {
    // 1. if the downstream req buffer is empty, try to send one request
    if ( nullptr == downstream.req_buf ) {
        for ( auto & entry : _buffer ) {
            if ( Entry::UNSENT == entry.status ) {
                downstream.req_buf = entry.pkt;
                entry.status = Entry::SENT;
                break;
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
        if ( Entry::RESPONDED == entry.status ) {
            upstream.resp_buf = entry.pkt;
            _buffer.pop_front ();
        }
    }

    // 4. if the downstream resp buffer is not empty, update our list
    if ( nullptr != downstream.resp_buf ) {

        auto pkt = downstream.resp_buf;
        downstream.resp_buf = nullptr;

        bool found = false;
        for ( auto & entry : _buffer ) {
            if ( entry.match ( pkt ) ) {
                assert ( Entry::SENT == entry.status );
                found = true;
                entry.status = Entry::RESPONDED;
                entry.pkt = pkt;
                break;
            }
        }

        assert ( found );
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
