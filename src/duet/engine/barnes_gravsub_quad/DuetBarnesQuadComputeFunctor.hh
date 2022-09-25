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

class DuetBarnesQuadComputeFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            ac_channel <Block<64>> &    chan_input
            , ac_channel <Block<32>> &  chan_output
            , const Double &            pos0x_ci
            , const Double &            pos0y_ci
            , const Double &            pos0z_ci
            , const Double &            epssq_ci
            )
    {
        // take in constant arguments
        const Double pos0[3] = { pos0x_ci, pos0y_ci, pos0z_ci };
        Double drsq = epssq_ci;

        // pop loaded data
        Double pos[3], mass, quad[3][3];

        {
            Block<64> tmp;

            dequeue_data ( chan_input, tmp );
            unpack ( tmp, 1, mass );
            unpack ( tmp, 2, pos[0] );
            unpack ( tmp, 3, pos[1] );
            unpack ( tmp, 4, pos[2] );

            dequeue_data ( chan_input, tmp );
            unpack ( tmp, 5, quad[0][0] );
            unpack ( tmp, 6, quad[0][1] );
            unpack ( tmp, 7, quad[0][2] );

            dequeue_data ( chan_input, tmp );
            unpack ( tmp, 1, quad[1][1] );
            unpack ( tmp, 2, quad[1][2] );
            unpack ( tmp, 5, quad[2][2] );
        }

        quad[1][0] = quad[0][1];
        quad[2][0] = quad[0][2];
        quad[2][1] = quad[1][2];

        Double dr[3];
        #pragma unroll yes
        for ( int i = 0; i < 3; ++i ) {
            dr[i] = pos[i] - pos0[i];
            drsq += dr[i] * dr[i];
        }

        Double drabs = sqrt (drsq);
        Double phii = mass / drabs;
        Double mor3 = phii / drsq;
        Double ai[3];

        #pragma unroll yes
        for ( int i = 0; i < 3; ++i ) {
            ai[i] = dr[i] * mor3;
        }

        Double dr5inv ( 1.0f );
        dr5inv /= drsq * drsq * drabs;

        Double quaddr[3];
        #pragma unroll yes
        for ( int i = 0; i < 3; ++i ) {
            quaddr[i] = (Double) (0.f);
            #pragma unroll yes
            for ( int j = 0; j < 3; ++j ) {
                quaddr[i] += quad[i][j] * dr[j];
            }
        }

        Double drquaddr ( 0.f );
        #pragma unroll yes
        for ( int i = 0; i < 3; ++i ) {
            drquaddr += dr[i] * quaddr[i];
        }

        Double phiquad ( -0.5f );
        phiquad *= dr5inv * drquaddr;
        phii = phiquad - phii;

        Double five ( 5.0f );
        phiquad *= five / drsq;

        #pragma unroll yes
        for ( int i = 0; i < 3; ++i ) {
            ai[i] -= dr[i] * phiquad + quaddr[i] * dr5inv;
        }

        {
            Block<32> tmp;
            pack ( tmp, 0, phii );
            pack ( tmp, 1, ai[0] );
            pack ( tmp, 2, ai[1] );
            pack ( tmp, 3, ai[2] );
            enqueue_data ( chan_output, tmp );
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
    DuetBarnesQuadComputeFunctor ( DuetLane * lane, caller_id_t caller_id )
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
