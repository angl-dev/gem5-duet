#ifndef __DUET_DRIVER_HH
#define __DUET_DRIVER_HH

#include "params/DuetDriver.hh"
#include "sim/emul_driver.hh"

namespace gem5 {

class ThreadContext;

namespace duet {

class DuetWidget;
class DuetDriver : public EmulatedDriver
{
private:
    AddrRange   _range;

public:
    DuetDriver (const DuetDriverParams& p)
        : EmulatedDriver    ( p )
        , _range            ( p.range )
    {
    }

    int open (ThreadContext *tc, int mode, int flags) override;
    int ioctl (ThreadContext *tc, unsigned req, Addr buf) override;
    Addr mmap (ThreadContext *tc, Addr start, uint64_t length,
              int prot, int tgt_flags, int tgt_fd, off_t offset) override;
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_DRIVER_HH */
