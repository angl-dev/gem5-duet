from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine

class DuetNNMemLane (DuetSimpleLane):
    type        = "DuetNNMemLane"
    cxx_class   = "gem5::duet::DuetNNMemLane"
    cxx_header  = "duet/engine/nn/DuetNNMemLane.hh"

    transition_from_stage   = [0, 1]
    transition_to_stage     = [1, 1]
    transition_latency      = [1, 1]

class DuetNNComputeLane (DuetPipelinedLane):
    type        = "DuetNNComputeLane"
    cxx_class   = "gem5::duet::DuetNNComputeLane"
    cxx_header  = "duet/engine/nn/DuetNNComputeLane.hh"

    transition_from_stage   = [0]
    transition_to_stage     = [1]
    transition_latency      = [65]
    interval                = 1

class DuetNNReductionLane (DuetSimpleLane):
    type        = "DuetNNReductionLane"
    cxx_class   = "gem5::duet::DuetNNReductionLane"
    cxx_header  = "duet/engine/nn/DuetNNReductionLane.hh"

    transition_from_stage   = [0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    transition_to_stage     = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9]
    transition_latency      = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    postrun_latency         = 2

class DuetNNEngine (DuetEngine):
    type        = "DuetNNEngine"
    cxx_class   = "gem5::duet::DuetNNEngine"
    cxx_header  = "duet/engine/nn/DuetNNEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetNNMemLane (),
                DuetNNComputeLane (),
                DuetNNReductionLane ()
                ]
