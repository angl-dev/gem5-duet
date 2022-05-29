#include "duet/engine/naive/functor.hh"
#include "duet/engine/naive/lane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

NaiveLane::NaiveLane ( const DuetLaneParams & p )
    : DuetSimpleLane                ( p )
    , _next_caller_roundrobin       ( 0 )
{}

DuetFunctor * NaiveLane::_new_functor () {
    for ( DuetFunctor::caller_id_t i = 0;
            i < _engine->get_num_callers ();
            ++i )
    {
        DuetFunctor::chan_id_t id = {
            DuetFunctor::chan_id_t::ARG,
            _next_caller_roundrobin
        };
        auto & chan = _engine->get_chan_data ( id );

        if ( !chan.empty () ) {
            auto f = new NaiveFunctor ( this, _next_caller_roundrobin );
            ++ _next_caller_roundrobin;
            _next_caller_roundrobin %= _engine->get_num_callers ();
            return f;
        }
    }

    return nullptr;
}

}   // namespace duet
}   // namespace gem5
