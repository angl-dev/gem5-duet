#ifndef __DUET_NAIVE_FUNCTOR_HH
#define __DUET_NAIVE_FUNCTOR_HH

#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {

class NaiveFunctor : public DuetFunctor {
private:
    chan_data_t     * chan_arg;
    chan_req_t      * chan_req;
    chan_data_t     * chan_wdata;
    chan_data_t     * chan_rdata;

protected:
    void setup () override final;
    void run () override final;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NAIVE_FUNCTOR_HH */

