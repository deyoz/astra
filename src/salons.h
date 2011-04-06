#ifndef _SALONS_H_
#define _SALONS_H_

#include <string>
#include <vector>
#include <map>
#include <libxml/tree.h>
#include "astra_utils.h"
#include "astra_consts.h"
#include "basic.h"

namespace SALONS2
{

enum TReadStyle { rTripSalons, rComponSalons };

enum TModify { mNone, mDelete, mAdd, mChange };

struct TSalonPoint {
	int x;
	int y;
	int num;
	TSalonPoint() {
		x = 0;
		y = 0;
		num = 0;
	};
	TSalonPoint( int ax, int ay, int anum ) {
		x = ax;
		y = ay;
		num = anum;
	};
};

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

struct TPlaceLayer {
	int pax_id;
	int point_dep;
	int point_arv;
	ASTRA::TCompLayerType layer_type;
	int priority;
	BASIC::TDateTime time_create;
  TPlaceLayer( int vpax_id, int vpoint_dep, int vpoint_arv,
               ASTRA::TCompLayerType vlayer_type, BASIC::TDateTime vtime_create, int vpriority ) {
		pax_id = vpax_id;
		point_dep = vpoint_dep;
		point_arv = vpoint_arv;
		layer_type = vlayer_type;
		time_create = vtime_create;
		priority = vpriority;
  }
};

struct TPlaceWebTariff {
	std::string color;
	double value;
	std::string currency_id;
	TPlaceWebTariff() {
		value = 0.0;
	};
};

struct TCompSections {
  std::string name;
  int firstRowIdx;
  int lastRowIdx;
  TCompSections() {
    firstRowIdx = ASTRA::NoExists;
    lastRowIdx = ASTRA::NoExists;
  };
};

class TPlace {
  public:
    bool visible;
    int x, y, num;
    std::string elem_type;
    bool isplace;
    int xprior, yprior;
    int xnext, ynext;
    int agle;
    std::string clname;
    std::string xname, yname;
    bool passSel;
    std::vector<TRem> rems;
    std::vector<TPlaceLayer> layers;
    TPlaceWebTariff WebTariff;
    bool isPax;
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
      passSel = false;
      isPax = false;
    }
    void Assign( TPlace &pl ) {
      visible = pl.visible;
      x = pl.x;
      y = pl.y;
      elem_type = pl.elem_type;
      isplace = pl.isplace;
      xprior = pl.xprior;
      yprior = pl.yprior;
      xnext = pl.xnext;
      ynext = pl.ynext;
      agle = pl.agle;
      clname = pl.clname;
      xname = pl.xname;
      yname = pl.yname;
      layers = pl.layers;
      rems = pl.rems;
      WebTariff = pl.WebTariff;
      isPax = pl.isPax;
    }
    bool isLayer( ASTRA::TCompLayerType layer ) {
    	for (std::vector<TPlaceLayer>::iterator i=layers.begin(); i!=layers.end(); i++ ) {
    		if ( i->layer_type == layer )
    			return true;
    	};
    	return false;
    }
    void clearLayer( ASTRA::TCompLayerType layer, BASIC::TDateTime time_create ) {
    	isPax = false;
    	for (std::vector<TPlaceLayer>::iterator i=layers.begin(); i!=layers.end(); i++ ) {
    		if ( i->pax_id > 0 )
    			isPax = true;
    		if ( i->layer_type == layer && ( time_create <= 0 && i->time_create <= 0 || time_create == i->time_create ) ) {
    			layers.erase( i );
    			if ( isPax )
    			  break;
    		}
      }
    }
    void AddLayerToPlace( ASTRA::TCompLayerType l, BASIC::TDateTime time_create, int pax_id,
    	                   int point_dep, int point_arv, int priority ) {
   		std::vector<TPlaceLayer>::iterator i;
      for (i=layers.begin(); i!=layers.end(); i++) {
      	if ( priority < i->priority ||
      		   priority == i->priority &&
      		   time_create > i->time_create )
      		break;
      }
      TPlaceLayer pl( pax_id, point_dep, point_arv, l, time_create, priority );
    	layers.insert( i, pl );
    	if ( pax_id > 0 )
    		isPax = true;
    };
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

struct TLayerColor {
	std::string color;
	bool framework;
	TLayerColor() {
		framework = 0;
	}
};

struct TLayerProp
{
	std::string name;
	std::string name_view;
	std::string func_key;
  int priority;
  bool editable;
  bool notfree;
  TLayerProp() {
  	priority=999;
  	editable = false;
  	notfree = false;
  }
};


class TFilterLayers:public BitSet<ASTRA::TCompLayerType> {
	private:
	  int point_dep;
	public:
		bool CanUseLayer( ASTRA::TCompLayerType layer_type, int point_id );
		void getFilterLayers( int point_id );
};

//typedef std::map<TPlace*, std::vector<TPlaceLayer> > TPlacePaxs; // сортировка по приоритетам слоев

void getXYName( int point_id, std::string seat_no, std::string &xname, std::string &yname );

class TSalons {
  private:
  	TReadStyle readStyle;
  	bool drop_not_used_pax_layers;
    TFilterLayers FilterLayers;
  	std::map<ASTRA::TCompLayerType,TLayerProp> layers_priority;
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
    //TPlacePaxs PaxsOnPlaces;
    ~TSalons( );
    TSalons( int id, TReadStyle vreadStyle, bool vdrop_not_used_pax_layers=true );
    TPlaceList *CurrPlaceList();
    void SetCurrPlaceList( TPlaceList *newPlaceList );

