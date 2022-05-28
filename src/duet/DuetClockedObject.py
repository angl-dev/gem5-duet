from m5.params import *
from m5.proxy import *
from m5.objects import *

class DuetClockedObject (ClockedObject):
    type        = "DuetClockedObject"
    cxx_class   = "gem5::duet::DuetClockedObject"
    cxx_header  = "duet/engine/DuetClockedObject.hh"
