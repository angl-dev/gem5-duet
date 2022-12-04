#include "duet/engine/nn/DuetNNReductionLane.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/nn/DuetNNReductionFunctor.hh"

namespace gem5 {
namespace duet {

DuetNNReductionLane::DuetNNReductionLane ( const DuetSimpleLaneParams & p )
    : DuetSimpleLane        ( p )
{}

DuetFunctor * DuetNNReductionLane::new_functor () {

    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::PULL, 2 };
    auto & chan = engine->get_chan_data ( id );

    if ( !chan.empty () ) {
        auto data = chan.front ();
        chan.pop_front ();

        DuetFunctor::caller_id_t caller_id;
        memcpy ( &caller_id, data.get(), sizeof (DuetFunctor::caller_id_t) );

        return new DuetNNReductionFunctor ( this, caller_id );
    } else {
        return nullptr;
    }
}

}   // namespace duet
}   // namespace gem5
