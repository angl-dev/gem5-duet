#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

void DuetSimpleLane::pull_phase () {
    // if there is a running functor, check if we can advance it
    if ( _functor ) {

        // increment progress
        if ( Cycles(0) < _remaining )
            --_remaining;

        // can we unblock?
        if ( Cycles(0) == _remaining ) {
            auto caller_id  = _functor->get_caller_id ();
            auto chan_id    = _functor->get_blocking_chan_id ();

            switch ( chan_id.tag ) {
            case DuetFunctor::chan_id_t::INPUT:
                if ( _engine->can_pull_from_chan ( caller_id, chan_id ) )
                    _advance ();
                break;

            case DuetFunctor::chan_id_t::REQ:
            case DuetFunctor::chan_id_t::OUTPUT:
                // we handle these in the push phase
                break;

            default:
                panic ( "Invalid channel ID tag" );
            }
        }
    }

    // if there is no running functor, try to start a new one
    if ( !_functor ) {
        DuetFunctor::caller_id_t id;
        if ( _next_call ( id ) ) {
            _functor.reset ( _new_functor ( id ) );
            _remaining = Cycles(0);
        }
    }
}

void DuetSimpleLane::push_phase () {
    // check if we can unblock
    if ( _functor && Cycles(0) == _remaining ) {
        auto caller_id  = _functor->get_caller_id ();
        auto chan_id    = _functor->get_blocking_chan_id ();

        switch ( chan_id.tag ) {
            case DuetFunctor::chan_id_t::INPUT:
                // we handle these in the pull phase
                break;

            case DuetFunctor::chan_id_t::REQ:
            case DuetFunctor::chan_id_t::OUTPUT:
                if ( _engine->can_push_to_chan ( caller_id, chan_id ) )
                    _advance ();
                break;

            default:
                panic ( "Invalid channel ID tag" );
        }
    }
}

void DuetSimpleLane::_advance () {
    auto prev = _functor->get_stage ();

    if ( _functor->advance () ) {
        decltype (_functor) _nullptr;
        std::swap ( _functor, _nullptr );
    } else {
        auto next = _functor->get_stage ();
        _remaining = _get_latency ( prev, next );
    }
}

bool DuetSimpleLane::has_work () {
    return _functor;
}

}   // namespace duet
}   // namespace gem5
