#pragma once

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class BatchedBarnesMemLane : public DuetSimpleLane {
 private:
  DuetFunctor::caller_id_t _next_caller_roundrobin;

 protected:
  DuetFunctor* new_functor() override final;

 public:
  BatchedBarnesMemLane(const DuetSimpleLaneParams& p);
};

}  // namespace duet
}  // namespace gem5
