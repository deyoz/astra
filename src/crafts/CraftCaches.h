#ifndef SEATSCACHE_H
#define SEATSCACHE_H

#include "salons.h"
#include "oralib.h"
#include <queue>

namespace CraftCache
{

struct CraftSeats {
  int point_dep;
  int crc32;
  SALONS2::CraftSeats list;
  CraftSeats( int vpoint_dep, int vcrc32 ) {
    point_dep = vpoint_dep;
    crc32 = vcrc32;
  }
  ~CraftSeats();
};

struct CraftKey {
  int point_dep;
  std::string cls;
  CraftKey( const int vpoint_dep, const std::string &vcls ):point_dep(vpoint_dep),cls(vcls){}
  bool operator<(const CraftKey &a) const {
    if ( a.point_dep < point_dep )
      return true;
    else
      if ( a.point_dep > point_dep )
        return false;
      else
        if ( a.cls < cls )
          return true;
        else
          return false;
  }
  bool operator!=(const CraftKey &a) const {
    return ( a.point_dep != this->point_dep ||
             a.cls != this->cls );
  }
};


class CraftCaches
{
private:
  const unsigned int MAX_ELEM_SIZE = 100;
  TQuery *SeatsQry;
  std::map<CraftKey,CraftSeats> caches;
  std::queue<CraftKey> qCaches;
  void checkSize();
  SALONS2::CraftSeats& read( const CraftKey &key );
  SALONS2::CraftSeats& read( int point_dep, const std::string &cls );
  void copy( const SALONS2::CraftSeats& src, SALONS2::CraftSeats &dest );
public:
  CraftCaches();
  static CraftCaches *Instance() {
    static CraftCaches *_instance = 0;
    if ( !_instance ) {
      _instance = new CraftCaches();
    }
    return _instance;
  }
  int getCurrentCRC32( int point_dep );
  void get( int point_dep, const std::string &cls, SALONS2::CraftSeats& list );
  void drop( const CraftKey &key );
  void drop( );
  void clear();
};

}

#endif // SEATSCACHE_H
