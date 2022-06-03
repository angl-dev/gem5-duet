#ifndef __DUET_BARNES_COMPUTE_FUNCTOR_HH
#define __DUET_BARNES_COMPUTE_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"

    #define sqrt(x) (x).sqrt<AC_RND_CONV,true>()
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    #include <math.h>
    
    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetBarnesComputeFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            chan_data_t &       chan_input
            , chan_data_t &     chan_output
            , const Double &    pos0x_ci
            , const Double &    pos0y_ci
            , const Double &    pos0z_ci
            , const Double &    epssq_ci
            )
    {
        const Double pos0[3] = { pos0x_ci, pos0y_ci, pos0z_ci };
        Double drsq = epssq_ci;

        Double dr[3];
        for ( int i = 0; i < 3; ++i ) {
            Double tmp;
            dequeue_data ( chan_input, tmp );       // same stage!

            dr[i] = tmp - pos0[i];
            drsq += dr[i] * dr[i];
        }

        Double mass;
        dequeue_data ( chan_input, mass );

        Double drabs = sqrt (drsq);
        Double phii = mass / drabs;

        enqueue_data ( chan_output, phii );

        Double mor3 = phii / drsq;
        for ( int i = 0; i < 3; ++i ) {
            Double deltaAcc = dr[i] * mor3;
            enqueue_data ( chan_output, deltaAcc ); // same stage!
        }
    }
#ifndef __DUET_HLS
private:
    chan_data_t   * _chan_input;
    chan_data_t   * _chan_output;

    Double          _pos0x_ci;
    Double          _pos0y_ci;
    Double          _pos0z_ci;
    Double          _epssq_ci;

protected:
    void run () override final;

public:
    DuetBarnesComputeFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_BARNES_COMPUTE_FUNCTOR_HH */
