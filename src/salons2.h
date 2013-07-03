#ifndef _SALONS2_H_
#define _SALONS2_H_

#include <string>
#include <vector>
#include <map>
#include <libxml/tree.h>
#include "astra_utils.h"
#include "salons.h"

enum TModify { mNone, mDelete, mAdd, mChange };

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
      passSel = false;
    }
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
};

struct TLayerPriority {
	std::string code;
	std::string layer;
	int priority;
};

class TFilterLayers:public BitSet<ASTRA::TCompLayerType> {
	private:
	  int point_dep;
	public:
		bool CanUseLayer( ASTRA::TCompLayerType layer_type, int point_dep );
		void getFilterLayers( int point_id );

};

class TSalons {
  private:
  	SALONS2::TReadStyle readStyle;
    TFilterLayers FilterLayers;
  	std::map<std::string,int> status_priority;
  	std::vector<TLayerPriority> layer_priority;
    TPlaceList* FCurrPlaceList;
    bool pr_lat_seat;
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
    TSalons( int id, SALONS2::TReadStyle vreadStyle );
    TPlaceList *CurrPlaceList();
    void SetCurrPlaceList( TPlaceList *newPlaceList );

    void Clear( );

    bool getLatSeat() { return pr_lat_seat; };
    void Build( xmlNodePtr salonsNode );
    void Read( bool wo_invalid_seat_no = false );
    void Write( );
    void Parse( xmlNodePtr salonsNode );
    void verifyValidRem( std::string rem_name, std::string class_name );
};

namespace SALONS
{
	typedef std::pair<int,TPlace> TSalonSeat;
  void GetCompParams( int comp_id, xmlNodePtr dataNode );
  void SetLayer( const std::map<std::string,int> &layer_priority, const std::string &layer, TPlace &pl );
  void ClearLayer( const std::map<std::string,int> &layer_priority, const std::string &layer, TPlace &pl );
  void SetFree( const std::string &layer, TPlace &pl );
  void SetBlock( const std::string &layer, TPlace &pl );
}

#endif /*_SALONS2_H_*/

