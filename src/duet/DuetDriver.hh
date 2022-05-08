#ifndef __DUET_DRIVER_HH
#define __DUET_DRIVER_HH

#include "sim/emul_driver.hh"

namespace gem5
{

class ThreadContext;

class DuetDriver : public EmulatedDriver
{
private:
    uint32_t    _value;

public:
    DuetDriver (const EmulatedDriverParams& p)
        : EmulatedDriver (p), _value (0x0)
    {
    }

    int open (ThreadContext *tc, int mode, int flags) override;
    int ioctl (ThreadContext *tc, unsigned req, Addr buf) override;
};

}

#endif /* #ifndef __DUET_DRIVER_HH */
