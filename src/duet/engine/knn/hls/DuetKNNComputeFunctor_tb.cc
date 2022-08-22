#define __DUET_HLS

#include "stdio.h"
#include "knn/DuetKNNComputeFunctor.hh"

int main ( int argc, char * argv[] ) {
    DuetKNNComputeFunctor dut;

    DuetFunctor::chan_data_t chan_input, chan_output;
    const DuetFunctor::Double pos0x ( 0.5f ),
                              pos0y ( 0.5f ),
                              pos0z ( 0.5f ),
                              pos0w ( 0.5f ),
                            //   epssq ( 1e-8 ),
                              posx  ( 0.7f ),
                              posy  ( 0.2f ),
                              posz  ( 0.3f ),
                              posw  ( 0.4f );


    chan_input.write ( posx.data_ac_int () );
    chan_input.write ( posy.data_ac_int () );
    chan_input.write ( posz.data_ac_int () );
    chan_input.write ( posw.data_ac_int () );
   

    dut.kernel ( chan_input, chan_output, pos0x, pos0y, pos0z, pos0w );

    DuetFunctor::Double dist;
    dist.set_data ( chan_output.read () );

    printf ( "dist (%f)\n", dist.to_double () );

    return 0;
}
