#ifndef __DUET_NAIVE_ENGINE_HH
#define __DUET_NAIVE_ENGINE_HH

#include "params/NaiveEngine.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

class NaiveEngine : public DuetEngine {
public:
    NaiveEngine ( const NaiveEngineParams & p )
        : DuetEngine ( p )
    {}

protected:
    softreg_id_t             get_num_softregs ()     const override final;
    DuetFunctor::caller_id_t get_num_memory_chans () const override final;

    bool handle_softreg_write (
            softreg_id_t                softreg_id
            , uint64_t                  value
            ) override final;
    bool handle_softreg_read (
            softreg_id_t                softreg_id
            , uint64_t                & value
            ) override final;
    void try_send_mem_req_all () override final;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NAIVE_ENGINE_HH */
