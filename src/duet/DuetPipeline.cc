#include "DuetPipeline.hh"
#include "debug/DuetPipeline.hh"
#include "base/trace.hh"

namespace gem5 {
namespace duet {

bool DuetPipeline::UpstreamPort::recvTimingReq ( PacketPtr pkt ) {
    // wake up widget if it is sleeping
    if ( _owner->_is_sleeping ) {
        assert ( nullptr == _req_buf );

        DPRINTF ( DuetPipeline, "DuetPipeline (%s) recvd REQ (%s) while sleeping\n",
                _owner->name(), pkt->print() );

        _req_buf = pkt;
        _owner->_wakeup ();
        return true;
    }

    // block until the cycle is processed
    if ( _owner->_is_pre_do_cycle ()
            || nullptr != _req_buf )
    {
        if (_owner->_is_pre_do_cycle () )
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) blocked REQ (%s): before do_cycle\n",
                    _owner->name(), pkt->print() );
        else
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) blocked REQ (%s): buffer unavail\n",
                    _owner->name(), pkt->print() );

        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) recvd REQ (%s)\n",
                _owner->name(), pkt->print() );

        _req_buf = pkt;
        return true;
    }
}

void DuetPipeline::UpstreamPort::recvRespRetry () {
    assert ( _is_this_waiting_for_retry );

    if ( _owner->_is_pre_do_cycle () ) {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) got RESP retry: before do_cycle\n",
                _owner->name() );
        _is_this_waiting_for_retry = false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) got RESP retry\n",
                _owner->name() );
        _try_send_resp ();
    }
}

void DuetPipeline::UpstreamPort::_try_send_resp () {
    panic_if ( nullptr == _resp_buf, "No response to send!" );
    if ( sendTimingResp ( _resp_buf ) ) {
        // success!
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) sent RESP (%s)\n",
                _owner->name(), _resp_buf->print() );

        _resp_buf = nullptr;
        _is_this_waiting_for_retry = false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot send RESP (%s)\n",
                _owner->name(), _resp_buf->print() );

        _is_this_waiting_for_retry = true;
    }
}

bool DuetPipeline::DownstreamPort::recvTimingResp ( PacketPtr pkt ) {
    // block until the cycle is processed
    if ( _owner->_is_pre_do_cycle ()
            || nullptr != _resp_buf )
    {
        if ( _owner->_is_pre_do_cycle () )
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) blocked RESP (%s): before do_cycle\n",
                    _owner->name(), pkt->print() );
        else
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) blocked RESP (%s): buffer unavail\n",
                    _owner->name(), pkt->print() );

        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) recvd RESP (%s)\n",
                _owner->name(), pkt->print() );

        _resp_buf = pkt;
        return true;
    }
}

void DuetPipeline::DownstreamPort::recvReqRetry () {
    assert ( _is_this_waiting_for_retry );

    // block until the cycle is processed
    if ( _owner->_is_pre_do_cycle () ) {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) got REQ retry: before do_cycle\n",
                _owner->name() );
        _is_this_waiting_for_retry = false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) got REQ retry\n",
                _owner->name() );
        _try_send_req ();
    }
}

void DuetPipeline::DownstreamPort::_try_send_req () {
    panic_if ( nullptr == _req_buf, "No request to send!" );
    if ( sendTimingReq ( _req_buf ) ) {
        // success!
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) sent REQ (%s)\n",
                _owner->name(), _req_buf->print() );
        _req_buf = nullptr;
        _is_this_waiting_for_retry = false;
    } else {
        DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot send REQ (%s)\n",
                _owner->name(), _req_buf->print() );
        _is_this_waiting_for_retry = true;
    }
}

DuetPipeline::DuetPipeline ( const DuetPipelineParams & p )
    : ClockedObject         ( p )
    , _downstream_port      ( p.name + ".downstream", this )
    , _downward_stages      ()
    , _downward_latency     ( p.downward_latency )
    , _downward_interval    ( p.downward_interval )
    , _upstream_port        ( p.name + ".upstream", this )
    , _upward_stages        ()
    , _upward_latency       ( p.upward_latency )
    , _upward_interval      ( p.upward_interval )
    , _e_do_cycle           ( [this]{ _do_cycle(); }, name() )
    , _latest_cycle_plus1   ( 0 )
    , _is_sleeping          ( false )
{}

