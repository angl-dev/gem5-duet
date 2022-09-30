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
            , ac_channel <Block<64>> &  chan_output
            , const Double &            pos0x_ci
            , const Double &            pos0y_ci
            , const Double &            pos0z_ci
            , const Double &            pos0w_ci
            )
    {
        // take in constant arguments
        const Double pos0[4] = { pos0x_ci, pos0y_ci, pos0z_ci, pos0w_ci };
        Double drsq[32] = { 0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0 };

        // pop loaded data
        Double pos[32][4];

        {
            Block<64> tmp;

            for(int i = 0; i < 32; i += 2 ) {
                dequeue_data ( chan_input, tmp ); // 0, 16 times
                unpack ( tmp, 0, pos[i][0] );
                unpack ( tmp, 1, pos[i][1] );
                unpack ( tmp, 2, pos[i][2] );
                unpack ( tmp, 3, pos[i][3] );
                unpack ( tmp, 4, pos[i+1][0] );
                unpack ( tmp, 5, pos[i+1][1] );
                unpack ( tmp, 6, pos[i+1][2] );
                unpack ( tmp, 7, pos[i+1][3] );
            }


        }

        Double dr[32][4];

        #pragma unroll yes
        for ( int i = 0; i < 32; ++i ) {
            #pragma unroll yes
            for ( int j = 0; j < 4; ++j ) {
                dr[i][j] = pos[i][j] - pos0[j];
                drsq[i] += dr[i][j] * dr[i][j];
            }
        }

        Double drabs[32];
        
        #pragma unroll yes
        for ( int i = 0; i < 32; ++i ) {
            drabs[i] = sqrt (drsq[i]);
        }

        {
            Block<64> tmp;

            // 32 results to send
            // send only 4 times
            for(int i = 0; i < 32; i += 8 ) {
                pack ( tmp, 0, drabs[i] );
                pack ( tmp, 1, drabs[i+1] );
                pack ( tmp, 2, drabs[i+2] );
                pack ( tmp, 3, drabs[i+3] );
                pack ( tmp, 4, drabs[i+4] );
                pack ( tmp, 5, drabs[i+5] );
                pack ( tmp, 6, drabs[i+6] );
                pack ( tmp, 7, drabs[i+7] );
                enqueue_data ( chan_output, tmp ); // 1, 4 times
            }
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
