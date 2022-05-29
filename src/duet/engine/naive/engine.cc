#include "duet/engine/naive/engine.hh"
#include "duet/engine/naive/lane.hh"

namespace gem5 {
namespace duet {

NaiveEngine::NaiveEngine ( const DuetEngineParams & p )
    : DuetEngine ( p )
{
    for ( DuetFunctor::caller_id_t i = 0; i < _num_callers; ++i ) {
        _chan_arg_by_id.emplace_back ();
        _chan_arg_by_id[i].emplace ( 0,
                std::make_unique (new DuetFunctor::chan_data_t ()) );
    }

    _chan_req_by_id.emplace ( 0, new DuetFunctor::chan_req_t () );
    _chan_wdata_by_id.emplace ( 0, new DuetFunctor::chan_data_t () );
    _chan_rdata_by_id.emplace ( 0, new DuetFunctor::chan_data_t () );
}

DuetEngine::softreg_id_t NaiveEngine::_get_num_softregs () const {
    return _num_callers;
}

bool NaiveEngine::_handle_softreg_write (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                  value
        )
{
    auto & chan = _chan_arg_by_id[softreg_id][0];

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

bool NaiveEngine::_handle_softreg_read (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                & value
        )
{
    auto & chan = _chan_arg_by_id[softreg_id][0];

    if ( !chan->empty () ) {
        auto raw = chan->front ();
        chan->pop_front ();
        memcpy ( &value, raw.get(), 8 );
        return true;
    } else {
        return false;
    }
}

void NaiveEngine::_try_send_mem_req_all () {
    auto & chan = _chan_req_by_id [0];
    if ( chan->empty () )
        return;

    auto req = chan.front ();
    DuetFunctor::raw_data_t data;

    switch ( req.tag ) {
    case DuetFunctor::REQTYPE_LD:
        if ( _try_send_mem_req_one ( 0, req, data ) )
            chan.pop_front ();
        break;

    case DuetFunctor::REQTYPE_ST:
        {
            auto & datachan = _chan_wdata_by_id [0];
            if ( datachan.empty () )
                return;

            data = datachan.front ();
            if ( _try_send_mem_req_one ( 0, req, data ) ) {
                chan.pop_front ();
                datachan.pop_front ();
            }
        }
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
}

bool NaiveEngine::_try_recv_mem_resp_one (
        uint16_t                    chan_id
        , DuetFunctor::raw_data_t   data
        )
{
    assert ( 0 == chan_id );
    auto & chan = _chan_rdata_by_id [0];

    if ( 0 == _fifo_capacity
            || chan.size () < _fifo_capacity )
    {
        chan.push_back ( data );
        return true;
    } else {
        return false;
    }
}

}   // namespace duet
}   // namespace gem5
