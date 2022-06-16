#define __DUET_HLS

#include "stdio.h"
#include "fmm/DuetFmmVLIComputeFunctor.hh"

int main ( int argc, char * argv[] ) {
    DuetFmmVLIComputeFunctor dut;

    DuetFunctor::chan_data_t chan_input, chan_output;
    DuetFunctor::S64 Expansion_Terms = 16;

    dut.kernel ( chan_input, chan_output, Expansion_Terms );

    return 0;
}
