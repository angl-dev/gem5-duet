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
    addr_t addr;
    dequeue_data ( *chan_arg, addr );

    // send load request
    enqueue_req ( *chan_req, REQTYPE_LD, sizeof (uint64_t), addr );

    // get response data
    uint64_t data;
    dequeue_data ( *chan_rdata, data );

    // do some computation
    data += 1;

    // send store request
    enqueue_data ( *chan_wdata, data );
    enqueue_req ( *chan_req, REQTYPE_ST, sizeof (uint64_t), addr );
}

}   // namespace duet
}   // namespace gem5
