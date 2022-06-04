#ifndef __DUET_CLOCKED_OBJECT_HH
#define __DUET_CLOCKED_OBJECT_HH

#include "params/DuetClockedObject.hh"
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
        DuetClockedObject * owner;
        bool                is_peer_waiting_for_retry;
        bool                is_this_waiting_for_retry;
        PacketPtr           req_buf;
        PacketPtr           resp_buf;

    public:
        UpstreamPort ( const std::string & name
                , DuetClockedObject * owner
                , PortID id = InvalidPortID
                )
            : ResponsePort                  ( name, owner, id )
            , owner                         ( owner )
            , is_peer_waiting_for_retry     ( false )
            , is_this_waiting_for_retry     ( false )
            , req_buf                       ( nullptr )
            , resp_buf                      ( nullptr )
        {}

        Tick recvAtomic ( PacketPtr pkt ) override {
            panic ( "recvAtomic unimpl." );
        }

    public:
        virtual bool recvTimingReq ( PacketPtr pkt ) override final;
        virtual void recvRespRetry () override final;
                void try_send_resp ();
                void exchange ();
    };

    /* Port to downstream */
    class DownstreamPort : public RequestPort {
    public:
        DuetClockedObject * owner;
        bool                is_peer_waiting_for_retry;
        bool                is_this_waiting_for_retry;
        PacketPtr           req_buf;
        PacketPtr           resp_buf;

    public:
        DownstreamPort ( const std::string & name
                , DuetClockedObject * owner
                , PortID id = InvalidPortID
                )
            : RequestPort                   ( name, owner, id )
            , owner                         ( owner )
            , is_peer_waiting_for_retry     ( false )
            , is_this_waiting_for_retry     ( false )
            , req_buf                       ( nullptr )
            , resp_buf                      ( nullptr )
        {}

    public:
        virtual bool recvTimingResp ( PacketPtr pkt ) override final;
        virtual void recvReqRetry () override final;
                void try_send_req ();
                void exchange ();
    };

private:
    EventFunctionWrapper    _e_do_cycle;
    Cycles                  _latest_cycle_plus1;
    bool                    _is_sleeping;

private:
    void _do_cycle ();

    // API for UpstreamPort/DownstreamPort
    bool is_sleeping () const { return _is_sleeping; }
    bool is_update_phase () const { return curCycle() >= _latest_cycle_plus1; }
    bool is_exchange_phase () const { return curCycle() < _latest_cycle_plus1; }

protected:
    void wakeup ();

    virtual void update () {};
    virtual void exchange () {};
    virtual bool has_work () = 0;

public:
    DuetClockedObject ( const DuetClockedObjectParams & p )
        : ClockedObject         ( p )
        , _e_do_cycle           ( [this]{ _do_cycle(); }, name() )
        , _latest_cycle_plus1   ( 0 )
        , _is_sleeping          ( true )
    {}
};

}   // namespace duet
}   // namespace gem5

#endif /* #define __DUET_CLOCKED_OBJECT_HH */
