#ifndef __DUET_FMM_VLI_COMPUTE_FUNCTOR_HH
#define __DUET_FMM_VLI_COMPUTE_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
    #include <ac_complex.h>
    #include <ac_math.h>

    typedef ac_complex <ac_ieee_float64> Complex;
    #define sqrt(x) (x).sqrt<AC_RND_CONV,true>()
    #define log(x) ac_log_pwl(x)
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    #include "duet/engine/complex.hh"
    
    namespace gem5 {
    namespace duet {

        typedef ComplexTmpl<double> Complex;

#endif /* #ifdef __DUET_HLS */

class DuetFmmVLIComputeFunctor : public DuetFunctor {
private:
    constexpr static double Inv ( int i ) {
        if ( i == 0 ) return i;
        else return 1.0 / i;
    }

    constexpr static double OverInc ( int i ) {
        return i / ( i + 1 );
    }

    constexpr static double C ( int i, int j ) {
        if ( j == 0 )
            return 1.0;
        else if ( j == 1 )
            return (double) i;
        else if ( j == i + 1 )
            return 0.0;
        else if ( j >= 2 && j <= i )
            return C ( i - 1, j ) + C ( i - 1, j - 1 );
        else {
            assert ( false );
            return 0.0;
        }
    }

public:
    #pragma hls_design top
    void kernel (
            chan_data_t &   chan_input
            , chan_data_t & chan_output
            , const S64     Expansion_Terms
            )
    {
        // constants
        const static Double RealOne = 1.0;
        const static Double RealZero = 0.0;
        const static Complex One ( RealOne, RealZero );
        const static Complex Zero ( RealZero, RealZero );

#define MAX_EXPANSION_TERMS 40

        // local variables
        Complex z0_pow_minus_n[MAX_EXPANSION_TERMS];
        Complex temp_exp[MAX_EXPANSION_TERMS];
        Complex mp_expansion[MAX_EXPANSION_TERMS];

        // read inputs
        Complex source_pos, dest_pos;
        dequeue_data ( chan_input, source_pos.r() );
        dequeue_data ( chan_input, source_pos.i() );
        dequeue_data ( chan_input, dest_pos.r() );
        dequeue_data ( chan_input, dest_pos.i() );

        for ( int i = 0; i < Expansion_Terms; ++i ) {
            dequeue_data ( chan_input, mp_expansion[i].r() );
            dequeue_data ( chan_input, mp_expansion[i].i() );
        }

        Complex z0 = source_pos - dest_pos;
        Complex z0_inv = One / z0;

        z0_pow_minus_n[0] = One;
        temp_exp[0] = mp_expansion[0];
        for ( int i = 1; i < Expansion_Terms; ++i ) {
            z0_pow_minus_n[i] = z0_pow_minus_n[i - 1] * z0_inv;
            temp_exp[i] = z0_pow_minus_n[i] * mp_expansion[i];
        }

        for ( int i = 0; i < Expansion_Terms; ++i ) {
            Complex result_exp = Zero;

            for ( int j = 1; j < Expansion_Terms; ++j ) {
                Double r = C ( i+j-1, j-1 );
                Complex temp ( r, RealZero );
                temp *= temp_exp[j];
                if ( (j&0x1) == 0x0 ) {
                    result_exp += temp;
                } else {
                    result_exp -= temp;
                }
            }

            result_exp *= z0_pow_minus_n[i];

            if ( i == 0 ) {
                Double r = log(sqrt(z0.mag_sqr()));
                Complex temp ( r, RealZero );
                temp *= mp_expansion[0];
                result_exp += temp;
            } else {
                Double r = Inv(i);
                Complex temp ( r, RealZero );
                temp *= z0_pow_minus_n[i];
                temp *= mp_expansion[0];
                result_exp -= temp;
            }

            // output
            enqueue_data ( chan_output, result_exp.r() );
            enqueue_data ( chan_output, result_exp.i() );
        }
    }

#ifndef __DUET_HLS
private:
    chan_data_t     * _chan_input;
    chan_data_t     * _chan_output;
    S64               _expansion_terms;

protected:
    void run () override final;

public:
    DuetFmmVLIComputeFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_FMM_VLI_COMPUTE_FUNCTOR_HH */

