#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetLane::DuetLane ( const DuetLaneParams & p )
    : SimObject     ( p )
    , engine        ( nullptr )
{
}

bool DuetLane::push_default_retcode (
        DuetFunctor::caller_id_t    caller_id
        )
{
    DuetFunctor::chan_id_t id = {
        DuetFunctor::chan_id_t::RET,
        caller_id
    };

    if ( engine->can_push_to_chan ( id ) ) {
        auto retcode = DuetFunctor::RETCODE_DEFAULT;
        auto & chan = engine->get_chan_data ( id );
        auto & data = chan.emplace_back (
                new uint8_t[sizeof (DuetFunctor::retcode_t)] );
        memcpy ( data.get(), &retcode, sizeof (DuetFunctor::retcode_t) );
        return true;
    } else {
        return false;
    }
}

DuetFunctor::chan_req_t & DuetLane::get_chan_req (
        DuetFunctor::chan_id_t      chan_id
        )
{
    return engine->get_chan_req ( chan_id );
}

DuetFunctor::chan_data_t & DuetLane::get_chan_data (
        DuetFunctor::chan_id_t      chan_id
        )
{
    return engine->get_chan_data ( chan_id );
}

}   // namespace duet
}   // namespace gem5
