#include "debug/DuetEngine.hh"
#include "debug/DuetEngineDetailed.hh"
#include "duet/engine/DuetPipelinedLane.hh"
#include "duet/engine/DuetEngine.hh"
#include "base/trace.hh"

namespace gem5 {
namespace duet {

DuetPipelinedLane::DuetPipelinedLane ( const DuetPipelinedLaneParams & p )
    : DuetLane      ( p )
    , _interval     ( p.interval )
{}

void DuetPipelinedLane::pull_phase () {

    auto status = Execution::NONSTALL;

    // 1. process all executions
    DPRINTF ( DuetEngine, "[DuetEngine] Cycle %d\n", engine->curCycle() );

    for ( auto it = _exec_list.begin (); _exec_list.end () != it; ) {

        if ( it->functor->is_done () ) {
            DPRINTF ( DuetEngine, "  [Exec] %s, FIN:%u (%u)\n",

                    Execution::SPECULATIVE == it->status ? "speculative" :
                    Execution::PENDING == it->status ? "normal" : "[error]",

                    uint64_t (it->countdown),
                    uint64_t (it->total)
                    );
        } else {
            DPRINTF ( DuetEngine, "  [Exec] %s, S%u:%u (%u)\n",

                    Execution::SPECULATIVE == it->status ? "speculative" :
                    Execution::PENDING == it->status ? "normal" : "[error]",

                    it->functor->get_stage (),
                    uint64_t (it->countdown),
                    uint64_t (it->total)
                    );
        }

        // 1.1 if the pipeline is already stalled, mark stall
        if ( Execution::STALL == status ) {
            if ( Execution::SPECULATIVE != it->status )
                it->status = Execution::STALL;
            ++it;
            continue;
        }

        // 1.2 if the execution was speculatively advanced in the previous cycle
        // but turns out it should not have progressed, stall the pipeline
        if ( Execution::SPECULATIVE == it->status ) {
            status = Execution::STALL;
            ++it;
            continue;
        }

        assert ( Execution::PENDING == it->status );

        // 1.3 can we progress?
        if ( Cycles(0) < it->countdown ) {
            (it++)->status = status;
            continue;
        }

        // 1.4 if the execution has finished ...
        if ( it->functor->is_done () ) {
            // speculate that retcode can be pushed in the push phase in this
            // cycle
            ++it;
            status = Execution::SPECULATIVE;
            continue;
        }

        // 1.5 if we are blocked by a channel, can we unblock?
        auto chan_id = it->functor->get_blocking_chan_id ();

        switch ( chan_id.tag ) {
        case DuetFunctor::chan_id_t::RDATA:
        case DuetFunctor::chan_id_t::ARG:
        case DuetFunctor::chan_id_t::PULL:
            if ( engine->can_pull_from_chan ( chan_id ) ) {
                auto prev = it->functor->get_stage ();

                if ( it->functor->advance () ) {
                    it->functor->finishup ();

                    if ( it->functor->use_default_retcode () ) {
                        it->countdown = Cycles (1);
                        it->status = status;
                        ++it;
                    } else {
                        it = _exec_list.erase ( it );
                    }
                } else {
                    auto next = it->functor->get_stage ();
                    it->countdown = get_latency ( prev, next );
                    it->status = status;
                    ++it;
                }
            } else {
                it->status = status = Execution::STALL;
                ++it;
            }
            break;

        case DuetFunctor::chan_id_t::REQ:
        case DuetFunctor::chan_id_t::WDATA:
        case DuetFunctor::chan_id_t::RET:
        case DuetFunctor::chan_id_t::PUSH:
            // speculate that we can push in the push phase in this cycle
            status = Execution::SPECULATIVE;
            ++it;
            break;

        default:
            panic ( "Invalid channel ID tag" );
        }
    }

    // 2. check if we can initaite a new execution
    if ( Execution::STALL == status )
        return;

    else if ( !_exec_list.empty () ) {
        auto & e = _exec_list.back ();
        if ( _interval > e.total )
            return;
    }

    // yes we can. try it
    auto f = new_functor ();
    if ( nullptr != f ) {
        f->setup ();
        f->advance ();  // get to the first blocking point

        auto & e = _exec_list.emplace_back ( Cycles(1), f );
        e.status = status;
    }
}

void DuetPipelinedLane::push_phase () {

    auto status = Execution::NONSTALL;

    // 1. process all executions
    for ( auto it = _exec_list.begin (); _exec_list.end () != it; ) {

        // 1.1 if the execution is stalled during the pull phase, we are done
        if ( Execution::STALL == it->status )
            break;

        // 1.2 if the pipeline is already stalled, mark stall
        if ( Execution::STALL == status ) {
            if ( Execution::SPECULATIVE != it->status )
                it->status = Execution::STALL;
            ++it;
            continue;
        }

        // 1.3 is the countdown 0?
        if ( Cycles(0) < it->countdown ) {
            if ( Execution::SPECULATIVE == it->status ) {
                if ( Execution::NONSTALL == status ) 
                    it->status = status;
            } else {
                assert ( Execution::NONSTALL == it->status );
            }

            ++it;
            continue;
        }

        // 1.4 if the execution has finished
        if ( it->functor->is_done () ) {
            if ( push_default_retcode ( it->functor->get_caller_id () ) )
                it = _exec_list.erase ( it );
            else {
                it->status = status = Execution::STALL;
                ++it;
            }
            continue;
        }

        // 1.5 if we are blocked by a channel, can we unblock?
        auto chan_id = it->functor->get_blocking_chan_id ();

        switch ( chan_id.tag ) {
        case DuetFunctor::chan_id_t::RDATA:
        case DuetFunctor::chan_id_t::ARG:
        case DuetFunctor::chan_id_t::PULL:
            panic ( "Unprocessed read-channel block" );
            break;

        case DuetFunctor::chan_id_t::REQ:
        case DuetFunctor::chan_id_t::WDATA:
        case DuetFunctor::chan_id_t::RET:
        case DuetFunctor::chan_id_t::PUSH:
            if ( engine->can_push_to_chan ( chan_id ) ) {
                auto prev = it->functor->get_stage ();

                if ( it->functor->advance () ) {
                    it->functor->finishup ();

                    if ( it->functor->use_default_retcode () ) {
                        it->countdown = Cycles (1);
                        it->status = status;
                        ++it;
                    } else {
                        it = _exec_list.erase ( it );
                    }
                } else {
                    auto next = it->functor->get_stage ();
                    it->countdown = get_latency ( prev, next );
                    it->status = status;
                    ++it;
                }
            } else {
                it->status = status = Execution::STALL;
                ++it;
            }
            break;

        default:
            panic ( "Invalid channel ID tag" );
        }
    }

    // 2. update cycle count and status
    for ( auto it = _exec_list.begin (); _exec_list.end () != it; ++it ) {
        switch ( it->status ) {
        case Execution::PENDING:
            panic ( "A stage is still pending after all the processing!" );

        case Execution::STALL:
            it->status = Execution::PENDING;
            break;

        case Execution::NONSTALL:
            assert ( Cycles(0) < it->countdown );
            -- (it->countdown);
            ++ (it->total);
            it->status = Execution::PENDING;
            break;

        default:
            break;
        }
    }
}

bool DuetPipelinedLane::has_work () {
    return !_exec_list.empty ();
}

}   // namespace duet
}   // namespace gem5
