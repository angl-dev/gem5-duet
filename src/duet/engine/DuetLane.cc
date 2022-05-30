#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

DuetLane::DuetLane ( const DuetLaneParams & p )
    : SimObject     ( p )
    , engine        ( nullptr )
{
}

DuetFunctor::chan_req_t & DuetLane::get_chan_req (
        DuetFunctor::chan_id_t      chan_id
        )
{
    return engine->get_chan_req ( chan_id );
}

DuetFunctor::chan_data_t & DuetLane::get_chan_data (
        DuetFunctor::chan_id_t      chan_id
        )
{
    return engine->get_chan_data ( chan_id );
}

}   // namespace duet
}   // namespace gem5
