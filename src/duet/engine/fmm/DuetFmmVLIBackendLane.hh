#ifndef __DUET_FMM_VLI_BACKEND_LANE_HH
#define __DUET_FMM_VLI_BACKEND_LANE_HH

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetFmmVLIBackendLane : public DuetSimpleLane {
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetFmmVLIBackendLane ( const DuetSimpleLaneParams & p )
        : DuetSimpleLane ( p )
    {}
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_FMM_VLI_BACKEND_LANE_HH */
