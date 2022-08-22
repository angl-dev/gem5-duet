#ifndef __DUET_BARNES_ACCUMULATOR_FUNCTOR_HH
#define __DUET_BARNES_ACCUMULATOR_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    
    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetKNNReductionFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            chan_data_t &       chan_input
            , const Double &    result_ci
            ,       Double &    result_co
            )
    {
        Double ci[1] = { result_ci };
        Double tmp[1];

        dequeue_data ( chan_input, tmp[0] );
        
        // Make this min operator
        ci[0] += tmp[0];

        result_co = ci[0];
    }

#ifndef __DUET_HLS
private:
    chan_data_t   * _chan_input;

    // Minimum distance computed so far
    Double          _result;

protected:
    void run () override final;

public:
    DuetKNNReductionFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
    void finishup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_BARNES_ACCUMULATOR_FUNCTOR_HH */
