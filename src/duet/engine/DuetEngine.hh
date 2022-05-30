#ifndef __DUET_ENGINE_HH
#define __DUET_ENGINE_HH

#include <vector>
#include <memory>
#include <utility>

#include "params/DuetEngine.hh"
#include "duet/DuetClockedObject.hh"
#include "duet/engine/DuetFunctor.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "mem/port.hh"

namespace gem5 {

class System;
class Process;

namespace duet {

class DuetLane;
class DuetEngine : public DuetClockedObject {
// ===========================================================================
// == Type Definitions =======================================================
// ===========================================================================
private:

    /* SRI Port */
    class SRIPort : public UpstreamPort {
    private:
        DuetEngine *    owner;

    public:
        SRIPort ( const std::string & name
                , DuetEngine * owner
                , PortID id = InvalidPortID
                )
            : UpstreamPort      ( name, owner, id )
            , owner             ( owner )
        {}

        void recvFunctional ( PacketPtr pkt ) override final {
            panic ( "recvFunctional unimpl." );
        }

    public:
        AddrRangeList getAddrRanges () const override final;
    };

    /* Memory Port */
    class MemoryPort : public DownstreamPort {
    public:
        MemoryPort ( const std::string & name
                , DuetEngine * owner
                , PortID id = InvalidPortID
                )
            : DownstreamPort    ( name, owner, id )
        {}

        void recvRangeChange () override {};
    };

public:
    typedef uint32_t            softreg_id_t;

// ===========================================================================
// == Paramterized Member Variables ==========================================
// ===========================================================================
private:
    System                                    * _system;
    Process                                   * _process;   // to access the page table
    unsigned                                    _fifo_capacity;
    DuetFunctor::caller_id_t                    _num_callers;
    Addr                                        _baseaddr;
    std::vector <DuetLane*>                     _lanes;
    SRIPort                                     _sri_port;
    std::vector <MemoryPort>                    _mem_ports;

private:
    // -- Channels -----------------------------------------------------------
    //  Argument/Return channels: one per caller
    std::vector <std::unique_ptr <DuetFunctor::chan_data_t>> _chan_arg_by_id;
    std::vector <std::unique_ptr <DuetFunctor::chan_data_t>> _chan_ret_by_id;

    //  memory channels -- shared among callers
    std::vector <std::unique_ptr <DuetFunctor::chan_req_t>>  _chan_req_by_id;
    std::vector <std::unique_ptr <DuetFunctor::chan_data_t>> _chan_wdata_by_id;
    std::vector <std::unique_ptr <DuetFunctor::chan_data_t>> _chan_rdata_by_id;

    //  inter-lane channels -- shared among callers
    std::vector <std::unique_ptr <DuetFunctor::chan_data_t>> _chan_int_by_id;

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
private:
    // we need a requestor ID to be able to send out memory requests
    RequestorID                 _requestorId;

// ===========================================================================
// == Implementing Virtual Methods ===========================================
// ===========================================================================
protected:
    void update () override final;
    void exchange () override final;
    bool has_work () override final;

// ===========================================================================
// == API for DuetLane =======================================================
// ===========================================================================
public:
    DuetFunctor::chan_req_t & get_chan_req (
            DuetFunctor::chan_id_t      chan_id
            );

    DuetFunctor::chan_data_t & get_chan_data (
            DuetFunctor::chan_id_t      chan_id
            );

    bool can_push_to_chan (
            DuetFunctor::chan_id_t      chan_id
            );

    bool can_pull_from_chan (
            DuetFunctor::chan_id_t      chan_id
            );

    DuetFunctor::caller_id_t get_num_callers () const { return _num_callers; }

// ===========================================================================
// == API for Subclasses =====================================================
// ===========================================================================
protected:
    bool try_send_mem_req_one (
            uint16_t                    chan_id
            , DuetFunctor::mem_req_t    req
            , DuetFunctor::raw_data_t   data
            );

    bool handle_argchan_push (
            DuetFunctor::caller_id_t    caller_id
            , uint64_t                  value
            );

    bool handle_retchan_pull (
            DuetFunctor::caller_id_t    caller_id
            , uint64_t                & value
            );

// ===========================================================================
// == Virtual Methods ========================================================
// ===========================================================================
protected:
    virtual softreg_id_t             get_num_softregs ()        const = 0;
    virtual DuetFunctor::caller_id_t get_num_memory_chans ()    const { return 0; };
    virtual DuetFunctor::caller_id_t get_num_interlane_chans () const { return 0; };

    virtual bool handle_softreg_write (
            softreg_id_t    softreg_id
            , uint64_t      value
            ) = 0;

    virtual bool handle_softreg_read (
            softreg_id_t    softreg_id
            , uint64_t    & value
            ) = 0;

    virtual void try_send_mem_req_all () = 0;

// ===========================================================================
// == General API ============================================================
// ===========================================================================
public:
    DuetEngine ( const DuetEngineParams & p );
    Port & getPort ( const std::string & if_name
            , PortID idx = InvalidPortID ) override;
    void init () override;
};

}   // namespace duet
}   // namespace gem5

#endif  /* #ifndef __DUET_ENGINE_HH */
