#ifndef __DUET_BARNES_QUAD_MEM_FUNCTOR_HH
#define __DUET_BARNES_QUAD_MEM_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"
    
    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetKNNMemFunctor : public DuetFunctor {
public:
    #pragma hls_design top
    void kernel (
            chan_data_t &   chan_arg
            , chan_req_t &  chan_req
            )
    {
        addr_t nodeptr;
        dequeue_data ( chan_arg, nodeptr );

        // load pos[0], pos[1], and pos[2]
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 16 );
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 24 );
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 32 );

        // load mass(p)
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 8 );

        // load quad(p): xx, xy, xz, yy, yz, zz
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 104 );   // xx
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 112 );   // xy
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 120 );   // xz
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 136 );   // yy
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 144 );   // yz
        enqueue_req ( chan_req, REQTYPE_LD, sizeof (double), nodeptr + 168 );   // zz
    }

#ifndef __DUET_HLS
private:
    chan_data_t     * _chan_arg;
    chan_req_t      * _chan_req;

protected:
    void run () override final;

public:
    DuetKNNMemFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_BARNES_QUAD_MEM_FUNCTOR_HH */
