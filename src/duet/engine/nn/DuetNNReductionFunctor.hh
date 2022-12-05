#pragma once

#include <algorithm>
#include <array>
#include <iostream>

#ifdef __DUET_HLS
#include "hls/functor.hh"

#else /* #ifdef __DUET_HLS */
#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

#define MIN(x, y) ((x) < (y) ? (x) : (y))

class DuetNNReductionFunctor : public DuetFunctor {
 public:
#pragma hls_design top
  void kernel(ac_channel<Block<16>>& chan_input, chan_req_t& chan_req,
              ac_channel<Block<64>>& chan_wdata, const uint64_t _out_addr) {
    std::array<Double, 32> sorted;

    // std::cout << "1111111111111111111" << std::endl;

    for (int i = 0; i < 16; ++i) {
      Block<16> tmp;
      dequeue_data(chan_input, tmp);   // 0

      // std::cout << "2222222222222222222222" << std::endl;

#pragma unroll yes
      for (int j = 0; j < 2; ++j) {
        Double din;
        unpack(tmp, j, din);
        sorted[i * 2 + j] = din;
      }
    }

    std::sort(sorted.begin(), sorted.end());

    // std::cout << "33333333333333333333333" << std::endl;

    // for (int i = 0; i < 32; ++i) {
    //   std::cout << i << ": " << sorted[i] << std::endl;
    // }

    for (int i = 0; i < 4; ++i) {
        Block<64> tmp;
        for (int j = 0; j < 8; ++j) {
            pack(tmp, j, sorted[i*8+j]);
        }
        enqueue_data ( chan_wdata, tmp );                                // 1
        enqueue_req  ( chan_req,   REQTYPE_ST, 64, _out_addr + i * 64 ); // 2
    }

    /*
    Block<64> tmp;
    for (int i = 0; i < 8; ++i) {
      pack(tmp, i, sorted[i]);
    }
    enqueue_data(chan_wdata, tmp);                     // 1
    enqueue_req(chan_req, REQTYPE_ST, 64, _out_addr);  // 2

    Block<64> tmp2;
    for (int i = 0; i < 8; ++i) {
      pack(tmp2, i, sorted[8 + i]);
    }
    enqueue_data(chan_wdata, tmp2);                             // 3
    enqueue_req(chan_req, REQTYPE_ST, 64, _out_addr + 1 * 64);  // 4

    Block<64> tmp3;
    for (int i = 0; i < 8; ++i) {
      pack(tmp3, i, sorted[16 + i]);
    }
    enqueue_data(chan_wdata, tmp3);                             // 5
    enqueue_req(chan_req, REQTYPE_ST, 64, _out_addr + 2 * 64);  // 6

    Block<64> tmp4;
    for (int i = 0; i < 8; ++i) {
      pack(tmp4, i, sorted[24 + i]);
    }
    enqueue_data(chan_wdata, tmp4);                             // 7
    enqueue_req(chan_req, REQTYPE_ST, 64, _out_addr + 3 * 64);  // 8
    */
  }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_input;
  chan_req_t* _chan_req;
  chan_data_t* _chan_wdata;
  uint64_t _out_addr;
  uint64_t _result_ready;

 protected:
  void run() override final;

 public:
  DuetNNReductionFunctor(DuetLane* lane, caller_id_t caller_id)
      : DuetFunctor(lane, caller_id) {}

  void setup() override final;
  void finishup() override final;
#endif
};

#ifndef __DUET_HLS
}  // namespace gem5
}  // namespace duet
#endif /* #ifndef __DUET_HLS */
