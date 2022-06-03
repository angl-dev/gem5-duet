#ifndef __DUET_LANE_HH
#define __DUET_LANE_HH

#include <map>
#include <utility>

#include "params/DuetLane.hh"
#include "sim/sim_object.hh"
#include "duet/engine/DuetFunctor.hh"

namespace gem5 {
namespace duet {

class DuetEngine;
class DuetLane : public SimObject {
// ===========================================================================
// == Parameterized Member Variables =========================================
// ===========================================================================
protected:
    DuetEngine        * engine;
    Cycles              prerun_latency;
    Cycles              postrun_latency;
    std::map <std::pair <DuetFunctor::stage_t, DuetFunctor::stage_t>
        , Cycles>       _transition_latency;

// ===========================================================================
// == API for subclesses =====================================================
// ===========================================================================
public:
    DuetLane ( const DuetLaneParams & p );

protected:
    bool push_default_retcode ( DuetFunctor::caller_id_t caller_id );
    Cycles get_latency (
            DuetFunctor::stage_t    from
            , DuetFunctor::stage_t  to
            ) const;

// ===========================================================================
// == Virtual Methods ========================================================
// ===========================================================================
protected:
    virtual DuetFunctor * new_functor () = 0;

// ===========================================================================
// == API for DuetEngine =====================================================
// ===========================================================================
public:
    /*
     * In each cycle, DuetEngine calls `pull_phase` on every lane first, then
     * `push_phase` on every lane
     */
    virtual void pull_phase () = 0;
    virtual void push_phase () = 0;

    /*
     * DuetEngine calls `has_work` to check if it can go to sleep
     */
    virtual bool has_work () = 0;

    void set_engine ( DuetEngine * e ) { engine = e; }
    DuetEngine * get_engine () const { return engine; }
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_LANE_HH */
