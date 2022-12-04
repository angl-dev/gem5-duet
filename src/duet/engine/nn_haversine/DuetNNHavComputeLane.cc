#include "duet/engine/nn_haversine/DuetNNHavComputeLane.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/nn_haversine/DuetNNHavComputeFunctor.hh"

namespace gem5 {
namespace duet {

DuetNNHavComputeLane::DuetNNHavComputeLane(const DuetPipelinedLaneParams& p)
    : DuetPipelinedLane(p) {}

DuetFunctor* DuetNNHavComputeLane::new_functor() {
  DuetFunctor::chan_id_t id = {DuetFunctor::chan_id_t::PULL, 1};
  auto& chan = engine->get_chan_data(id);

  if (!chan.empty()) {
    auto data = chan.front();
    chan.pop_front();

    DuetFunctor::caller_id_t caller_id;
    memcpy(&caller_id, data.get(), sizeof(DuetFunctor::caller_id_t));

    return new DuetNNHavComputeFunctor(this, caller_id);
  } else {
    return nullptr;
  }
}

}  // namespace duet
}  // namespace gem5
