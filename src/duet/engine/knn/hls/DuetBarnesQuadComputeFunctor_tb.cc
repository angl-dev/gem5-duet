#define __DUET_HLS

#include "stdio.h"
#include "barnes_gravsub_quad/DuetBarnesQuadComputeFunctor.hh"

int main ( int argc, char * argv[] ) {
    DuetBarnesQuadComputeFunctor dut;

    DuetFunctor::chan_data_t chan_input, chan_output;
    const DuetFunctor::Double pos0x ( 0.5f ),
                              pos0y ( 0.5f ),
                              pos0z ( 0.5f ),
                              epssq ( 1e-8 ),
                              posx  ( 0.7f ),
                              posy  ( 0.2f ),
                              posz  ( 0.3f ),
                              mass  ( 1.0f ),
                              xx    ( 0.5f ),
                              xy    ( 0.5f ),
                              xz    ( 0.5f ),
                              yy    ( 0.5f ),
                              yz    ( 0.5f ),
                              zz    ( 0.5f );

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

    dut.kernel ( chan_input, chan_output, pos0x, pos0y, pos0z, epssq );

    DuetFunctor::Double phii, accx, accy, accz;

    phii.set_data ( chan_output.read () );
    accx.set_data ( chan_output.read () );
    accy.set_data ( chan_output.read () );
    accz.set_data ( chan_output.read () );

    printf ( "phii (%f), acc (%f, %f, %f)\n",
            phii.to_double (), accx.to_double (), accy.to_double (), accz.to_double () );

    return 0;
}
