from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane, DuetPipelinedLane
from m5.objects.DuetEngine import DuetEngine

class NaiveLane (DuetSimpleLane):
    type        = "NaiveLane"
    cxx_class   = "gem5::duet::NaiveLane"
    cxx_header  = "duet/engine/naive/lane.hh"

class NaivePipelinedLane (DuetPipelinedLane):
    type        = "NaivePipelinedLane"
    cxx_class   = "gem5::duet::NaivePipelinedLane"
    cxx_header  = "duet/engine/naive/ppllane.hh"

class NaiveEngine (DuetEngine):
    type        = "NaiveEngine"
    cxx_class   = "gem5::duet::NaiveEngine"
    cxx_header  = "duet/engine/naive/engine.hh"

    def __init__ (self, **kwargs):
        pipelined = kwargs.pop ('pipelined', False)

        super().__init__(**kwargs)

        if pipelined:
            self.lanes = [NaivePipelinedLane()]
        else:
            self.lanes = [NaiveLane()]
