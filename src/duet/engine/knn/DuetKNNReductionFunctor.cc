#include "duet/engine/knn/DuetKNNReductionFunctor.hh"
#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetKNNReductionFunctor::setup () {
    chan_id_t id = { chan_id_t::PULL, 0 };
    _chan_input = &get_chan_data ( id );

    _result = lane->get_engine()->template get_constant <Double> ( caller_id, "result" );
}

void DuetKNNReductionFunctor::run () {
    Double tmp[1];
    kernel ( *_chan_input, _result, tmp[0] );
    _result = tmp[0];
}

void DuetKNNReductionFunctor::finishup () {
    lane->get_engine()->template set_constant <Double> ( caller_id, "result", _result );

    uint64_t cnt = lane->get_engine()->template get_constant <uint64_t> ( caller_id, "cnt" );
    lane->get_engine()->template set_constant <uint64_t> ( caller_id, "cnt", ++cnt );

    lane->get_engine()->stats_exec_done ( caller_id );
}

}   // namespace duet
}   // namespace gem5
