from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine


class BatchedBarnesMemLane (DuetSimpleLane):
    type = "BatchedBarnesMemLane"
    cxx_class = "gem5::duet::BatchedBarnesMemLane"
    cxx_header = "duet/engine/batched_barnes/BatchedBarnesMemLane.hh"

    transition_from_stage = [0, 1]
    transition_to_stage = [1, 1]
    transition_latency = [1, 1]


class BatchedBarnesComputeLane (DuetPipelinedLane):
    type = "BatchedBarnesComputeLane"
    cxx_class = "gem5::duet::BatchedBarnesComputeLane"
    cxx_header = "duet/engine/batched_barnes/BatchedBarnesComputeLane.hh"

    transition_from_stage = [0]
    transition_to_stage = [1]
    transition_latency = [65]
    interval = 1


class BatchedBarnesReductionLane (DuetSimpleLane):
    type = "BatchedBarnesReductionLane"
    cxx_class = "gem5::duet::BatchedBarnesReductionLane"
    cxx_header = "duet/engine/batched_barnes/BatchedBarnesReductionLane.hh"

    transition_from_stage = [0]
    transition_to_stage = [0]
    transition_latency = [1]
    postrun_latency = 2


class BatchedBarnesEngine (DuetEngine):
    type = "BatchedBarnesEngine"
    cxx_class = "gem5::duet::BatchedBarnesEngine"
    cxx_header = "duet/engine/batched_barnes/BatchedBarnesEngine.hh"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
            BatchedBarnesMemLane(),
            BatchedBarnesComputeLane(),
            BatchedBarnesReductionLane()
        ]
