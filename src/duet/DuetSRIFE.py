from m5.params import *
from m5.objects.ClockedObject import ClockedObject

class DuetSRIFE (ClockedObject):
    type        = "DuetSRIFE"
    cxx_class   = "gem5::DuetSRIFE"
    cxx_header  = "duet/DuetSRIFE.hh"

    range       = Param.AddrRange ("Physical memory space")
    inport      = ResponsePort ("Input port")
