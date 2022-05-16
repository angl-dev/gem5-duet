from m5.params import *
from m5.objects.ClockedObject import ClockedObject

class DuetYAMIFE (ClockedObject):
    type        = "DuetYAMIFE"
    cxx_class   = "gem5::duet::DuetYAMIFE"
    cxx_header  = "duet/DuetYAMIFE.hh"

    outport     = RequestPort ("Output port")
