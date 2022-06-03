#include "duet/engine/barnes_gravsub/DuetBarnesAccumulatorFunctor.hh"
#include "duet/engine/DuetLane.hh"
#include "duet/engine/DuetEngine.hh"

namespace gem5 {
namespace duet {

void DuetBarnesAccumulatorFunctor::setup () {
    chan_id_t id = { chan_id_t::PULL, 0 };
    _chan_input = &get_chan_data ( id );

    _phii = lane->get_engine()->template get_constant <Double> ( caller_id, "phii" );
    _accx = lane->get_engine()->template get_constant <Double> ( caller_id, "accx" );
    _accy = lane->get_engine()->template get_constant <Double> ( caller_id, "accy" );
    _accz = lane->get_engine()->template get_constant <Double> ( caller_id, "accz" );
}

void DuetBarnesAccumulatorFunctor::run () {
    Double tmp[4];
    kernel ( *_chan_input, _phii, _accx, _accy, _accz,
            tmp[0], tmp[1], tmp[2], tmp[3] );
    _phii = tmp[0];
    _accx = tmp[1];
    _accy = tmp[2];
    _accz = tmp[3];
}

void DuetBarnesAccumulatorFunctor::finishup () {
    lane->get_engine()->template set_constant <Double> ( caller_id, "phii", _phii );
    lane->get_engine()->template set_constant <Double> ( caller_id, "accx", _accx );
    lane->get_engine()->template set_constant <Double> ( caller_id, "accy", _accy );
    lane->get_engine()->template set_constant <Double> ( caller_id, "accz", _accz );
}

}   // namespace duet
}   // namespace gem5
