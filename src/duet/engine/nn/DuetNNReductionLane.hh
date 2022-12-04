#pragma once

#include "duet/engine/DuetSimpleLane.hh"

namespace gem5 {
namespace duet {

class DuetNNReductionLane : public DuetSimpleLane
{
protected:
    DuetFunctor * new_functor () override final;

public:
    DuetNNReductionLane ( const DuetSimpleLaneParams & p );
};

}   // namespace duet
}   // namespace gem5