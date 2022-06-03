from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane
from m5.objects.DuetEngine import DuetEngine

class DuetBarnesMemLane (DuetSimpleLane):
    type        = "DuetBarnesMemLane"
    cxx_class   = "gem5::duet::DuetBarnesMemLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesMemLane.hh"

class DuetBarnesComputeLane (DuetSimpleLane):
    type        = "DuetBarnesComputeLane"
    cxx_class   = "gem5::duet::DuetBarnesComputeLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesComputeLane.hh"

class DuetBarnesAccumulatorLane (DuetSimpleLane):
    type        = "DuetBarnesAccumulatorLane"
    cxx_class   = "gem5::duet::DuetBarnesAccumulatorLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesAccumulatorLane.hh"

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
