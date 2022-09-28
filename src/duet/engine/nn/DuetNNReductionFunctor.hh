#ifndef __DUET_NN_REDUCTION_FUNCTOR_HH
#define __DUET_NN_REDUCTION_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"

    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetNNReductionFunctor : public DuetFunctor
{
public:
    #pragma hls_design top
    void kernel (
            ac_channel <Block<8>> &    chan_input
            , const Double &            result_ci
            ,       Double &            result_co
            )
    {
        Double ci[1] = { result_ci };

        Block<8> tmp;
        dequeue_data ( chan_input, tmp );

        Double min_dist;
        unpack ( tmp, i, min_dist );       
        ci[0] = ci[0] < min_dist ? ci[0] : min_dist ;

        result_co = ci[0];
    }

#ifndef __DUET_HLS
private:
    chan_data_t   * _chan_input;

    Double          _result;

protected:
    void run () override final;

public:
    DuetNNReductionFunctor ( DuetLane * lane, caller_id_t caller_id )
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

#endif /* #ifndef __DUET_NN_REDUCTION_FUNCTOR_HH */
