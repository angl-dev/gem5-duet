#include "duet/engine/nn_haversine/DuetNNHavEngine.hh"
namespace gem5 {
namespace duet {

DuetEngine::softreg_id_t DuetNNHavEngine::get_num_softregs() const {
  return num_softreg_per_caller * (get_num_callers() + 1);
}

DuetFunctor::caller_id_t DuetNNHavEngine::get_num_memory_chans() const {
  return 1;
}

DuetFunctor::caller_id_t DuetNNHavEngine::get_num_interlane_chans() const {
  return 3;
}

unsigned DuetNNHavEngine::get_max_stats_waittime() const { return 5000; }

unsigned DuetNNHavEngine::get_max_stats_exectime() const { return 2000; }

bool DuetNNHavEngine::handle_softreg_write(DuetEngine::softreg_id_t softreg_id,
                                           uint64_t value) {
  if (softreg_id < num_softreg_per_caller || softreg_id >= get_num_softregs())
    return true;

  // starting from 16, 16 registers per caller ( but only 9 are useful )
  DuetFunctor::caller_id_t caller_id = softreg_id / num_softreg_per_caller - 1;
  softreg_id %= num_softreg_per_caller;

  switch (softreg_id) {
    case 0:  // arg
      if (handle_argchan_push(caller_id, value)) {
        stats_call_recvd(caller_id);
        return true;
      } else {
        return false;
      }

    case 1:  // pos0x
      set_constant(caller_id, "pos0x", value);
      return true;

    case 2:  // pos0y
      set_constant(caller_id, "pos0y", value);
      return true;

    default:  // cnt, result
      return true;
  }
}

bool DuetNNHavEngine::handle_softreg_read(DuetEngine::softreg_id_t softreg_id,
                                          uint64_t& value) {
  if (softreg_id < num_softreg_per_caller || softreg_id >= get_num_softregs()) {
    value = 0;
    return true;
  }

  // starting from 16, 16 registers per caller ( but only 9 are useful )
  DuetFunctor::caller_id_t caller_id = softreg_id / num_softreg_per_caller - 1;
  softreg_id %= num_softreg_per_caller;

  switch (softreg_id) {
    case 0:  // ret
      return handle_retchan_pull(caller_id, value);

    case 1:  // pos0x
      value = get_constant<uint64_t>(caller_id, "pos0x");
      return true;

    case 2:  // pos0y
      value = get_constant<uint64_t>(caller_id, "pos0y");
      return true;

    case 3:  // cnt
      value = get_constant<uint64_t>(caller_id, "cnt");
      return true;

    case 4:  // result
      value = get_constant<uint64_t>(caller_id, "result");
      set_constant(caller_id, "result", double(999999999999.f));
      set_constant<uint64_t>(caller_id, "cnt", 0);
      return true;

    default:
      value = 0;
      return true;
  }
}

void DuetNNHavEngine::try_send_mem_req_all() { try_send_mem_req_one(0); }

void DuetNNHavEngine::init() {
  DuetEngine::init();

  for (DuetFunctor::caller_id_t caller_id = 0; caller_id < get_num_callers();
       ++caller_id) {
    set_constant<uint64_t>(caller_id, "cnt", 0);
    set_constant(caller_id, "result", double(999999.f));
  }
}

}  // namespace duet
}  // namespace gem5
