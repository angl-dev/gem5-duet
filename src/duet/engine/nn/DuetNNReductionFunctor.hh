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
            ac_channel <Block<32>> &    chan_input

            , const Double &            phii_ci
            , const Double &            accx_ci
            , const Double &            accy_ci
            , const Double &            accz_ci

            ,       Double &            phii_co
            ,       Double &            accx_co
            ,       Double &            accy_co
            ,       Double &            accz_co
            )
    {
        Double ci[4] = { phii_ci, accx_ci, accy_ci, accz_ci };

        Block<32> tmp;
        dequeue_data ( chan_input, tmp );

        #pragma unroll yes
        for ( int i = 0; i < 4; i++ ) {
            Double inc;
            unpack ( tmp, i, inc );
            ci[i] += inc;
        }

        phii_co = ci[0];
        accx_co = ci[1];
        accy_co = ci[2];
        accz_co = ci[3];
    }

#ifndef __DUET_HLS
private:
    chan_data_t   * _chan_input;

    Double          _phii;
    Double          _accx;
    Double          _accy;
    Double          _accz;

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
