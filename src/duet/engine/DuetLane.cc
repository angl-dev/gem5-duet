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

}   // namespace duet
}   // namespace gem5
