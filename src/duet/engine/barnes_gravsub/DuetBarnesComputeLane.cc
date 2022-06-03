#include "duet/engine/barnes_gravsub/DuetBarnesComputeFunctor.hh"
#include "duet/engine/barnes_gravsub/DuetBarnesComputeLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetBarnesComputeLane::DuetBarnesComputeLane ( const DuetSimpleLaneParams & p )
    : DuetSimpleLane            ( p )
{}

DuetFunctor * DuetBarnesComputeLane::new_functor () {

    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::PULL, 1 };
    auto & chan = engine->get_chan_data ( id );

    if ( !chan.empty () ) {
        auto data = chan.front ();
        chan.pop_front ();

        DuetFunctor::caller_id_t caller_id;
        memcpy ( &caller_id, data.get(), sizeof (DuetFunctor::caller_id_t) );

        return new DuetBarnesComputeFunctor ( this, caller_id );
    } else {
        return nullptr;
    }
}

}   // namespace duet
}   // namespace gem5
