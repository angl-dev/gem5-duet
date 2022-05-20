from m5.params import *
from m5.objects.Process import EmulatedDriver

class DuetDriver (EmulatedDriver):
    type = "DuetDriver"
    cxx_class = "gem5::duet::DuetDriver"
    cxx_header = "duet/DuetDriver.hh"

    dev = Param.DuetWidget ("Underlying widget")
