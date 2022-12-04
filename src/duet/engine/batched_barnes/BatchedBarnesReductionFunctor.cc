#include "duet/engine/batched_barnes/BatchedBarnesReductionFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void BatchedBarnesReductionFunctor::setup() {
  chan_id_t id = {chan_id_t::PULL, 0};
  _chan_input = &get_chan_data(id);

  _accx = lane->get_engine()->template get_constant<Double>(caller_id, "accx");
  _accy = lane->get_engine()->template get_constant<Double>(caller_id, "accy");
  _accz = lane->get_engine()->template get_constant<Double>(caller_id, "accz");
}

void BatchedBarnesReductionFunctor::run() {
  Double tmp[3];
  kernel(*_chan_input, _accx, _accy, _accz, tmp[0], tmp[1], tmp[2]);
  _accx = tmp[0];
  _accy = tmp[1];
  _accz = tmp[2];
}

void BatchedBarnesReductionFunctor::finishup() {
  lane->get_engine()->template set_constant<Double>(caller_id, "accx", _accx);
  lane->get_engine()->template set_constant<Double>(caller_id, "accy", _accy);
  lane->get_engine()->template set_constant<Double>(caller_id, "accz", _accz);

  uint64_t cnt =
      lane->get_engine()->template get_constant<uint64_t>(caller_id, "cnt");
  lane->get_engine()->template set_constant<uint64_t>(caller_id, "cnt", ++cnt);

  lane->get_engine()->stats_exec_done(caller_id);
}

}  // namespace duet
}  // namespace gem5
