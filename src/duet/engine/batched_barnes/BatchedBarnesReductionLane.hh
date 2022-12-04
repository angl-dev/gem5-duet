#pragma once

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class BatchedBarnesReductionLane : public DuetSimpleLane {
 protected:
  DuetFunctor* new_functor() override final;

 public:
  BatchedBarnesReductionLane(const DuetSimpleLaneParams& p);
};

}  // namespace duet
}  // namespace gem5