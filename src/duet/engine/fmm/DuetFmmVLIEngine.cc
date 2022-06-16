#include "duet/engine/fmm/DuetFmmVLIEngine.hh"

namespace gem5 {
namespace duet {

DuetEngine::softreg_id_t DuetFmmVLIEngine::get_num_softregs () const {
    // one "expansion_terms" + one "cost" per caller + one "cnt" per caller
    return num_softreg_per_caller * (get_num_callers () + 1);
}

DuetFunctor::caller_id_t DuetFmmVLIEngine::get_num_memory_chans () const {
    // one for frontend (load), one for backend (store)
    return 2;
}

DuetFunctor::caller_id_t DuetFmmVLIEngine::get_num_interlane_chans () const {
    // two for caller forwarding
    // one for passing data from compute to backend
    // one for passing argument from frontend to backend
    return 4;
}

unsigned DuetFmmVLIEngine::get_max_stats_waittime () const {
    return 5000;
}

unsigned DuetFmmVLIEngine::get_max_stats_exectime () const {
    return 3000;
}

bool DuetFmmVLIEngine::handle_softreg_write (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                  value
        )
{
    // 0: expansion_terms
    if ( 0 == softreg_id ) {
        set_constant ( "expansion_terms", value );
    } else if ( softreg_id % num_softreg_per_caller == 0 ) {
        DuetFunctor::caller_id_t caller_id = softreg_id / num_softreg_per_caller - 1;
        if ( handle_argchan_push ( caller_id, value ) ) {
            auto got_one = _argchan_got_one [ caller_id ];
            _argchan_got_one [ caller_id ] = !got_one;
            if ( got_one )
                stats_call_recvd ( caller_id );
            return true;
        } else {
            return false;
        }
    }

    // others: ignore
    return true;
}

bool DuetFmmVLIEngine::handle_softreg_read (
        DuetEngine::softreg_id_t    softreg_id
        , uint64_t                & value
        )
{
    // 0: expansion_terms
    if ( 0 == softreg_id ) {
        value = get_constant <uint64_t> ( 0, "expansion_terms" );
    } else if ( num_softreg_per_caller > softreg_id ) {
        value = 0;
    } else {

        DuetFunctor::caller_id_t caller_id = softreg_id / num_softreg_per_caller - 1;
        softreg_id %= num_softreg_per_caller;

        switch ( softreg_id ) {
        case 1:     // cnt
            value = get_constant <uint64_t> ( caller_id, "cnt" );
            break;

        case 2:     // cost
            value = get_constant <uint64_t> ( caller_id, "cost" );
            set_constant <int64_t>  ( caller_id, "cost", 0 );
            set_constant <uint64_t> ( caller_id, "cnt", 0 );
            break;

        default:
            value = 0;
        }
    }

    return true;
}

void DuetFmmVLIEngine::try_send_mem_req_all () {
    try_send_mem_req_one ( 1 );         // prioritize store
    try_send_mem_req_one ( 0, true );   // send load request and reserve a slot to prevent deadlock
}

void DuetFmmVLIEngine::init () {
    DuetEngine::init ();

    std::vector <bool> tmp ( get_num_callers (), false );
    std::swap ( tmp, _argchan_got_one );

    for ( DuetFunctor::caller_id_t caller_id = 0;
            caller_id < get_num_callers ();
            ++ caller_id )
    {
        set_constant <uint64_t> ( caller_id, "cnt", 0 );
        set_constant <int64_t> ( caller_id, "cost", 0 );
    }

    set_constant <int64_t> ( "expansion_terms", 40ll );
}

}   // namespace duet
}   // namespace gem5
