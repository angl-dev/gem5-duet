import m5, os
from m5.objects import *

num_threads = 4

range_      = AddrRange('512MB')
nc_range    = AddrRange(0xE102980000, size='4kB')

system = System(
        mem_mode = 'timing',
        mem_ranges = [range_, nc_range],
        )

system.clk_domain = SrcClockDomain( clock = '1GHz', voltage_domain = VoltageDomain() )
system.cpus = [TimingSimpleCPU() for _ in range(num_threads)]
system.mem_ctrl = MemCtrl(
        clk_domain = DerivedClockDomain(
            clk_domain = system.clk_domain,
            clk_divider = 4
            ),
        dram = DDR3_1600_8x8( range = range_ ),
        )
system.naive = DuetWidgetNaive (
        range = nc_range,
        latency_per_stage = [10, 10, 10, 10],
        interval_per_stage = [1, 1, 1, 1],
        )
system.membus = SystemXBar()
afifo = DuetAsyncFIFO(
        stage = 10,
        capacity = 2,
        upstream_clk_domain = system.clk_domain,
        downstream_clk_domain = system.mem_ctrl.clk_domain
        )
system.afifo = afifo

for cpu in system.cpus:
    cpu.createInterruptController()
    cpu.icache_port = system.membus.cpu_side_ports
    cpu.dcache_port = system.membus.cpu_side_ports

system.system_port              = system.membus.cpu_side_ports
system.afifo.upstream_port      = system.membus.mem_side_ports
system.afifo.downstream_port    = system.mem_ctrl.port
system.naive.sri_port           = system.membus.mem_side_ports
system.naive.mem_port           = system.membus.cpu_side_ports

binary = os.path.join (os.path.dirname (os.path.abspath(__file__)),
        "../../tests/test-progs/duet/bin/riscv/linux/test_naive")
process = Process(
        cmd = [binary],
        drivers = DuetDriver(
            filename = "duet",
            range = nc_range
            )
        )

system.workload = SEWorkload.init_compatible (binary)
# system.workload.wait_for_remote_gdb = True

for cpu in system.cpus:
    cpu.workload = process
    cpu.createThreads()
system.naive.process = process

root = Root (full_system = False, system = system)
m5.instantiate ()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
