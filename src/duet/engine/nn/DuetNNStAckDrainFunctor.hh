#pragma once

#ifdef __DUET_HLS
#error Code not designed for HLS.
#endif /* #ifdef __DUET_HLS */

#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {

class DuetNNStAckDrainFunctor : public DuetFunctor {
public:
    void kernel (
            ac_channel<Block<64>>& chan_resp
            )
    {
        for (int i = 0; i < 4; ++i) {
            dequeue_token ( chan_resp );
        }
    }

private:
    chan_data_t * _chan_resp;

protected:
    void run() override final;

public:
    DuetNNStAckDrainFunctor(DuetLane* lane, caller_id_t caller_id)
        : DuetFunctor(lane, caller_id) {}

    void setup() override final;
    void finishup() override final;
};

}  // namespace gem5
}  // namespace duet
