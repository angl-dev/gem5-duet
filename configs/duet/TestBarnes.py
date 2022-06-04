import m5, os
from m5.objects import *

num_threads = 4
range_      = AddrRange('8192MB')

nc_base     = 0xE10298000
nc_range    = AddrRange(nc_base, size='4kB')

class L1Cache(Cache):
    size = '16kB'
    assoc = 4
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 4
    tgts_per_mshr = 20

class L2Cache(Cache):
    size = '256kB'
    assoc = 8
    tag_latency = 4
    data_latency = 4
    response_latency = 4
    mshrs = 20
    tgts_per_mshr = 12

# -- System, main memory and various XBars -----------------------------------
system = System (
        mem_mode = 'timing',
        mem_ranges = [range_, nc_range]
        )
system.clk_domain = SrcClockDomain( clock = '1GHz', voltage_domain = VoltageDomain() )
system.mem_ctrl = MemCtrl( dram = DDR3_1600_8x8( range = range_ ) )
system.srixbar = NoncoherentXBar(
        width = 32, frontend_latency = 1, forward_latency = 0, response_latency = 1
        )
system.l2bus = L2XBar()
system.membus = SystemXBar()

# -- CPUs and Caches ---------------------------------------------------------
system.cpus = [TimingSimpleCPU() for _ in range(num_threads)]
for cpu in system.cpus:
    cpu.icache = L1Cache ( is_read_only = True )
    cpu.dcache = L1Cache ( addr_ranges = [range_] )
    cpu.xbar = NoncoherentXBar(
            width = 32, frontend_latency = 1, forward_latency = 0, response_latency = 1
            )

    cpu.icache_port = cpu.icache.cpu_side
    cpu.dcache_port = cpu.xbar.cpu_side_ports
    cpu.xbar.mem_side_ports = cpu.dcache.cpu_side
    cpu.xbar.mem_side_ports = system.srixbar.cpu_side_ports

    cpu.dcache.mem_side = system.l2bus.cpu_side_ports
    cpu.icache.mem_side = system.l2bus.cpu_side_ports

    cpu.createInterruptController()

# -- Duet Engine: Barnes -----------------------------------------------------
system.engine = DuetBarnesEngine (
        num_callers = num_threads,
        baseaddr = nc_base
        )
system.engine.clk_domain = DerivedClockDomain(
        clk_domain = system.clk_domain,
        clk_divider = 5
        );
system.engine.sri_afifo = DuetAsyncFIFO (
        stage = 2, capacity = 256, 
        upstream_clk_domain = system.clk_domain,
        downstream_clk_domain = system.engine.clk_domain
        )
system.engine.sri_afifo.upstream_port = system.srixbar.mem_side_ports
system.engine.sri_afifo.downstream_port = system.engine.sri_port 
system.engine.mem_afifo = DuetAsyncFIFO (
        stage = 2, capacity = 256, 
        upstream_clk_domain = system.engine.clk_domain,
        downstream_clk_domain = system.clk_domain
        )
system.engine.rob = DuetReorderBuffer ();
system.engine.dcache = L1Cache (
        addr_ranges = [range_]
        , tgts_per_mshr = 1
        , mshrs = 64
        )
system.engine.mem_ports = system.engine.mem_afifo.upstream_port
system.engine.mem_afifo.downstream_port = system.engine.rob.upstream
system.engine.rob.downstream = system.engine.dcache.cpu_side
system.engine.dcache.mem_side = system.l2bus.cpu_side_ports

# -- L2 Cache ----------------------------------------------------------------
system.l2 = L2Cache ()
system.l2.cpu_side = system.l2bus.mem_side_ports
system.l2.mem_side = system.membus.cpu_side_ports
system.mem_ctrl.port = system.membus.mem_side_ports
system.system_port = system.membus.cpu_side_ports

binary = os.path.join (os.path.dirname (os.path.abspath(__file__)),
        "../../tests/test-progs/duet/bin/riscv/linux/test_barnes")
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
system.engine.process = process

root = Root (full_system = False, system = system)
m5.instantiate ()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
