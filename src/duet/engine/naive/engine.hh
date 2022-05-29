#ifndef __DUET_NAIVE_ENGINE_HH
#define __DUET_NAIVE_ENGINE_HH

#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

class NaiveEngine : public DuetEngine {
public:
    NaiveEngine ( const DuetEngineParams & p );

protected:
    softreg_id_t _get_num_softregs () const override final;
    bool _handle_softreg_write (
            softreg_id_t                softreg_id
            , uint64_t                  value
            ) override final;
    bool _handle_softreg_read (
            softreg_id_t                softreg_id
            , uint64_t                & value
            ) override final;
    void _try_send_mem_req_all () override final;
    bool _try_recv_mem_resp_one (
            uint16_t                    chan_id
            , DuetFunctor::raw_data_t   data
            ) override final;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NAIVE_ENGINE_HH */
