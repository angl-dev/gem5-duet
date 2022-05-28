from m5.params import *
from m5.proxy import *
from m5.objects import *

class DuetLane (SimObject):
    type                    = "DuetLane"
    cxx_class               = "gem5::duet::DuetLane"
    cxx_header              = "duet/engine/DuetLane.hh"

    engine                  = Param.DuetEngine ( "Parent engine" )
    transition_from_stage   = VectorParam.UInt32 ( "From stages ..." )
    transition_to_stage     = VectorParam.UInt32 ( "To stages ..." )
    transition_latency      = VectorParam.Cycles ( "Latency ..." )

    abstract                = True
