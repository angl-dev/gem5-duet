#include "debug/DuetEngine.hh"
#include "debug/DuetEngineDetailed.hh"
#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"
#include "sim/system.hh"
#include "sim/process.hh"
#include "base/trace.hh"
#include "mem/packet_access.hh"

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

DuetEngine::Stats::Stats ( DuetEngine & engine )
    : statistics::Group ( &engine )
    , engine            ( engine )
    , ADD_STAT ( blocktime, statistics::units::Cycle::get (),
            "Total time (#cycles) that the SRI channel is blocked" )
    , ADD_STAT ( busytime,  statistics::units::Cycle::get (),
            "Total time (#cycles) that the engine is busy" )
    , ADD_STAT ( waittime,  statistics::units::Cycle::get (),
            "Execution waiting time (#cycles)" )
    , ADD_STAT ( exectime,  statistics::units::Cycle::get (),
            "Execution time (#cycles)" )
{}

void DuetEngine::Stats::regStats () {
    statistics::Group::regStats ();

    blocktime.flags ( statistics::total );
    blocktime.reset ();

    busytime.flags ( statistics::total );
    busytime.reset ();

    unsigned max_waittime = engine.get_max_stats_waittime ();
    if ( max_waittime < 100 ) max_waittime = 100;
    unsigned waittime_bucketsize = max_waittime / 10;
    if ( waittime_bucketsize > 100 ) waittime_bucketsize = 100;

    waittime
        .init  ( 0, max_waittime, waittime_bucketsize )
        .flags ( statistics::nozero | statistics::nonan | statistics::dist );

    unsigned max_exectime = engine.get_max_stats_exectime ();
    if ( max_exectime < 100 ) max_exectime = 100;
    unsigned exectime_bucketsize = max_exectime / 10;
    if ( exectime_bucketsize > 100 ) exectime_bucketsize = 100;

    exectime
        .init  ( 0, max_exectime, exectime_bucketsize )
        .flags ( statistics::nozero | statistics::nonan | statistics::dist );
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
    , _writeclean               ( p.writeclean )
    , _stats                    ( *this )
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
    if ( nullptr != _sri_port.req_buf && !_is_blocked ) {
        _is_blocked = true;
        _blocked_from = curCycle ();
    }

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

    //  2. if ROBs contain ack'ed responses, push into return channels
    std::vector <bool> chan_pushed ( get_num_memory_chans(), false );
    for ( auto & rob : _rob ) {
        auto & entry = rob.front ();
        if ( ROBEntry::RESPONDED == entry.status
                && curTick () >= entry.readyAfter
                && !chan_pushed [entry.chan_id] )
        {
            _chan_rdata_by_id [entry.chan_id]->push_back ( entry.data );
            -- _reservations_by_id [entry.chan_id];
            chan_pushed [entry.chan_id] = true;
            rob.pop_front ();
        }
    }

    //  3. send as many memory requests as we can
    try_send_mem_req_all ();

    //  4. call pull_phase on all lanes
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

    //  2. if memory response buffer contains a valid response, see if we
    //     can accept it
    for ( int i = 0; i < _mem_ports.size (); ++i ) {
        auto & port = _mem_ports[i];
        auto pkt = port.resp_buf;
        if ( nullptr == pkt ) continue;

        // search ROB
        for ( auto & entry : _rob[i] ) {
            if ( pkt->req == entry.req ) {

                assert ( ROBEntry::SENT == entry.status );
                entry.status = ROBEntry::RESPONDED;
                entry.readyAfter = clockEdge ( Cycles(1) )
                    + pkt->headerDelay + pkt->payloadDelay;

                if ( pkt->isRead () ) {
                    entry.data.reset ( new uint8_t [ entry.size ] );
                    if ( pkt->getSize () != entry.size ) {
                        auto baseaddr = pkt->getBlockAddr ( _system->cacheLineSize() );
                        std::memcpy (
                                entry.data.get (),
                                pkt->getPtr <uint8_t> () + pkt->getAddr() - baseaddr,
                                entry.size
                                );
                    } else {
                        std::memcpy (
                                entry.data.get (),
                                pkt->getPtr <uint8_t> (),
                                entry.size
                                );
                    }
                }

                port.resp_buf = nullptr;
                delete pkt;
                break;
            }
        }

        assert ( nullptr == port.resp_buf );
    }

    //  3. call push_phase on all lanes
    for ( auto & lane : _lanes )
        lane->push_phase ();

    if ( nullptr == _sri_port.req_buf && _is_blocked ) {
        _is_blocked = false;
        _stats.blocktime += curCycle () - _blocked_from;
    }
}

