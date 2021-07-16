#include "CraftCaches.h"


#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

namespace CraftCache {

CraftSeats::~CraftSeats()
{
  LogTrace(TRACE5) << __func__ << " point_dep=" << point_dep;
  list.Clear();
}

CraftCaches::CraftCaches()
{
  LogTrace(TRACE5) << __func__;
  SeatsQry.reset(new DB::TQuery(PgOra::getROSession("TRIP_COMP_ELEMS"), STDLOG));
  SeatsQry->SQLText =
    "SELECT num, x, y, elem_type, xprior, yprior, agle,"
    "       xname, yname, class "
    " FROM trip_comp_elems "
    "WHERE point_id = :point_id "
    "ORDER BY num, x desc, y desc";
  SeatsQry->DeclareVariable( "point_id", otInteger );
}

void CraftCaches::checkSize()
{
  if ( caches.size() > MAX_ELEM_SIZE ) {
    LogTrace(TRACE5) << __func__ << " more then " << MAX_ELEM_SIZE << " crafts cached!";
    drop( );
  }
}

void CraftCaches::drop( )
{
  if ( !qCaches.empty() ) {
    CraftKey key = qCaches.front();
    qCaches.pop();
    std::map<CraftKey,CraftSeats>::iterator icraft = caches.find( key );
    if ( icraft != caches.end() ) {
      caches.erase( icraft );
    }
  }
}

void CraftCaches::drop( const CraftKey &key )
{
  std::map<CraftKey,CraftSeats>::iterator icraft = caches.find( key );
  if ( icraft != caches.end() ) {
    LogTrace(TRACE5) << __func__ << ", remove craft point_dep=" << key.point_dep << ",cls=" << key.cls;
    caches.erase( icraft );
    std::queue<CraftKey> qC;
    while ( !qCaches.empty() ) {
      CraftKey p = qCaches.front();
      qCaches.pop();
      if ( p != key ) {
        qC.emplace( p );
      }
    }
    qCaches = qC;
  }
}

void CraftCaches::copy( const SALONS2::CraftSeats& src, SALONS2::CraftSeats &dest )
{
  dest.Clear();
  for ( const auto l : src ) {
    SALONS2::TPlaceList* ls = new SALONS2::TPlaceList();
    *ls = *l;
    dest.push_back( ls );
  }
}

void CraftCaches::get( int point_dep, const std::string &cls, SALONS2::CraftSeats& list )
{
   LogTrace(TRACE5)<<__func__ << " point_dep=" << point_dep << ",cls=" << cls;
   CraftKey key(point_dep,cls);
   std::map<CraftKey,CraftSeats>::iterator icraft = caches.find( key );
   if ( icraft != caches.end() &&
        SALONS2::CompCheckSum::keyFromDB( point_dep ).total_crc32 == icraft->second.total_crc32 ) {
     copy( icraft->second.list, list );
     LogTrace(TRACE5)<<__func__ << " use cache point_dep=" << point_dep << ",cls=" << cls;
   }
   else {
     copy( read( key ), list );
     LogTrace(TRACE5)<<__func__ << " read DB finished point_dep=" << point_dep << ",cls=" << cls;
   }
   checkSize();
}

SALONS2::CraftSeats& CraftCaches::read( const int point_dep, const std::string &cls )
{
  return read(CraftKey(point_dep,cls));
}

SALONS2::CraftSeats& CraftCaches::read( const CraftKey &key )
{
  drop( key );
  SeatsQry->SetVariable( "point_id", key.point_dep );
  SeatsQry->Execute();
  std::pair<std::map<CraftKey,CraftSeats>::iterator,bool> res = caches.emplace( std::piecewise_construct,
                                                                                std::forward_as_tuple(key),
                                                                                std::forward_as_tuple(key.point_dep,
                                                                                SALONS2::CompCheckSum::keyFromDB( key.point_dep ).total_crc32) );
  if ( res.second ) {
    res.first->second.list.read( *SeatsQry, key.cls );
    qCaches.push( key );
    return res.first->second.list;
  }
  throw EXCEPTIONS::Exception( "unknown read craft exception" );
}


void CraftCaches::clear()
{
   caches.clear();
}

} //end namespace CraftCache