    void Clear( );

    bool placeIsFree( TPlace* p ) {
    	for ( std::vector<TPlaceLayer>::iterator i=p->layers.begin(); i!=p->layers.end(); i++ ) {
    		if ( layers_priority[ i->layer_type ].notfree )
    			return false;
      }
      return true;
    };

    int getPriority( ASTRA::TCompLayerType layer_type ) {
    	if ( layers_priority.find( layer_type ) != layers_priority.end() )
    		return layers_priority[ layer_type ].priority;
    	else
    		return 10000;
    };
    bool isEditableLayer( ASTRA::TCompLayerType layer_type ) {
    	if ( layers_priority.find( layer_type ) == layers_priority.end() )
    		return false;
    	return layers_priority[ layer_type ].editable;
    };
    bool getLatSeat() { return pr_lat_seat; };
    void BuildLayersInfo( xmlNodePtr salonsNode );
    void Build( xmlNodePtr salonsNode );
    void Read( );
    void Write( );
    void Parse( xmlNodePtr salonsNode );
    void verifyValidRem( std::string rem_name, std::string class_name );
};

	typedef std::pair<int,TPlace> TSalonSeat;
	bool Checkin( int pax_id );
  bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull );
  void GetTripParams( int trip_id, xmlNodePtr dataNode );
  void GetCompParams( int comp_id, xmlNodePtr dataNode );
  int GetCompId( const std::string craft, const std::string bort, const std::string airline,
                 std::string airp,  int f, int c, int y );
  int AutoSetCraft( int point_id, std::string &craft, int comp_id );
  int SetCraft( int point_id, std::string &craft, int comp_id );
  void InitVIP( int point_id );
  void setTRIP_CLASSES( int point_id );
  void SetLayer( const std::map<std::string,int> &layer_priority, ASTRA::TCompLayerType layer, TPlace &pl );
  void ClearLayer( ASTRA::TCompLayerType layer, TPlace &pl );
  void SetFree( const std::string &layer, TPlace &pl );
  void SetBlock( const std::string &layer, TPlace &pl );
  void getSalonChanges( TSalons &OldSalons, std::vector<TSalonSeat> &seats );
  bool getSalonChanges( TSalons &OldSalons, TSalons &NewSalon, std::vector<TSalonSeat> &seats );
  void BuildSalonChanges( xmlNodePtr dataNode, const std::vector<TSalonSeat> &seats );
  bool salonChangesToText( TSalons &OldSalons, TSalons &NewSalons, std::vector<std::string> &referStrs, bool pr_set_base, int line_len );
  void ParseCompSections( xmlNodePtr sectionsNode, std::vector<TCompSections> &CompSections );
  void getLayerPlacesCompSection( TSalons &NSalons, TCompSections &compSection,
                                  bool only_high_layer, std::map<ASTRA::TCompLayerType, int> &uselayers_count );
  bool ChangeCfg( TSalons &NewSalons, TSalons &OldSalons );
  bool EqualSalon( TPlaceList* oldsalon, TPlaceList* newsalon, bool equal_seats_cfg );
  bool IsMiscSet( int point_id, int misc_type );
} // END namespace SALONS2

#endif /*_SALONS2_H_*/

