#pragma once

#include <boost/type_index.hpp>

#include "exception.h"

namespace ServerFramework
{

class BaseEitherExc : public comtech::Exception
{
public:
    BaseEitherExc( char const *file, int line, std::string const &msg )
        : comtech::Exception( "SYSTEM", file, line, "", msg )
    {}

    virtual ~BaseEitherExc()
    {}
private:
};

template< typename T >
class EitherExc : public BaseEitherExc
{
public:
    EitherExc( char const *file, int line )
        : BaseEitherExc( file, line, std::string( "unable to read value of type : " ) +
                boost::typeindex::type_id_with_cvr< T >().pretty_name() )
    {
    }
};

} // ServerFramework

template< typename L, typename R >
class Either
{
    static_assert( !std::is_same< std::decay_t< L >, std::decay_t< R > >::value, "both types are equal" );

public:
    Either( L const &l ) : type_( Left ), l_( l ) {}
    Either( L &&l ) : type_( Left ), l_( std::move( l ) ) {}
    Either( R const &r ) : type_( Right ), r_( r ) {}
    Either( R &&r ) : type_( Right ), r_( std::move( r ) ) {}
    ~Either()
    {
        clean();
    }

    Either( Either const &ei ) : type_( ei.type_ ) { ctor( ei ); }
    Either( Either &&ei ) : type_( ei.type_ ) { ctor( std::move( ei ) ); }

    Either &operator=( Either const &ei ) { return assign( ei ); }
    Either &operator=( Either &&ei ) { return assign( std::move( ei ) ); }

    L const &left() const
    {
        getAssert( Left, __FILE__, __LINE__ );
        return l_;
    }
    L &left()
    {
        getAssert( Left, __FILE__, __LINE__ );
        return l_;
    }

    R const &right() const
    {
        getAssert( Right, __FILE__, __LINE__ );
        return r_;
    }
    R &right()
    {
        getAssert( Right, __FILE__, __LINE__ );
        return r_;
    }

    bool isLeft() const noexcept { return type_ == Left; }

    bool operator==(Either const &rhs ) const
    {
        return isLeft() == rhs.isLeft()
            ? ( isLeft() ? left() == rhs.left() : right() == rhs.right() )
            : false;
    }
    bool operator!=(Either const &rhs ) const { return !( *this == rhs ); }

private:
    enum Type { Left, Right };
    Type type_;
    union
    {
        L l_;
        R r_;
    };

    template< bool lv, typename V >
    static constexpr decltype( auto ) cst( V &&v ) noexcept
    {
        using VT = std::decay_t< V >;
        return static_cast< std::conditional_t< lv, VT const &, VT && > >( v );
    }

    template< typename ET >
    Either &assign( ET &&et )
    {
        static_assert( std::is_same< std::decay_t< ET >, Either< L, R > >::value, "types must be equal" );
        bool constexpr isLv = std::is_reference< ET >::value;
        if( type_ == et.type_ )
        {
            if( type_ == Left )
                l_ = cst< isLv >( et.l_ );
            else
                r_ = cst< isLv >( et.r_ );
        }
        else
        {
            clean();
            if( et.type_ == Left )
                new ( &l_ ) L( cst< isLv >( et.l_ ) );
            else
                new ( &r_ ) R( cst< isLv >( et.r_ ) );
            type_ = et.type_;
        }
        return *this;
    }

    template< typename ET >
    void ctor( ET &&et )
    {
        static_assert( std::is_same< std::decay_t< ET >, Either< L, R > >::value, "types must be equal" );
        bool constexpr isLv = std::is_reference< ET >::value;
        if( type_ == Left )
            new ( &l_ ) L( cst< isLv >( et.l_ ) );
        else
            new ( &r_ ) R( cst< isLv >( et.r_ ) );
    }

    void clean()
    {
        type_ == Left ? l_.~L() : r_.~R();
    }

    void getAssert( Type tp, char const *file, int line ) const
    {
        if( tp == type_ )
            return;
        if( tp == Left )
            throw ServerFramework::EitherExc< L >( file, line );
        else
            throw ServerFramework::EitherExc< R >( file, line );
    }
};
