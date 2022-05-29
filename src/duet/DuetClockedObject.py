from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.ClockedObject import ClockedObject

class DuetClockedObject (ClockedObject):
    type        = "DuetClockedObject"
    cxx_class   = "gem5::duet::DuetClockedObject"
    cxx_header  = "duet/DuetClockedObject.hh"
    abstract    = True
