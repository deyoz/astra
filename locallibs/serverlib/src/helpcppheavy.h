#pragma once

#include <fstream>
#include <string>
#include <set>
#include <algorithm>

#include "exception.h"

namespace
{

template< typename M >
struct MapFindTrait
{
    typedef typename M::iterator Iter;
    typedef typename M::mapped_type Res;
};

template< typename M >
struct MapFindTrait< M const >
{
    typedef typename M::const_iterator Iter;
    typedef typename M::mapped_type const Res;
};

} // unnamed

namespace MonadMaybe { // для связывания функций проверки в цепочку
template<typename T, typename U>
T operator>>(const T& t, const U& f)
{
    return t ? t : f();
}

} // MonadMaybe

template <typename Container> void readFile(std::string const & filename,Container &data)    
{
    std::ifstream is(filename.c_str());
    if(!is.good()){
        throw comtech::Exception("cannot open "+filename);
    }
    data.assign( (std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>() ));
}


template <typename Container> void writeFile(std::string const & filename,Container const &data)    
{
    std::ofstream os(filename.c_str(),std::ios_base::trunc);
    if(!os.good()){
        throw comtech::Exception("cannot open "+filename);
    }
    std::ostreambuf_iterator<char> out (os); 
    copy(data.begin(),data.end(),out);
}

template< typename C >
bool hasDuplicates( C const &cnt )
{
    std::set< typename C::value_type > tmp;
    typename C::const_iterator it( cnt.begin() );
    typename C::const_iterator const it_end( cnt.end() );
    for( ; it != it_end; ++it )
        if( !tmp.insert( *it ).second )
            return true;
    return false;
}

template<typename C, typename P>
typename std::conditional<
    std::is_const<typename std::remove_reference<C>::type>::value,
    const typename std::decay<C>::type::value_type*,
    typename std::decay<C>::type::value_type*>::type findInContainerIf(C&& cont, P p)
{
    static_assert(!std::is_rvalue_reference<decltype(std::forward<C>(cont))>::value, "does not work for rvalue");
    auto it = std::find_if(cont.begin(), cont.end(), p);
    return it == cont.end() ? nullptr : &*it;
}

template< typename M >
typename MapFindTrait< M >::Res *mapFind( M &m, typename M::key_type const &k )
{
    typename MapFindTrait< M >::Iter const it( m.find( k ) );
    return it == m.end() ? nullptr : &it->second;
}

template< typename T, typename C >
void appendCont( T &v, C const &c )
{
    v.insert( v.end(), c.begin(), c.end() );
}

namespace Set
{

// lhs, rhs must be sorted

template< typename C >
C difference( C const &lhs, C const &rhs )
{
    C ret;
    std::set_difference( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter( ret, ret.begin() ) );
    return ret;
}

template< typename C >
C intersection( C const &lhs, C const &rhs )
{
    C ret;
    std::set_intersection( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter( ret, ret.begin() ) );
    return ret;
}

} // Set

namespace HelpCpp
{

template< typename T >
struct Diff
{
    typedef std::vector< T > Vals;
    typedef std::pair< T, T > ValPair;
    typedef std::vector< ValPair > ValPairs;
    Vals added;
    ValPairs modified;
    Vals deleted;
    bool empty() const
    {
        return added.empty() && modified.empty() && deleted.empty();
    }
};

template< typename Cnt, typename IdFn >
Diff< typename Cnt::value_type > makeDiff( Cnt const &lhs, Cnt const &rhs, IdFn const &fn )
{
    typedef typename Cnt::value_type val_t;
    Diff< val_t > ret;
    for( auto const &l : lhs )
    {
        auto const lid( fn( l ) );
        auto const it( std::find_if( rhs.begin(), rhs.end(), [ &lid, fn ]( val_t const &v ){ return fn( v ) == lid; } ) );
        if( it == rhs.end() )
            ret.deleted.push_back( l );
        else
            if( !( l == *it ) )
                ret.modified.push_back( std::make_pair( l, *it ) );
    }
    for( auto const &r : rhs )
    {
        auto const rid( fn( r ) );
        auto const it( std::find_if( lhs.begin(), lhs.end(), [ &rid, fn ]( val_t const &v ){ return fn( v ) == rid; } ) );
        if( it == lhs.end() )
            ret.added.push_back( r );
    }
    return ret;
}

} // HelpCpp
