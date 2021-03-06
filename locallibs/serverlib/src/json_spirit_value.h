#pragma once

//          Copyright John W. Wilkinson 2007 - 2014
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.08

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <boost/variant.hpp>
#include "json_spirit_fwd.h"

namespace json_spirit
{
    enum Value_type{ obj_type, array_type, str_type, bool_type, int_type, real_type, null_type };

    std::string value_type_to_string( Value_type vtype );

    struct Null{};

    template< class Config >    // Config determines whether the value uses std::string or std::wstring and
                                // whether JSON Objects are represented as vectors or maps
    class Value_impl
    {
    public:

        typedef Config Config_type;
        typedef typename Config::String_type String_type;
        typedef typename Config::Object_type Object;
        typedef typename Config::Array_type Array;
        typedef typename String_type::const_pointer Const_str_ptr;  // eg const char*

        Value_impl();  // creates null value
        Value_impl( Const_str_ptr      value );
        Value_impl( const String_type& value );
        Value_impl( const Object&      value );
        Value_impl( const Array&       value );
        Value_impl( bool               value );
        Value_impl( int                value );
        Value_impl( int64_t            value );
        Value_impl( uint64_t           value );
        Value_impl( double             value );

        template< class Iter >
        Value_impl( Iter first, Iter last );    // constructor from containers, e.g. std::vector or std::list

        template< BOOST_VARIANT_ENUM_PARAMS( typename T ) >
        Value_impl( const boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >& variant ); // constructor for compatible variant types

        Value_impl( const Value_impl& other );

        bool operator==( const Value_impl& lhs ) const;

        Value_impl& operator=( const Value_impl& lhs );

        Value_type type() const;

        bool is_uint64() const;
        bool is_null() const;

        const String_type& get_str()    const;
        const Object&      get_obj()    const;
        const Array&       get_array()  const;
        bool               get_bool()   const;
        int                get_int()    const;
        int64_t            get_int64()  const;
        uint64_t           get_uint64() const;
        double             get_real()   const;

        Object& get_obj();
        Array&  get_array();

        template< typename T > T get_value() const;  // example usage: int    i = value.get_value< int >();
                                                     // or             double d = value.get_value< double >();

        static const Value_impl null;

    private:

        void check_type( const Value_type vtype ) const;

        typedef boost::variant< boost::recursive_wrapper< Object >, boost::recursive_wrapper< Array >,
                                String_type, bool, int64_t, double, Null, uint64_t > Variant;

        Variant v_;

        class Variant_converter_visitor : public boost::static_visitor< Variant >
        {
        public:

              template< typename T, typename A, template< typename, typename > class Cont >
              Variant operator()( const Cont< T, A >& cont ) const
              {
                  return Array( cont.begin(), cont.end() );
              }

              Variant operator()( int i ) const
              {
                  return static_cast< int64_t >( i );
              }

              template<class T>
              Variant operator()( const T& t ) const
              {
                  return t;
              }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    // implementation

    inline bool operator==( const Null&, const Null& )
    {
        return true;
    }

    template< class Config >
    const Value_impl< Config > Value_impl< Config >::null;

