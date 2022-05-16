#include "duet/widget/widget.hh"

namespace gem5 {
namespace duet {

DuetWidget::DuetWidget ( const DuetWidgetParams & p )
    : SimObject             ( p )
    , _fifo_capacity        ( p.fifo_capacity )
    , _latency_per_stage    ( p.latency_per_stage )
    , _interval_per_stage   ( p.interval_per_stage )
{}

Cycles DuetWidget::_get_latency ( int stage ) const {
    assert ( stage >= 0 );

    panic_if ( stage >= _latency_per_stage.size(),
            "Latency not specified for stage %d",
            stage );

    return _latency_per_stage [stage];
}

Cycles DuetWidget::_get_interval ( int stage ) const {
    assert ( stage >= 0 );

    panic_if ( stage >= _interval_per_stage.size(),
            "Initiation interval not specified for stage %d",
            stage );

    return _interval_per_stage [stage];
}

bool DuetWidget::invoke ( const uintptr_t & arg ) {
    if ( _exec_list_per_stage.empty() )
        _exec_list_per_stage.emplace_back ();

    // get stage 0
    auto & stage = _exec_list_per_stage [0];

    // get the latest execution
    if ( !stage.empty() ) {
        // get the latency and interval of stage 0
        Cycles latency = _get_latency (0);
        Cycles interval = _get_interval (0);

        if ( interval == 0 )
            // stage 0 does not allow multi-entry
            return false;

        const auto & exec = stage.back ();
        if ( latency - interval < exec->get_remaining() )
            return false;
    }

    // start a new execution
    stage.emplace_back ( new Execution (this, arg) );
}

void DuetWidget::advance () {

    // iterate through stages in reverse order
    for ( int stage = _exec_list_per_stage.size() - 1;
            stage >= 0;
            --stage )
    {
        bool stall = false;
        auto & exec_list = _exec_list_per_stage [stage];

        // iterate through each execution in a stage in normal order
        for ( auto it_exec = exec_list.begin ();
                it_exec != exec_list.end ();
                // no auto increment since we might be modifying the list while
                // iterating
            )
        {
            auto & exec = *it_exec;

            // check remaining cycles
            if ( exec->get_remaining () > Cycles (1) ) {
                exec->decrement_remaining ();
                ++it_exec;
                continue;
            }

            // time to advance kernel
            auto functor = exec->get_functor ();

            // check the blocker of the execution
            switch ( functor->get_blocker () ) {
            case AbstractDuetWidgetFunctor::BLOCKER_INV :
                {
                    panic ( "Invalid blocker" );
                    break;
                }

            case AbstractDuetWidgetFunctor::BLOCKER_REQ_HEADER :
                {
                    auto & chan = functor->get_blocking_chan_req_header (); 

                    // see if the FIFO can take the request
                    if ( chan.size() < _fifo_capacity ) {
                        // yes
                        functor->advance ();

                        // move the execution to the correct stage
                        if ( functor->get_stage () < stage ) {
                            // feedback loop. Only allowed if all in-between
                            // stages are single-entry
                            for ( auto s = functor->get_stage ();
                                    s < stage;
                                    s++ )
                            {
                                panic_if ( _get_interval (s) != 0,
                                        "Jumping back from stage. %d to %d, but %d is multi-entry",
                                        stage, functor->get_stage (), s );
                            }

                            // TODO
                        }

                    } else {
                        // no. stall
                        stall = true;
                    }

                    break;
                }
            }

            if ( 
        }
    }
}

}   // namespace duet
}   // namespace gem5
