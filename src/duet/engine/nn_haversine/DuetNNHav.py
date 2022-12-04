from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine

class DuetNNHavMemLane (DuetSimpleLane):
    type        = "DuetNNHavMemLane"
    cxx_class   = "gem5::duet::DuetNNHavMemLane"
    cxx_header  = "duet/engine/nn_haversine/DuetNNHavMemLane.hh"

    transition_from_stage   = [0, 1]
    transition_to_stage     = [1, 1]
    transition_latency      = [1, 1]

class DuetNNHavComputeLane (DuetPipelinedLane):
    type        = "DuetNNHavComputeLane"
    cxx_class   = "gem5::duet::DuetNNHavComputeLane"
    cxx_header  = "duet/engine/nn_haversine/DuetNNHavComputeLane.hh"

    transition_from_stage   = [0]
    transition_to_stage     = [1]
    transition_latency      = [65]
    interval                = 1

class DuetNNHavReductionLane (DuetSimpleLane):
    type        = "DuetNNHavReductionLane"
    cxx_class   = "gem5::duet::DuetNNHavReductionLane"
    cxx_header  = "duet/engine/nn_haversine/DuetNNHavReductionLane.hh"

    transition_from_stage   = [0]
    transition_to_stage     = [0]
    transition_latency      = [1]
    postrun_latency         = 2

class DuetNNHavEngine (DuetEngine):
    type        = "DuetNNHavEngine"
    cxx_class   = "gem5::duet::DuetNNHavEngine"
    cxx_header  = "duet/engine/nn_haversine/DuetNNHavEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetNNHavMemLane (),
                DuetNNHavComputeLane (),
                DuetNNHavReductionLane ()
                ]
