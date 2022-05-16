from m5.params import *
from m5.SimObject import SimObject

class DuetWidget (SimObject):
    type        = "DuetWidget"
    cxx_class   = "gem5::duet::DuetWidget"
    cxx_header  = "duet/widget/widget.hh"

    fifo_capacity       = Param.Unsigned ( 64, "FIFO capacity" )
    latency_per_stage   = VectorParam.Cycles ( "Latency per stage, in cycles" )
    interval_per_stage  = VectorParam.Cycles (
            "Initiation inverval per stage, in cycles. "
            "0 -> multi-entry not allowed" )
