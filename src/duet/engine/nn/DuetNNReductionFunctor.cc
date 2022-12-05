#include "duet/engine/nn/DuetNNReductionFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNReductionFunctor::setup() {
  {
    chan_id_t id = {chan_id_t::PULL, 0};
    _chan_input = &get_chan_data(id);
  }

  {
    chan_id_t id = {chan_id_t::REQ, 1};
    _chan_req = &get_chan_req(id);
  }

  {
    chan_id_t id = {chan_id_t::WDATA, 1};
    _chan_wdata = &get_chan_data(id);
  }

  _out_addr = lane->get_engine()->template get_constant<uint64_t>(caller_id,
                                                                  "out_addr");
}

void DuetNNReductionFunctor::run() {
  std::cout << "void DuetNNReductionFunctor::run()" << std::endl;

  // Double tmp[32];
  kernel(*_chan_input, *_chan_req, *_chan_wdata, _out_addr);
}

void DuetNNReductionFunctor::finishup() {
}

}  // namespace duet
}  // namespace gem5
