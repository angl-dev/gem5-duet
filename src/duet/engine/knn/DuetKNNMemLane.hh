#ifndef __DUET_BARNES_QUAD_MEM_LANE_HH
#define __DUET_BARNES_QUAD_MEM_LANE_HH

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetKNNMemLane : public DuetSimpleLane {
private:
    DuetFunctor::caller_id_t    _next_caller_roundrobin;

protected:
    DuetFunctor * new_functor () override final;

public:
    DuetKNNMemLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_BARNES_QUAD_MEM_LANE_HH */
