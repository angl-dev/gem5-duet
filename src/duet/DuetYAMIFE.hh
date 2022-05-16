#ifndef __DUET_YAMI_FE_HH
#define __DUET_YAMI_FE_HH

#include "params/DuetYAMIFE.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"

namespace gem5 {
namespace duet {

class DuetYAMIFE : public ClockedObject
{
private:
    /* OutputPort Type */
    class OutPort : public RequestPort
    {
    private:
        DuetYAMIFE * _owner;

    public:
        OutPort ( const std::string & name
                , DuetYAMIFE * owner
                , PortID id = InvalidPortID
                )
            : RequestPort ( name, owner, id )
              , _owner ( owner )
        {
        }

    protected:
        bool recvTiminResp ( PacketPtr pkt ) override;
        void recvReqRetry () override;
        void recvRangeChange () override {};
    };

    OutPort     _outport;   // output port
    PacketPtr   _req;       // current request

    enum ReqState {
        REQ_STATE_INVALID   = 0,
        REQ_STATE_BLOCKED,
        REQ_STATE_SENT
    };
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_YAMI_FE_HH */
