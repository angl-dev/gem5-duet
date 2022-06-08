import sys, os, argparse

import m5
from m5.objects import *
from m5.util import addToPath

addToPath('../')

from common import ObjectList
from common.Options import ListCpu, ListMem

# ============================================================================
# == Define Argument Parser ==================================================
# ============================================================================
def add_common_arguments (parser):
    # global configurations
    parser.add_argument ('--sys-clock',         dest='clk',         type=str, default='1GHz')
    parser.add_argument ('--cacheline-size',    dest='clsize',      type=int, default=64)
    parser.add_argument ('--duet-baseaddr',     dest='duet_addr',   type=int, default=0xE00000000)
    parser.add_argument ('--duet-memspace',     dest='duet_size',   type=str, default='1GB')
    
    # cpus
    parser.add_argument ('--list-cpu-types',    action=ListCpu,     nargs=0)
    parser.add_argument ('--cpu-type',          dest='cputype',     type=str, default='TimingSimpleCPU',
                                                choices=ObjectList.cpu_list.get_names() )
    parser.add_argument ('-n', '--num-cpus',    dest='numcpus',     type=int, default=1, metavar='N')
    
    # main memory
    parser.add_argument ('--list-mem-types',    action=ListMem,     nargs=0)
    parser.add_argument ('--mem-type',          dest='memtype',     type=str, default='DDR3_1600_8x8',
                                                choices=ObjectList.mem_list.get_names() )
    parser.add_argument ('--mem-size',          dest='memsize',     type=str, default='8192MB')

    # main bus
    parser.add_argument ('--mem-xbar-width',    dest='mxwidth',     type=int, default=32)
    parser.add_argument ('--mem-xbar-latency',  dest='mxlat',       type=int, default=0)
    
    # cache hierarchy (2-level snoopy cache)
    #   - CPUs' L1D caches
    parser.add_argument ('--l1d-size',          dest='l1d_size',    type=str, default='64kB')
    parser.add_argument ('--l1d-assoc',         dest='l1d_assoc',   type=int, default=4)
    parser.add_argument ('--l1d-tag-latency',   dest='l1d_tlat',    type=int, default=1)
    parser.add_argument ('--l1d-data-latency',  dest='l1d_dlat',    type=int, default=1)
    parser.add_argument ('--l1d-resp-latency',  dest='l1d_rlat',    type=int, default=1)
    parser.add_argument ('--l1d-mshrs',         dest='l1d_mshrs',   type=int, default=8)
    
    #   - CPUs' L1I caches
    parser.add_argument ('--l1i-size',          dest='l1i_size',    type=str, default='64kB')
    parser.add_argument ('--l1i-assoc',         dest='l1i_assoc',   type=int, default=4)
    parser.add_argument ('--l1i-tag-latency',   dest='l1i_tlat',    type=int, default=1)
    parser.add_argument ('--l1i-data-latency',  dest='l1i_dlat',    type=int, default=1)
    parser.add_argument ('--l1i-resp-latency',  dest='l1i_rlat',    type=int, default=1)
    parser.add_argument ('--l1i-mshrs',         dest='l1i_mshrs',   type=int, default=8)
    
    #   - L2 cache
    parser.add_argument ('--l2',                dest='l2_en',       action='store_true', default=False)
    parser.add_argument ('--l2-size',           dest='l2_size',     type=str, default='256kB')
    parser.add_argument ('--l2-assoc',          dest='l2_assoc',    type=int, default=8)
    parser.add_argument ('--l2-tag-latency',    dest='l2_tlat',     type=int, default=2)
    parser.add_argument ('--l2-data-latency',   dest='l2_dlat',     type=int, default=2)
    parser.add_argument ('--l2-resp-latency',   dest='l2_rlat',     type=int, default=2)
    parser.add_argument ('--l2-mshrs',          dest='l2_mshrs',    type=int, default=16)
    
    #   - LLC cache
    parser.add_argument ('--llc-size',          dest='llc_size',    type=str, default='2MB')
    parser.add_argument ('--llc-assoc',         dest='llc_assoc',   type=int, default=8)
    parser.add_argument ('--llc-tag-latency',   dest='llc_tlat',    type=int, default=4)
    parser.add_argument ('--llc-data-latency',  dest='llc_dlat',    type=int, default=4)
    parser.add_argument ('--llc-resp-latency',  dest='llc_rlat',    type=int, default=4)
    parser.add_argument ('--llc-mshrs',         dest='llc_mshrs',   type=int, default=16)

    #   - LLC bus
    parser.add_argument ('--llc-xbar-width',    dest='llcxwidth',   type=int, default=32)
    parser.add_argument ('--llc-xbar-latency',  dest='llcxlat',     type=int, default=0)
    
    # workload
    parser.add_argument ('-b', '--binary',      dest='binary',      type=str)
    parser.add_argument ('--arguments',         dest='arguments',   type=str, nargs='*', default=[])
    parser.add_argument ('-i', '--stdin',       dest='stdin',       type=str,    default=None)
    parser.add_argument ('-o', '--stdout',      dest='stdout',      type=str,    default=None)
    parser.add_argument (      '--stderr',      dest='stderr',      type=str,    default=None)
    parser.add_argument ('--wait-gdb',          dest='gdb', action='store_true', default=False)

    # Common arguments for Duet engines
    parser.add_argument ('--async-fifo-stages',     dest='afstage', type=int, default=2)
    parser.add_argument ('--async-fifo-capacity',   dest='afcap',   type=int, default=256)

