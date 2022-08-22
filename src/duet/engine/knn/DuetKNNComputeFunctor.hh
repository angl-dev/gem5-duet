#ifndef __DUET_BARNES_QUAD_COMPUTE_FUNCTOR_HH
#define __DUET_BARNES_QUAD_COMPUTE_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"

    #define sqrt(x) (x).sqrt<AC_RND_CONV,true>()
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    #include <math.h>
    
    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetKNNComputeFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            chan_data_t &       chan_input
            , chan_data_t &     chan_output
            , const Double &    pos0x_ci
            , const Double &    pos0y_ci
            , const Double &    pos0z_ci
            , const Double &    pos0w_ci
            )
    {
        // take in constant arguments
        const Double pos0[4] = { pos0x_ci, pos0y_ci, pos0z_ci, pos0z_wi };

        // pop loaded data
        Double pos[4];
        dequeue_data ( chan_input, pos[0] );
        dequeue_data ( chan_input, pos[1] );
        dequeue_data ( chan_input, pos[2] );
        dequeue_data ( chan_input, pos[3] );
        
        // KNN kernel
        Double drsq;
        Double dr[4];
        for ( int i = 0; i < 4; ++i ) {
            dr[i] = pos[i] - pos0[i];
            drsq += dr[i] * dr[i];
        }


        // Some how get the mininmum distance sofar. 
        // probabaly this should be done in Accumulator lane 
        
        // And compare that value with `drsq` like
        //  result = min(drsq, min_sofar)



        // enqueue_data ( chan_output, result );
    }

#ifndef __DUET_HLS
private:
    chan_data_t   * _chan_input;
    chan_data_t   * _chan_output;

    Double          _pos0x_ci;
    Double          _pos0y_ci;
    Double          _pos0z_ci;
    Double          _pos0w_ci;

protected:
    void run () override final;

public:
    DuetKNNComputeFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_BARNES_QUAD_COMPUTE_FUNCTOR_HH */
