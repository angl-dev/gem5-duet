#ifndef __DUET_BARNES_ACCUMULATOR_LANE_HH
#define __DUET_BARNES_ACCUMULATOR_LANE_HH

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetKNNReductionLane : public DuetSimpleLane {
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetKNNReductionLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_BARNES_ACCUMULATOR_LANE_HH */
