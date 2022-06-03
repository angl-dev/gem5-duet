#include "duet/engine/barnes_gravsub/DuetBarnesEngine.hh"

namespace gem5 {
namespace duet {

DuetEngine::softreg_id_t DuetBarnesEngine::get_num_softregs () const {
    return 8 * (get_num_callers () + 1);
}

DuetFunctor::caller_id_t DuetBarnesEngine::get_num_memory_chans () const {
    return 1;
}

DuetFunctor::caller_id_t DuetBarnesEngine::get_num_interlane_chans () const {
    return 3;
}

bool DuetBarnesEngine::handle_softreg_write (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                  value
        )
{
    // 0: epssq 
    if ( 0 == softreg_id ) {
        set_constant ( "epssq", value );
        return true;
    } else if ( softreg_id < 8 || softreg_id >= get_num_softregs () ) {
        return true;
    }

    // starting from 8, 8 registers per caller
    DuetFunctor::caller_id_t caller_id = softreg_id / 8 - 1;
    softreg_id %= 8;

    switch ( softreg_id ) {
    case 0:     // arg
        return handle_argchan_push ( caller_id, value );

    case 1:     // pos0x
        set_constant ( caller_id, "pos0x", value );
        return true;

    case 2:     // pos0y
        set_constant ( caller_id, "pos0y", value );
        return true;

    case 3:     // pos0z
        set_constant ( caller_id, "pos0z", value );
        return true;

    default:    // phii, accx, accy, accz
        return true;
    }
}

bool DuetBarnesEngine::handle_softreg_read (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                & value
        )
{
    // 0: epssq 
    if ( 0 == softreg_id ) {
        value = get_constant <uint64_t> ( 0, "epssq" );
        return true;
    } else if ( softreg_id < 8 || softreg_id >= get_num_softregs () ) {
        value = 0;
        return true;
    }

    // starting from 8, 8 registers per caller
    DuetFunctor::caller_id_t caller_id = softreg_id / 8 - 1;
    softreg_id %= 8;

    switch ( softreg_id ) {
    case 0:     // ret
        return handle_retchan_pull ( caller_id, value );

    case 1:     // pos0x
        value = get_constant <uint64_t> ( caller_id, "pos0x" );
        return true;

    case 2:     // pos0y
        value = get_constant <uint64_t> ( caller_id, "pos0y" );
        return true;

    case 3:     // pos0z
        value = get_constant <uint64_t> ( caller_id, "pos0z" );
        return true;

    case 4:     // phii
        value = get_constant <uint64_t> ( caller_id, "phii" );
        set_constant ( caller_id, "phii", double (0.f) );
        return true;

    case 5:     // accx
        value = get_constant <uint64_t> ( caller_id, "accx" );
        set_constant ( caller_id, "accx", double (0.f) );
        return true;

    case 6:     // accy
        value = get_constant <uint64_t> ( caller_id, "accy" );
        set_constant ( caller_id, "accy", double (0.f) );
        return true;

    case 7:     // accz
        value = get_constant <uint64_t> ( caller_id, "accz" );
        set_constant ( caller_id, "accz", double (0.f) );
        return true;

    default:
        assert ( false );
        return true;
    }
}

void DuetBarnesEngine::try_send_mem_req_all () {
    try_send_mem_req_one ( 0 );
}

void DuetBarnesEngine::init () {
    DuetEngine::init ();

    for ( DuetFunctor::caller_id_t caller_id = 0;
            caller_id < get_num_callers ();
            ++caller_id )
    {
        set_constant ( caller_id, "phii", double (0.f) );
        set_constant ( caller_id, "accx", double (0.f) );
        set_constant ( caller_id, "accy", double (0.f) );
        set_constant ( caller_id, "accz", double (0.f) );
    }

    set_constant ( "epssq", double (0.f) );
}

}   // namespace duet
}   // namespace gem5
