#ifndef __DUET_REORDER_BUFFER_HH
#define __DUET_REORDER_BUFFER_HH

#include <list>

#include "params/DuetReorderBuffer.hh"
#include "duet/DuetClockedObject.hh"

namespace gem5 {
namespace duet {

class DuetReorderBuffer : public DuetClockedObject {
// ===========================================================================
// == Type Definitions =======================================================
// ===========================================================================
private:

    /* Upstream port */
    class Upstream : public UpstreamPort {
    private:
        DuetReorderBuffer * owner;

    public:
        Upstream ( const std::string &  name
                , DuetReorderBuffer *   owner
                , PortID                id = InvalidPortID
                )
            : UpstreamPort  ( name, owner, id )
            , owner         ( owner )
        {}

        void recvFunctional ( PacketPtr pkt ) override final {
            panic ( "recvFunctional unimpl." );
        }

    public:
        AddrRangeList getAddrRanges () const override final {
            return owner->downstream.getAddrRanges ();
        }
    };

    /* Downstream port */
    class Downstream : public DownstreamPort {
    private:
        DuetReorderBuffer * owner;

    public:
        Downstream ( const std::string &    name
                , DuetReorderBuffer *       owner
                , PortID                    id = InvalidPortID
                )
            : DownstreamPort    ( name, owner, id )
            , owner             ( owner )
        {}

        void recvRangeChange () override final {
            owner->upstream.sendRangeChange ();
        }
    };

    /* buffer entry */
    struct Entry {
        enum { UNSENT, SENT, RESPONDED }    status;
        PacketPtr                           pkt;
        RequestPtr                          req;
        Tick                                readyAfter;

        Entry ( decltype ( status ) status,
                PacketPtr           pkt )
            : status        ( status )
            , pkt           ( pkt )
            , req           ( pkt->req )
            , readyAfter    ( 0 )
        {}
    };

// ===========================================================================
// == Paramterized Member Variables ==========================================
// ===========================================================================
private:
    unsigned            _capacity;
    Upstream            upstream;
    Downstream          downstream;

// ===========================================================================
// == Non-Parameterized Member Variables =====================================
// ===========================================================================
private:
    std::list <Entry>   _buffer;

// ===========================================================================
// == Implementing Virtual Methods ===========================================
// ===========================================================================
protected:
    void update () override final;
    void exchange () override final;
    bool has_work () override final;

// ===========================================================================
// == General API ============================================================
// ===========================================================================
public:
    DuetReorderBuffer ( const DuetReorderBufferParams & p );
    Port & getPort ( const std::string & if_name
            , PortID idx = InvalidPortID ) override;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_REORDER_BUFFER_HH */