void DuetEngine::exchange () {
    //  1. send out SRI response if possible, then send out retry notifications
    _sri_port.exchange ();

    //  2. send out memory requests if possible, then send out retry
    //  notifications
    for ( auto & port : _mem_ports )
        port.exchange ();
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

void DuetEngine::stats_call_recvd (
        DuetFunctor::caller_id_t    caller_id
        )
{
    _received [caller_id].push_back ( curCycle() );
}

void DuetEngine::stats_exec_start (
        DuetFunctor::caller_id_t    caller_id
        )
{
    Cycles recvd = _received [caller_id].front ();
    _received [caller_id].pop_front ();

    Cycles wait = curCycle () - recvd;
    _stats.waittime.sample ( wait, 1 );

    _started [caller_id].push_back ( curCycle () );

    if ( !_is_busy ) {
        _is_busy    = true;
        _busy_from  = curCycle ();
    }
}

void DuetEngine::stats_exec_done (
        DuetFunctor::caller_id_t    caller_id
        )
{
    Cycles start = _started [caller_id].front ();
    _started [caller_id].pop_front ();

    Cycles exec = curCycle () - start;
    _stats.exectime.sample ( exec, 1 );

    for ( auto & l : _received )
        if ( !l.empty () )
            return;

    for ( auto & l : _started )
        if ( !l.empty () )
            return;

    _is_busy = false;
    _stats.busytime += curCycle () - _busy_from;
}

bool DuetEngine::try_send_mem_req_one (
        uint16_t                    chan_id
        , bool                      reserve
        )
{
    // check if we can make a reservation
    if ( reserve
            && 0 != _fifo_capacity
            && (_reservations_by_id [ chan_id ]
                + _chan_rdata_by_id [ chan_id ]->size()
                >= _fifo_capacity )
       )
    { return false; }

    // check if there is a pending request in that channel
    auto & chan_req     = _chan_req_by_id [ chan_id ];
    if ( chan_req->empty () )
        return false;

    // find a memory port whose req_buf is empty
    auto port = _mem_ports.begin ();
    auto rob = _rob.begin ();
    for ( ; _mem_ports.end () != port; ++port, ++rob )
        if ( nullptr == port->req_buf )
            break;
    if ( _mem_ports.end () == port )
        return false;

    // get duet request
    auto req = chan_req->front ();

    // translate address
    Addr paddr;
    panic_if ( !_process->pTable->translate ( req.addr, paddr ),
            "Memory translation failed" );

    // get the data channel in case we need it 
    auto & chan_data    = _chan_wdata_by_id [chan_id];

    // create packets
    RequestPtr gem5req = nullptr;
    PacketPtr pkt = nullptr;
    switch ( req.type ) {
    case DuetFunctor::REQTYPE_LD:
        if ( _writeclean ) {
            gem5req = std::make_shared <Request> (
                    paddr & ~(Addr(_system->cacheLineSize()-1)),
                    _system->cacheLineSize (),
                    Request::ARCH_BITS & (Request::FlagsType) chan_id,
                    _requestorId
                    );
            pkt = new Packet ( gem5req, MemCmd::ReadExReq );
        } else {
            gem5req = std::make_shared <Request> (
                    paddr,
                    req.size,
                    Request::ARCH_BITS & (Request::FlagsType) chan_id,
                    _requestorId
                    );
            pkt = new Packet ( gem5req, Packet::makeReadCmd ( gem5req ) );
        }
        pkt->allocate ();
        break;

    case DuetFunctor::REQTYPE_ST:
        if ( chan_data->empty () )  // data not ready
            return false;

        gem5req = std::make_shared <Request> (
                paddr,
                req.size,
                Request::ARCH_BITS & (Request::FlagsType) chan_id,
                _requestorId
                );
        if ( _writeclean )
            pkt = new Packet ( gem5req, MemCmd::WriteClean );
        else
            pkt = new Packet ( gem5req, Packet::makeWriteCmd ( gem5req ) );
        pkt->allocate ();
        pkt->setData ( chan_data->front().get() );
        chan_data->pop_front ();
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

    // buffer packet for sending
    port->req_buf = pkt;
    DPRINTF ( DuetEngine, "Send REQ %s @CHAN %u\n",
            pkt->print (), chan_id );

    // register in reorder buffer
    rob->emplace_back ( chan_id, pkt, req.size,
            pkt->cmd.needsResponse () ? ROBEntry::SENT
            : ROBEntry::RESPONDED );

    // pop channels
    chan_req->pop_front ();
    ++_reservations_by_id[chan_id];
    return true;
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

template <>
void DuetEngine::set_constant <uint64_t> (
        std::string                 key
        , const uint64_t          & value
        )
{
    auto ret = _constants.emplace ( key, value );
    if ( !ret.second ) {
        ret.first->second = value;
    }
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
        _constants_per_caller.emplace_back ( new decltype (_constants) () );
    }

    for ( DuetFunctor::caller_id_t i = 0; i < get_num_memory_chans (); ++i) {
        _reservations_by_id.emplace_back ( 0 );
        _chan_req_by_id.emplace_back   ( new DuetFunctor::chan_req_t () );
        _chan_wdata_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
        _chan_rdata_by_id.emplace_back ( new DuetFunctor::chan_data_t () );
    }

    for ( DuetFunctor::caller_id_t i = 0; i < get_num_interlane_chans (); ++i)
        _chan_int_by_id.emplace_back   ( new DuetFunctor::chan_data_t () );

    _rob.resize ( _mem_ports.size() );

    _is_blocked     = false;
    _blocked_from   = Cycles (0);
    _is_busy        = false;
    _busy_from      = Cycles (0);
    _received.resize ( get_num_callers () );
    _started.resize ( get_num_callers () );
}

template <>
uint64_t DuetEngine::get_constant <uint64_t> (
        DuetFunctor::caller_id_t    caller_id
        , std::string               key
        ) const
{
    auto it = _constants_per_caller [caller_id]->find ( key );

    if ( _constants_per_caller [caller_id]->end () != it )
        return it->second;

    auto jt = _constants.find ( key );

    if ( _constants.end () != jt )
        return jt->second;
    else
        return 0;
}

template <>
void DuetEngine::set_constant <uint64_t> (
        DuetFunctor::caller_id_t    caller_id
        , std::string               key
        , const uint64_t            & value
        )
{
    auto ret = _constants_per_caller [caller_id]->emplace ( key, value );
    if ( !ret.second ) {
        ret.first->second = value;
    }
}

}   // namespace duet
}   // namespace gem5
