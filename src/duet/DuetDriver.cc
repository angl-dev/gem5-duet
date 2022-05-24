#include "duet/DuetDriver.hh"
#include "duet/widget/widget.hh"
#include "arch/riscv/page_size.hh"
#include "debug/DuetDriver.hh"
#include "cpu/thread_context.hh"
#include "mem/port_proxy.hh"
#include "mem/page_table.hh"
#include "sim/process.hh"
#include "sim/syscall_emul_buf.hh"

namespace gem5 {
namespace duet { 

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
    return -EINVAL;

    /*
    SETranslatingPortProxy se_proxy(tc);

    switch (req) {
        case 0:         // read!
        {
            DPRINTF (DuetDriver, "ioctl(%s): READ(0) to addr 0x%08x (value=0x%08x)\n",
                    filename, buf, _value);

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

            DPRINTF (DuetDriver, "ioctl(%s): WRITE(1) from addr 0x%08x (value=0x%08x->0x%08x)\n",
                    filename, buf, _value, next);

            _value = next;

            return 0;
        }

        default:        // undefined request code!
        {
            DPRINTF (DuetDriver, "ioctl(%s): UNDEFINED(%u)\n",
                    filename, req);
            return -EINVAL;
        }
    }
    */
}

Addr DuetDriver::mmap(ThreadContext *tc, Addr start, uint64_t length,
        int prot, int tgt_flags, int tgt_fd, off_t offset)
{
    auto process = tc->getProcessPtr();
    auto mem_state = process->memState;

    DPRINTF (DuetDriver, "mmap(%s) (offset: 0x%x, length: 0x%x)\n",
            filename, offset, length);
    panic_if ( offset + length > _range.size(),
            "Requested mapping [0x%x +: 0x%x] exceeds alloted range %s",
            offset, length, _range.to_string() );
    start = mem_state->extendMmap (length);

    // Note: In fact, "Uncacheable" flag does not have any effect
    process->pTable->map(start, _range.start() + offset, length,
            EmulationPageTable::Uncacheable);

    return start;
}

}   // namespace duet
}   // namespace gem5
