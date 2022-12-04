#include "duet/engine/nn_haversine/DuetNNHavMemFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNHavMemFunctor::setup() {
  chan_id_t id = {chan_id_t::ARG, 0};
  _chan_arg = &get_chan_data(id);

  id.tag = chan_id_t::REQ;
  _chan_req = &get_chan_req(id);
}

void DuetNNHavMemFunctor::run() { kernel(*_chan_arg, *_chan_req); }

}  // namespace duet
}  // namespace gem5
