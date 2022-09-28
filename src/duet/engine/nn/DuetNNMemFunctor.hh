#ifndef __DUET_NN_MEM_FUNCTOR_HH
#define __DUET_NN_MEM_FUNCTOR_HH

#ifdef __DUET_HLS
    #include "hls/functor.hh"
#else /* #ifdef __DUET_HLS */
    #include "duet/engine/DuetFunctor.hh"

    namespace gem5 {
    namespace duet {
#endif /* #ifdef __DUET_HLS */

class DuetNNMemFunctor : public DuetFunctor
{
public:
    #pragma hls_design top
    void kernel (
            ac_channel <U64> &  chan_arg
            , chan_req_t &      chan_req
            )
    {
        addr_t nodeptr;
        dequeue_data ( chan_arg, nodeptr );

        // assume >= 64B cache line size:

        // load mass(p) = +8, pos[0] = +16, pos[1] = +24, pos[2] = +32
        enqueue_req ( chan_req, REQTYPE_LD, 64, nodeptr );

        // load quad(p): xx = +104, xy = +112, xz = +120
        enqueue_req ( chan_req, REQTYPE_LD, 64, nodeptr + 64 );

        // load quad(p): yy = +136, yz = +144, zz = +168
        enqueue_req ( chan_req, REQTYPE_LD, 64, nodeptr + 128 );
    }

#ifndef __DUET_HLS
private:
    chan_data_t     * _chan_arg;
    chan_req_t      * _chan_req;

protected:
    void run () override final;

public:
    DuetNNMemFunctor ( DuetLane * lane, caller_id_t caller_id )
        : DuetFunctor ( lane, caller_id )
    {}

    void setup () override final;
#endif
};

#ifndef __DUET_HLS
    }   // namespace gem5
    }   // namespace duet
#endif /* #ifndef __DUET_HLS */

#endif /* #ifndef __DUET_NN_MEM_FUNCTOR_HH */
