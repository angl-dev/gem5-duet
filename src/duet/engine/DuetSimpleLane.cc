#include "debug/DuetEngine.hh"
#include "debug/DuetEngineDetailed.hh"
#include "duet/engine/DuetSimpleLane.hh"
#include "duet/engine/DuetEngine.hh"
#include "base/trace.hh"

namespace gem5 {
namespace duet {

DuetSimpleLane::DuetSimpleLane ( const DuetSimpleLaneParams & p )
    : DuetLane      ( p )
    , _functor      ( nullptr )
    , _remaining    ( 0 )
{}

void DuetSimpleLane::pull_phase () {
    // if there is a running functor, check if we can advance it
    if ( _functor ) {

        if ( _functor->is_done () )
            DPRINTF ( DuetEngine, "Cycle %u, FIN:%u\n",
                    engine->curCycle(), _remaining );
        else
            DPRINTF ( DuetEngine, "Cycle %u, S%u:%u\n",
                    engine->curCycle(), _functor->get_stage(), _remaining );

        // increment progress
        if ( Cycles(0) < _remaining )
            --_remaining;

        // can we unblock?
        if ( Cycles(0) == _remaining ) {

            // if it has finished ...
            if ( _functor->is_done () ) {
                if ( !_functor->use_default_retcode () ) {
                    // .. and does not push retcode, finishup and reset
                    _functor->finishup ();
                    _functor.reset();
                } else {
                    // .. otherwise, process in the push phase
                    return;
                }
            } else {
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
    }

    // if there is no running functor, try to start a new one
    else {
        _functor.reset ( new_functor () );
        _remaining = prerun_latency + Cycles(1);

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
        {
            _functor->finishup ();
            _functor.reset ();
        }

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
                if ( engine->can_push_to_chan ( chan_id ) )
                    _advance ();
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
        _remaining = get_latency ( prev, next );
    } else {
        _remaining = postrun_latency + Cycles(1);
    }
}

bool DuetSimpleLane::has_work () {
    return bool(_functor);
}

}   // namespace duet
}   // namespace gem5
