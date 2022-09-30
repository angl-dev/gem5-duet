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
        Double drsq[4] = { 0.0, 0.0, 0.0, 0.0 };

        // pop loaded data
        Double pos[4][4];

        {
            Block<64> tmp;

            // 4*8 = 32
            dequeue_data ( chan_input, tmp ); // 0
            unpack ( tmp, 0, pos[0][0] );
            unpack ( tmp, 1, pos[0][1] );
            unpack ( tmp, 2, pos[0][2] );
            unpack ( tmp, 3, pos[0][3] );
            unpack ( tmp, 4, pos[1][0] );
            unpack ( tmp, 5, pos[1][1] );
            unpack ( tmp, 6, pos[1][2] );
            unpack ( tmp, 7, pos[1][3] );

            dequeue_data ( chan_input, tmp ); // 1
            unpack ( tmp, 0, pos[2][0] );
            unpack ( tmp, 1, pos[2][1] );
            unpack ( tmp, 2, pos[2][2] );
            unpack ( tmp, 3, pos[2][3] );
            unpack ( tmp, 4, pos[3][0] );
            unpack ( tmp, 5, pos[3][1] );
            unpack ( tmp, 6, pos[3][2] );
            unpack ( tmp, 7, pos[3][3] );
        }

        Double dr[4][4];

        #pragma unroll yes
        for ( int i = 0; i < 4; ++i ) {
            #pragma unroll yes
            for ( int j = 0; j < 4; ++j ) {
                dr[i][j] = pos[i][j] - pos0[j];
                drsq[i] += dr[i][j] * dr[i][j];
            }
        }

        Double drabs[4];
        
        #pragma unroll yes
        for ( int i = 0; i < 4; ++i ) {
            drabs[i] = sqrt (drsq[i]);
        }

        {
            Block<32> tmp;
            pack ( tmp, 0, drabs[0] );
            pack ( tmp, 1, drabs[1] );
            pack ( tmp, 2, drabs[2] );
            pack ( tmp, 3, drabs[3] );
            enqueue_data ( chan_output, tmp ); // 2
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
