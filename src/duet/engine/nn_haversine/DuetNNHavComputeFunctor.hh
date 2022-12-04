#pragma once

// #include <iostream>

#ifdef __DUET_HLS
#include "hls/functor.hh"

#define sqrt(x) (x).sqrt<AC_RND_CONV, true>()
#else /* #ifdef __DUET_HLS */
#include <math.h>

#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

#define TO_RADIANS(x) ((x)*M_PI / 180.0)

class DuetNNHavComputeFunctor : public DuetFunctor {
 public:
#pragma hls_design top
  void kernel(ac_channel<Block<64>>& chan_input,
              ac_channel<Block<32>>& chan_output, const Double& pos0x_ci,
              const Double& pos0y_ci) {
    // take in constant arguments
    const Double lat2 = pos0x_ci;
    const Double lon2 = pos0y_ci;
    const Double lat2_rad = TO_RADIANS(lat2);
    // const Double lon2_rad = TO_RADIANS(lon2);

    // pop loaded data
    Double pos[4][2];

    {
      Block<64> tmp;
      dequeue_data(chan_input, tmp);
      unpack(tmp, 0, pos[0][0]);
      unpack(tmp, 1, pos[0][1]);
      unpack(tmp, 2, pos[1][0]);
      unpack(tmp, 3, pos[1][1]);
      unpack(tmp, 4, pos[2][0]);
      unpack(tmp, 5, pos[2][1]);
      unpack(tmp, 6, pos[3][0]);
      unpack(tmp, 7, pos[3][1]);
    }

    Double dists[4];

#pragma unroll yes
    for (int i = 0; i < 4; ++i) {
      const Double lat1 = pos[i][0];
      const Double lon1 = pos[i][1];

      const Double dLat = TO_RADIANS((lat2 - lat1));
      const Double dLon = TO_RADIANS((lon2 - lon1));
      const Double lat1_rad = TO_RADIANS(lat1);

      Double a = pow(sin(dLat / 2), 2) +
                 pow(sin(dLon / 2), 2) * cos(lat1_rad) * cos(lat2_rad);
      constexpr double rad = 6371.0;
      Double c = 2 * asin(sqrt(a));
      dists[i] = rad * c;
    }

    // std::cout << "dists[0]: " << pos[0][0] << ": " << dists[0] << std::endl;
    // std::cout << "dists[1]: " << pos[1][0] << ": " << dists[1] << std::endl;
    // std::cout << "dists[2]: " << pos[2][0] << ": " << dists[2] << std::endl;
    // std::cout << "dists[3]: " << pos[3][0] << ": " << dists[3] << std::endl;

    // send 1 32-byte data (4 doubles)
    {
      Block<32> tmp;
      pack(tmp, 0, dists[0]);
      pack(tmp, 1, dists[1]);
      pack(tmp, 2, dists[2]);
      pack(tmp, 3, dists[3]);
      enqueue_data(chan_output, tmp);
    }
  }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_input;
  chan_data_t* _chan_output;

  Double _pos0x_ci;
  Double _pos0y_ci;

 protected:
  void run() override final;

 public:
  DuetNNHavComputeFunctor(DuetLane* lane, caller_id_t caller_id)
      : DuetFunctor(lane, caller_id) {}

  void setup() override final;
#endif
};

#ifndef __DUET_HLS
}  // namespace gem5
}  // namespace duet
#endif /* #ifndef __DUET_HLS */
