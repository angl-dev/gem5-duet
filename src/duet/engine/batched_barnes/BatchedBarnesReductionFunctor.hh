#pragma once

#ifdef __DUET_HLS
#include "hls/functor.hh"

#else /* #ifdef __DUET_HLS */
#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

#define MIN(x, y) ((x) < (y) ? (x) : (y))

class BatchedBarnesReductionFunctor : public DuetFunctor {
 public:
#pragma hls_design top
  void kernel(ac_channel<Block<48>>& chan_input, const Double& accx_ci,
              const Double& accy_ci, const Double& accz_ci, Double& accx_co,
              Double& accy_co, Double& accz_co) {
    Double ci[3] = {accx_ci, accy_ci, accz_ci};

    // 16 times
    for (int i = 0; i < 16; ++i) {
      Block<48> tmp;  // Each tmp contains two particles.
      dequeue_data(chan_input, tmp);

#pragma unroll yes
      for (int j = 0; j < 3; j++) {
        Double inc;
        Double inc2;
        unpack(tmp, j, inc);
        unpack(tmp, j + 3, inc2);
        ci[j] += inc + inc2;
      }
    }

    accx_co = ci[0];
    accy_co = ci[1];
    accz_co = ci[2];
  }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_input;

  Double _accx;
  Double _accy;
  Double _accz;

 protected:
  void run() override final;

 public:
  BatchedBarnesReductionFunctor(DuetLane* lane, caller_id_t caller_id)
      : DuetFunctor(lane, caller_id) {}

  void setup() override final;
  void finishup() override final;
#endif
};

#ifndef __DUET_HLS
}  // namespace gem5
}  // namespace duet
#endif /* #ifndef __DUET_HLS */
