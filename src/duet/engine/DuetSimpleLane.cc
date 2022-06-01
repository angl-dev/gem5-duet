#include "duet/engine/DuetSimpleLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetSimpleLane::DuetSimpleLane ( const DuetSimpleLaneParams & p )
    : DuetLane      ( p )
    , _functor      ( nullptr )
    , _remaining    ( 0 )
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

void DuetSimpleLane::pull_phase () {
    // if there is a running functor, check if we can advance it
    if ( _functor ) {

        // increment progress
        if ( Cycles(0) < _remaining )
            --_remaining;

        // can we unblock?
        if ( Cycles(0) == _remaining ) {

            // if it has finished, process it in the push phase
            if ( _functor->is_done () )
                return;

            auto chan_id    = _functor->get_blocking_chan_id ();

            switch ( chan_id.tag ) {
            case DuetFunctor::chan_id_t::RDATA:
            case DuetFunctor::chan_id_t::ARG:
            case DuetFunctor::chan_id_t::PULL:
                if ( engine->can_pull_from_chan ( chan_id ) )
                    _advance ();
                break;

            case DuetFunctor::chan_id_t::REQ:
            case DuetFunctor::chan_id_t::WDATA:
            case DuetFunctor::chan_id_t::RET:
            case DuetFunctor::chan_id_t::PUSH:
                // we handle these in the push phase
                break;

            default:
                panic ( "Invalid channel ID tag" );
            }
        }
    }

    // if there is no running functor, try to start a new one
    else {
        _functor.reset ( new_functor () );
        _remaining = Cycles(0);

        if ( _functor ) {
            _functor->setup ();
            _functor->advance ();   // trivial 0->1 transition
        }
    }
}

void DuetSimpleLane::push_phase () {
    if ( !_functor || Cycles(0) < _remaining )
        return;

    // is the current execution done?
    if ( _functor->is_done () ) {

        if ( !_functor->use_default_retcode ()
                || push_default_retcode ( _functor->get_caller_id () ) )
            _functor.reset ();

    } else {

        auto chan_id    = _functor->get_blocking_chan_id ();

        switch ( chan_id.tag ) {
            case DuetFunctor::chan_id_t::RDATA:
            case DuetFunctor::chan_id_t::ARG:
            case DuetFunctor::chan_id_t::PULL:
                // we handle these in the pull phase
                break;

            case DuetFunctor::chan_id_t::REQ:
            case DuetFunctor::chan_id_t::WDATA:
            case DuetFunctor::chan_id_t::RET:
            case DuetFunctor::chan_id_t::PUSH:
                if ( engine->can_push_to_chan ( chan_id ) ) {
                    _advance ();

                    if ( _functor->is_done ()
                            && !_functor->use_default_retcode () )
                        _functor.reset ();
                }
                break;

            default:
                panic ( "Invalid channel ID tag" );
        }

    }
}

void DuetSimpleLane::_advance () {
    auto prev = _functor->get_stage ();

    if ( !_functor->advance () ) {
        auto next = _functor->get_stage ();
        auto key = std::make_pair ( prev, next );
        auto it = _transition_latency.find ( key );

        panic_if ( _transition_latency.end() == it,
                "No latency assigned for transition from stage %u to %u",
                prev, next );

        _remaining = it->second;
    } else if ( _functor->use_default_retcode () ) {
        _remaining = Cycles (1);
    } else {
        _remaining = Cycles (0);
    }
}

bool DuetSimpleLane::has_work () {
    return bool(_functor);
}

}   // namespace duet
}   // namespace gem5
