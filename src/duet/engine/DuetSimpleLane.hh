#ifndef __DUET_SIMPLE_LANE_HH
#define __DUET_SIMPLE_LANE_HH

#include "params/DuetSimpleLane.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {

class DuetSimpleLane : public DuetLane {
protected:
    std::unique_ptr <DuetFunctor>   _functor;
    Cycles                          _remaining;

    std::map <std::pair <DuetFunctor::stage_t, DuetFunctor::stage_t>
        , Cycles>       _transition_latency;

// ===========================================================================
// == API for subclesses =====================================================
// ===========================================================================
public:
    DuetSimpleLane ( const DuetSimpleLaneParams & p );

protected:
    Cycles get_latency (
            DuetFunctor::stage_t    prev
            , DuetFunctor::stage_t  next
            );

// ===========================================================================
// == Implementing virtual methods ===========================================
// ===========================================================================
public:
    void pull_phase () override final;
    void push_phase () override final;
    bool has_work () override final;

// ===========================================================================
// == Internal ===============================================================
// ===========================================================================
private:
    void _advance ();
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_SIMPLE_LANE_HH */
