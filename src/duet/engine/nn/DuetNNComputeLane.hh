#pragma once

#include "duet/engine/DuetPipelinedLane.hh"

namespace gem5 {
namespace duet {

class DuetNNComputeLane : public DuetPipelinedLane
{
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetNNComputeLane ( const DuetPipelinedLaneParams & p );
};

}   // namespace duet
}   // namespace gem5
