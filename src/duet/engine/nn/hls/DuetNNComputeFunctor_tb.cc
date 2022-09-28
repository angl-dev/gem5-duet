#define __DUET_HLS

#include "nn/DuetNNComputeFunctor.hh"
#include "stdio.h"

int main ( int argc, char * argv[] ) {
    DuetNNComputeFunctor dut;

    ac_channel <DuetFunctor::Block<64>> chan_input;
    ac_channel <DuetFunctor::Block<8>> chan_output;

    const DuetFunctor::Double pos0x ( 0.5f ),
                              pos0y ( 0.5f ),
                              pos0z ( 0.5f ),
                              pos0w ( 0.5f ),
                              posx  ( 0.7f ),
                              posy  ( 0.2f ),
                              posz  ( 0.3f ),
                              posw  ( 0.4f );

    DuetFunctor::Block<64> tmp;
    chan_input.write ( tmp );
    chan_input.write ( tmp );
    chan_input.write ( tmp );

    /*
    chan_input.write ( posx.data_ac_int () );
    chan_input.write ( posy.data_ac_int () );
    chan_input.write ( posz.data_ac_int () );
    chan_input.write ( mass.data_ac_int () );
    chan_input.write ( xx  .data_ac_int () );
    chan_input.write ( xy  .data_ac_int () );
    chan_input.write ( xz  .data_ac_int () );
    chan_input.write ( yy  .data_ac_int () );
    chan_input.write ( yz  .data_ac_int () );
    chan_input.write ( zz  .data_ac_int () );
    */

    dut.kernel ( chan_input, chan_output, pos0x, pos0y, pos0z, pos0w );

    DuetFunctor::Double result;

    DuetFunctor::Block<8> ot;

    result.set_data ( ot.template slc<32> (0) );

    printf ( "phii (%f)\n",
            result.to_double () );

    return 0;
}
