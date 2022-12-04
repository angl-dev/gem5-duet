#pragma once

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetNNHavReductionLane : public DuetSimpleLane {
 protected:
  DuetFunctor* new_functor() override final;

 public:
  DuetNNHavReductionLane(const DuetSimpleLaneParams& p);
};

}  // namespace duet
}  // namespace gem5