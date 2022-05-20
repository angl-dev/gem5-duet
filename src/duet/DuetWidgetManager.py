from m5.params import *
from m5.objects import *
from m5.objects.ClockedObject import ClockedObject

class DuetWidgetManager (ClockedObject):
    type        = "DuetWidgetManager"
    cxx_class   = "gem5::duet::DuetWidgetManager"
    cxx_header  = "duet/DuetWidgetManager.hh"

    system          = Param.System ( Parent.any, "System object" )
    fifo_capacity   = Param.Unsigned ( 64, "Capacity of internal FIFOs" )
    range           = Param.AddrRange ("Register addresses")
    sri_port        = ResponsePort ("SRI response port")
    mem_port        = RequestPort ("Memory request port")
    widget          = Param.DuetWidget ("Widget class")
