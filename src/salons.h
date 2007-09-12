#ifndef _SALONS_H_
#define _SALONS_H_

#include <string>
#include <vector>
#include <libxml/tree.h>

enum TReadStyle { rTripSalons, rComponSalons };

enum TModify { mNone, mDelete, mAdd, mChange };

struct TPoint {
  public:
  int x;
  int y;
  TPoint() {
    x = 0;
    y = 0;
  }
  TPoint( int ax, int ay ) {
    x = ax;
    y = ay;
  }
};

struct TRem {
  std::string rem;
  bool pr_denial;
};

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
    bool passSel;
    std::vector<TRem> rems;
    TPlace() {
      x = -1;
      y = -1;
      visible = false;
      isplace = false;
      xprior = -1;
      yprior = -1;
      xnext = -1;
      ynext = -1;
      pr_smoke = false;
      not_good = false;
      status = "FP";
      pr_free = true;
      block = false;
    }
    void Assign( TPlace &pl );
};

typedef std::vector<TPlace> TPlaces;
typedef TPlaces::iterator IPlace;

class TPlaceList {
  private:
    std::vector<std::string> xs, ys;
  public:
    TPlaces places;
    int num;
    TPlace *place( int idx );
    TPlace *place( TPoint &p );
    int GetPlaceIndex( TPoint &p );
    int GetPlaceIndex( int x, int y );
    int GetXsCount();
    int GetYsCount();
    bool ValidPlace( TPoint &p );
    std::string GetPlaceName( TPoint &p );
    std::string GetXsName( int x );
    std::string GetYsName( int y );
    bool GetisPlaceXY( std::string placeName, TPoint &p );
    void Add( TPlace &pl );
};

class TSalons {
  private:
    TPlaceList* FCurrPlaceList;
  public:
    int trip_id;
    int comp_id;
    std::string airline;
    std::string airp;    	
    std::string craft;
    std::string bort;
    std::string descr;
    std::string classes;
    TModify modify;
    std::string ClName;
    std::vector<TPlaceList*> placelists;
    ~TSalons( );
    TSalons();
    TPlaceList *CurrPlaceList();
    void SetCurrPlaceList( TPlaceList *newPlaceList );

    void Clear( );
    static bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull );
    static void GetTripParams( int trip_id, xmlNodePtr dataNode );
    static void GetCompParams( int comp_id, xmlNodePtr dataNode );
    void Build( xmlNodePtr salonsNode );
    void Read( TReadStyle readStyle );
    void Write( TReadStyle readStyle );
    void Parse( xmlNodePtr salonsNode );
    void verifyValidRem( std::string rem_name, std::string class_name );
};

#endif /*_SALONS_H_*/

