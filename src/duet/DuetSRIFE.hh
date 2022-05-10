#ifndef __DUET_SRI_FE_HH
#define __DUET_SRI_FE_HH

#include "params/DuetSRIFE.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"

namespace gem5
{

class DuetDriver;

class DuetSRIFE : public ClockedObject
{
private:
    friend class DuetDriver;

    /* InputPort Type */
    class InPort : public ResponsePort
    {
    private:
        bool    _is_upstream_waiting_for_retry;

    public:
        InPort ( const std::string & name
                , DuetSRIFE * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort ( name, owner, id )
              , _is_upstream_waiting_for_retry (false)
        {
        }

        AddrRangeList getAddrRanges() const override;

    protected:
        Tick recvAtomic ( PacketPtr pkt ) override
        {
            panic ( "recvAtomic unimpl." );
        }

        void recvFunctional ( PacketPtr pkt ) override
        {
            panic ( "recvFunctional unimpl." );
        }

        bool recvTimingReq ( PacketPtr pkt ) override;
        void recvRespRetry () override;
    };

    AddrRange   _range;   // memory space
    InPort      _inport;     // input port

    uint8_t     * _raw_data;
    PacketPtr   _pending_resp;

    EventFunctionWrapper    _event;

    /* handle a request in functional mode */
    void handleFunctional ( PacketPtr pkt );

    /* handle a request in timing mode */
    bool handleTiming ( PacketPtr pkt );

    /* event handler: try sending _pending_resp thru _inport */
    void trySendResp ();

public:
    /* Constructor */
    DuetSRIFE ( const DuetSRIFEParams & p );

    /* Destructor */
    ~DuetSRIFE ();

    /* get port by name and ID */
    Port & getPort ( const std::string & if_name
            , PortID idx = InvalidPortID ) override;

    /* send memory range during init */
    void init() override;
};

}

#endif /* #ifndef __DUET_SRI_FE_HH */
