#pragma once

#include <functional>

namespace HelpCpp
{

namespace details
{

template< typename P, typename T >
struct FieldPredFn
{
    typedef P ( T::*FnT )() const;
    FieldPredFn( FnT fn, P const &p ) : p_( p ), fn_( fn ) {}
    bool operator()( T const &t ) const { return ( t.*fn_ )() == p_; }

private:
    P p_;
    FnT fn_;
};

template< typename P, typename T >
struct FieldPredDt
{
    typedef P T::*DtT;
    FieldPredDt( DtT dt, P const &p ) : p_( p ), dt_( dt ) {}
    bool operator()( T const &t ) const { return t.*dt_ == p_; }

private:
    P p_;
    DtT dt_;
};

} // details

template< typename P, typename T >
details::FieldPredDt< P, T > eqPred( P T::*dt, P const &p )
{
    return details::FieldPredDt< P, T >( dt, p );
}

template< typename P, typename T >
details::FieldPredFn< P, T > eqPred( P ( T::*fn )() const, P const &p )
{
    return details::FieldPredFn< P, T >( fn, p );
}

} // HelpCpp

