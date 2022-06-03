#define __DUET_HLS

#include "stdio.h"
#include "barnes_gravsub/DuetBarnesMemFunctor.hh"

int main ( int argc, char * argv[] ) {
    DuetBarnesMemFunctor dut;

    DuetBarnesMemFunctor::chan_data_t   chan_arg;
    DuetBarnesMemFunctor::chan_req_t    chan_req;

    DuetFunctor::U64 arg = 0x1000;
    chan_arg.write ( arg );
    dut.kernel ( chan_arg, chan_req );

    DuetFunctor::U64 ref[4] = {
        arg + 16,
        arg + 24,
        arg + 32,
        arg + 8
    };

    DuetFunctor::U64 ret[4] = {
        chan_req.read (),
        chan_req.read (),
        chan_req.read (),
        chan_req.read ()
    };

    bool fail = false;
    for ( int i = 0; i < 4; ++i ) {
        if ( ret[i] != ref[i] ) {
            printf ( "[Error] ref[%d] = 0x%llx != ret[%d] = 0x%llx\n",
                    i, ref[i].to_uint64(), i, ret[i].to_uint64() );
            fail = true;
        }
    }

    if ( !fail )
        printf ( "[Info] Pass!\n" );

    return 0;
}
