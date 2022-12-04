#include "duet/engine/batched_barnes/BatchedBarnesComputeLane.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/batched_barnes/BatchedBarnesComputeFunctor.hh"

namespace gem5 {
namespace duet {

BatchedBarnesComputeLane::BatchedBarnesComputeLane(
    const DuetPipelinedLaneParams& p)
    : DuetPipelinedLane(p) {}

DuetFunctor* BatchedBarnesComputeLane::new_functor() {
  DuetFunctor::chan_id_t id = {DuetFunctor::chan_id_t::PULL, 1};
  auto& chan = engine->get_chan_data(id);

  if (!chan.empty()) {
    auto data = chan.front();
    chan.pop_front();

    DuetFunctor::caller_id_t caller_id;
    memcpy(&caller_id, data.get(), sizeof(DuetFunctor::caller_id_t));

    return new BatchedBarnesComputeFunctor(this, caller_id);
  } else {
    return nullptr;
  }
}

}  // namespace duet
}  // namespace gem5
