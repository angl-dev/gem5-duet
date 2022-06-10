from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine
from m5.objects.DuetBarnes import DuetBarnesAccumulatorLane

class DuetBarnesQuadMemLane (DuetSimpleLane):
    type        = "DuetBarnesQuadMemLane"
    cxx_class   = "gem5::duet::DuetBarnesQuadMemLane"
    cxx_header  = "duet/engine/barnes_gravsub_quad/DuetBarnesQuadMemLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,6,7,8,9]
    transition_to_stage     = [1,2,3,4,5,6,7,8,9,10]
    transition_latency      = [1,1,1,1,1,1,1,1,1,1]

class DuetBarnesQuadComputeLane (DuetPipelinedLane):
    type        = "DuetBarnesQuadComputeLane"
    cxx_class   = "gem5::duet::DuetBarnesQuadComputeLane"
    cxx_header  = "duet/engine/barnes_gravsub_quad/DuetBarnesQuadComputeLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,6,7,8,  9,10,11]
    transition_to_stage     = [1,2,3,4,5,6,7,8,9, 10,11,11]
    transition_latency      = [1,1,1,1,1,1,1,1,1,150, 1, 1]
    interval                = 14

class DuetBarnesQuadEngine (DuetEngine):
    type        = "DuetBarnesQuadEngine"
    cxx_class   = "gem5::duet::DuetBarnesQuadEngine"
    cxx_header  = "duet/engine/barnes_gravsub_quad/DuetBarnesQuadEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetBarnesQuadMemLane (), 
                DuetBarnesQuadComputeLane (),
                DuetBarnesAccumulatorLane ()
                ]

    @property
    def maxaddr (self):
        return self.baseaddr + ( self.num_callers + 1 ) * 128
