#include "duet/engine/nn/DuetNNComputeFunctor.hh"

#include "duet/engine/DuetEngine.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

void DuetNNComputeFunctor::setup () {
    chan_id_t id = { chan_id_t::RDATA, 0 };
    _chan_input = &get_chan_data ( id );

    id.tag = chan_id_t::PUSH;
    _chan_output = &get_chan_data ( id );

    _pos0x_ci = lane->get_engine()->template get_constant <Double> ( caller_id, "pos0x" );
    _pos0y_ci = lane->get_engine()->template get_constant <Double> ( caller_id, "pos0y" );
    _pos0z_ci = lane->get_engine()->template get_constant <Double> ( caller_id, "pos0z" );
    _pos0w_ci = lane->get_engine()->template get_constant <Double> ( caller_id, "pos0w" );

}

void DuetNNComputeFunctor::run () {
    kernel ( *_chan_input, *_chan_output,
            _pos0x_ci, _pos0y_ci, _pos0z_ci, _pos0w_ci );
}

}   // namespace duet
}   // namespace gem5
