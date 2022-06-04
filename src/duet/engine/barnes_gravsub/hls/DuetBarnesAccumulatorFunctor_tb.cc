#define __DUET_HLS

#include "stdio.h"
#include "barnes_gravsub/DuetBarnesAccumulatorFunctor.hh"

int main ( int argc, char * argv[] ) {
    DuetBarnesAccumulatorFunctor dut;

    DuetFunctor::chan_data_t chan_input;

    DuetFunctor::Double phii ( 1.0f ), phio,
                        accx ( 1.0f ), accxo,
                        accy ( 1.0f ), accyo,
                        accz ( 1.0f ), acczo;

    chan_input.write ( 0 );
    chan_input.write ( 0 );
    chan_input.write ( 0 );
    chan_input.write ( 0 );

    dut.kernel ( chan_input, phii, accx, accy, accz, phio, accxo, accyo, acczo );

    return 0;
}
