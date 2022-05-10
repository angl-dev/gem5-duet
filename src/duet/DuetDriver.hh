#ifndef __DUET_DRIVER_HH
#define __DUET_DRIVER_HH

#include "params/DuetDriver.hh"
#include "sim/emul_driver.hh"

namespace gem5
{

class ThreadContext;
class DuetSRIFE;

class DuetDriver : public EmulatedDriver
{
private:
    DuetSRIFE * _dev;

public:
    DuetDriver (const DuetDriverParams& p)
        : EmulatedDriver (p)
          , _dev ( p.dev )
    {
    }

    int open (ThreadContext *tc, int mode, int flags) override;
    int ioctl (ThreadContext *tc, unsigned req, Addr buf) override;
    Addr mmap(ThreadContext *tc, Addr start, uint64_t length,
              int prot, int tgt_flags, int tgt_fd, off_t offset) override;
};

}

#endif /* #ifndef __DUET_DRIVER_HH */
