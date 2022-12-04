#include "duet/engine/batched_barnes/BatchedBarnesMemFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void BatchedBarnesMemFunctor::setup() {
  chan_id_t id = {chan_id_t::ARG, 0};
  _chan_arg = &get_chan_data(id);

  id.tag = chan_id_t::REQ;
  _chan_req = &get_chan_req(id);
}

void BatchedBarnesMemFunctor::run() { kernel(*_chan_arg, *_chan_req); }

}  // namespace duet
}  // namespace gem5
