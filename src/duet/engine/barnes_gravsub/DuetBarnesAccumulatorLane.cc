#include "duet/engine/barnes_gravsub/DuetBarnesAccumulatorFunctor.hh"
#include "duet/engine/barnes_gravsub/DuetBarnesAccumulatorLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetBarnesAccumulatorLane::DuetBarnesAccumulatorLane ( const DuetSimpleLaneParams & p )
    : DuetSimpleLane            ( p )
{}

DuetFunctor * DuetBarnesAccumulatorLane::new_functor () {

    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::PULL, 2 };
    auto & chan = engine->get_chan_data ( id );

    if ( !chan.empty () ) {
        auto data = chan.front ();
        chan.pop_front ();

        DuetFunctor::caller_id_t caller_id;
        memcpy ( &caller_id, data.get(), sizeof (DuetFunctor::caller_id_t) );

        return new DuetBarnesAccumulatorFunctor ( this, caller_id );
    } else {
        return nullptr;
    }
}

}   // namespace duet
}   // namespace gem5
