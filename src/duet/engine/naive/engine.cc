#include "duet/engine/naive/engine.hh"
#include "duet/engine/naive/lane.hh"

namespace gem5 {
namespace duet {

DuetEngine::softreg_id_t NaiveEngine::get_num_softregs () const {
    return get_num_callers ();
}

DuetFunctor::caller_id_t NaiveEngine::get_num_memory_chans () const {
    return 1;
}

bool NaiveEngine::handle_softreg_write (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                  value
        )
{
    return handle_argchan_push ( softreg_id, value );
}

bool NaiveEngine::handle_softreg_read (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                & value
        )
{
    return handle_retchan_pull ( softreg_id, value );
}

void NaiveEngine::try_send_mem_req_all () {
    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::REQ, 0 };
    auto & chan = get_chan_req ( id );
    if ( chan.empty () )
        return;

    auto req = chan.front ();
    DuetFunctor::raw_data_t data;

    switch ( req.type ) {
    case DuetFunctor::REQTYPE_LD:
        if ( try_send_mem_req_one ( 0, req, data ) )
            chan.pop_front ();
        break;

    case DuetFunctor::REQTYPE_ST:
        {
            id.tag = DuetFunctor::chan_id_t::WDATA;
            auto & datachan = get_chan_data ( id );
            if ( datachan.empty () )
                return;

            data = datachan.front ();
            if ( try_send_mem_req_one ( 0, req, data ) ) {
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

}   // namespace duet
}   // namespace gem5
