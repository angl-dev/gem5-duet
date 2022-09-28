#ifndef __DUET_NN_REDUCTION_LANE_HH
#define __DUET_NN_REDUCTION_LANE_HH

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetNNReductionLane : public DuetSimpleLane
{
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetNNReductionLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NN_REDUCTION_LANE_HH */
