#ifndef __DUET_WIDGET_HH
#define __DUET_WIDGET_HH

#include <vector>

#include "duet/widget/functor_abstract.hh"
#include "params/DuetWidget.hh"
#include "sim/sim_object.hh"

namespace gem5 {
namespace duet {

/*
 * Each implemented functor must subclass this class.
 */
class DuetWidget : public SimObject {
private:

    /* Class representing the execution of a functor pipeline. */ 
    class Execution {
    private:
        DuetWidget *                                _widget;
        Cycles                                      _remaining;
        std::unique_ptr <AbstractDuetWidgetFunctor> _functor;

    public:
        Execution ( DuetWidget * widget, const uintptr_t & arg );
        Cycles                      get_remaining () const;
        void                        decrement_remaining ();
        AbstractDuetWidgetFunctor * get_functor () const;
    };

private:
    unsigned                _fifo_capacity;
    std::vector <Cycles>    _latency_per_stage;
    std::vector <Cycles>    _interval_per_stage;

    std::vector <std::list <std::unique_ptr <Execution>>>
        _exec_list_per_stage;

private:
    Cycles _get_latency  ( int stage ) const;
    Cycles _get_interval ( int stage ) const;

protected:
    virtual AbstractDuetWidgetFunctor * _new_functor (
        const uintptr_t & arg ) = 0;

public:
    DuetWidget ( const DuetWidgetParams & p );

    // invoke the managed widget
    //  return false if the pipeline cannot start a new execution, e.g. due to
    //  unmet initiation interval or stalled stages
    bool invoke ( const uintptr_t & arg );

    // check if there is anythong to do in the next cycle
    bool has_work () const;

    // advance as many stages as possible
    void advance ();
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_WIDGET_HH */
