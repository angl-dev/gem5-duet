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
    // Native C/C++
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

    // Duet-extensions
    typedef ac_int <48, false>  addr_t;
    typedef ac_channel <U64>    chan_req_t;

    template <unsigned int BYTES>
    using Block = ac_int <(BYTES << 3), false>;

// ===========================================================================
// == API for subclasses =====================================================
// ===========================================================================
protected:
    /* -----------------------------------------------------------------------
     * enqueue_req:
     *  Enqueue a memory request to the specified channel
     * -------------------------------------------------------------------- */
    void enqueue_req (
            chan_req_t        & chan
            , mem_req_type_t    type
            , size_t            size
            , addr_t            addr
            )
    { chan.write ( make_req ( type, size, addr ) ); }

    /* -----------------------------------------------------------------------
     * enqueue_data:
     *  Enqueue a data element to the specified channel
     * -------------------------------------------------------------------- */
    // template for ac_int family
    template <typename T, int W, bool S>
    void enqueue_data (
            ac_channel<T>         & chan
            , const ac_int<W, S>  & data
            )
    {
        assert (T::width >= W);
        assert (T::width % W == 0);

        T packed;
        #pragma unroll yes
        for (int i = 0; i < T::width; i+=W) {
            packed.template set_slc<W> (i, data);
        }

        chan.write ( packed );
    }

    // overload template for float32
    template <typename T>
    void enqueue_data (
            ac_channel<T>         & chan
            , Float               & data
            )
    {
        U32 tmp = data.data ();
        this->template enqueue_data <T, 32, false> (chan, tmp);
    }

    // overload template for float64
    template <typename T>
    void enqueue_data (
            ac_channel<T>         & chan
            , Double              & data
            )
    {
        U64 tmp = data.data ();
        this->template enqueue_data <T, 64, false> (chan, tmp);
    }

    /* -----------------------------------------------------------------------
     * dequeue_data:
     *  Dequeue a data element from the specified channel
     * -------------------------------------------------------------------- */
    // template for ac_int family
    template <typename T, int W, bool S>
    void dequeue_data (
            ac_channel<T>     & chan
            , ac_int<W, S>    & data
            )
    {
        assert (T::width >= W);
        assert (T::width % W == 0);

        T packed = chan.read ();
        data = packed.template slc <W> (0);
    }

    // overload template for float32
    template <typename T>
    void dequeue_data (
            ac_channel<T>     & chan
            , Float           & data
            )
    {
        U32 tmp;
        this->template dequeue_data <T, 32, false> (chan, tmp);
        data.set_data ( tmp );
    }

    // overload template for float64
    template <typename T>
    void dequeue_data (
            ac_channel<T>     & chan
            , Double          & data
            )
    {
        U64 tmp;
        this->template dequeue_data <T, 64, false> (chan, tmp);
        data.set_data ( tmp );
    }

    /* -----------------------------------------------------------------------
     * dequeue_token:
     *  Dequeue a store ACK from the specified channel
     * -------------------------------------------------------------------- */
    template <typename T>
    void dequeue_token (
            ac_channel<T>     & chan
            )
    { chan.read (); }

    /* -----------------------------------------------------------------------
     * unpack:
     *  Extract sub-word data from a long word
     * -------------------------------------------------------------------- */
    // template for ac_int family
    template <typename T, int W, bool S>
    void unpack (
            const T           & packed
            , int               offset
            , ac_int<W, S>    & data
            )
    {
        data = packed.template slc <W> ( W * offset );
    }

    // overload template for float32
    template <typename T>
    void unpack (
            const T           & packed
            , int               offset
            , Float           & data
            )
    {
        data.set_data ( packed.template slc <32> ( 32 * offset ) );
    }

    // overload template for float64
    template <typename T>
    void unpack (
            const T           & packed
            , int               offset
            , Double          & data
            )
    {
        data.set_data ( packed.template slc <64> ( 64 * offset ) );
    }

    /* -----------------------------------------------------------------------
     * pack:
     *  Fill sub-word data into a long word
     * -------------------------------------------------------------------- */
    // template for ac_int family
    template <typename T, int W, bool S>
    void pack (
            T                     & packed
            , int                   offset
            , const ac_int<W, S>  & data
            )
    {
        packed.template set_slc <W> ( W * offset, data );
    }

    // overload template for float32
    template <typename T>
    void pack (
            T                 & packed
            , int               offset
            , const Float     & data
            )
    {
        packed.template set_slc <32> ( 32 * offset, data.data_ac_int () );
    }

    // overload template for float64
    template <typename T>
    void pack (
            T                 & packed
            , int               offset
            , const Double    & data
            )
    {
        packed.template set_slc <64> ( 64 * offset, data.data_ac_int () );
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
