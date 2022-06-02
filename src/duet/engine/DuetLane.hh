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
    friend class DuetFunctor;

    DuetEngine        * engine;

// ===========================================================================
// == API for subclesses =====================================================
// ===========================================================================
public:
    DuetLane ( const DuetLaneParams & p );

protected:
    bool push_default_retcode ( DuetFunctor::caller_id_t caller_id );

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
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_LANE_HH */
