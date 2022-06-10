#include "duet/widget/widget.hh"
#include "sim/system.hh"
#include "sim/process.hh"
#include "mem/packet_access.hh"

namespace gem5 {
namespace duet {

AddrRangeList DuetWidget::SRIPort::getAddrRanges () const {
    AddrRangeList ret;
    ret.push_back ( _owner->_range );
    return ret;
}

bool DuetWidget::SRIPort::recvTimingReq ( PacketPtr pkt ) {
    // wake up widget if it is sleeping
    if ( _owner->_is_sleeping ) {
        assert ( nullptr == _req_buf );

        _req_buf = pkt;
        _owner->_wakeup ();
        return true;
    }

    // block if we are in pre-do_cycle phase, or if we cannot buffer more packets
    if ( _owner->_is_pre_do_cycle ()
            || nullptr != _req_buf )
    {
        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        _req_buf = pkt;
        return true;
    }
}

void DuetWidget::SRIPort::recvRespRetry () {
    assert ( _is_this_waiting_for_retry );

    // postpone retrying after do_cycle
    if ( _owner->_is_pre_do_cycle () )
        _is_this_waiting_for_retry = false;

    else
        _try_send_resp ();
}

void DuetWidget::SRIPort::_try_send_resp () {
    panic_if ( nullptr == _resp_buf, "No SRI response to send!" );
    if ( sendTimingResp ( _resp_buf ) ) {
        // success!
        _resp_buf = nullptr;
        _is_this_waiting_for_retry = false;
    } else {
        _is_this_waiting_for_retry = true;
    }
}

bool DuetWidget::MemoryPort::recvTimingResp ( PacketPtr pkt ) {

    // block if we are in pre-do_cycle phase, or if we cannot buffer more packets
    if ( _owner->_is_pre_do_cycle ()
            || nullptr != _resp_buf )
    {
        _is_peer_waiting_for_retry = true;
        return false;
    } else {
        _resp_buf = pkt;
        return true;
    }
}

void DuetWidget::MemoryPort::recvReqRetry () {
    assert ( _is_this_waiting_for_retry );

    // postpone retrying after do_cycle
    if ( _owner->_is_pre_do_cycle () )
        _is_this_waiting_for_retry = false;

    else
        _try_send_req ();
}

void DuetWidget::MemoryPort::_try_send_req () {
    panic_if ( nullptr == _req_buf, "No memory request to send!" );
    if ( sendTimingReq ( _req_buf ) ) {
        // success!
        _req_buf = nullptr;
        _is_this_waiting_for_retry = false;
    } else {
        _is_this_waiting_for_retry = true;
    }
}

DuetWidget::DuetWidget ( const DuetWidgetParams & p )
    : ClockedObject         ( p )
    , _system               ( p.system )
    , _process              ( p.process )
    , _sri_port             ( p.name + ".sri_port", this )
    , _mem_port             ( p.name + ".mem_port", this )
    , _fifo_capacity        ( p.fifo_capacity )
    , _range                ( p.range )
    , _latency_per_stage    ( p.latency_per_stage )
    , _interval_per_stage   ( p.interval_per_stage )
    , _requestorId          ( p.system->getRequestorId (this) )
    , _latest_cycle_plus1   ( 0 )
    , _is_sleeping          ( true )
    , _e_do_cycle           ( [this]{ _do_cycle(); }, name() )
{
    // validate "range"
    if ( _range.interleaved() ) {
        panic ( "DuetWidgetManager does not support interleaved address range" );
    } else if ( (_range.start() & 0x7) || (_range.end() & 0x7) ) {
        panic ( "DuetWidgetManager address range is not aligned to and/or multiples of 8B" );
    }

    // threads
    tid_t max_threads = (_range.end() - _range.start()) >> 3;
    _retcode_fifo_vec.resize ( max_threads );

    // memory channels
    auto chan_count = _get_chan_count ();

    _chan_req_header.reset (
            new AbstractDuetWidgetFunctor::chan_req_header_t [chan_count]
            );

    _chan_req_data.reset (
            new AbstractDuetWidgetFunctor::chan_req_data_t [chan_count]
            );

    _chan_resp_data.reset (
            new AbstractDuetWidgetFunctor::chan_resp_data_t [chan_count]
            );
}

Cycles DuetWidget::_get_latency ( int stage ) const {
    assert ( stage >= 0 );

    panic_if ( stage >= _latency_per_stage.size(),
            "Latency not specified for stage %d",
            stage );

    return _latency_per_stage [stage];
}

Cycles DuetWidget::_get_interval ( int stage ) const {
    assert ( stage >= 0 );

    panic_if ( stage >= _interval_per_stage.size(),
            "Initiation interval not specified for stage %d",
            stage );

    return _interval_per_stage [stage];
}

bool DuetWidget::_move_to_stage ( DuetWidget::Execution & e ) {
    auto stage = e.functor->get_stage ();

    // create the list for the stage if absent
    for ( auto i = _exec_list_per_stage.size(); i <= stage; ++i ) {
        _exec_list_per_stage.emplace_back ( new std::list <Execution>() );
    }

    auto latency = _get_latency (stage);
    auto exec_list = _exec_list_per_stage [stage].get();

    if ( !exec_list->empty() ) {
        Cycles interval = _get_interval ( stage );

        if ( interval == 0 )
            // does not allow multi-entry
            return false;

        const auto & last = exec_list->back ();
        if ( latency - interval < last.remaining )
            // cannot start due to initiation interval
            return false;
    }

    // can move
    exec_list->emplace_back ( e.caller, latency );
    std::swap ( e.functor, exec_list->back().functor );
    return true;
}

void DuetWidget::_do_cycle () {
    // 1. if SRI request buffer contains a load (join), and SRI response buffer
    //      is unused: check retcode FIFO and respond correspondingly
    if ( nullptr != _sri_port._req_buf
            && nullptr == _sri_port._resp_buf
            && _sri_port._req_buf->isRead () )
    {
        auto pkt = _sri_port._req_buf;
        _sri_port._req_buf = nullptr;
        auto tid = _get_tid_from_addr ( pkt->getAddr () );

        uint64_t retcode = AbstractDuetWidgetFunctor::RETCODE_RUNNING;
        if ( !_retcode_fifo_vec[tid].empty() ) {
            retcode = _retcode_fifo_vec[tid].front ();
            _retcode_fifo_vec[tid].pop_front ();
        }

        pkt->setLE ( retcode );
        pkt->makeResponse ();
        _sri_port._resp_buf = pkt;
    }

    // 2. if memory request buffer is unused, see if we can prepare a new request
    if ( nullptr == _mem_port._req_buf )
        _mem_port._req_buf = _pop_mem_req ();

    // 3. advance as many execution as possible
    _advance ();

    // 4. if memory response buffer contains a valid load response, see if we can
    //      consume it and push into the response channel
    if ( nullptr != _mem_port._resp_buf ) {
        auto pkt = _mem_port._resp_buf;

        if ( pkt->isRead() ) {
            auto chan_id = pkt->req->getFlags() & Request::ARCH_BITS;

            if ( _chan_resp_data[chan_id].size () < _fifo_capacity ) {
                AbstractDuetWidgetFunctor::resp_data_t data (
                        new uint8_t[pkt->getSize ()] );
                std::memcpy ( data.get(), pkt->getPtr<uint8_t>(), pkt->getSize() );
                _chan_resp_data[chan_id].push_back ( data );
                _mem_port._resp_buf = nullptr;
                delete pkt;
            }
        } else {
            _mem_port._resp_buf = nullptr;
            delete pkt;
        }
    }

    // 5. if invocation FIFO is not empty, and new executions can be initiated,
    //      pop one argument from the invocation FIFO
    if ( _invoke () )
        _invocation_fifo.pop_front ();

    // 6. if SRI request buffer contains a store (invocation), and invocation
    //      FIFO is not full, and SRI response buffer is unused, try to push
    //      into invocation fifo
    if ( nullptr != _sri_port._req_buf
            && nullptr == _sri_port._resp_buf
            && _sri_port._req_buf->isWrite ()
            && _invocation_fifo.size () < _fifo_capacity )
    {
        auto pkt = _sri_port._req_buf;
        _sri_port._req_buf = nullptr;
        auto tid = _get_tid_from_addr ( pkt->getAddr () );

        uintptr_t arg = pkt->getLE<uintptr_t> ();
        _invocation_fifo.emplace_back ( tid, arg );

        pkt->makeResponse ();
        _sri_port._resp_buf = pkt;
    }

    // 7. increment cycle count
    ++_latest_cycle_plus1;

    // 8. send out memory request / SRI response if possible
    if ( nullptr != _mem_port._req_buf
            && !_mem_port._is_this_waiting_for_retry )
        _mem_port._try_send_req ();

    if ( nullptr != _sri_port._resp_buf
            && !_sri_port._is_this_waiting_for_retry )
        _sri_port._try_send_resp ();

    // 9. send out retry notifications if necessary
    if ( nullptr == _mem_port._resp_buf
            && _mem_port._is_peer_waiting_for_retry )
    {
        _mem_port._is_peer_waiting_for_retry = false;
        _mem_port.sendRetryResp ();
    }

    if ( nullptr == _sri_port._req_buf
            && _sri_port._is_peer_waiting_for_retry )
    {
        _sri_port._is_peer_waiting_for_retry = false;
        _sri_port.sendRetryReq ();
    }

    // 10. check if there are work to do in the next cycle
    if ( _has_work () )
        schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );
    else
        _is_sleeping = true;
}

DuetWidget::tid_t DuetWidget::_get_tid_from_addr ( Addr addr ) const {
    return (addr - _range.start ()) >> 3;
}

PacketPtr DuetWidget::_pop_mem_req () {

    for ( int chan_id = _get_chan_count () - 1; chan_id >= 0; --chan_id ) {
        if ( !_chan_req_header[chan_id].empty () ) {
            auto header = _chan_req_header[chan_id].front ();

            // translate address 
            Addr paddr;
            panic_if ( !_process->pTable->translate ( header.addr, paddr ),
                    "Memory translation failed" );

            // make a new request
            RequestPtr req = std::make_shared <Request> (
                    paddr,
                    1 << header.size_lg2,
                    (Request::FlagsType) chan_id,
                    _requestorId
                    );

            // get data and construct packet
            PacketPtr pkt = nullptr;
            switch ( header.type ) {
            case AbstractDuetWidgetFunctor::REQTYPE_LD :
                pkt = new Packet ( req, Packet::makeReadCmd ( req ) );
                pkt->allocate ();
                break;

            case AbstractDuetWidgetFunctor::REQTYPE_ST :
                if ( _chan_req_data[chan_id].empty () )
                    break;

                pkt = new Packet ( req, Packet::makeWriteCmd ( req ) );
                pkt->allocate ();
                pkt->setData ( _chan_req_data[chan_id].front().get() );
                _chan_req_data[chan_id].pop_front ();
                break;

#define _TMP_UNSUPPORTED_REQ_TYPE(_t_) \
            case AbstractDuetWidgetFunctor::_t_ :           \
                panic ( "Unsupported request type: _t_" );  \
                break;

            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_LR)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_SC)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_SWAP)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_ADD)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_AND)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_OR)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_XOR)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_MAX)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_MAXU)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_MIN)
            _TMP_UNSUPPORTED_REQ_TYPE (REQTYPE_MINU)

