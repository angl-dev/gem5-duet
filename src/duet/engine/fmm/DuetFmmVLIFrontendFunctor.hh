#ifndef __DUET_FMM_VLI_FRONTEND_FUNCTOR_HH
#define __DUET_FMM_VLI_FRONTEND_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    
    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetFmmVLIFrontendFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            chan_data_t &   chan_arg        // two arguments per call
            , chan_req_t &  chan_req
            , chan_data_t & chan_arg_fwd    // forward dst_box_ptr to backend
            , const S64     Expansion_Terms
            )
    {
        addr_t  src_box_ptr, dst_box_ptr;
        dequeue_data ( chan_arg, src_box_ptr );
        dequeue_data ( chan_arg, dst_box_ptr );

        // load source_box->x_center, y_center 
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), src_box_ptr + 8 );
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), src_box_ptr + 16 );

        // load dest_box->x_center, y_center
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), dst_box_ptr + 8 );
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), dst_box_ptr + 16 );

        // load source_box->mp_expansion[0 .. Expansion_Terms]
        for ( int i = 0; i < Expansion_Terms; ++i ) {
            enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), src_box_ptr + 1216 + (i << 4) );
            enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), src_box_ptr + 1224 + (i << 4) );
        }

        // forward dst_box_ptr to backend
        enqueue_data ( chan_arg_fwd, dst_box_ptr );
    }

#ifndef __DUET_HLS
private:
    chan_data_t     * _chan_arg;
    chan_req_t      * _chan_req;
    chan_data_t     * _chan_arg_fwd;
    S64               _expansion_terms;

protected:
    void run () override final;

public:
    DuetFmmVLIFrontendFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_FMM_VLI_FRONTEND_FUNCTOR_HH */
