import sys, os, argparse

import m5, math
from m5.objects import *
from m5.util import addToPath

addToPath('../')

from duet.util import add_common_arguments, build_system_and_process, integrate

parser = argparse.ArgumentParser ()
add_common_arguments ( parser )

args = parser.parse_args ()
system, process = build_system_and_process ( args )

system.engines = [DuetNNHavEngine (
        num_callers = args.numcpus,
        baseaddr    = args.duet_addr + (i << 13),   # 8kB per engine
        ) for i in range (args.numengines)]

for engine in system.engines:
    integrate ( args, system, process, engine )

# ------ Yanwen addition -----

system.cpus[0].workload[0].release = "99.99.99"

# ----------------------------

process.drivers = DuetDriver (
        filename = "duet",
        range = AddrRange ( args.duet_addr, size=args.duet_size )
        )

root = Root (full_system = False, system = system)
m5.instantiate ()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
