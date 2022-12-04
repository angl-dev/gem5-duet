#pragma once

#ifdef __DUET_HLS
#include "hls/functor.hh"

#define sqrt(x) (x).sqrt<AC_RND_CONV, true>()
#else /* #ifdef __DUET_HLS */
#include <math.h>

#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

class BatchedBarnesComputeFunctor : public DuetFunctor {
 public:
#pragma hls_design top
  void kernel(ac_channel<Block<64>>& chan_input,
              ac_channel<Block<48>>& chan_output, const Double& pos0x_ci,
              const Double& pos0y_ci, const Double& pos0z_ci,
              const Double& epssq_ci) {
    // take in constant arguments
    const Double pos0[3] = {pos0x_ci, pos0y_ci, pos0z_ci};

    // pop loaded data
    Double pos[2][3];
    Double mass[2];

    {
      Block<64> tmp;
      dequeue_data(chan_input, tmp);  // 0
      unpack(tmp, 0, pos[0][0]);
      unpack(tmp, 1, pos[0][1]);
      unpack(tmp, 2, pos[0][2]);
      unpack(tmp, 3, mass[0]);
      unpack(tmp, 4, pos[1][0]);
      unpack(tmp, 5, pos[1][1]);
      unpack(tmp, 6, pos[1][2]);
      unpack(tmp, 7, mass[1]);
    }

    Double acc[2][3];

#pragma unroll yes
    for (int i = 0; i < 2; ++i) {
      Double diff[3];
      Double drsq = epssq_ci;
      Double inv_dist = 0.0;
      Double inv_dist3 = 0.0;

      // Check if the data is meaningful. Currently we use negative
      // 'pos.data[0]' values to represent meaningless input data.
      if (pos[i][0] < 0.0) {
#pragma unroll yes
        for (int j = 0; j < 3; ++j) {
          acc[i][j] = 0.0;
        }
      } else {
#pragma unroll yes
        for (int j = 0; j < 3; ++j) {
          diff[j] = pos[i][j] - pos0[j];
          drsq += diff[j] * diff[j];
        }

        inv_dist = 1.0 / sqrt(drsq);
        inv_dist3 = inv_dist * inv_dist * inv_dist;

#pragma unroll yes
        for (int j = 0; j < 3; ++j) {
          acc[i][j] = diff[j] * inv_dist3 * mass[i];
        }
      }
    }

    // send 1 48-byte data
    {
      Block<48> tmp;
      pack(tmp, 0, acc[0][0]);
      pack(tmp, 1, acc[0][1]);
      pack(tmp, 2, acc[0][2]);
      pack(tmp, 3, acc[1][0]);
      pack(tmp, 4, acc[1][1]);
      pack(tmp, 5, acc[1][2]);
      enqueue_data(chan_output, tmp);  // 1
    }
  }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_input;
  chan_data_t* _chan_output;

  Double _pos0x_ci;
  Double _pos0y_ci;
  Double _pos0z_ci;
  Double _epssq_ci;

 protected:
  void run() override final;

 public:
  BatchedBarnesComputeFunctor(DuetLane* lane, caller_id_t caller_id)
      : DuetFunctor(lane, caller_id) {}

  void setup() override final;
#endif
};

#ifndef __DUET_HLS
}  // namespace gem5
}  // namespace duet
#endif /* #ifndef __DUET_HLS */
