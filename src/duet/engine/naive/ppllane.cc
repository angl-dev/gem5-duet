#include "duet/engine/naive/functor.hh"
#include "duet/engine/naive/ppllane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

NaivePipelinedLane::NaivePipelinedLane ( const DuetPipelinedLaneParams & p )
    : DuetPipelinedLane         ( p )
    , _next_caller_roundrobin   ( 0 )
{}

DuetFunctor * NaivePipelinedLane::new_functor () {
    for ( DuetFunctor::caller_id_t i = 0;
            i < engine->get_num_callers ();
            ++i )
    {
        DuetFunctor::chan_id_t id = {
            DuetFunctor::chan_id_t::ARG,
            _next_caller_roundrobin
        };

        ++ _next_caller_roundrobin;
        _next_caller_roundrobin %= engine->get_num_callers ();

        auto & chan = engine->get_chan_data ( id );
        if ( !chan.empty () ) {
            auto f = new NaiveFunctor ( this, id.id );
            return f;
        }
    }

    return nullptr;
}

}   // namespace gem5
}   // namespace duet
