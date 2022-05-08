#include "duet/DuetDriver.hh"
#include "debug/DuetDriver.hh"
#include "cpu/thread_context.hh"
#include "mem/port_proxy.hh"
#include "sim/process.hh"
#include "sim/syscall_emul_buf.hh"

namespace gem5
{

int DuetDriver::open (ThreadContext *tc, int mode, int flags)
{
    DPRINTF (DuetDriver, "Opened %s\n", filename);
    auto process = tc->getProcessPtr();
    auto device_fd_entry = std::make_shared<DeviceFDEntry>(this, filename);
    int tgt_fd = process->fds->allocFD(device_fd_entry);
    return tgt_fd;
}

int DuetDriver::ioctl (ThreadContext *tc, unsigned req, Addr buf)
{
    SETranslatingPortProxy se_proxy(tc);
    auto process = tc->getProcessPtr();

    switch (req) {
        case 0:         // read!
        {
            DPRINTF (DuetDriver, "ioctl: READ(0) to addr 0x%08x (value=0x%08x)\n",
                    buf, _value);

            TypedBufferArg<uint32_t> args(buf);
            *args = _value;
            args.copyOut (se_proxy);

            return 0;
        }

        case 1:         // write!
        {
            TypedBufferArg<uint32_t> args(buf);
            args.copyIn (se_proxy);

            uint32_t next = *args;

            DPRINTF (DuetDriver, "ioctl: WRITE(1) from addr 0x%08x (value=0x%08x->0x%08x)\n",
                    buf, _value, next);

            _value = next;

            return 0;
        }

        default:        // undefined request code!
        {
            DPRINTF (DuetDriver, "ioctl: UNDEFINED(%u)\n", req);
            return -EINVAL;
        }
    }
}

}
