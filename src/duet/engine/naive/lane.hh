#ifndef __DUET_NAIVE_LANE_HH
#define __DUET_NAIVE_LANE_HH

#include "duet/engine/SimpleDuetLane.hh"

namespace gem5 {
namespace duet {

class NaiveFunctor;
class NaiveLane : public SimpleDuetLane {
private:
    DuetFunctor::caller_id_t    _next_caller_roundrobin;

protected:
    DuetFunctor * _new_functor () override final;

public:
    NaiveLane ( const DuetLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_NAIVE_LANE_HH */
