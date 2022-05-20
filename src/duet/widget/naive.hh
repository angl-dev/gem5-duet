#ifndef __DUET_WIDGET_NAIVE_HH
#define __DUET_WIDGET_NAIVE_HH

#include "duet/widget/functor_tmpl.hh"
#include "duet/widget/widget.hh"
#include "params/DuetWidgetNaive.hh"

namespace gem5 {
namespace duet {

class DuetWidgetNaive : public DuetWidget {
private:
    class Functor : public DuetWidgetFunctorTmpl<> {
    public:
        void kernel (
                addr_t                  arg
                , chan_req_header_t *   chan_req_header
                , chan_req_data_t *     chan_req_data
                , chan_resp_data_t *    chan_resp_data
                , retcode_t *           retcode
                ) override;
    };

protected:
    AbstractDuetWidgetFunctor * _new_functor () override;

public:
    DuetWidgetNaive ( const DuetWidgetNaiveParams& p )
        : DuetWidget ( p )
    {}
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_WIDGET_NAIVE_FUNCTOR_HH */
