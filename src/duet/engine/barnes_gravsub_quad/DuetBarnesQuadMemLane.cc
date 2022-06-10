#include "duet/engine/barnes_gravsub_quad/DuetBarnesQuadMemFunctor.hh"
#include "duet/engine/barnes_gravsub_quad/DuetBarnesQuadMemLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetBarnesQuadMemLane::DuetBarnesQuadMemLane ( const DuetSimpleLaneParams & p )
    : DuetSimpleLane            ( p )
    , _next_caller_roundrobin   ( 0 )
{}

DuetFunctor * DuetBarnesQuadMemLane::new_functor () {
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
            auto f = new DuetBarnesQuadMemFunctor ( this, id.id );

            // notify other lanes
            for ( DuetFunctor::caller_id_t j = 1; j <= 2; ++j ) {
                DuetFunctor::chan_id_t id2 = {
                    DuetFunctor::chan_id_t::PUSH,
                    j
                };

                auto & chan2 = engine->get_chan_data ( id2 );
                auto & data = chan2.emplace_back (
                        new uint8_t[sizeof (DuetFunctor::caller_id_t)] );
                memcpy ( data.get(), &id.id, sizeof (DuetFunctor::caller_id_t) );
            }

            return f;
        }
    }

    return nullptr;
}

}   // namespace duet
}   // namespace gem5
