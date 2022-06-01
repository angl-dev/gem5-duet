import m5, os
from m5.objects import *

num_threads = 4
range_      = AddrRange('8192MB')

nc_base     = 0xE10298000
nc_range    = AddrRange(nc_base, size='4kB')

system = System (
        mem_mode = 'timing',
        mem_ranges = [range_, nc_range]
        )

system.clk_domain = SrcClockDomain( clock = '1GHz', voltage_domain = VoltageDomain() )
system.cpus = [TimingSimpleCPU() for _ in range(num_threads)]
system.mem_ctrl = MemCtrl( dram = DDR3_1600_8x8( range = range_ ) )
system.engine = NaiveEngine ( pipelined = True )
system.membus = SystemXBar()

for cpu in system.cpus:
    cpu.createInterruptController()
    cpu.icache_port = system.membus.cpu_side_ports
    cpu.dcache_port = system.membus.cpu_side_ports

# system.engine.lanes[0].transition_from_stage = [0, 1, 2, 3, 4]
# system.engine.lanes[0].transition_to_stage   = [1, 2, 3, 4, 5]
# system.engine.lanes[0].transition_latency    = [1, 1, 1, 1, 1]
system.engine.lanes[0].latency = [5000, 5000, 5000, 5000]
system.engine.lanes[0].interval = 4999
system.engine.fifo_capacity = 32

system.system_port          = system.membus.cpu_side_ports
system.engine.num_callers   = num_threads
system.engine.baseaddr      = nc_base
system.engine.sri_port      = system.membus.mem_side_ports
system.engine.mem_ports     = system.membus.cpu_side_ports
system.mem_ctrl.port        = system.membus.mem_side_ports

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
