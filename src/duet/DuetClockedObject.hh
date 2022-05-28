#ifndef __DUET_CLOCKED_OBJECT_HH
#define __DUET_CLOCKED_OBJECT_HH

#include "params/ClockedObject.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include "mem/packet.hh"

namespace gem5 {
namespace duet {

class DuetClockedObject : public ClockedObject {
protected:

    /* Port to upstream */
    class UpstreamPort : public ResponsePort {
    public:
        DuetClockedObject * _owner;
        bool                _is_peer_waiting_for_retry;
        bool                _is_this_waiting_for_retry;
        PacketPtr           _req_buf;
        PacketPtr           _resp_buf;

    public:
        UpstreamPort ( const std::string & name
                , DuetClockedObject * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort                  ( name, owner, id )
            , _owner                        ( owner )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
            , _req_buf                      ( nullptr )
            , _resp_buf                     ( nullptr )
        {}

        Tick recvAtomic ( PacketPtr pkt ) override {
            panic ( "recvAtomic unimpl." );
        }

    public:
        virtual bool recvTimingReq ( PacketPtr pkt ) override final;
        virtual void recvRespRetry () override final;
                void _try_send_resp ();
    };

    /* Port to downstream */
    class DownstreamPort : public RequestPort {
    public:
        DuetClockedObject * _owner;
        bool                _is_peer_waiting_for_retry;
        bool                _is_this_waiting_for_retry;
        PacketPtr           _req_buf;
        PacketPtr           _resp_buf;

    public:
        DownstreamPort ( const std::string & name
                , DuetClockedObject * owner
                , PortID id = InvalidPortID
                )
            : RequestPort                   ( name, owner, id )
            , _owner                        ( owner )
            , _is_peer_waiting_for_retry    ( false )
            , _is_this_waiting_for_retry    ( false )
            , _req_buf                      ( nullptr )
            , _resp_buf                     ( nullptr )
        {}

    public:
        virtual bool recvTimingResp ( PacketPtr pkt ) override final;
        virtual void recvReqRetry () override final;
                void _try_send_req ();
    };

private:
    EventFunctionWrapper    _e_do_cycle;
    Cycles                  _latest_cycle_plus1;
    bool                    _is_sleeping;

private:
    void _do_cycle ();

protected:
    void _wakeup ();
    bool _is_process_phase () const { return curCycle() >= _latest_cycle_plus1; }
    bool _is_exchange_phase () const { return curCycle() < _latest_cycle_plus1; }

    virtual void _process () {};
    virtual void _exchange () {};
    virtual bool _has_work () = 0;

public:
    DuetPipeline ( const ClockedObjectParams & p )
        : ClockedObject         ( p )
        , _e_do_cycle           ( [this]{ _do_cycle(); }, name() )
        , _latest_cycle_plus1   ( 0 )
        , _is_sleeping          ( false )
    {}
};

}   // namespace duet
}   // namespace gem5

#endif /* #define __DUET_CLOCKED_OBJECT_HH */