    template< class Config >
    Value_impl< Config >::Value_impl()
    :   v_( Null() )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( const Const_str_ptr value )
    :  v_( String_type( value ) )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( const String_type& value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( const Object& value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( const Array& value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( bool value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( int value )
    :   v_( static_cast< int64_t >( value ) )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( int64_t value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( uint64_t value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( double value )
    :   v_( value )
    {
    }

    template< class Config >
    Value_impl< Config >::Value_impl( const Value_impl< Config >& other )
    :   v_( other.v_ )
    {
    }

    template< class Config >
    template< class Iter >
    Value_impl< Config >::Value_impl( Iter first, Iter last )
    :   v_( Array( first, last ) )
    {
    }

    template< class Config >
    template< BOOST_VARIANT_ENUM_PARAMS( typename T ) >
    Value_impl< Config >::Value_impl( const boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >& variant )
    :   v_( boost::apply_visitor( Variant_converter_visitor(), variant) )
    {
    }

    template< class Config >
    Value_impl< Config >& Value_impl< Config >::operator=( const Value_impl& lhs )
    {
        Value_impl tmp( lhs );

        std::swap( v_, tmp.v_ );

        return *this;
    }

    template< class Config >
    bool Value_impl< Config >::operator==( const Value_impl& lhs ) const
    {
        if( this == &lhs ) return true;

        if( type() != lhs.type() ) return false;

        return v_ == lhs.v_;
    }

    template< class Config >
    Value_type Value_impl< Config >::type() const
    {
        if( is_uint64() )
        {
            return int_type;
        }

        return static_cast< Value_type >( v_.which() );
    }

    template< class Config >
    bool Value_impl< Config >::is_uint64() const
    {
        return v_.which() == null_type + 1;
    }

    template< class Config >
    bool Value_impl< Config >::is_null() const
    {
        return type() == null_type;
    }

    template< class Config >
    void Value_impl< Config >::check_type( const Value_type vtype ) const
    {
        if( type() != vtype )
        {
            std::ostringstream os;

            os << "get_value< " << value_type_to_string( vtype ) << " > called on " << value_type_to_string( type() ) << " Value";

            throw std::runtime_error( os.str() );
        }
    }

    template< class Config >
    const typename Config::String_type& Value_impl< Config >::get_str() const
    {
        check_type( str_type );

        return *boost::get< String_type >( &v_ );
    }

    template< class Config >
    const typename Value_impl< Config >::Object& Value_impl< Config >::get_obj() const
    {
        check_type( obj_type );

        return *boost::get< Object >( &v_ );
    }

    template< class Config >
    const typename Value_impl< Config >::Array& Value_impl< Config >::get_array() const
    {
        check_type( array_type );

        return *boost::get< Array >( &v_ );
    }

    template< class Config >
    bool Value_impl< Config >::get_bool() const
    {
        check_type( bool_type );

        return boost::get< bool >( v_ );
    }

    template< class Config >
    int Value_impl< Config >::get_int() const
    {
        check_type( int_type );

        return static_cast< int >( get_int64() );
    }

    template< class Config >
    int64_t Value_impl< Config >::get_int64() const
    {
        check_type( int_type );

        if( is_uint64() )
        {
            return static_cast< int64_t >( get_uint64() );
        }

        return boost::get< int64_t >( v_ );
    }

    template< class Config >
    uint64_t Value_impl< Config >::get_uint64() const
    {
        check_type( int_type );

        if( !is_uint64() )
        {
            return static_cast< uint64_t >( get_int64() );
        }

        return boost::get< uint64_t >( v_ );
    }

    template< class Config >
    double Value_impl< Config >::get_real() const
    {
        if( type() == int_type )
        {
            return is_uint64() ? static_cast< double >( get_uint64() )
                               : static_cast< double >( get_int64() );
        }

        check_type( real_type );

        return boost::get< double >( v_ );
    }

    template< class Config >
    typename Value_impl< Config >::Object& Value_impl< Config >::get_obj()
    {
        check_type( obj_type );

        return *boost::get< Object >( &v_ );
    }

    template< class Config >
    typename Value_impl< Config >::Array& Value_impl< Config >::get_array()
    {
        check_type( array_type );

        return *boost::get< Array >( &v_ );
    }

    // converts a C string, ie. 8 bit char array, to a string object
    //
    template < class String_type >
    String_type to_str( const char* c_str )
    {
        String_type result;

        for( const char* p = c_str; *p != 0; ++p )
        {
            result += *p;
        }

        return result;
    }

    //

    namespace internal_
    {
        template< typename T >
        struct Type_to_type
        {
        };

        template< class Value >
        int get_value( const Value& value, Type_to_type< int > )
        {
            return value.get_int();
        }

        template< class Value >
        int64_t get_value( const Value& value, Type_to_type< int64_t > )
        {
            return value.get_int64();
        }

        template< class Value >
        uint64_t get_value( const Value& value, Type_to_type< uint64_t > )
        {
            return value.get_uint64();
        }

        template< class Value >
        double get_value( const Value& value, Type_to_type< double > )
        {
            return value.get_real();
        }

        template< class Value >
        typename Value::String_type get_value( const Value& value, Type_to_type< typename Value::String_type > )
        {
            return value.get_str();
        }

        template< class Value >
        typename Value::Array get_value( const Value& value, Type_to_type< typename Value::Array > )
        {
            return value.get_array();
        }

        template< class Value >
        typename Value::Object get_value( const Value& value, Type_to_type< typename Value::Object > )
        {
            return value.get_obj();
        }

        template< class Value >
        bool get_value( const Value& value, Type_to_type< bool > )
        {
            return value.get_bool();
        }
    }

    template< class Config >
    template< typename T >
    T Value_impl< Config >::get_value() const
    {
        return internal_::get_value( *this, internal_::Type_to_type< T >() );
    }
}
