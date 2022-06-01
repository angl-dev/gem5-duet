#ifndef __DUET_PIPELINED_LANE_HH
#define __DUET_PIPELINED_LANE_HH

#include "params/DuetPipelinedLane.hh"
#include "duet/engine/DuetLane.hh"

namespace gem5 {
namespace duet {
    
class DuetPipelinedLane : public DuetLane {
// ===========================================================================
// == Type Definitions =======================================================
// ===========================================================================
protected:
    typedef struct _Execution {
        Cycles                          countdown;
        std::unique_ptr <DuetFunctor>   functor;

        enum { PENDING, STALL, NONSTALL, SPECULATIVE }  status;
        Cycles                          total;

        _Execution (
                Cycles          countdown
                , DuetFunctor * functor = nullptr
                )
            : countdown     ( countdown )
            , functor       ( functor )
            , status        ( PENDING )
            , total         ( 0 )
        {}

    } Execution;

// ===========================================================================
// == Parameterized Member Variables =========================================
// ===========================================================================
protected:
    Cycles                  _interval;
    std::vector <Cycles>    _latency;

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
protected:
    std::list <Execution>   _exec_list;

// ===========================================================================
// == API for subclesses =====================================================
// ===========================================================================
public:
    DuetPipelinedLane ( const DuetPipelinedLaneParams & p );

// ===========================================================================
// == Implementing virtual methods ===========================================
// ===========================================================================
public:
    void pull_phase () override final;
    void push_phase () override final;
    bool has_work () override final;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_PIPELINED_LANE_HH */
