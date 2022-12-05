#include "duet/engine/nn/DuetNNReductionFunctor.hh"

#include <algorithm>
#include <array>
#include <iostream>

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNReductionFunctor::setup() {
  chan_id_t id = {chan_id_t::PULL, 0};
  _chan_input = &get_chan_data(id);

  id.tag = chan_id_t::REQ;
  _chan_req = &get_chan_req(id);

  id.tag = chan_id_t::WDATA;
  _chan_wdata = &get_chan_data(id);

  _out_addr = lane->get_engine()->template get_constant<uint64_t>(caller_id,
                                                                  "out_addr");
}

void DuetNNReductionFunctor::run() {
  std::cout << "void DuetNNReductionFunctor::run()" << std::endl;

  // Double sorted[32];
  std::array<Double, 32> sorted;

  std::cout << "111111111111111111111111111" << std::endl;

  for (int i = 0; i < 16; ++i) {
    Block<16> tmp;
    dequeue_data(*_chan_input, tmp);  // 0

    std::cout << "2222222222222222222222" << std::endl;

    // #pragma unroll yes
    for (int j = 0; j < 2; ++j) {
      Double din;
      unpack(tmp, j, din);
      sorted[i * 2 + j] = din;
    }

    std::cout << "33333333333333333333333" << std::endl;
  }

  std::sort(sorted.begin(), sorted.end());

  for (int i = 0; i < 32; ++i) {
    std::cout << i << ": " << sorted[i] << std::endl;
  }

  Block<64> tmp;
  for (int i = 0; i < 8; ++i) {
    pack(tmp, i, sorted[i]);
  }
  enqueue_data(*_chan_wdata, tmp);                     // 1
  enqueue_req(*_chan_req, REQTYPE_ST, 64, _out_addr);  // 2

  Block<64> tmp2;
  for (int i = 0; i < 8; ++i) {
    pack(tmp2, i, sorted[8 + i]);
  }
  enqueue_data(*_chan_wdata, tmp2);                             // 3
  enqueue_req(*_chan_req, REQTYPE_ST, 64, _out_addr + 1 * 64);  // 4

  Block<64> tmp3;
  for (int i = 0; i < 8; ++i) {
    pack(tmp3, i, sorted[16 + i]);
  }
  enqueue_data(*_chan_wdata, tmp3);                             // 5
  enqueue_req(*_chan_req, REQTYPE_ST, 64, _out_addr + 2 * 64);  // 6

  Block<64> tmp4;
  for (int i = 0; i < 8; ++i) {
    pack(tmp4, i, sorted[24 + i]);
  }
  enqueue_data(*_chan_wdata, tmp4);                             // 7
  enqueue_req(*_chan_req, REQTYPE_ST, 64, _out_addr + 3 * 64);  // 8

  std::cout << " ---------- _out_addr: " << _out_addr << std::endl;

  // make sure all stores are committed into the memory system
  for (int i = 0; i < 16; ++i) {
    dequeue_token(*_chan_input);
  }
}

void DuetNNReductionFunctor::finishup() {
  // lane->get_engine()->template set_constant<Double>(caller_id, "result",
  //                                                   _result);

  uint64_t cnt =
      lane->get_engine()->template get_constant<uint64_t>(caller_id, "cnt");
  lane->get_engine()->template set_constant<uint64_t>(caller_id, "cnt", ++cnt);

  lane->get_engine()->stats_exec_done(caller_id);
}

}  // namespace duet
}  // namespace gem5
