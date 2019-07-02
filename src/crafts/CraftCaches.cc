#include "CraftCaches.h"


#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

namespace CraftCache {


CraftSeats::~CraftSeats()
{
  LogTrace(TRACE5) << __func__ << " point_dep=" << point_dep;
  list.get()->Clear();
}

CraftCaches::CraftCaches()
{
  LogTrace(TRACE5) << __func__;
  SeatsQry = new TQuery(&OraSession);
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
    drop( qCaches.front() );
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


int CraftCaches::getCurrentCRC32( int point_dep )
{
  return SALONS2::getCRC_Comp( point_dep );
}

void CraftCaches::copy( const std::shared_ptr<SALONS2::CraftSeats>& src, SALONS2::CraftSeats &dest )
{
  dest.Clear();
  for ( auto l : *src.get() ) {
    SALONS2::TPlaceList* ls = new SALONS2::TPlaceList();
    *ls = *l;
    dest.push_back( ls );
  }
}

SALONS2::CraftSeats CraftCaches::get( int point_dep, const std::string &cls )
{
   LogTrace(TRACE5)<<__func__ << " point_dep=" << point_dep << ",cls=" << cls;
   CraftKey key(point_dep,cls);
   SALONS2::CraftSeats dest;
   std::map<CraftKey,CraftSeats>::iterator icraft = caches.find( key );
   if ( icraft != caches.end() &&
        getCurrentCRC32( point_dep ) == icraft->second.crc32 ) {
     copy( icraft->second.list, dest );
     LogTrace(TRACE5)<<__func__ << " use cache";
   }
   else {
     copy( read( key ), dest );
     LogTrace(TRACE5)<<__func__ << " read DB finished";
   }
   return dest;
}

std::shared_ptr<SALONS2::CraftSeats>& CraftCaches::read( const int point_dep, const std::string &cls )
{
  CraftKey key(point_dep,cls);
  return read(key);
}

std::shared_ptr<SALONS2::CraftSeats>& CraftCaches::read( const CraftKey &key )
{
  drop( key );
  int crc32 = getCurrentCRC32( key.point_dep );
  SALONS2::CraftSeats *list = new SALONS2::CraftSeats();
  SeatsQry->SetVariable( "point_id", key.point_dep );
  SeatsQry->Execute();
  list->read( *SeatsQry, key.cls );
  std::pair<std::map<CraftKey,CraftSeats>::iterator,bool> res = caches.emplace( std::piecewise_construct,
                                                                                std::forward_as_tuple(key),
                                                                                std::forward_as_tuple(key.point_dep, crc32, list) );
  if ( res.second ) {
    qCaches.push( key );
    checkSize();
    return res.first->second.list;
  }
  throw EXCEPTIONS::Exception( "unknown read craft exception" );
}


void CraftCaches::clear()
{
   caches.clear();
}

} //end namespace CraftCache
