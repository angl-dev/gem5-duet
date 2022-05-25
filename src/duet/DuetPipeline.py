from m5.params import *
from m5.objects.ClockedObject import ClockedObject

class DuetPipeline (ClockedObject):
    type                = "DuetPipeline"
    cxx_class           = "gem5::duet::DuetPipeline"
    cxx_header          = "duet/DuetPipeline.hh"

    upstream            = ResponsePort  ( "Response port to upstream" )
    downstream          = RequestPort   ( "Request port to downstream" )
    upward_latency      = Param.Cycles  ( 1, "Upward latency" )
    upward_interval     = Param.Cycles  ( 1, "Upward interval" )
    downward_latency    = Param.Cycles  ( 1, "Downward latency" )
    downward_interval   = Param.Cycles  ( 1, "Downward interval" )
