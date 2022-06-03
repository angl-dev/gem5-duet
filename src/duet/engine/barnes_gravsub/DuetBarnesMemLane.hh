#ifndef __DUET_BARNES_MEM_LANE_HH
#define __DUET_BARNES_MEM_LANE_HH

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetBarnesMemLane : public DuetSimpleLane {
private:
    DuetFunctor::caller_id_t    _next_caller_roundrobin;

protected:
    DuetFunctor * new_functor () override final;

public:
    DuetBarnesMemLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_BARNES_MEM_LANE_HH */
