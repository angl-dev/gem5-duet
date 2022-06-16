#ifndef __DUET_FMM_VLI_ENGINE_HH
#define __DUET_FMM_VLI_ENGINE_HH

#include "params/DuetFmmVLIEngine.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

/*
 * Softregs:
 *  0: "expansion_terms"
 *  1: undefined
 *  2: undefined
 *  3: undefined
 *
 *  4: arg for caller 0
 *  5: "cnt" for caller 0
 *  6: "cost" for caller 0
 *  7: undefined
 *
 *  ...
 *
 * channels:
 *  { ARG, 0 }:         arguments (2 per call)
 *  { REQ, 0 }:         frontend load request
 *  { RDATA, 0 }:       frontend load response
 *  { REQ, 1 }:         backend load/store request
 *  { RDATA, 1 }:       backend load response
 *  { WDATA, 1 }:       backend store request
 *  { PUSH/PULL, 0 }:   caller ID passing from frontend to compute
 *  { PUSH/PULL, 1 }:   caller ID passing from frontend to backend
 *  { PUSH/PULL, 2 }:   argument forwarding from frontend to backend
 *  { PUSH/PULL, 3 }:   data from compute to backend
 */
class DuetFmmVLIEngine : public DuetEngine {
private:
    static const constexpr softreg_id_t num_softreg_per_caller  = 4;
    std::vector <bool>      _argchan_got_one;

public:
    DuetFmmVLIEngine ( const DuetFmmVLIEngineParams & p )
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

#endif /* #ifndef __DUET_FMM_VLI_ENGINE_HH */
