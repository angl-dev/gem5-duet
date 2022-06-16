#ifndef __DUET_FMM_COMPLEX_HH
#define __DUET_FMM_COMPLEX_HH

#include <math.h>

namespace gem5 {
namespace duet {

template <typename T>
struct ComplexTmpl {
    T _r, _i;

    ComplexTmpl ()
        : _r ( 0 ), _i ( 0 )
    {}

    constexpr ComplexTmpl ( T r, T i )
        : _r ( r ), _i ( i )
    {};

    T & r () { return _r; }
    T & i () { return _i; }
    const T & r () const { return _r; }
    const T & i () const { return _i; }

    T mag_sqr () const
    { return _r * _r + _i * _i; }

    ComplexTmpl operator+ ( const ComplexTmpl & other ) const
    { return ComplexTmpl ( _r + other._r, _i + other._i ); }

    ComplexTmpl operator- ( const ComplexTmpl & other ) const
    { return ComplexTmpl ( _r - other._r, _i - other._i ); }

    ComplexTmpl operator* ( const ComplexTmpl & other ) const
    {
        return ComplexTmpl (
                _r * other._r - _i * other._i,
                _r * other._i + _i * other._r
                );
    }

    ComplexTmpl operator/ ( const ComplexTmpl & other ) const
    {
        T _denom = 1;
        _denom /= ( other.mag_sqr() );
        return ComplexTmpl (
                (_r * other._r + _i * other._i) * _denom,
                (_i * other._r - _r * other._i) * _denom
                );
    }

    ComplexTmpl & operator= ( const ComplexTmpl & other )
    {
        _r = other._r;
        _i = other._i;
        return *this;
    }

    ComplexTmpl & operator-= ( const ComplexTmpl & other )
    {
        _r -= other._r;
        _i -= other._i;
        return *this;
    }

    ComplexTmpl & operator+= ( const ComplexTmpl & other )
    {
        _r += other._r;
        _i += other._i;
        return *this;
    }

    ComplexTmpl & operator*= ( const ComplexTmpl & other )
    {
        T r = _r * other._r - _i * other._i;
        T i = _r * other._i + _i * other._r;
        _r = r;
        _i = i;
        return *this;
    }
};

}   // namespace duet
}   // namespace gem5

#endif /* #ifndef __DUET_FMM_COMPLEX_HH */
