#pragma once

#ifdef __DUET_HLS
#include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetNNHavMemFunctor : public DuetFunctor {
 public:
#pragma hls_design top
  void kernel(ac_channel<U64>& chan_arg, chan_req_t& chan_req) {
    addr_t nodeptr;
    dequeue_data(chan_arg, nodeptr);  // 0

    // assume >= 64B cache line size:
    // load pos[0] = +8, pos[1] = +16, pos[2] = +24, pos[] = +32

    // Loading 32 particles at once
    for (int i = 0; i < 1024; i += 64) {
      enqueue_req(chan_req, REQTYPE_LD, 64, nodeptr + i);  // 1
    }
  }

#ifndef __DUET_HLS
 private:
  chan_data_t* _chan_arg;
  chan_req_t* _chan_req;

 protected:
  void run() override final;

 public:
  DuetNNHavMemFunctor(DuetLane* lane, caller_id_t caller_id)
      : DuetFunctor(lane, caller_id) {}

  void setup() override final;
#endif
};

#ifndef __DUET_HLS
}  // namespace gem5
}  // namespace duet
#endif /* #ifndef __DUET_HLS */
