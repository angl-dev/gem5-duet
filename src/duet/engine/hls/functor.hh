#include <stdint.h>
#include <ac_int.h>
#include <ac_std_float.h>
#include <ac_channel.h>

class DuetFunctor {
// ===========================================================================
// == Types that are visible to both GEM5 and HLS ============================
// ===========================================================================
public:
    typedef enum _mem_req_type_t : uint8_t {
        REQTYPE_INV = 0
        , REQTYPE_LD
        , REQTYPE_ST
        , REQTYPE_LR
        , REQTYPE_SC
        , REQTYPE_SWAP
        , REQTYPE_ADD
        , REQTYPE_AND
        , REQTYPE_OR
        , REQTYPE_XOR
        , REQTYPE_MAX
        , REQTYPE_MAXU
        , REQTYPE_MIN
        , REQTYPE_MINU
        , REQTYPE_FENCE
    } mem_req_type_t;

    typedef enum _retcode_t : uint64_t {
        RETCODE_DEFAULT = uint64_t(-1)  // functor returned without explicitly
                                        // setting the return code. Simlar to
                                        // returning "void"
        , RETCODE_RUNNING = 0           // functor still running
    } retcode_t;

// ===========================================================================
// == HLS-specific types =====================================================
// ===========================================================================
public:
    typedef ac_int <8, false>   Bool;
    typedef ac_int <8, false>   U8;
    typedef ac_int <8, true>    S8;
    typedef ac_int <16, false>  U16;
    typedef ac_int <16, true>   S16;
    typedef ac_int <32, false>  U32;
    typedef ac_int <32, true>   S32;
    typedef ac_int <64, false>  U64;
    typedef ac_int <64, true>   S64;
    typedef ac_int <64, false>  UInt;
    typedef ac_int <64, true>   Int;
    typedef ac_ieee_float32     Float;
    typedef ac_ieee_float64     Double;
    typedef ac_int <48, false>  addr_t;
    typedef ac_channel <U64>    chan_req_t;
    typedef ac_channel <U64>    chan_data_t;

// ===========================================================================
// == API for subclasses =====================================================
// ===========================================================================
protected:
    /* -----------------------------------------------------------------------
     * enqueue_req:
     *  Enqueue a memory request to the specified channel
     * -------------------------------------------------------------------- */
    void enqueue_req (
            chan_req_t &        chan
            , mem_req_type_t    type
            , size_t            size
            , addr_t            addr
            )
    { chan.write ( make_req ( type, size, addr ) ); }

    /* -----------------------------------------------------------------------
     * enqueue_data:
     *  Enqueue a data element to the specified channel
     * -------------------------------------------------------------------- */
    // generic implementation for ac_int family
    template <typename T>
    void enqueue_data (
            chan_data_t &       chan
            , const T &         data
            )
    {
        U64 packed ( data );
        chan.write ( packed );
    }

    /* -----------------------------------------------------------------------
     * dequeue_data:
     *  Dequeue a data element from the specified channel
     * -------------------------------------------------------------------- */
    // generic implementation for ac_int family
    template <typename T>
    void dequeue_data (
            chan_data_t &       chan
            , T &               data
            )
    {
        U64 packed = chan.read ();
        data = packed.template slc <T::width> (0);
    }

    /* -----------------------------------------------------------------------
     * dequeue_token:
     *  Dequeue a store ACK from the specified channel
     * -------------------------------------------------------------------- */
    void dequeue_token (
            chan_data_t &       chan
            )
    { chan.read (); }

    template <typename T_data, typename T_packed>
    void unpack (
            const T_packed    & packed
            , int               offset
            , T_data          & data
            )
    {
        data = packed.template slc <T_data::width> ( T_data::width * offset );
    }

// ===========================================================================
// == API for testbench ======================================================
// ===========================================================================
public:
    U64 make_req (
            mem_req_type_t      type
            , size_t            size
            , addr_t            addr
            ) const
    {
        U64 req;
        
        U8 _type ( type );
        req.template set_slc <8> ( 0, _type );

        U8 _size ( size );
        req.template set_slc <8> ( 8, _size );

        req.template set_slc <48> ( 16, addr );

        return req;
    }
};

// specialization for float (float32)
template <>
void DuetFunctor::enqueue_data <DuetFunctor::Float> (
        DuetFunctor::chan_data_t &      chan
        , const DuetFunctor::Float &    data
        )
{
    U64 packed;
    packed.template set_slc <32> ( 0, data.data_ac_int() );
    chan.write ( packed );
}

// specialization for double (float64)
template <>
void DuetFunctor::enqueue_data <DuetFunctor::Double> (
        DuetFunctor::chan_data_t &      chan
        , const DuetFunctor::Double &   data
        )
{
    chan.write ( data.data_ac_int () );
}

// specialization for float (float32)
template <>
void DuetFunctor::dequeue_data <DuetFunctor::Float> (
        DuetFunctor::chan_data_t &      chan
        , DuetFunctor::Float &          data
        )
{
    U64 packed = chan.read ();
    data.set_data ( packed.template slc <32> (0) );
}

template <>
void DuetFunctor::unpack <DuetFunctor::Float, DuetFunctor::U64> (
        const DuetFunctor::U64    & packed
        , int                       offset
        , DuetFunctor::Float      & data
        )
{ data.set_data ( this->template unpack <U32> ( packed, offset ) ); }

// specialization for double (float64)
template <>
void DuetFunctor::dequeue_data <DuetFunctor::Double> (
        DuetFunctor::chan_data_t &      chan
        , DuetFunctor::Double &         data
        )
{
    U64 packed = chan.read ();
    data.set_data ( packed );
}
