#pragma once

#ifdef __DUET_HLS
#include "hls/functor.hh"

#else /* #ifdef __DUET_HLS */
#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

#define MIN(x,y) ((x) < (y) ? (x) : (y))

class DuetNNReductionFunctor : public DuetFunctor {
 public:
// #pragma hls_design top
//   void kernel(
//           ac_channel<Block<16>>& chan_input,
//           const Double& result_ci,
//           Double& result_co)
//   {
//     Double min_[2] = {result_ci, result_ci};

//     // 16 times
//     for (int i = 0; i < 16; ++i ) {
//         Block<16> tmp;
//         dequeue_data ( chan_input, tmp );

//         #pragma unroll yes
//         for ( int j = 0; j < 2; ++j ) {
//             Double din;
//             unpack ( tmp, j, din );
//             min_[j] = MIN (min_[j], din);
//         }
//     }

//     result_co = MIN (min_[0], min_[1]);
//   }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_input;
  chan_req_t * _chan_req;
  chan_data_t* _chan_wdata;
  uint64_t _out_addr;
  // Double _result;

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
