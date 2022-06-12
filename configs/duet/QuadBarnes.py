import sys, os, argparse

import m5, math
from m5.objects import *
from m5.util import addToPath

addToPath('../')

from duet.util import add_common_arguments, build_system_and_process

parser = argparse.ArgumentParser ()
add_common_arguments ( parser )

parser.add_argument ( "--num-engines",  dest="numengines",  type=int, default=1 )
parser.add_argument ( "--duet-clk",     dest="duetclk",     type=str, default="200MHz" )
parser.add_argument ( "--duet-cache",   dest="duetcache",   type=str, default="hard"
        , choices=["soft","hard","both","none", "io"] )

args = parser.parse_args ()
system, process = build_system_and_process ( args )

system.engines = [DuetBarnesQuadEngine (
        num_callers = args.numcpus,
        baseaddr    = args.duet_addr + (i << 13),   # 8kB per engine
        ) for i in range (args.numengines)]

for engine in system.engines:
    engine.process = process
    engine.clk_domain = SrcClockDomain (
            clock = args.duetclk,
            voltage_domain = VoltageDomain () )
    engine.sri_afifo = DuetAsyncFIFO (
            stage = args.afstage, capacity = args.afcap, 
            upstream_clk_domain = system.clk_domain,
            downstream_clk_domain = engine.clk_domain
            )
    engine.sri_afifo.upstream_port = system.ocdbus.mem_side_ports
    engine.sri_afifo.downstream_port = engine.sri_port
    
    engine.rob = DuetReorderBuffer ()
    engine.mem_ports = engine.rob.upstream

    # create soft cache if specified
    if args.duetcache in ["soft", "both"]:
        engine.softcache = Cache (
                size                = args.l1d_size,
                assoc               = args.l1d_assoc,
                tag_latency         = args.l1d_tlat,
                data_latency        = args.l1d_dlat,
                response_latency    = args.l1d_rlat,
                mshrs               = args.l1d_mshrs,
                tgts_per_mshr       = 8,
                addr_ranges         = [ AddrRange ( args.memsize ) ],
                )
        engine.rob.downstream = engine.softcache.cpu_side

    # create hard cache if specified
    if args.duetcache in ["hard", "both"]:
        engine.hardcache = Cache (
                clk_domain          = system.clk_domain,
                size                = args.l2_size,
                assoc               = args.l2_assoc,
                tag_latency         = args.l2_tlat,
                data_latency        = args.l2_dlat,
                response_latency    = args.l2_rlat,
                mshrs               = args.l2_mshrs,
                tgts_per_mshr       = 8,
                addr_ranges         = [ AddrRange ( args.memsize ) ],
                )
        engine.hardcache.mem_side = system.llcbus.cpu_side_ports

    # instantiate interconnects
    if args.duetcache == "both":
        # use L2XBar for this case
        ratio = int(math.ceil(
            float (engine.clk_domain.clock[0].period) /
            float (system.clk_domain.clock[0].period)
            ))
        engine.xbar = L2XBar (
                clk_domain          = system.clk_domain,
                width               = args.clsize,
                frontend_latency    = ratio,
                forward_latency     = args.afstage,
                response_latency    = ratio * args.afstage + 1
                )
        engine.softcache.mem_side = engine.xbar.cpu_side_ports
        engine.hardcache.cpu_side = engine.xbar.mem_side_ports

    elif args.duetcache == "soft":
        engine.mem_afifo = DuetAsyncFIFO (
                stage = args.afstage,
                capacity = args.afcap,
                snooping = True,
                upstream_clk_domain = engine.clk_domain,
                downstream_clk_domain = system.clk_domain
                )
        engine.softcache.mem_side = engine.mem_afifo.upstream_port
        engine.mem_afifo.downstream_port = system.llcbus.cpu_side_ports

    else:
        engine.mem_afifo = DuetAsyncFIFO (
                stage = args.afstage,
                capacity = args.afcap,
                snooping = False,
                upstream_clk_domain = engine.clk_domain,
                downstream_clk_domain = system.clk_domain
                )
        engine.rob.downstream = engine.mem_afifo.upstream_port
        if args.duetcache == "hard":
            engine.mem_afifo.downstream_port = engine.hardcache.cpu_side
        elif args.duetcache == "none":
            engine.mem_afifo.downstream_port = system.llcbus.cpu_side_ports
        elif args.duetcache == "io":
            engine.mem_afifo.downstream_port = system.membus.cpu_side_ports
    
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
