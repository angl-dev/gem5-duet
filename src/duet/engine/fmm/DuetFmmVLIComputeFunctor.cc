#include "duet/engine/fmm/DuetFmmVLIComputeFunctor.hh"
#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetFmmVLIComputeFunctor::setup () {
    {
        chan_id_t id = { chan_id_t::RDATA, 0 };
        _chan_input = &get_chan_data ( id );
    }

    {
        chan_id_t id = { chan_id_t::PUSH, 3 };
        _chan_output = &get_chan_data ( id );
    }

    _expansion_terms = lane->get_engine()->template get_constant <S64> (
            caller_id, "expansion_terms" );
}

void DuetFmmVLIComputeFunctor::run () {
    kernel ( *_chan_input, *_chan_output, _expansion_terms );
}

}   // namespace duet
}   // namespace gem5