#undef _TMP_UNSUPPORTED_REQ_TYPE

            default :
                panic ( "Invalid request type" );
            }

            if ( nullptr != pkt ) {
                _chan_req_header[chan_id].pop_front ();
                return pkt;
            }
        }
    }

    return nullptr;
}

void DuetWidget::_advance () {

    // iterate through stages in reverse order
    for ( int stage = _exec_list_per_stage.size() - 1;
            stage >= 0;
            --stage )
    {
        auto exec_list = _exec_list_per_stage [stage].get();

        // iterate through each execution in a stage in normal order
        for ( auto exec = exec_list->begin ();
                exec != exec_list->end ();
                // no auto increment since we may modify the list while iterating
            )
        {
            // check remaining cycles
            if ( exec->remaining == Cycles (0) ) {
                //  stage + 1 was full when this execution was ready to advance
                if ( _move_to_stage ( *exec ) ) {
                    exec = exec_list->erase ( exec );
                    continue;
                } else {
                    return;     // stall. end of iteration
                }
            } else {
                --(exec->remaining);
            }

            if ( exec->remaining > Cycles (0) ) {
                ++exec;
                continue;
            }

            // time to advance kernel
            auto & functor = exec->functor;

            // check if we can advance
            if ( nullptr != functor->get_blocking_chan_req_header () ) {
                if ( functor->get_blocking_chan_req_header()->size() >= _fifo_capacity ) {
                    // no. stall and return
                    return;
                }
            } else if ( nullptr != functor->get_blocking_chan_req_data () ) {
                if ( functor->get_blocking_chan_req_data()->size() >= _fifo_capacity ) {
                    // no. stall and return
                    return;
                }
            } else if ( nullptr != functor->get_blocking_chan_resp_data () ) {
                if ( functor->get_blocking_chan_resp_data()->empty() ) {
                    // no. stall and return
                    return;
                }
            } else {
                panic ( "Execution is not blocked by any channel" );
            }

            // advance
            auto retcode = functor->advance ();

            switch ( retcode ) {
            case AbstractDuetWidgetFunctor::RETCODE_RUNNING :
                // not finished
                // move the execution to the correct (next) stage
                panic_if ( functor->get_stage () != stage + 1,
                        "Pipeline jumped from stage %d to %d",
                        stage, functor->get_stage () );

                if ( !_move_to_stage ( *exec ) )
                    // cannot forward. stall
                    return;
                else
                    break;

            default:
                // RETCODE FIFOs should always have enough space
                _retcode_fifo_vec[exec->caller].push_back ( retcode );
            }

            exec = exec_list->erase ( exec );
        }
    }
}

