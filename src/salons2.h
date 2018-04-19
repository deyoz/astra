#ifndef _SALONS2_H_
#define _SALONS2_H_

#include <string>
#include <vector>
#include <map>
#include <libxml/tree.h>
#include "astra_utils.h"
#include "salons.h"

class TPlace {
  public:
    bool selected;
    bool visible;
    int x, y;
    std::string elem_type;
    bool isplace;
    int xprior, yprior;
    int xnext, ynext;
    int agle;
    std::string clname;
    bool pr_smoke;
    bool not_good;
    std::string xname, yname;
    std::string status;
    bool pr_free;
    bool block;
    std::vector<SALONS2::TRem> rems;
    std::vector<std::string> layers;
    TPlace() {
      x = -1;
      y = -1;
      visible = false;
      isplace = false;
      xprior = -1;
      yprior = -1;
      xnext = -1;
      ynext = -1;
      agle = 0;
      pr_smoke = false;
      not_good = false;
      status = "FP";
      pr_free = true;
      block = false;
    }
};

typedef std::vector<TPlace> TPlaces;
typedef TPlaces::iterator IPlace;

/*class TPlaceList {
  private:
    std::vector<std::string> xs, ys;
  public:
    TPlaces places;
    int num;
    TPlace *place( int idx );
    TPlace *place( SALONS2::TPoint &p );
    int GetPlaceIndex( SALONS2::TPoint &p );
    int GetPlaceIndex( int x, int y );
    int GetXsCount();
    int GetYsCount();
    bool ValidPlace( SALONS2::TPoint &p );
    std::string GetPlaceName( SALONS2::TPoint &p );
    std::string GetXsName( int x );
    std::string GetYsName( int y );
    bool GetisPlaceXY( std::string placeName, SALONS2::TPoint &p );
    void Add( TPlace &pl );
};*/

struct TLayerPriority {
    std::string code;
    std::string layer;
    int priority;
};

#endif /*_SALONS2_H_*/

