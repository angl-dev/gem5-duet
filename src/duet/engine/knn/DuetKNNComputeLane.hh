#ifndef __DUET_BARNES_QUAD_COMPUTE_LANE_HH
#define __DUET_BARNES_QUAD_COMPUTE_LANE_HH

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class DuetKNNComputeLane : public DuetPipelinedLane {
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetKNNComputeLane ( const DuetPipelinedLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_BARNES_QUAD_COMPUTE_LANE_HH */
