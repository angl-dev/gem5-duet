#include "duet/widget/naive.hh"
#include <string.h>

namespace gem5 {
namespace duet {

void DuetWidgetNaive::Functor::kernel (
        uintptr_t   arg
        , DuetWidgetNaive::Functor::chan_req_header_t * chan_req_header
        , DuetWidgetNaive::Functor::chan_req_data_t *   chan_req_data
        , DuetWidgetNaive::Functor::chan_resp_data_t *  chan_resp_data
        , DuetWidgetNaive::Functor::retcode_t *         retcode
        )
{
    // send load request
    mem_req_header_t ld = { REQTYPE_LD, 3, arg };
    enqueue_req_header ( 0, chan_req_header[0], ld );

    // receive load response
    resp_data_t resp;
    dequeue_resp_data ( 1, chan_resp_data[0], resp );

    // convert data type
    uint64_t * pdata = reinterpret_cast <uint64_t *> ( resp.get() );

    // add 1
    *pdata += 1;

    // prepare data for store request
    enqueue_req_data ( 2, chan_req_data[0], resp );

    // send store request
    mem_req_header_t st = { REQTYPE_ST, 3, arg };
    enqueue_req_header ( 3, chan_req_header[0], st );

    // mark completion
    *retcode = RETCODE_SUCCESS;
}

AbstractDuetWidgetFunctor * DuetWidgetNaive::_new_functor () {
    return new Functor ();
}

}   // namespace duet
}   // namespace gem5
