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
    void run () override final;

public:
    NaiveFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    ~NaiveFunctor () {}

    void setup () override final;

    bool use_default_retcode () const override final { return true; }
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NAIVE_FUNCTOR_HH */

