#include "duet/engine/barnes_gravsub_quad/DuetKNNComputeFunctor.hh"
#include "duet/engine/barnes_gravsub_quad/DuetKNNComputeLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetKNNComputeLane::DuetKNNComputeLane ( const DuetPipelinedLaneParams & p )
    : DuetPipelinedLane         ( p )
{}

DuetFunctor * DuetKNNComputeLane::new_functor () {

    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::PULL, 1 };
    auto & chan = engine->get_chan_data ( id );

    if ( !chan.empty () ) {
        auto data = chan.front ();
        chan.pop_front ();

        DuetFunctor::caller_id_t caller_id;
        memcpy ( &caller_id, data.get(), sizeof (DuetFunctor::caller_id_t) );

        return new DuetKNNComputeFunctor ( this, caller_id );
    } else {
        return nullptr;
    }
}

}   // namespace duet
}   // namespace gem5
