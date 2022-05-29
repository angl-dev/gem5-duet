from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.SimObject import SimObject

class DuetLane (SimObject):
    type                    = "DuetLane"
    cxx_class               = "gem5::duet::DuetLane"
    cxx_header              = "duet/engine/DuetLane.hh"
    abstract                = True

    transition_from_stage   = VectorParam.UInt32 ( "From stages ..." )
    transition_to_stage     = VectorParam.UInt32 ( "To stages ..." )
    transition_latency      = VectorParam.Cycles ( "Latency ..." )

class DuetSimpleLane (DuetLane):
    type                    = "DuetSimpleLane"
    cxx_class               = "gem5::duet::DuetSimpleLane"
    cxx_header              = "duet/engine/DuetSimpleLane.hh"
    abstract                = True
