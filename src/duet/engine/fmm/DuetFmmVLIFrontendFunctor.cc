#include "duet/engine/fmm/DuetFmmVLIFrontendFunctor.hh"
#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetFmmVLIFrontendFunctor::setup () {
    {
        chan_id_t id = { chan_id_t::ARG, 0 };
        _chan_arg = &get_chan_data ( id );
    }

    {
        chan_id_t id = { chan_id_t::REQ, 0 };
        _chan_req = &get_chan_req ( id );
    }

    {
        chan_id_t id = { chan_id_t::PUSH, 2 };
        _chan_arg_fwd = &get_chan_data ( id );
    }

    _expansion_terms = lane->get_engine()->template get_constant <S64> (
            caller_id, "expansion_terms" );
}

void DuetFmmVLIFrontendFunctor::run () {
    kernel ( *_chan_arg, *_chan_req, *_chan_arg_fwd, _expansion_terms );
}

}   // namespace duet
}   // namespace gem5
