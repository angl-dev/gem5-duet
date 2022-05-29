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
protected:
    /* SRI Port */
    class SRIPort : public UpstreamPort {
    private:
        DuetEngine *    _owner;

    public:
        SRIPort ( const std::string & name
                , DuetEngine * owner
                , PortID id = InvalidPortID
                )
            : UpstreamPort      ( name, owner, id )
            , _owner            ( owner )
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
protected:
    System                                    * _system;
    Process                                   * _process;   // to access the page table
    unsigned                                    _fifo_capacity;
    DuetFunctor::caller_id_t                    _num_callers;
    Addr                                        _baseaddr;
    std::vector <DuetLane*>                     _lanes;
    SRIPort                                     _sri_port;
    std::vector <MemoryPort>                    _mem_ports;

protected:
    // -- Channels -----------------------------------------------------------
    //  Argument/Return channels: per caller
    std::vector <std::map <uint16_t,
        std::unique_ptr <DuetFunctor::chan_data_t>>>    _chan_arg_by_id;
    std::vector <std::map <uint16_t, 
        std::unique_ptr <DuetFunctor::chan_data_t>>>    _chan_ret_by_id;

    //  memory channels -- shared among callers
    std::map <uint16_t,
        std::unique_ptr <DuetFunctor::chan_req_t>>      _chan_req_by_id;
    std::map <uint16_t,
        std::unique_ptr <DuetFunctor::chan_data_t>>     _chan_wdata_by_id;
    std::map <uint16_t,
        std::unique_ptr <DuetFunctor::chan_data_t>>     _chan_rdata_by_id;

    //  inter-lane channels -- shared among callers
    std::map <uint16_t,
        std::unique_ptr <DuetFunctor::chan_data_t>>     _chan_int_by_id;

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
protected:
    // we need a requestor ID to be able to send out memory requests
    RequestorID                 _requestorId;

// ===========================================================================
// == Implementing Virtual Methods ===========================================
// ===========================================================================
protected:
    void _update () override final;
    void _exchange () override final;
    bool _has_work () override final;

// ===========================================================================
// == API for DuetLane =======================================================
// ===========================================================================
public:
    DuetFunctor::chan_req_t & get_chan_req (
            DuetFunctor::caller_id_t    caller_id
            , DuetFunctor::chan_id_t    chan_id
            );

    DuetFunctor::chan_data_t & get_chan_data (
            DuetFunctor::caller_id_t    caller_id
            , DuetFunctor::chan_id_t    chan_id
            );

    bool can_push_to_chan (
            DuetFunctor::caller_id_t    caller_id
            , DuetFunctor::chan_id_t    chan_id
            );

    bool can_pull_from_chan (
            DuetFunctor::caller_id_t    caller_id
            , DuetFunctor::chan_id_t    chan_id
            );

    DuetFunctor::caller_id_t get_num_callers () const { return _num_callers; }

// ===========================================================================
// == API for Subclasses =====================================================
// ===========================================================================
protected:
    bool _try_send_mem_req_one (
            uint16_t                    chan_id
            , DuetFunctor::mem_req_t    req
            , DuetFunctor::raw_data_t   data
            );

// ===========================================================================
// == Virtual Methods ========================================================
// ===========================================================================
protected:
    virtual softreg_id_t _get_num_softregs () const = 0;

    virtual bool _handle_softreg_write (
            softreg_id_t    softreg_id
            , uint64_t      value
            ) = 0;

    virtual bool _handle_softreg_read (
            softreg_id_t    softreg_id
            , uint64_t    & value
            ) = 0;

    virtual void _try_send_mem_req_all () = 0;

    virtual bool _try_recv_mem_resp_one (
            uint16_t                    chan_id
            , DuetFunctor::raw_data_t   data
            ) = 0;

// ===========================================================================
// == General API ============================================================
// ===========================================================================
public:
    DuetEngine ( const DuetEngineParams & p );
};

}   // namespace duet
}   // namespace gem5

#endif  /* #ifndef __DUET_ENGINE_HH */