void DuetPipeline::_do_cycle () {
    // 1. pipeline
    // 1.1 downward
    do {
        auto it = _downward_stages.begin ();
        if ( _downward_stages.end () == it )
            break;

        if ( Cycles(0) == it->first ) {
            if ( nullptr == _downstream_port._req_buf ) {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) passed REQ (%s) to downstream buffer\n",
                        name(), it->second->print() );
                _downstream_port._req_buf = it->second;
                it = _downward_stages.erase ( it );
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) downward stalled: buffer unavail\n",
                        name() );
                break;
            }
        }

        for ( ; _downward_stages.end () != it; ++it ) {
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) staging REQ (%s) (%llu)\n",
                    name(), it->second->print(), uint64_t(it->first) );
            --(it->first);
        }
    } while ( false );

    // 1.2 upward
    do {
        auto it = _upward_stages.begin ();
        if ( _upward_stages.end () == it )
            break;

        if ( Cycles(0) == it->first ) {
            if ( nullptr == _upstream_port._resp_buf ) {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) passed RESP (%s) to upstream buffer\n",
                        name(), it->second->print() );
                _upstream_port._resp_buf = it->second;
                it = _upward_stages.erase ( it );
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) upward stalled: buffer unavail\n",
                        name() );
                break;
            }
        }

        for ( ; _upward_stages.end () != it; ++it )
            DPRINTF ( DuetPipeline, "DuetPipeline (%s) staging RESP (%s) (%llu)\n",
                    name(), it->second->print(), it->first );
            --(it->first);
    } while ( false );

    // 2. take in upstream.req_buf and downstream.resp_buf
    // 2.1 downstream
    if ( nullptr != _upstream_port._req_buf ) {
        if ( Cycles (0) == _downward_latency ) {
            // move to downstream.req_buf
            if ( nullptr == _downstream_port._req_buf ) {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) take in REQ (%s) to downstream buffer\n",
                        name(), _upstream_port._req_buf->print() );
                _downstream_port._req_buf = _upstream_port._req_buf;
                _upstream_port._req_buf = nullptr;
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot take in REQ (%s): buffer unavail\n",
                        name(), _upstream_port._req_buf->print() );
            }
        } else {
            // push into the pipeline
            auto it = _downward_stages.rbegin ();
            if ( _downward_stages.rend () == it ||
                    it->first + _downward_interval < _downward_latency )
            {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) take in REQ (%s) to stages (%llu)\n",
                        name(), _upstream_port._req_buf->print(), uint64_t (_downward_latency - 1) );
                _downward_stages.emplace_back (
                        _downward_latency - 1
                        , _upstream_port._req_buf
                        );
                _upstream_port._req_buf = nullptr;
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot take in REQ (%s): interval\n",
                        name(), _upstream_port._req_buf->print() );
            }
        }
    }

    // 2.2 upstream
    if ( nullptr != _downstream_port._resp_buf ) {
        if ( Cycles (0) == _upward_latency ) {
            // move to upstream.resp_buf
            if ( nullptr == _upstream_port._resp_buf ) {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) take in RESP (%s) to upstream buffer\n",
                        name(), _downstream_port._resp_buf->print() );
                _upstream_port._resp_buf = _downstream_port._resp_buf;
                _downstream_port._resp_buf = nullptr;
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot take in RESP (%s): buffer unavail\n",
                        name(), _downstream_port._resp_buf->print() );
            }
        } else {
            // push into the pipeline
            auto it = _upward_stages.rbegin ();
            if ( _upward_stages.rend () == it ||
                    it->first + _upward_interval < _upward_latency )
            {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) take in RESP (%s) to stages (%llu)\n",
                        name(), _downstream_port._resp_buf->print(), uint64_t (_upward_latency - 1) );
                _upward_stages.emplace_back (
                        _upward_latency - 1
                        , _downstream_port._resp_buf
                        );
                _downstream_port._resp_buf = nullptr;
            } else {
                DPRINTF ( DuetPipeline, "DuetPipeline (%s) cannot take in RESP (%s): interval\n",
                        name(), _downstream_port._resp_buf->print() );
            }
        }
    }

    // 3. we have done with this cycle
    ++_latest_cycle_plus1;

    // 4. send things out
    if ( nullptr != _downstream_port._req_buf
            && !_downstream_port._is_this_waiting_for_retry )
        _downstream_port._try_send_req ();

    if ( nullptr != _upstream_port._resp_buf
            && !_upstream_port._is_this_waiting_for_retry )
        _upstream_port._try_send_resp ();

    if ( nullptr == _upstream_port._req_buf
            && _upstream_port._is_peer_waiting_for_retry )
    {
        _upstream_port._is_peer_waiting_for_retry = false;
        _upstream_port.sendRetryReq ();
    }

    if ( nullptr == _downstream_port._resp_buf
            && _downstream_port._is_peer_waiting_for_retry )
    {
        _downstream_port._is_peer_waiting_for_retry = false;
        _downstream_port.sendRetryResp ();
    }

    // 5. schedule the next cycle
    if ( _has_work () )
        schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    else
        _is_sleeping = true;
}

void DuetPipeline::_wakeup () {
    schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    _latest_cycle_plus1 = curCycle() + Cycles(1);
    _is_sleeping = false;
}

bool DuetPipeline::_has_work () {
    return nullptr != _upstream_port._req_buf
        || nullptr != _upstream_port._resp_buf
        || nullptr != _downstream_port._req_buf
        || nullptr != _downstream_port._resp_buf
        || !_downward_stages.empty ()
        || !_upward_stages.empty ();
}

}   // namespace duet
}   // namespace gem5