# ============================================================================
# == Build Base System based on Arguments ====================================
# ============================================================================
def build_system_and_process ( args ):

    # -- System --------------------------------------------------------------
    memrange = AddrRange ( args.memsize )
    ncrange = AddrRange ( args.duet_addr, size=args.duet_size )

    system = System (
            mem_mode = 'timing',
            mem_ranges = [memrange, ncrange],
            )
    system.clk_domain = SrcClockDomain (
            clock = args.clk,
            voltage_domain = VoltageDomain (),
            )

    # -- Main Memory and Bus -------------------------------------------------
    system.mem_ctrl = MemCtrl (
            dram = ObjectList.mem_list.get ( args.memtype ) (
                range = memrange )
            )
    system.membus = SystemXBar (
            width = args.mxwidth,
            forward_latency = args.mxlat,
            response_latency = args.mxlat
            )
    system.ocdbus = NoncoherentXBar (
            width = 32,
            frontend_latency = 0,
            forward_latency = 0,
            response_latency = 0
            )
    system.llcbus = L2XBar (
            width = args.llcxwidth,
            forward_latency = args.llcxlat,
            response_latency = args.llcxlat
            )
    system.system_port = system.membus.cpu_side_ports
    system.mem_ctrl.port = system.membus.mem_side_ports

    # -- CPUs and L1 Caches --------------------------------------------------
    system.cpus = [ ObjectList.cpu_list.get ( args.cputype ) ()
            for _ in range ( args.numcpus ) ]
    for cpu in system.cpus:
        cpu.icache = Cache (
                size                = args.l1i_size,
                assoc               = args.l1i_assoc,
                tag_latency         = args.l1i_tlat,
                data_latency        = args.l1i_dlat,
                response_latency    = args.l1i_rlat,
                mshrs               = args.l1i_mshrs,
                tgts_per_mshr       = 8,
                is_read_only        = True,
                )
        cpu.dcache = Cache (
                size                = args.l1d_size,
                assoc               = args.l1d_assoc,
                tag_latency         = args.l1d_tlat,
                data_latency        = args.l1d_dlat,
                response_latency    = args.l1d_rlat,
                mshrs               = args.l1d_mshrs,
                tgts_per_mshr       = 8,
                addr_ranges         = memrange
                )
        # Note: extra latency is added between l2 xbar and SRI
        cpu.xbar = NoncoherentXBar (
                width               = 32,
                frontend_latency    = 0,
                forward_latency     = 0,
                response_latency    = 0
                )

        cpu.icache_port = cpu.icache.cpu_side
        cpu.dcache_port = cpu.xbar.cpu_side_ports
        cpu.xbar.mem_side_ports = cpu.dcache.cpu_side
        cpu.xbar.mem_side_ports = system.ocdbus.cpu_side_ports

        cpu.icache.mem_side = system.llcbus.cpu_side_ports
        if not args.l2_en:
            cpu.dcache.mem_side = system.llcbus.cpu_side_ports
        else:
            cpu.l2dcache = Cache (
                    size                = args.l2_size,
                    assoc               = args.l2_assoc,
                    tag_latency         = args.l2_tlat,
                    data_latency        = args.l2_dlat,
                    response_latency    = args.l2_rlat,
                    mshrs               = args.l2_mshrs,
                    tgts_per_mshr       = 8
                    )
            cpu.dcache.mem_side = cpu.l2dcache.cpu_side
            cpu.l2dcache.mem_side = system.llcbus.cpu_side_ports

        cpu.createInterruptController ()

    # -- LLC Caches -----------------------------------------------------------
    system.llc = Cache (
            size                = args.llc_size,
            assoc               = args.llc_assoc,
            tag_latency         = args.llc_tlat,
            data_latency        = args.llc_dlat,
            response_latency    = args.llc_rlat,
            mshrs               = args.llc_mshrs,
            tgts_per_mshr       = 32
            )
    system.llc.cpu_side = system.llcbus.mem_side_ports
    system.llc.mem_side = system.membus.cpu_side_ports

    # -- executable ----------------------------------------------------------
    binary = os.path.abspath ( args.binary )
    process = Process (
            executable = binary,
            cwd = os.getcwd (),
            gid = os.getgid (),
            cmd = [ os.path.abspath ( args.binary ) ] + args.arguments,
            )
    if args.stdin:
        process.input = args.stdin
    if args.stdout:
        process.output = args.stdout
    if args.stderr:
        process.stderr = args.stderr

    system.workload = SEWorkload.init_compatible ( binary )
    for cpu in system.cpus:
        cpu.workload = process
        cpu.createThreads ()
    if args.gdb:
        system.workload.wait_for_remote_gdb = True

    return system, process
