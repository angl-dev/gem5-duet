#ifndef __DUET_FMM_VLI_BACKEND_FUNCTOR_HH
#define __DUET_FMM_VLI_BACKEND_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    #include "duet/engine/complex.hh"
    
    namespace gem5 {
    namespace duet {

        typedef ComplexTmpl<double> Complex;

#endif /* #ifdef __DUET_HLS */

class DuetFmmVLIBackendFunctor : public DuetFunctor {
private:
    constexpr static S64 V_LIST_COST ( S64 a ) {
        return 1.08 * ((35.9 * a * a) + (133.6 * a));
    }

public:
    #pragma hls_design top
    void kernel (
            chan_data_t   & chan_arg
            , chan_data_t & chan_input
            , chan_req_t  & chan_req
            , chan_data_t & chan_wdata
            , chan_data_t & chan_rdata
            , const S64     Expansion_Terms
            ,       S64   & cost
            )
    {
        addr_t dst_box_ptr;
        dequeue_data ( chan_arg, dst_box_ptr );

        for ( int i = 0; i < Expansion_Terms; ++i ) {
            enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), dst_box_ptr + 1856 + (i << 4) );
            enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), dst_box_ptr + 1864 + (i << 4) );
        }
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (S64), dst_box_ptr + 3272 );

        cost += V_LIST_COST ( Expansion_Terms );

        for ( int i = 0; i < Expansion_Terms; ++i ) {
            Complex local, update;
            dequeue_data ( chan_rdata, local.r () );
            dequeue_data ( chan_rdata, local.i () );
            dequeue_data ( chan_input, update.r () );
            dequeue_data ( chan_input, update.i () );
            local += update;
            enqueue_data ( chan_wdata, local.r () );
            enqueue_req ( chan_req, REQTYPE_ST, sizeof (double), dst_box_ptr + 1856 + (i << 4) );
            enqueue_data ( chan_wdata, local.i () );
            enqueue_req ( chan_req, REQTYPE_ST, sizeof (double), dst_box_ptr + 1864 + (i << 4) );
        }

        // make sure all stores are committed into the memory system
        for ( int i = 0; i < Expansion_Terms; ++i ) {
            dequeue_token ( chan_rdata );
            dequeue_token ( chan_rdata );
        }
    }

#ifndef __DUET_HLS
private:
    chan_data_t     * _chan_arg;
    chan_data_t     * _chan_input;
    chan_req_t      * _chan_req;
    chan_data_t     * _chan_wdata;
    chan_data_t     * _chan_rdata;
    S64               _expansion_terms;
    S64               _cost;

protected:
    void run () override final;

public:
    DuetFmmVLIBackendFunctor ( DuetLane * lane, caller_id_t caller_id )
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

#endif /* #ifndef __DUET_FMM_VLI_BACKEND_FUNCTOR_HH */
