from m5.params import *
from m5.proxy import *
from m5.objects import *
from m5.objects.DuetClockedObject import DuetClockedObject

class DuetReorderBuffer (DuetClockedObject):
    type            = "DuetReorderBuffer"
    cxx_class       = "gem5::duet::DuetReorderBuffer"
    cxx_header      = "duet/engine/DuetReorderBuffer.hh"

    capacity        = Param.Unsigned ( 64, "Max. number of entries in the reorder buffer" )
    upstream        = ResponsePort ( "Port for upstream" )
    downstream      = RequestPort ( "Port for downstream" )

