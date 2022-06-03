from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine

class DuetBarnesMemLane (DuetSimpleLane):
    type        = "DuetBarnesMemLane"
    cxx_class   = "gem5::duet::DuetBarnesMemLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesMemLane.hh"

    def __init__ (self, **kwargs):
        ft = kwargs.pop ( "transition_from_stage", [0,1,2,3] )
        tt = kwargs.pop ( "transition_to_stage",   [1,2,3,4] )
        l  = kwargs.pop ( "transition_latency",    [1,1,1,1] )

        super().__init__(**kwargs)

        self.transition_from_stage = ft
        self.transition_to_stage   = tt
        self.transition_latency    = l

class DuetBarnesComputeLane (DuetPipelinedLane):
    type        = "DuetBarnesComputeLane"
    cxx_class   = "gem5::duet::DuetBarnesComputeLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesComputeLane.hh"

    def __init__ (self, **kwargs):
        ft = kwargs.pop ( "transition_from_stage", [0,0,1,  2,3] )
        tt = kwargs.pop ( "transition_to_stage",   [0,1,2,  3,3] )
        l  = kwargs.pop ( "transition_latency",    [1,1,148,5,1] )
        ii = kwargs.pop ( "interval",              8 )

        super().__init__(**kwargs)

        self.transition_from_stage = ft
        self.transition_to_stage   = tt
        self.transition_latency    = l
        self.interval              = ii

class DuetBarnesAccumulatorLane (DuetSimpleLane):
    type        = "DuetBarnesAccumulatorLane"
    cxx_class   = "gem5::duet::DuetBarnesAccumulatorLane"
    cxx_header  = "duet/engine/barnes_gravsub/DuetBarnesAccumulatorLane.hh"

    def __init__ (self, **kwargs):
        ft = kwargs.pop ( "transition_from_stage", [0] )
        tt = kwargs.pop ( "transition_to_stage",   [0] )
        l  = kwargs.pop ( "transition_latency",    [1] )
        tl = kwargs.pop ( "postrun_latency",       9 )

        super().__init__(**kwargs)

        self.transition_from_stage = ft
        self.transition_to_stage   = tt
        self.transition_latency    = l
        self.postrun_latency       = tl

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
