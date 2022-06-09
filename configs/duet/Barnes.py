import sys, os, argparse

import m5
from m5.objects import *
from m5.util import addToPath

addToPath('../')

from duet.util import add_common_arguments, build_system_and_process

parser = argparse.ArgumentParser ()
add_common_arguments ( parser )

parser.add_argument ( "--duet-cache", dest="duetcache", type=str, default="hard"
        , choices=["soft","hard","both","none", "io"] )

args = parser.parse_args ()
system, process = build_system_and_process ( args )

system.engine = DuetBarnesEngine (
        num_callers = args.numcpus,
        baseaddr    = args.duet_addr
        )
system.engine.clk_domain = SrcClockDomain (
        clock = '500MHz',
        voltage_domain = VoltageDomain () )
system.engine.sri_afifo = DuetAsyncFIFO (
        stage = 2, capacity = 256, 
        upstream_clk_domain = system.clk_domain,
        downstream_clk_domain = system.engine.clk_domain
        )
system.engine.sri_afifo.upstream_port = system.ocdbus.mem_side_ports
system.engine.sri_afifo.downstream_port = system.engine.sri_port

system.engine.rob = DuetReorderBuffer ()
system.engine.mem_ports = system.engine.rob.upstream
if args.duetcache in ["soft", "both"]:
    system.engine.mem_afifo = DuetAsyncFIFO (
            stage = 4, capacity = 256, snooping = True,
            upstream_clk_domain = system.engine.clk_domain,
            downstream_clk_domain = system.clk_domain
            )
    system.engine.softcache = Cache (
            size                = args.l1d_size,
            assoc               = args.l1d_assoc,
            tag_latency         = args.l1d_tlat,
            data_latency        = args.l1d_dlat,
            response_latency    = args.l1d_rlat,
            mshrs               = args.l1d_mshrs,
            tgts_per_mshr       = 8,
            addr_ranges         = [ AddrRange ( args.memsize ) ],
            )
    system.engine.rob.downstream = system.engine.softcache.cpu_side
    system.engine.softcache.mem_side = system.engine.mem_afifo.upstream_port
else:
    system.engine.mem_afifo = DuetAsyncFIFO (
            stage = 4, capacity = 256,
            upstream_clk_domain = system.engine.clk_domain,
            downstream_clk_domain = system.clk_domain
            )
    system.engine.rob.downstream = system.engine.mem_afifo.upstream_port

if args.duetcache in ["hard", "both"]:
    system.engine.hardcache = Cache (
            clk_domain          = system.clk_domain,
            size                = args.l1d_size,
            assoc               = args.l1d_assoc,
            tag_latency         = args.l1d_tlat,
            data_latency        = args.l1d_dlat,
            response_latency    = args.l1d_rlat,
            mshrs               = args.l1d_mshrs,
            tgts_per_mshr       = 8,
            addr_ranges         = [ AddrRange ( args.memsize ) ],
            )
    system.engine.mem_afifo.downstream_port = system.engine.hardcache.cpu_side
    system.engine.hardcache.mem_side = system.llcbus.cpu_side_ports
elif args.duetcache == "io":
    system.engine.mem_afifo.downstream_port = system.membus.cpu_side_ports
else:
    system.engine.mem_afifo.downstream_port = system.llcbus.cpu_side_ports

process.drivers = DuetDriver (
        filename = "duet",
        range = AddrRange ( args.duet_addr, size=args.duet_size )
        )
system.engine.process = process

root = Root (full_system = False, system = system)
m5.instantiate ()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
