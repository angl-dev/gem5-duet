#include "duet/engine/nn/DuetNNStAckDrainFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNStAckDrainFunctor::setup() {
  {
    chan_id_t id = {chan_id_t::RDATA, 1};
    _chan_resp = &get_chan_data(id);
  }
}

void DuetNNStAckDrainFunctor::run() {
  kernel(*_chan_resp);
}

void DuetNNStAckDrainFunctor::finishup() {
  lane->get_engine()->template set_constant<uint64_t>(caller_id, "result_ready",
                                                      1);

  uint64_t cnt =
      lane->get_engine()->template get_constant<uint64_t>(caller_id, "cnt");
  lane->get_engine()->template set_constant<uint64_t>(caller_id, "cnt", ++cnt);

  lane->get_engine()->stats_exec_done(caller_id);
}

}  // namespace duet
}  // namespace gem5
