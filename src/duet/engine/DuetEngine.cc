#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"
#include "sim/system.hh"
#include "sim/process.hh"

namespace gem5 {
namespace duet {

AddrRangeList DuetEngine::SRIPort::getAddrRanges () const {
    AddrRangeList list;
    AddrRange range ( owner->_baseaddr
            , owner->_baseaddr + ( owner->get_num_softregs () << 3 )
            );
    list.push_back ( range );
    return list;
}

DuetEngine::DuetEngine ( const DuetEngineParams & p )
    : DuetClockedObject         ( p )
    , _system                   ( p.system )
    , _process                  ( p.process )
    , _fifo_capacity            ( p.fifo_capacity )
    , _num_callers              ( p.num_callers )
    , _baseaddr                 ( p.baseaddr )
    , _lanes                    ( p.lanes )
    , _sri_port                 ( p.name + ".sri_port", this )
{
    _requestorId = p.system->getRequestorId (this);

    for ( int i = 0; i < p.port_mem_ports_connection_count; ++i ) {
        std::string portName = csprintf ( "%s.mem_ports[%d]", name(), i );
        _mem_ports.emplace_back ( portName, this, i );
    }

    for ( auto & lane : _lanes )
        lane->set_engine ( this );
}

void DuetEngine::update () {
    // -- pull phase ---------------------------------------------------------
    //  1. if SRI request buffer contains a load, and SRI response buffer is
    //     unused: check if we can handle the access
    if ( nullptr != _sri_port.req_buf
            && nullptr == _sri_port.resp_buf
            && _sri_port.req_buf->isRead () )
    {
        auto pkt = _sri_port.req_buf;
        softreg_id_t id = ( pkt->getAddr () - _baseaddr ) >> 3;

        uint64_t value;
        if ( handle_softreg_read ( id, value ) ) {
            pkt->setLE ( value );
            pkt->makeResponse ();
            _sri_port.req_buf = nullptr;
            _sri_port.resp_buf = pkt;
        }
    }

    //  2. send as many memory requests as we can
    try_send_mem_req_all ();

    //  3. call pull_phase on all lanes
    for ( auto & lane : _lanes )
        lane->pull_phase ();

    // -- push phase ---------------------------------------------------------
    //  1. if SRI request buffer contains a store, and SRI response buffer is
    //     unused: check if we can handle the access
    if ( nullptr != _sri_port.req_buf
            && nullptr == _sri_port.resp_buf
            && _sri_port.req_buf->isWrite () )
    {
        auto pkt = _sri_port.req_buf;
        softreg_id_t id = ( pkt->getAddr () - _baseaddr ) >> 3;
        uint64_t value = pkt->getLE<uint64_t> ();

        if ( handle_softreg_write ( id, value ) ) {
            pkt->makeResponse ();
            _sri_port.req_buf = nullptr;
            _sri_port.resp_buf = pkt;
        }
    }

    //  2. if memory response buffer contains a valid load response, see if we
    //     can receive it
    for ( auto & port : _mem_ports ) {
        auto pkt = port.resp_buf;

        if ( nullptr != pkt ) {

            if ( pkt->isRead () ) {
                uint16_t chan_id = pkt->req->getFlags() & Request::ARCH_BITS;
                auto & chan = _chan_rdata_by_id [chan_id];

                if ( 0 == _fifo_capacity
                        || chan->size () < _fifo_capacity )
                {
                    DuetFunctor::raw_data_t data (
                            new uint8_t[ pkt->getSize() ]);
                    std::memcpy (
                            data.get(),
                            pkt->getPtr<uint8_t>(),
                            pkt->getSize()
                            );
                    chan->push_back ( data );
                    port.resp_buf = nullptr;
                    delete pkt;
                }
            } else {
                port.resp_buf = nullptr;
                delete pkt;
            }
        }
    }

    //  3. call push_phase on all lanes
    for ( auto & lane : _lanes )
        lane->push_phase ();
}

void DuetEngine::exchange () {
    //  1. send out SRI response if possible, then send out retry notifications
    if ( nullptr != _sri_port.resp_buf
            && !_sri_port.is_this_waiting_for_retry )
        _sri_port.try_send_resp ();

    if ( nullptr == _sri_port.req_buf
            && _sri_port.is_peer_waiting_for_retry )
    {
        _sri_port.is_peer_waiting_for_retry = false;
        _sri_port.sendRetryReq ();
    }

    //  2. send out memory requests if possible, then send out retry
    //  notifications
    for ( auto & port : _mem_ports ) {
        if ( nullptr != port.req_buf
                && !port.is_this_waiting_for_retry )
            port.try_send_req ();

        if ( nullptr == port.resp_buf
                && port.is_peer_waiting_for_retry )
        {
            port.is_peer_waiting_for_retry = false;
            port.sendRetryResp ();
        }
    }
}

bool DuetEngine::has_work () {
    if ( _sri_port.req_buf || _sri_port.resp_buf )
        return true;

    for ( auto & port : _mem_ports )
        if ( port.req_buf || port.resp_buf )
            return true;

    for ( auto & lane : _lanes )
        if ( lane->has_work () )
            return true;

    return false;
}

DuetFunctor::chan_req_t & DuetEngine::get_chan_req (
        DuetFunctor::chan_id_t      chan_id
        )
{
    assert ( DuetFunctor::chan_id_t::REQ == chan_id.tag );
    return *( _chan_req_by_id [chan_id.id] );
}

DuetFunctor::chan_data_t & DuetEngine::get_chan_data (
        DuetFunctor::chan_id_t      chan_id
        )
{
    switch ( chan_id.tag ) {
    case DuetFunctor::chan_id_t::WDATA:
        return *( _chan_wdata_by_id [chan_id.id] );

    case DuetFunctor::chan_id_t::RDATA:
        return *( _chan_rdata_by_id [chan_id.id] );

    case DuetFunctor::chan_id_t::ARG:
        return *( _chan_arg_by_id [chan_id.id] );

    case DuetFunctor::chan_id_t::RET:
        return *( _chan_ret_by_id [chan_id.id] );

    case DuetFunctor::chan_id_t::PULL:
    case DuetFunctor::chan_id_t::PUSH:
        return *( _chan_int_by_id [chan_id.id] );

    default:
        panic ( "Invalid data channel tag" );
    }
}

bool DuetEngine::can_push_to_chan (
        DuetFunctor::chan_id_t      chan_id
        )
{
    switch ( chan_id.tag ) {
    case DuetFunctor::chan_id_t::REQ:
        return (0 == _fifo_capacity
                || _chan_req_by_id [chan_id.id]->size() < _fifo_capacity);

    case DuetFunctor::chan_id_t::WDATA:
        return (0 == _fifo_capacity
                || _chan_wdata_by_id [chan_id.id]->size() < _fifo_capacity);

    case DuetFunctor::chan_id_t::RDATA:
        panic ( "Trying to push to RDATA channel" );

    case DuetFunctor::chan_id_t::ARG:
        panic ( "Trying to push to ARG channel" );

    case DuetFunctor::chan_id_t::RET:
        return (0 == _fifo_capacity
                || _chan_ret_by_id [chan_id.id]->size() < _fifo_capacity);

    case DuetFunctor::chan_id_t::PULL:
        panic ( "Trying to push to PULL channel" );

    case DuetFunctor::chan_id_t::PUSH:
        return (0 == _fifo_capacity
                || _chan_int_by_id [chan_id.id]->size() < _fifo_capacity);

    default:
        panic ( "Invalid channel tag" );
    }

    return false;
}

bool DuetEngine::can_pull_from_chan (
        DuetFunctor::chan_id_t      chan_id
        )
{
    switch ( chan_id.tag ) {
    case DuetFunctor::chan_id_t::REQ:
        panic ( "Trying to pull from REQ channel" );

    case DuetFunctor::chan_id_t::WDATA:
        panic ( "Trying to pull from WDATA channel" );

    case DuetFunctor::chan_id_t::RDATA:
        return !_chan_rdata_by_id [chan_id.id]->empty ();

    case DuetFunctor::chan_id_t::ARG:
        return !_chan_arg_by_id [chan_id.id]->empty ();

    case DuetFunctor::chan_id_t::RET:
        panic ( "Trying to pull from RET channel" );

    case DuetFunctor::chan_id_t::PULL:
        return !_chan_int_by_id [chan_id.id]->empty ();

    case DuetFunctor::chan_id_t::PUSH:
        panic ( "Trying to pull from PUSH channel" );

    default:
        panic ( "Invalid channel tag" );
    }

    return false;
}

bool DuetEngine::try_send_mem_req_one (
        uint16_t                    chan_id
        , DuetFunctor::mem_req_t    req
        , DuetFunctor::raw_data_t   data
        )
{
    // find a memory port whose req_buf is empty
    for ( auto & port : _mem_ports ) {
        if ( nullptr == port.req_buf ) {
            // translate address
            Addr paddr;
            panic_if ( !_process->pTable->translate ( req.addr, paddr ),
                    "Memory translation failed" );

            // make a new request
            RequestPtr req_ = std::make_shared <Request> (
                    paddr,
                    req.size,
                    (Request::FlagsType) chan_id,
                    _requestorId
                    );

            // get data and construct packet
            PacketPtr pkt = nullptr;
            switch ( req.type ) {
            case DuetFunctor::REQTYPE_LD :
                pkt = new Packet ( req_, Packet::makeReadCmd ( req_ ) );
                pkt->allocate ();
                break;

            case DuetFunctor::REQTYPE_ST :
                pkt = new Packet ( req_, Packet::makeWriteCmd ( req_ ) );
                pkt->allocate ();
                pkt->setData ( data.get() );
                break;

#define _TMP_UNSUPPORTED_REQ_TYPE(_t_) \
            case DuetFunctor::_t_ :           \
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

            port.req_buf = pkt;
            return true;
        }
    }

    return false;
}

bool DuetEngine::handle_argchan_push (
        DuetFunctor::caller_id_t    caller_id
        , uint64_t                  value
        )
{
    auto & chan = _chan_arg_by_id [caller_id];

    if ( 0 == _fifo_capacity
            || chan->size() < _fifo_capacity )
    {
        DuetFunctor::raw_data_t raw ( new uint8_t[8] );
        memcpy ( raw.get(), &value, 8 );
        chan->push_back ( raw );
        return true;
    } else {
        return false;
    }
}

bool DuetEngine::handle_retchan_pull (
        DuetFunctor::caller_id_t    caller_id
        , uint64_t                & value
        )
{
    auto & chan = _chan_ret_by_id [caller_id];

    if ( !chan->empty () ) {
        auto raw = chan->front ();
        chan->pop_front ();
        memcpy ( &value, raw.get(), 8 );
    } else {
        value = DuetFunctor::RETCODE_RUNNING;
    }

    return true;
}

Port & DuetEngine::getPort (
        const std::string & if_name
        , PortID idx
        )
{
    if ( if_name == "sri_port" ) {
        panic_if ( InvalidPortID != idx,
                "sri_port is not a vector port" );
        return _sri_port;
    } else if ( if_name == "mem_ports" )
        return _mem_ports[idx];
    else 
        return SimObject::getPort ( if_name, idx );
}

void DuetEngine::init () {
    ClockedObject::init ();

    if ( _sri_port.isConnected() )
        _sri_port.sendRangeChange ();

    for ( DuetFunctor::caller_id_t i = 0; i < get_num_callers (); ++i ) {
        _chan_arg_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
        _chan_ret_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
    }

    for ( DuetFunctor::caller_id_t i = 0; i < get_num_memory_chans (); ++i) {
        _chan_req_by_id.emplace_back   ( new DuetFunctor::chan_req_t () );
        _chan_wdata_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
        _chan_rdata_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
    }

    for ( DuetFunctor::caller_id_t i = 0; i < get_num_interlane_chans (); ++i)
        _chan_int_by_id.emplace_back   ( new DuetFunctor::chan_data_t () );
}

}   // namespace duet
}   // namespace gem5
