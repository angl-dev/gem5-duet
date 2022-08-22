from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine
from m5.objects.DuetBarnes import DuetBarnesAccumulatorLane

class DuetKNNMemLane (DuetSimpleLane):
    type        = "DuetKNNMemLane"
    cxx_class   = "gem5::duet::DuetKNNMemLane"
    cxx_header  = "duet/engine/knn/DuetKNNMemLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,6,7,8,9]
    transition_to_stage     = [1,2,3,4,5,6,7,8,9,10]
    transition_latency      = [1,1,1,1,1,1,1,1,1,1]

class DuetKNNComputeLane (DuetPipelinedLane):
    type        = "DuetKNNComputeLane"
    cxx_class   = "gem5::duet::DuetKNNComputeLane"
    cxx_header  = "duet/engine/knn/DuetKNNComputeLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,6,7,8,  9,10,11]
    transition_to_stage     = [1,2,3,4,5,6,7,8,9, 10,11,11]
    transition_latency      = [1,1,1,1,1,1,1,1,1,141, 7, 3]
    interval                = 14

class DuetKNNEngine (DuetEngine):
    type        = "DuetKNNEngine"
    cxx_class   = "gem5::duet::DuetKNNEngine"
    cxx_header  = "duet/engine/knn/DuetKNNEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetKNNMemLane (), 
                DuetKNNComputeLane (),
                DuetBarnesAccumulatorLane ()
                ]
