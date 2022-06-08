from m5.params import *
from m5.proxy import Parent
from m5.SimObject import SimObject
from m5.objects.ClockedObject import ClockedObject

class DuetAsyncFIFOCtrl (ClockedObject):
    type            = "DuetAsyncFIFOCtrl"
    cxx_class       = "gem5::duet::DuetAsyncFIFOCtrl"
    cxx_header      = "duet/DuetAsyncFIFOCtrl.hh"

    is_upstream     = Param.Bool ("If this is an upstream ctrl or a downstream ctrl")

class DuetAsyncFIFO (SimObject):
    type            = "DuetAsyncFIFO"
    cxx_class       = "gem5::duet::DuetAsyncFIFO"
    cxx_header      = "duet/DuetAsyncFIFO.hh"

    stage           = Param.Cycles      ( 2, "Synchronizer stage" )
    capacity        = Param.Unsigned    ( 64, "FIFO capacity (per direction)" )
    upstream_port   = ResponsePort      ( "Response port to upstream" )
    downstream_port = RequestPort       ( "Request port to downstream" )
    upstream_ctrl   = Param.DuetAsyncFIFOCtrl ( "Upstream ctrl" )
    downstream_ctrl = Param.DuetAsyncFIFOCtrl ( "Downstream ctrl" )
    snooping        = Param.Bool        ( False, "If the Async FIFO supports snooping across" )

    def __init__ (self, **kwargs):
        upstream_clk_domain = kwargs.pop ("upstream_clk_domain", None)
        downstream_clk_domain = kwargs.pop ("downstream_clk_domain", None)

        super().__init__(**kwargs)

        self.upstream_ctrl = DuetAsyncFIFOCtrl ( is_upstream = True )
        if upstream_clk_domain:
            self.upstream_ctrl.clk_domain = upstream_clk_domain

        self.downstream_ctrl = DuetAsyncFIFOCtrl ( is_upstream = False )
        if downstream_clk_domain:
            self.downstream_ctrl.clk_domain = downstream_clk_domain

    def __getattr__ (self, attr):
        if attr == "upstream_clk_domain":
            return self.upstream_ctrl.clk_domain
        elif attr == "downstream_clk_domain":
            return self.downstream_ctrl.clk_domain
        else:
            return super().__getattr__(attr)

    def __setattr__ (self, attr, val):
        if attr == "upstream_clk_domain":
            self.upstream_ctrl.clk_domain = val
        elif attr == "downstream_clk_domain":
            self.downstream_ctrl.clk_domain = val
        else:
            super().__setattr__(attr, val)
