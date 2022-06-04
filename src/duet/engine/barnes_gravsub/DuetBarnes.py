from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine

class DuetBarnesMemLane (DuetSimpleLane):
    type        = "DuetBarnesMemLane"
    cxx_class   = "gem5::duet::DuetBarnesMemLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesMemLane.hh"

    transition_from_stage   = [0,1,2,3]
    transition_to_stage     = [1,2,3,4]
    transition_latency      = [1,1,1,1]

class DuetBarnesComputeLane (DuetPipelinedLane):
    type        = "DuetBarnesComputeLane"
    cxx_class   = "gem5::duet::DuetBarnesComputeLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesComputeLane.hh"

    transition_from_stage   = [0,0,1,  2,3]
    transition_to_stage     = [0,1,2,  3,3]
    transition_latency      = [1,1,148,5,1]
    interval                = 8

class DuetBarnesAccumulatorLane (DuetSimpleLane):
    type        = "DuetBarnesAccumulatorLane"
    cxx_class   = "gem5::duet::DuetBarnesAccumulatorLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesAccumulatorLane.hh"

    transition_from_stage   = [0]
    transition_to_stage     = [0]
    transition_latency      = [1]
    postrun_latency         = 9

class DuetBarnesEngine (DuetEngine):
    type        = "DuetBarnesEngine"
    cxx_class   = "gem5::duet::DuetBarnesEngine"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetBarnesMemLane (), 
                DuetBarnesComputeLane (),
                DuetBarnesAccumulatorLane ()
                ]
