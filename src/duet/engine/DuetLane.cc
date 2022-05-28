#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetLane::DuetLane ( const DuetLaneParms & p )
    : DuetClockedObject ( p )
    , _engine           ( p.engine )
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

Cycles DuetLane::_get_latency (
        DuetFunctor::stage_t    prev
        , DuetFunctor::stage_t  next
        )
{
    auto key = std::make_pair ( prev, next );
    auto it = _transition_latency.find ( key );

    panic_if ( _transition_latency.end() == it,
            "No latency assigned for transition from stage %u to %u",
            prev, next );

    return it->second;
}

DuetFunctor::chan_req_t & DuetLane::get_chan_req (
        DuetFunctor::caller_id      caller_id
        , DuetFunctor::chan_id_t    chan_id
        )
{
    return _engine->get_chan_req ( caller_id, chan_id );
}

DuetFunctor::chan_data_t & DuetLane::get_chan_data (
        DuetFunctor::caller_id      caller_id
        , DuetFunctor::chan_id_t    chan_id
        )
{
    return _engine->get_chan_data ( caller_id, chan_id );
}

}   // namespace duet
}   // namespace gem5
