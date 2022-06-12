#include "duet/engine/fmm/DuetFmmVLIBackendFunctor.hh"
#include "duet/engine/fmm/DuetFmmVLIBackendLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetFunctor * DuetFmmVLIBackendLane::new_functor () {

    DuetFunctor::chan_id_t id = { DuetFunctor::chan_id_t::PULL, 1 };
    auto & chan = engine->get_chan_data ( id );

    if ( !chan.empty () ) {
        auto data = chan.front ();
        chan.pop_front ();

        DuetFunctor::caller_id_t caller_id;
        memcpy ( &caller_id, data.get(), sizeof (DuetFunctor::caller_id_t) );

        return new DuetFmmVLIBackendFunctor ( this, caller_id );
    } else {
        return nullptr;
    }
}

}   // namespace duet
}   // namespace gem5
