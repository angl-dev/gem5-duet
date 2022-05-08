from m5.objects.Process import EmulatedDriver

class DuetDriver (EmulatedDriver):
    type = "DuetDriver"
    cxx_class = "gem5::DuetDriver"
    cxx_header = "duet/DuetDriver.hh"
