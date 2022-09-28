#ifndef __DUET_NN_COMPUTE_LANE_HH
#define __DUET_NN_COMPUTE_LANE_HH

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class DuetNNComputeLane : public DuetPipelinedLane
{
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetNNComputeLane ( const DuetPipelinedLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NN_COMPUTE_LANE_HH */
