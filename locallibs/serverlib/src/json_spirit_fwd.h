#pragma once

#include <string>
#include <vector>
#include <map>

// comment out the value types you don't need to reduce build times and intermediate file sizes
//#define JSON_SPIRIT_VALUE_ENABLED
//#define JSON_SPIRIT_WVALUE_ENABLED
#define JSON_SPIRIT_MVALUE_ENABLED
//#define JSON_SPIRIT_WMVALUE_ENABLED

namespace json_spirit
{

template< class Config >
class Value_impl;

// map objects

#if defined( JSON_SPIRIT_MVALUE_ENABLED ) || defined( JSON_SPIRIT_WMVALUE_ENABLED )
template< class String >
struct Config_map
{
    typedef String String_type;
    typedef Value_impl< Config_map > Value_type;
    typedef std::vector< Value_type > Array_type;
    typedef std::map< String_type, Value_type > Object_type;
    typedef std::pair< const String_type, Value_type > Pair_type;

    static Value_type& add( Object_type& obj, const String_type& name, const Value_type& value )
    {
        return obj[ name ] = value;
    }

    static const String_type& get_name( const Pair_type& pair )
    {
        return pair.first;
    }

    static const Value_type& get_value( const Pair_type& pair )
    {
        return pair.second;
    }
};
#endif

// typedefs for ASCII

#ifdef JSON_SPIRIT_MVALUE_ENABLED
typedef Config_map< std::string > mConfig;

typedef mConfig::Value_type  mValue;
typedef mConfig::Object_type mObject;
typedef mConfig::Array_type  mArray;
#endif
typedef Config_map< std::string > mConfig;

typedef mConfig::Value_type  mValue;
typedef mConfig::Object_type mObject;
typedef mConfig::Array_type  mArray;

} // json_spirit
