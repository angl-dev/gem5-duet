#include "duet/engine/DuetSimpleLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetSimpleLane::pull_phase () {
    // if there is a running functor, check if we can advance it
    if ( _functor && !_functor->is_done () ) {

        // increment progress
        if ( Cycles(0) < _remaining )
            --_remaining;

        // can we unblock?
        if ( Cycles(0) == _remaining ) {
            auto chan_id    = _functor->get_blocking_chan_id ();

            switch ( chan_id.tag ) {
            case DuetFunctor::chan_id_t::RDATA:
            case DuetFunctor::chan_id_t::ARG:
            case DuetFunctor::chan_id_t::PULL:
                if ( _engine->can_pull_from_chan ( chan_id ) )
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
    if ( !_functor ) {
        _functor.reset ( _new_functor () );
        _remaining = Cycles(0);

        if ( _functor ) {
            _functor->setup ();
        }
    }
}

void DuetSimpleLane::push_phase () {
    if ( !_functor )
        return;

    // is the current execution done?
    if ( _functor->is_done () ) {
        assert ( !_functor->use_explicit_retcode () );

        DuetFunctor::chan_id_t id = {
            DuetFunctor::chan_id_t::RET,
            _functor->get_caller_id ()
        };

        if ( _engine->can_push_to_chan ( id ) ) {
            auto retcode = DuetFunctor::RETCODE_DEFAULT;
            auto & chan = _engine->get_chan_data ( id );
            auto & data = chan.emplace_back (
                    new uint8_t[sizeof (DuetFunctor::retcode_t)] );
            memcpy ( data.get(), &retcode, sizeof (DuetFunctor::retcode_t) );

            _functor.reset ();
        }

    } else if ( Cycles(0) == _remaining ) {

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
                if ( _engine->can_push_to_chan ( chan_id ) ) {
                    _advance ();
                    if ( _functor->is_done ()
                            && _functor->use_explicit_retcode () )
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
        _remaining = _get_latency ( prev, next );
    }
}

bool DuetSimpleLane::has_work () {
    return bool(_functor);
}

}   // namespace duet
}   // namespace gem5
