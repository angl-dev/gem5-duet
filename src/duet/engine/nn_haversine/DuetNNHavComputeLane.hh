#pragma once

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class DuetNNHavComputeLane : public DuetPipelinedLane {
 protected:
  DuetFunctor* new_functor() override final;

 public:
  DuetNNHavComputeLane(const DuetPipelinedLaneParams& p);
};

}  // namespace duet
}  // namespace gem5
