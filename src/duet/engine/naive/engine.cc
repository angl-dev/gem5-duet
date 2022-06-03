#include "duet/engine/naive/engine.hh"
#include "duet/engine/naive/lane.hh"

namespace gem5 {
namespace duet {

DuetEngine::softreg_id_t NaiveEngine::get_num_softregs () const {
    return get_num_callers ();
}

DuetFunctor::caller_id_t NaiveEngine::get_num_memory_chans () const {
    return 1;
}

bool NaiveEngine::handle_softreg_write (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                  value
        )
{
    return handle_argchan_push ( softreg_id, value );
}

bool NaiveEngine::handle_softreg_read (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                & value
        )
{
    return handle_retchan_pull ( softreg_id, value );
}

void NaiveEngine::try_send_mem_req_all () {
    try_send_mem_req_one ( 0 );
}

}   // namespace duet
}   // namespace gem5
