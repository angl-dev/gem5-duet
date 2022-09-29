#pragma once

#ifdef __DUET_HLS
    #include "hls/functor.hh"

    #define sqrt(x) (x).sqrt<AC_RND_CONV,true>()
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    #include <math.h>

    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetNNComputeFunctor : public DuetFunctor
{
public:
    #pragma hls_design top
    void kernel (
            ac_channel <Block<64>> &    chan_input
            , ac_channel <Block<32>> &  chan_output
            , const Double &            pos0x_ci
            , const Double &            pos0y_ci
            , const Double &            pos0z_ci
            , const Double &            pos0w_ci
            )
    {
        // take in constant arguments
        const Double pos0[4] = { pos0x_ci, pos0y_ci, pos0z_ci, pos0w_ci };
        Double drsq = 0.0;

        // pop loaded data
        Double pos[4];

        {
            Block<64> tmp;

            // 4*8 = 32
            dequeue_data ( chan_input, tmp ); // 0
            unpack ( tmp, 1, pos[0] );
            unpack ( tmp, 2, pos[1] );
            unpack ( tmp, 3, pos[2] );
            unpack ( tmp, 4, pos[3] );
        }

        Double dr[4];
        #pragma unroll yes
        for ( int i = 0; i < 4; ++i ) {
            dr[i] = pos[i] - pos0[i];
            drsq += dr[i] * dr[i];
        }

        Double drabs = sqrt (drsq);

        {
            Block<32> tmp;
            pack ( tmp, 0, drabs );
            enqueue_data ( chan_output, tmp ); // 1
        }
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
    DuetNNComputeFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */
