#pragma once

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetNNStAckDrainLane : public DuetSimpleLane
{
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetNNStAckDrainLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5

