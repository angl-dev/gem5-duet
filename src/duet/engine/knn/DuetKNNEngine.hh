#ifndef __DUET_KNN_ENGINE_HH
#define __DUET_KNN_ENGINE_HH

#include "params/DuetKNNEngine.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

class DuetKNNEngine : public DuetEngine {
private:
    static const constexpr softreg_id_t num_softreg_per_caller  = 16;

public:
    DuetKNNEngine ( const DuetKNNEngineParams & p )
        : DuetEngine ( p )
    {}

protected:
    softreg_id_t             get_num_softregs ()        const override final;
    DuetFunctor::caller_id_t get_num_memory_chans ()    const override final;
    DuetFunctor::caller_id_t get_num_interlane_chans () const override final;
    unsigned                 get_max_stats_waittime ()  const override final;
    unsigned                 get_max_stats_exectime ()  const override final;

    bool handle_softreg_write (
            softreg_id_t                softreg_id
            , uint64_t                  value
            ) override final;
    bool handle_softreg_read (
            softreg_id_t                softreg_id
            , uint64_t                & value
            ) override final;
    void try_send_mem_req_all () override final;
    void init () override final;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_KNN_ENGINE_HH */
