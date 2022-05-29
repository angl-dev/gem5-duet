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
    DuetEngine        * _engine;
    std::map <std::pair <DuetFunctor::stage_t, DuetFunctor::stage_t>
        , Cycles>       _transition_latency;

// ===========================================================================
// == API for subclesses =====================================================
// ===========================================================================
public:
    DuetLane ( const DuetLaneParams & p );

protected:
    Cycles _get_latency (
            DuetFunctor::stage_t    prev
            , DuetFunctor::stage_t  next
            );

// ===========================================================================
// == API for DuetFunctor ====================================================
// ===========================================================================
public:
    DuetFunctor::chan_req_t & get_chan_req (
            DuetFunctor::chan_id_t      chan_id
            );

    DuetFunctor::chan_data_t & get_chan_data (
            DuetFunctor::chan_id_t      chan_id
            );

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

    void set_engine ( DuetEngine * engine ) { _engine = engine; }
};

}   // namespace gem5
}   // namespace duet

#endif /* #ifndef __DUET_LANE_HH */
