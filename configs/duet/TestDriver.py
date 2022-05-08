import m5, os
from m5.objects import *

range_ = AddrRange('512MB')

system = System(
        clk_domain = SrcClockDomain(
            clock = '1GHz',
            voltage_domain = VoltageDomain(),
            ),
        mem_mode = 'timing',
        mem_ranges = [range_],
        cpu = TimingSimpleCPU(),
        mem_ctrl = MemCtrl(
            dram = DDR3_1600_8x8( range = range_ ),
            ),
        membus = SystemXBar(),
        )

system.cpu.createInterruptController()
system.cpu.icache_port  = system.membus.cpu_side_ports
system.cpu.dcache_port  = system.membus.cpu_side_ports
system.system_port      = system.membus.cpu_side_ports
system.mem_ctrl.port    = system.membus.mem_side_ports

binary = os.path.join (os.path.dirname (os.path.abspath(__file__)),
        "../../tests/test-progs/duet/bin/riscv/linux/test_driver")

process = Process(
        cmd = [binary],
        drivers = DuetDriver(filename = "duet")
        )
system.cpu.workload = process
system.cpu.createThreads()

system.workload = SEWorkload.init_compatible (binary)

root = Root (full_system = False, system = system)
m5.instantiate ()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
