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
  std::shared_ptr<SALONS2::CraftSeats> list;
  CraftSeats( const int &vpoint_dep,
              const int &vcrc32,
                 SALONS2::CraftSeats *vlist ):point_dep(vpoint_dep),crc32(vcrc32),list(vlist) {
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
  bool operator!=(const CraftKey &a) {
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
  std::shared_ptr<SALONS2::CraftSeats>& read( const CraftKey &key );
  std::shared_ptr<SALONS2::CraftSeats>& read( int point_dep, const std::string &cls );
  void copy( const std::shared_ptr<SALONS2::CraftSeats>& src, SALONS2::CraftSeats &dest );
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
  SALONS2::CraftSeats get( int point_dep, const std::string &cls );
  void drop( const CraftKey &key );
  void clear();
};

}

#endif // SEATSCACHE_H
