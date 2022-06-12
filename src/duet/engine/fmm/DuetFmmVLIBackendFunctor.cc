#include "duet/engine/fmm/DuetFmmVLIBackendFunctor.hh"
#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetFmmVLIBackendFunctor::setup () {
    {
        chan_id_t id = { chan_id_t::PULL, 2 };
        _chan_arg = &get_chan_data ( id );
    }

    {
        chan_id_t id = { chan_id_t::PULL, 3 };
        _chan_input = &get_chan_data ( id );
    }

    {
        chan_id_t id = { chan_id_t::REQ, 1 };
        _chan_req = &get_chan_req ( id );
    }

    {
        chan_id_t id = { chan_id_t::WDATA, 1 };
        _chan_wdata = &get_chan_data ( id );
    }

    {
        chan_id_t id = { chan_id_t::RDATA, 1 };
        _chan_rdata = &get_chan_data ( id );
    }

    _expansion_terms = lane->get_engine()->template get_constant <S64> (
            caller_id, "expansion_terms" );
    _cost = lane->get_engine()->template get_constant <S64> (
            caller_id, "cost" );
}

void DuetFmmVLIBackendFunctor::run () {
    kernel (
            *_chan_arg
            , *_chan_input
            , *_chan_req
            , *_chan_wdata
            , *_chan_rdata
            , _expansion_terms
            , _cost
           );

void DuetFmmVLIBackendFunctor::finishup () {
    lane->get_engine()->template set_constant <S64> (
            caller_id, "cost", _cost );

    uint64_t cnt = lane->get_engine()->template get_constant <uint64_t> ( caller_id, "cnt" );
    lane->get_engine()->template set_constant <uint64_t> ( caller_id, "cnt", ++cnt );

    lane->get_engine()->stats_exec_done ( caller_id );
}

}

}   // namespace duet
}   // namespace gem5
