#include "duet/engine/nn_haversine/DuetNNHavComputeFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNHavComputeFunctor::setup() {
  chan_id_t id = {chan_id_t::RDATA, 0};
  _chan_input = &get_chan_data(id);

  id.tag = chan_id_t::PUSH;
  _chan_output = &get_chan_data(id);

  _pos0x_ci =
      lane->get_engine()->template get_constant<Double>(caller_id, "pos0x");
  _pos0y_ci =
      lane->get_engine()->template get_constant<Double>(caller_id, "pos0y");
}

void DuetNNHavComputeFunctor::run() {
  kernel(*_chan_input, *_chan_output, _pos0x_ci, _pos0y_ci);
}

}  // namespace duet
}  // namespace gem5
