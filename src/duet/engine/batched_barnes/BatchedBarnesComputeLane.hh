#pragma once

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class BatchedBarnesComputeLane : public DuetPipelinedLane {
 protected:
  DuetFunctor* new_functor() override final;

 public:
  BatchedBarnesComputeLane(const DuetPipelinedLaneParams& p);
};

}  // namespace duet
}  // namespace gem5