bool DuetWidget::_invoke () {
    if ( !_invocation_fifo.empty () ) {
        auto caller = _invocation_fifo.front().first;
        auto arg    = _invocation_fifo.front().second;

        auto functor = _new_functor ();
        panic_if ( nullptr == functor,
                "_new_functor returned nullptr" );

        Execution e ( caller, _get_latency (0), functor );

        if ( _move_to_stage ( e ) ) {
            functor->invoke (
                    arg
                    , _chan_req_header.get ()
                    , _chan_req_data.get ()
                    , _chan_resp_data.get ()
                    );
            return true;
        }
    }

    return false;
}

void DuetWidget::_wakeup () {
    // schedule do_cycle for the next cycle
    schedule ( _e_do_cycle, clockEdge ( Cycles(1) ) );

    // update _latest_cycle_plus1
    _latest_cycle_plus1 = curCycle() + Cycles(1);

    // update my own state
    _is_sleeping = false;
}

bool DuetWidget::_has_work () const {

    if ( nullptr != _sri_port._req_buf
            || _sri_port._is_peer_waiting_for_retry
            || nullptr != _mem_port._resp_buf
            || _mem_port._is_peer_waiting_for_retry )
        return true;

    if ( !_invocation_fifo.empty () )
        return true;

    for ( const auto & exec_list : _exec_list_per_stage ) {
        if ( !exec_list->empty () )
            return true;
    }

    return false;
}

Port & DuetWidget::getPort ( const std::string & if_name, PortID idx ) {
    panic_if ( idx != InvalidPortID,
            "DuetWidget does not support vector ports." );

    if ( if_name == "sri_port" )
        return _sri_port;
    else if ( if_name == "mem_port" )
        return _mem_port;
    else
        return ClockedObject::getPort ( if_name, idx );
}

void DuetWidget::init () {
    ClockedObject::init ();

    if ( _sri_port.isConnected() )
        _sri_port.sendRangeChange ();
}

}   // namespace duet
}   // namespace gem5
