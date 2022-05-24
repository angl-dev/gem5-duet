from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.ClockedObject import ClockedObject

class DuetWidget (ClockedObject):
    type        = "DuetWidget"
    cxx_class   = "gem5::duet::DuetWidget"
    cxx_header  = "duet/widget/widget.hh"

    system              = Param.System ( Parent.any, "System object" )
    process             = Param.Process ( "Process running on the host processors" )
    fifo_capacity       = Param.Unsigned ( 64, "FIFO capacity" )
    range               = Param.AddrRange ( "Register addresses" )

    sri_port            = ResponsePort ( "SRI response port" )
    mem_port            = RequestPort ( "Memory request port" )

    latency_per_stage   = VectorParam.Cycles ( "Latency per stage, in cycles" )
    interval_per_stage  = VectorParam.Cycles (
            "Initiation inverval per stage, in cycles. "
            "0 -> multi-entry not allowed" )

    abstract            = True

class DuetWidgetNaive (DuetWidget):
    type        = "DuetWidgetNaive"
    cxx_class   = "gem5::duet::DuetWidgetNaive"
    cxx_header  = "duet/widget/naive.hh"

    abstract            = False
