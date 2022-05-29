#include "duet/engine/naive/functor.hh"

namespace gem5 {
namespace duet {

void NaiveFunctor::setup () {

    chan_id_t id = { chan_id_t::ARG, 0 };
    chan_arg    = &get_chan_data ( id );

    id.tag = chan_id_t::REQ;
    chan_req    = &get_chan_req ( id );

    id.tag = chan_id_t::WDATA;
    chan_wdata  = &get_chan_data ( id );

    id.tag = chan_id_t::RDATA;
    chan_rdata  = &get_chan_data ( id );
}

void NaiveFunctor::run () {
    // load argument
    uintptr_t addr;
    dequeue_data ( 1, *chan_arg, addr );

    // send load request
    request_load <uint64_t> ( 2, *chan_req, addr );

    // get response data
    uint64_t data;
    dequeue_data ( 3, *chan_rdata, data );

    // do some computation
    data += 1;

    // send store request
    request_store (
            4 /* auto-generate stage 5 */,
            *chan_req, *chan_wdata,
            addr, data );
}

}   // namespace duet
}   // namespace gem5
