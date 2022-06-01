#ifndef __DUET_NAIVEPPL_LANE_HH
#define __DUET_NAIVEPPL_LANE_HH

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class NaivePipelinedLane : public DuetPipelinedLane {
private:
    DuetFunctor::caller_id_t    _next_caller_roundrobin;

protected:
    DuetFunctor * new_functor () override final;

public:
    NaivePipelinedLane ( const DuetPipelinedLaneParams & p );
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_NAIVEPPL_LANE_HH */
