#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetLane::DuetLane ( const DuetLaneParams & p )
    : SimObject     ( p )
    , engine        ( nullptr )
{
    panic_if ( p.transition_from_stage.size () != p.transition_to_stage.size ()
            || p.transition_to_stage.size () != p.transition_latency.size (),
            "Transition latency vectors' sizes do not match." );

    for ( size_t i = 0; i < p.transition_from_stage.size(); ++i ) {
        auto ret = _transition_latency.emplace (
                std::make_pair ( p.transition_from_stage[i], p.transition_to_stage[i] )
                , p.transition_latency[i] );

        panic_if ( !ret.second,
                "Duplicate transition found in the transition latency vectors." );
    }
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

Cycles DuetLane::get_latency (
        DuetFunctor::stage_t    from
        , DuetFunctor::stage_t  to
        ) const
{
    auto key = std::make_pair ( from, to );
    auto it = _transition_latency.find ( key );

    panic_if ( _transition_latency.end() == it,
            "No latency assigned for transition from stage %u to %u",
            from, to );

    return it->second;
}

}   // namespace duet
}   // namespace gem5
