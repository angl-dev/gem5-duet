from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetLane import DuetSimpleLane
from m5.objects.DuetEngine import DuetEngine

class DuetFmmVLIFrontendLane (DuetSimpleLane):
    type        = "DuetFmmVLIFrontendLane"
    cxx_class   = "gem5::duet::DuetFmmVLIFrontendLane"
    cxx_header  = "duet/engine/fmm/DuetFmmVLIFrontendLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,6,7,7]
    transition_to_stage     = [1,2,3,4,5,6,7,6,8]
    transition_latency      = [1,1,1,1,1,1,1,1,1]

class DuetFmmVLIComputeLane (DuetSimpleLane):
    type        = "DuetFmmVLIComputeLane"
    cxx_class   = "gem5::duet::DuetFmmVLIComputeLane"
    cxx_header  = "duet/engine/fmm/DuetFmmVLIComputeLane.hh"

    transition_from_stage   = [0,1,2,3,4,5,  5,6, 7]
    transition_to_stage     = [1,2,3,4,5,4,  6,7, 6]
    transition_latency      = [1,1,1,1,1,1,521,1,24]

class DuetFmmVLIBackendLane (DuetSimpleLane):
    type        = "DuetFmmVLIBackendLane"
    cxx_class   = "gem5::duet::DuetFmmVLIBackendLane"
    cxx_header  = "duet/engine/fmm/DuetFmmVLIBackendLane.hh"

    transition_from_stage   = [0,1,2,2,3,4,5,6,7,8, 9,10,10,11,12]
    transition_to_stage     = [1,2,1,3,4,5,6,7,8,9,10, 3,11,12,11]
    transition_latency      = [1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1, 1]

class DuetFmmVLIEngine (DuetEngine):
    type        = "DuetFmmVLIEngine"
    cxx_class   = "gem5::duet::DuetFmmVLIEngine"
    cxx_header  = "duet/engine/fmm/DuetFmmVLIEngine.hh"

    def __init__ (self, **kwargs):
        super().__init__(**kwargs)

        self.lanes = [
                DuetFmmVLIFrontendLane (),
                DuetFmmVLIComputeLane (),
                DuetFmmVLIBackendLane (),
                ]
