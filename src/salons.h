#ifndef _SALONS_H_
#define _SALONS_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <libxml/tree.h>
#include "astra_utils.h"
#include "astra_misc.h"
#include "astra_consts.h"
#include "basic.h"
#include "images.h"
#include "base_tables.h"
#include "seats_utils.h"

namespace SALONS2
{
enum TReadStyle { rTripSalons, rComponSalons };

enum TModify { mNone, mDelete, mAdd, mChange };

enum TFindSetCraft { rsComp_NoCraftComps /* нет компоновок для типа ВС*/,
                     rsComp_NoFound /* по условиям компоновка не найдена */,
                     rsComp_NoChanges /* компоновка не изменилась */,
                     rsComp_Found /* найдена новая компоновка */ };
enum TCompareComps { ccXY,
                     ccXYVisible,
                     ccName,
                     ccNameVisible,
                     ccElemType,
                     ccElemTypeVisible,
                     ccClass,
                     ccXYPrior,
                     ccXYNext,
                     ccAgle,
                     ccRemarks,
                     ccLayers,
                     ccTariffs,
                     ccDrawProps };

typedef BitSet<TCompareComps> TCompareCompsFlags;

struct TSetsCraftPoints: public std::vector<int> {
   int comp_id;
   void Clear() {
     clear();
     comp_id = -1;
   };
   TSetsCraftPoints() {
     Clear();
   };
};

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

struct TSeatTariff {
	std::string color;
	double value;
	std::string currency_id;
	TSeatTariff() {
		value = 0.0;
	};
	bool equal( const TSeatTariff &seatTarif ) const {
    return ( color == seatTarif.color &&
             value == seatTarif.value &&
             currency_id == seatTarif.currency_id );
  }
  bool operator != (const TSeatTariff &seatTarif) const {
    return !equal( seatTarif );
  }
};

struct SeatTariffCompare {
  bool operator() ( const TSeatTariff &tariff1, const TSeatTariff &tariff2 ) const {
    if ( tariff1.color + tariff1.currency_id != tariff2.color + tariff2.currency_id ) {
      return ( tariff1.color + tariff1.currency_id < tariff2.color + tariff2.currency_id );
    }
    if ( tariff1.value != tariff2.value ) {
      return ( tariff1.value < tariff2.value );
    }
    return false;
  }
};


struct TCompSection {
  std::string name;
  int firstRowIdx;
  int lastRowIdx;
  int seats;
  TCompSection() {
    firstRowIdx = ASTRA::NoExists;
    lastRowIdx = ASTRA::NoExists;
    seats = -1;
  };
  void operator = (const TCompSection &compSection) {
    name = compSection.name;
    firstRowIdx = compSection.firstRowIdx;
    lastRowIdx = compSection.lastRowIdx;
    seats = compSection.seats;
  }
};

enum TDrawPropsType { dpInfantWoSeats, dpTranzitSeats, dpTypeNum };

struct TDrawPropInfo {
   std::string figure;
   std::string color;
   std::string name;
};

class TFilterLayers:public BitSet<ASTRA::TCompLayerType> {
	private:
	  int point_dep;
	public:
		bool CanUseLayer( ASTRA::TCompLayerType layer_type, int point_id, bool pr_takeoff );
		void getFilterLayers( int point_id, bool only_compon_props=false );
};

enum TPointDepNum { pdPrior, pdNext, pdCurrent };

struct TSeatLayer {
  int point_id;
  int point_dep;
  TPointDepNum point_dep_num;
  int point_arv;
  ASTRA::TCompLayerType layer_type;
  int pax_id;
  int crs_pax_id;
  BASIC::TDateTime time_create;
  bool inRoute;
  bool equal( const TSeatLayer &seatLayer ) const {
    return ( point_id == seatLayer.point_id &&
             point_dep == seatLayer.point_dep &&
             point_arv == seatLayer.point_arv &&
             layer_type == seatLayer.layer_type &&
             pax_id == seatLayer.pax_id &&
             crs_pax_id == seatLayer.crs_pax_id &&
             time_create == seatLayer.time_create &&
             inRoute == seatLayer.inRoute );
  }
  bool operator == (const TSeatLayer &seatLayer) const {
    return equal( seatLayer );
  };
  bool operator != (const TSeatLayer &seatLayer) const {
    return !equal( seatLayer );
  }
  TSeatLayer() {
    point_id = ASTRA::NoExists;
    point_dep = ASTRA::NoExists;
    point_dep_num = pdCurrent;
    point_arv = ASTRA::NoExists;
    layer_type = ASTRA::cltUnknown;
    pax_id = ASTRA::NoExists;
    crs_pax_id = ASTRA::NoExists;
    time_create = ASTRA::NoExists;
    inRoute = true;
  };
  int getPaxId() const {
    if ( pax_id != ASTRA::NoExists )
      return pax_id;
    if ( crs_pax_id != ASTRA::NoExists )
      return crs_pax_id;
    return ASTRA::NoExists;
  }
  std::string toString() const;
};

inline int SIGND( BASIC::TDateTime a ) {
	return (a > 0.0) - (a < 0.0);
};

bool compareSeatLayer( const TSeatLayer &layer1, const TSeatLayer &layer2 );

struct SeatLayerCompare {
  bool operator() ( const TSeatLayer &layer1, const TSeatLayer &layer2 ) const {
    return compareSeatLayer( layer1, layer2 );
  }
};

struct TSeatRemark {
  std::string value;
  bool pr_denial;
  bool equal( const TSeatRemark &seatRemark ) const {
    return ( value == seatRemark.value && pr_denial == seatRemark.pr_denial );
  };
  bool operator == (const TSeatRemark &seatRemark) const {
    return equal( seatRemark );
  };
  bool operator != (const TSeatRemark &seatRemark) const {
    return !equal( seatRemark );
  }
};

struct SeatRemarkCompare {
  bool operator() ( const TSeatRemark &remark1, const TSeatRemark &remark2 ) const {
    if ( remark1.value < remark2.value ) {
      return true;
    }
    if ( remark1.value > remark2.value ) {
      return false;
    }
    if ( remark1.pr_denial < remark2.pr_denial ) {
      return true;
    }
    if ( remark1.pr_denial > remark2.pr_denial ) {
      return false;
    }
    return false;
  }
};


class TPaxList;

struct classcomp {
  bool operator() (const char& lhs, const char& rhs) const
  {return lhs<rhs;}
};

bool isPropsLayer( ASTRA::TCompLayerType layer_type );



/* свойства
 1. определение сегмента для разметки
 2. определение множества пунктов вылета, которые влияют на разметку в нашем пункте
 3. фильтрация свойств места с помощью 1 исходя из откуда и куда свойтсво места
*/

struct PointAirpNum {
  int num;
  std::string airp;
  bool in_use;
  PointAirpNum( int vnum, const std::string &vairp, bool vin_use ) {
    num = vnum;
    airp = vairp;
    in_use = vin_use;
  }
  PointAirpNum(){
    in_use = false;
  };
};

struct TFilterRoutesSets {
  int point_dep;
  int point_arv;
  TFilterRoutesSets( int vpoint_dep, int vpoint_arv=ASTRA::NoExists ) {
    point_dep = vpoint_dep;
    point_arv = vpoint_arv;
  }
  bool operator != (const TFilterRoutesSets &routesSets) {
    return ( point_dep != routesSets.point_dep ||
             point_arv != routesSets.point_arv );
  }
};

class FilterRoutesProperty: public std::vector<TTripRouteItem> {
  private:
    int point_dep;
    int point_arv;
    int crc_comp;
    int comp_id;
    bool pr_craft_lat;
    std::map<int,PointAirpNum> pointNum;
    std::set<int> takeoffPoints;
    int readNum( int point_id, bool in_use );
  public:
    //определяем множество пунктов вылета по которым надо начитать информацию
    FilterRoutesProperty( );
    void Read( const TFilterRoutesSets &filterRoutesSets );
    //фильтр св-ва места исходя из его сегмента разметки
    //проверить на пересечение сегментов
    void operator = (const FilterRoutesProperty &filterRoutes) {
      point_dep = filterRoutes.point_dep;
      point_arv = filterRoutes.point_arv;
      crc_comp = filterRoutes.crc_comp;
      comp_id = filterRoutes.comp_id;
      pr_craft_lat = filterRoutes.pr_craft_lat;
      pointNum = filterRoutes.pointNum;
      clear();
      insert( end(), filterRoutes.begin(), filterRoutes.end() );
    }
    bool useRouteProperty( int vpoint_dep, int vpoint_arv = ASTRA::NoExists );
    bool IntersecRoutes( int point_dep1, int point_arv1,
                         int point_dep2, int point_arv2 );
    int getDepartureId() const {
      return point_dep;
    }
    int getArrivalId() const  {
      return point_arv;
    }
    TFilterRoutesSets getMaxRoute() const {
      TFilterRoutesSets route( point_dep, point_arv );
      if ( !empty() ) {
        route.point_dep = front().point_id;
        route.point_arv = back().point_id;
      }
      return route;
    }
    bool isCraftLat() const {
      return pr_craft_lat;
    }
    bool getCompId() const {
      return comp_id;
    }
    bool isTakeoff( int point_id ) {
      return ( takeoffPoints.find( point_id ) != takeoffPoints.end() );
    }
    void Build( xmlNodePtr node );
};

class TPlace {
  private:
    std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > lrss, save_lrss;
    std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
    std::map<int, TSeatTariff,classcomp> tariffs;
    std::map<int,TSeatLayer> drop_blocked_layers;
    bool CompareRems( const TPlace &seat ) const;
    bool CompareLayers( const TPlace &seat ) const;
    bool CompareTariffs( const TPlace &seat ) const;
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
    BitSet<TDrawPropsType> drawProps;
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
      save_lrss = pl.save_lrss;
      lrss = pl.lrss;
      rems = pl.rems;
      WebTariff = pl.WebTariff;
      isPax = pl.isPax;
      remarks = pl.remarks;
      tariffs = pl.tariffs;
      drop_blocked_layers = pl.drop_blocked_layers;
    }
    #warning 4. lrss, remarks, tariffs in Assign, isLayer, clearLayer, AddTariff ..
    bool isLayer( ASTRA::TCompLayerType layer, int pax_id = -1 ) {
    	for (std::vector<TPlaceLayer>::iterator i=layers.begin(); i!=layers.end(); i++ ) {
    		if ( i->layer_type == layer && ( pax_id == -1 || i->pax_id == pax_id ) )
    			return true;
    	};
    	return false;
    }
    void clearLayer( ASTRA::TCompLayerType layer, BASIC::TDateTime time_create ) {
    	isPax = false;
    	for (std::vector<TPlaceLayer>::iterator i=layers.begin(); i!=layers.end(); i++ ) {
    		if ( i->pax_id > 0 )
    			isPax = true;
    		if ( i->layer_type == layer && ( (time_create <= 0 && i->time_create <= 0) || time_create == i->time_create ) ) {
    			layers.erase( i );
    			if ( isPax )
    			  break;
    		}
      }
    }
    void AddLayer( int key, const TSeatLayer &seatLayer ) {
      lrss[ key ].insert( seatLayer );
    }
    void ClearLayers() {
      lrss.clear();
    }
    void ClearLayer( int key, const TSeatLayer &seatLayer ) {
      if ( lrss.find( key ) != lrss.end() &&
           lrss[ key ].find( seatLayer ) != lrss[ key ].end() ) {
         lrss[ key ].erase( seatLayer );
         if ( lrss[ key ].empty() ) {
           lrss.erase( key );
         }
      }
    }
    void GetLayers( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > &vlayers, bool with_base_layers ) const {
      vlayers.clear();
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::const_iterator ilayers=lrss.begin();
            ilayers!=lrss.end(); ilayers++ ) {
        if ( with_base_layers ) {
          vlayers = lrss;
          return;
        }
        for ( std::set<TSeatLayer,SeatLayerCompare>::const_iterator ilayer=ilayers->second.begin();
              ilayer!=ilayers->second.end(); ilayer++ ) {
          if ( ilayer->layer_type == ASTRA::cltProtect ||
               ilayer->layer_type == ASTRA::cltSmoke ||
               ilayer->layer_type == ASTRA::cltUncomfort ) {
            continue;
          }
          vlayers[ ilayers->first ].insert( *ilayer );
        }
      }
    }
    TSeatLayer getCurrLayer( int key ) {
      if ( lrss.find( key ) == lrss.end() ) {
        TSeatLayer seatLayer;
        return seatLayer;
      }
      return *lrss[ key ].begin();
    }
    void AddDropBlockedLayer( const TSeatLayer &layer ) {
      drop_blocked_layers[ layer.point_id ] =layer;
    }
    TSeatLayer getDropBlokedLayer( int point_id ) {
      if ( drop_blocked_layers.find( point_id ) != drop_blocked_layers.end() ) {
        return drop_blocked_layers[ point_id ];
      }
      else {
        return TSeatLayer();
      }
    }
    void RollbackLayers( FilterRoutesProperty &filterRoutes );
    void CommitLayers() {
      save_lrss = lrss;
    }
    void AddRemark( int key, const TSeatRemark &seatRemark ) {
      remarks[ key ].push_back( seatRemark );
    }
    void GetRemarks( std::map<int, std::vector<TSeatRemark>,classcomp > &vremarks ) const {
      vremarks = remarks;
    }
    void AddTariff( int key, const TSeatTariff &seatTariff ) {
      tariffs[ key ] = seatTariff;
    }
    void GetTariffs( std::map<int, TSeatTariff,classcomp> &vtariffs ) const {
      vtariffs = tariffs;
    }
    void AddTariff( const std::string &vcolor,
                    const double &vvalue,
	                  const std::string &vcurrency_id ) {
       WebTariff.color = vcolor;
       WebTariff.value = vvalue;
       WebTariff.currency_id = vcurrency_id;
    }
    void AddLayerToPlace( ASTRA::TCompLayerType l, BASIC::TDateTime time_create, int pax_id,
    	                   int point_dep, int point_arv, int priority ) {
   		std::vector<TPlaceLayer>::iterator i;
      for (i=layers.begin(); i!=layers.end(); i++) {
      	if ( priority < i->priority ||
      		   (priority == i->priority &&
      		    time_create > i->time_create) )
      		break;
      }
      TPlaceLayer pl( pax_id, point_dep, point_arv, l, time_create, priority );
    	layers.insert( i, pl );
    	if ( pax_id > 0 )
    		isPax = true;
    };
    bool isChange( const TPlace &seat, BitSet<TCompareComps> &flags ) const;
    void Build( xmlNodePtr node, bool pr_lat_seat, bool pr_update ) const;
    void Build( xmlNodePtr node, int point_dep, bool pr_lat_seat, bool pr_update,
                bool with_pax, const std::map<int,TPaxList> &pax_lists ) const;

};

typedef std::vector<TPlace> TPlaces;
typedef TPlaces::iterator IPlace;

struct TCompSectionLayers {
  TCompSection compSection;
  std::map<ASTRA::TCompLayerType,TPlaces> layersSeats;
};

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
    void clearSeats() {
      places.clear();
      xs.clear();
      ys.clear();
    }
};

enum TValidLayerType { layerMultiVerify, layerVerify, layerValid, layerLess, layerInvalid, layerNotRoute };

struct TWaitListReason {
  TValidLayerType layerStatus;
  TSeatLayer layer;
  TWaitListReason( const TValidLayerType &vlayerStatus, const TSeatLayer &vlayer ) {
    layerStatus = vlayerStatus;
    layer = vlayer;
  }
  TWaitListReason( ) {
    layerStatus = layerInvalid;
  }
};

struct CompareSeats {
  bool operator() ( const TPlace* seat1, const TPlace* seat2 ) const {
    if ( seat1->y != seat2->y ) {
      return ( seat1->y < seat2->y );
    }
    if ( seat1->x != seat2->x ) {
      return ( seat1->x < seat2->x );
    }
    return false;
  }
};


struct TPaxLayerSeats {
  std::set<TPlace*,CompareSeats> seats; //упорядоченные места
  TWaitListReason waitListReason;
};

class TLayersPax: public std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare> {
  public:
    static void dumpPaxLayers( const TSeatLayer &seatLayer,
                               const TPaxLayerSeats &seats,
                               const std::string &where,
                               const TPlace *seat = NULL );
};

struct CompareSeatRange  {
  bool operator() ( const TSeatRange &seat1, const TSeatRange &seat2 ) const {
    if ( seat1 != seat2 ) {
      return ( seat1 < seat2 );
    }
    return false;
  }
};

class TInvalidRange: public std::set<TSeatRange,CompareSeatRange> {
};

struct TPassSeat {
  int num;
  TSeat seat;
  TPassSeat() {
    num = ASTRA::NoExists;
  }
  TPassSeat( int vnum,
             const std::string &xname,
             const std::string &yname ) {
    num = vnum;
    seat = TSeat( yname, xname );
  }
};

class TPassSeats: public std::vector<TPassSeat> {
  public:
    bool operator == (const TPassSeats &seats) const {
      if ( size() != seats.size() ) {
        return false;
      }
      for ( std::vector<TPassSeat>::const_iterator iseat1=begin(),
            iseat2=seats.begin();
            iseat1!=end(), iseat2!=seats.end(); iseat1++, iseat2++ ) {
        if (  iseat1->num != iseat2->num ||
              iseat1->seat != iseat2->seat ) {
          return false;
        }
      }
      return true;
    }
};

struct TSalonPax {
  private:
  public:
    int grp_id; //+ sort
    int pax_id; //+
    std::string grp_status; //+
    int point_arv;
    unsigned int seats; //+
    std::string cl; //+
    int class_grp;
    int reg_no; //+
    ASTRA::TPerson pers_type; //+
    std::string surname; //+
    std::string name; //+
    bool pr_infant; //+
    bool pr_web;
    TLayersPax layers;
    TLayersPax save_layers;
    std::map<TSeatLayer,TInvalidRange,SeatLayerCompare> invalid_ranges;
    TSalonPax() {
      seats = 0;
      reg_no = ASTRA::NoExists;
      pr_infant = false;
      pax_id = ASTRA::NoExists;
      grp_id = ASTRA::NoExists;
      class_grp = ASTRA::NoExists;
      point_arv = ASTRA::NoExists;
      pr_web = false;
    }
    void get_seats( TWaitListReason &waitListReason,
                    TPassSeats &ranges ) const;
    std::string seat_no( const std::string &format, bool pr_lat_seat, TWaitListReason &waitListReason ) const;
    std::string prior_seat_no( const std::string &format, bool pr_lat_seat ) const;
};
                                //pax_id,TSalonPax
class TPaxList: public std::map<int,TSalonPax> {
  private:
  public:
    void InfantToSeatDrawProps();
    void TranzitToSeatDrawProps( int point_dep );
    void dumpValidLayers();
    bool isSeatInfant( int pax_id ) const  {
      TPaxList::const_iterator ipax = find( pax_id );
      if ( ipax == end() )
        return false;
      return ipax->second.pr_infant;
    }
    bool isWeb( int pax_id ) const {
      TPaxList::const_iterator ipax = find( pax_id );
      if ( ipax == end() )
        return false;
      return ipax->second.pr_web;
    }
    int getRegNo( int pax_id ) const {
      TPaxList::const_iterator ipax = find( pax_id );
      if ( ipax == end() )
        return ASTRA::NoExists;
      return ipax->second.reg_no;
    }
};

struct TFilterSets {
  std::string filterClass;
  FilterRoutesProperty filterRoutes;
  std::map<int,TFilterLayers> filtersLayers;
};

struct TComponSets {
  std::string airline;
  std::string airp;
  std::string craft;
  std::string descr;
  std::string bort;
  std::string classes;
  TModify modify;
  TComponSets() {
    Clear();
  }
  void Clear() {
    modify = mNone;
    airline.clear();
    airp.clear();
    craft.clear();
    descr.clear();
    bort.clear();
    classes.clear();
  }
  void Parse( xmlNodePtr reqNode );
};

struct TMenuLayer
{
	std::string name_view;
	std::string func_key;
  bool editable;
  bool notfree;
  TMenuLayer() {
  	editable = false;
  	notfree = false;
  }
};

void getXYName( int point_id, std::string seat_no, std::string &xname, std::string &yname );

class TSalons {
  private:
    TReadStyle readStyle;
    TFilterLayers FilterLayers;
    std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
    TPlaceList* FCurrPlaceList;
    bool pr_lat_seat;
    bool pr_owner;
  public:
    int trip_id;
    int comp_id;
    std::string FilterClass;
    std::vector<TPlaceList*> placelists;
    ~TSalons( );
    TSalons( int id, TReadStyle vreadStyle );
    TSalons( );
    void SetProps( const TFilterLayers &vfilterLayers,
                   TReadStyle vreadStyle,
                   bool vpr_lat_seat,
                   std::string vFilterClass,
                   const std::map<ASTRA::TCompLayerType,TMenuLayer> &vmenuLayers ) {
      FilterLayers = vfilterLayers;
      readStyle = vreadStyle;
      pr_lat_seat = vpr_lat_seat;
      FilterClass = vFilterClass;
      menuLayers = vmenuLayers;
    }
//    void getEditableFlightLayers( BitSet<ASTRA::TCompLayerType> &editabeLayers );
    TPlaceList *CurrPlaceList();
    void SetCurrPlaceList( TPlaceList *newPlaceList );

    void Clear( );
    bool placeIsFree( TPlace* p ) {
    	for ( std::vector<TPlaceLayer>::iterator i=p->layers.begin(); i!=p->layers.end(); i++ ) {
    		if ( menuLayers[ i->layer_type ].notfree )
    			return false;
      }
      return true;
    };
    int getPriority( ASTRA::TCompLayerType layer_type ) {
      BASIC_SALONS::TCompLayerType layer_elem;
      if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) ) {
        return layer_elem.getPriority();
      }
      else return 10000;
    };
    bool getLatSeat() { return pr_lat_seat; };
    void BuildLayersInfo( xmlNodePtr salonsNode,
                          const BitSet<TDrawPropsType> &props );
    void Build( xmlNodePtr salonsNode );
    void Read( bool drop_not_used_pax_layers=true );
    void Write( const TComponSets &compSets );
    void Parse( xmlNodePtr salonsNode );
};

struct ComparePassenger {
  bool operator() ( const TSalonPax &pax1, const TSalonPax &pax2 ) const {
    if ( pax1.grp_id != pax2.grp_id ) {
      return ( pax1.grp_id < pax2.grp_id );
    }
    if ( pax1.reg_no != pax2.reg_no ) {
      return ( pax1.reg_no < pax2.reg_no );
    }
    if ( pax1.pax_id != pax2.pax_id ) {
      return ( pax1.pax_id < pax2.pax_id );
    }
    return false;
  }
};

struct CompareGrpStatus {
  bool operator() ( const std::string grp_status1, const std::string grp_status2  ) const {
    TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
     const TGrpStatusTypesRow &row1 = (TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status1 );
     const TGrpStatusTypesRow &row2 = (TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status2 );
    if ( row1.priority != row2.priority ) {
      return ( row1.priority < row2.priority );
    }
    return false;
  }
};

struct CompareClass {
  bool operator() ( const std::string &class1, const std::string &class2 ) const {
    TBaseTable &classes=base_tables.get("classes");
    const TBaseTableRow &row1=classes.get_row("code",class1);
    const TBaseTableRow &row2=classes.get_row("code",class2);
    if ( row1.AsInteger( "priority" ) != row2.AsInteger( "priority" ) ) {
      return ( row1.AsInteger( "priority" ) < row2.AsInteger( "priority" ) );
    }
  	return false;
  }
};

struct CompareArv {
  bool operator() ( const int &point_arv1, const int &point_arv2 ) const {
    if ( point_arv1 != point_arv2 ) {
      return ( point_arv1 > point_arv2 );
    }
    return false;
  }
};

enum TWaitList { wlNotInit, wlYes, wlNo };
                                   //point_arv,class,grp_status,TSalonPax
class TSalonPassengers: public std::map<int,std::map<std::string,std::map<std::string,std::set<TSalonPax,ComparePassenger>,CompareGrpStatus >,CompareClass >,CompareArv > {
  private:
  public:
    int point_dep;
    bool pr_craft_lat;
    TWaitList status_wait_list;
    bool BuildWaitList( xmlNodePtr dataNode );
    TSalonPassengers( ) {
      point_dep = ASTRA::NoExists;
      pr_craft_lat = false;
      status_wait_list = wlNotInit;
    };
    bool isWaitList( );
    bool check_waitlist_alarm( const std::map<int,TPaxList> &pax_lists,
                               const std::set<int> &paxs_external_logged );
};

struct TAutoSeat {
  int pax_id;
  TSalonPoint point;
  std::vector<TSeat> ranges;
  int seats;
  bool pr_down;
};

class TAutoSeats: public std::vector<TAutoSeat>
{
  public:
    void WritePaxSeats( int point_dep, int pax_id );
};

class TSalonList: public std::vector<TPlaceList*> {
  private:
    TFilterSets filterSets;
    int comp_id;
    bool pr_craft_lat;
    void Clear();
    inline bool findSeat( std::map<int,TPlaceList*> &salons, TPlaceList** placelist,
                          const TSalonPoint &point_s );
    void ReadSeats( TQuery &Qry, const std::string &FilterClass );
    void ReadRemarks( TQuery &Qry, FilterRoutesProperty &filterSegments,
                      int prior_compon_props_point_id );
    void ReadLayers( TQuery &Qry, FilterRoutesProperty &filterSegments,
                     TFilterLayers &filterLayers, TPaxList &pax_list,
                     int prior_compon_props_point_id );
    void ReadTariff( TQuery &Qry, FilterRoutesProperty &filterSegments,
                     int prior_compon_props_point_id );
    void ReadPaxs( TQuery &Qry, TPaxList &pax_list );
    void ReadCrsPaxs( TQuery &Qry, TPaxList &pax_list );
    void validateLayersSeats( );
    void CommitLayers();
    void RollbackLayers();
  public:
    std::map<int,TPaxList> pax_lists;
    bool isCraftLat() const {
      return pr_craft_lat;
    }
    int getCompId() const {
      return comp_id;
    }
    TFilterRoutesSets getFilterRoutes() const {
      return TFilterRoutesSets( TFilterRoutesSets( filterSets.filterRoutes.getDepartureId(),
                                                   filterSets.filterRoutes.getArrivalId() ) );
    }
    std::string getFilterClass() const {
      return filterSets.filterClass;
    }
    int getDepartureId() const {
      return filterSets.filterRoutes.getDepartureId();
    }
    int getArrivalId() const {
      return filterSets.filterRoutes.getArrivalId();
    }
    void getEditableFlightLayers( BitSet<ASTRA::TCompLayerType> &editabeLayers );
    TSalonList( ) {
      pr_craft_lat = false;
      comp_id = ASTRA::NoExists;
    }
    ~TSalonList() {
      Clear();
    }
    void ReadCompon( int vcomp_id );
    void ReadFlight( const TFilterRoutesSets &filterRoutesSets,
                     const std::string &filterClass,
                     int prior_compon_props_point_id = ASTRA::NoExists );
    void Build( bool with_pax,
                xmlNodePtr salonsNode );
    void Parse( int vpoint_id, xmlNodePtr salonsNode );
    void WriteFlight( int vpoint_id );
    void WriteCompon( int &vcomp_id, const TComponSets &componSets );
    bool CreateSalonsForAutoSeats( TSalons &Salons,
                                   TFilterRoutesSets &filterRoutes,
                                   bool pr_departure_tariff_only,
                                   const std::vector<ASTRA::TCompLayerType> &grp_layers,
                                   bool &drop_web_passes );
    void JumpToLeg( const TFilterRoutesSets &routesSets );
    void getPaxs( TSalonPassengers &passengers );
    void getPaxLayer( int point_dep, int pax_id,
                      TSeatLayer &seatLayer,
                      std::set<TPlace*,CompareSeats> &seats ) const;
    bool check_waitlist_alarm_on_tranzit_routes( const TAutoSeats &autoSeats );
};

    void check_waitlist_alarm_on_tranzit_routes( int point_dep, const std::set<int> &paxs_external_logged );
    void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm,
                                                 const std::set<int> &paxs_external_logged );
    void check_waitlist_alarm_on_tranzit_routes( int point_dep );
    void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm );
    void WritePaxSeats( int point_dep, int pax_id, const std::vector<TSeatRange> &ranges );


//typedef std::map<TPlace*, std::vector<TPlaceLayer> > TPlacePaxs; // сортировка по приоритетам слоев


	typedef std::pair<int,TPlace> TSalonSeat;
	bool Checkin( int pax_id );
  bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull );
  void GetTripParams( int trip_id, xmlNodePtr dataNode );
  void GetCompParams( int comp_id, xmlNodePtr dataNode );
  bool isAutoCompChg( int point_id );
  void setManualCompChg( int point_id );
  int GetCompId( const std::string craft, const std::string bort, const std::string airline,
                 std::vector<std::string> airps,  int f, int c, int y );
  TFindSetCraft AutoSetCraft( bool pr_tranzit_routes, int point_id, TSetsCraftPoints &points );
  TFindSetCraft AutoSetCraft( int point_id, TSetsCraftPoints &points );
  TFindSetCraft AutoSetCraft( int point_id );
  void InitVIP( int point_id );
  void setTRIP_CLASSES( int point_id );
  //void getSalonChanges( TSalons &OldSalons, std::vector<TSalonSeat> &seats );
  bool getSalonChanges( const std::vector<TPlaceList*> &list1, bool pr_craft_lat1,
                        const std::vector<TPlaceList*> &list2, bool pr_craft_lat2,
                        std::vector<TSalonSeat> &seats );
  void getSalonChanges( const TSalonList &salonList,
                        std::vector<TSalonSeat> &seats );
  void getSalonChanges( TSalons &OldSalons, std::vector<TSalonSeat> &seats );
  void BuildSalonChanges( xmlNodePtr dataNode, const std::vector<TSalonSeat> &seats );
  void BuildSalonChanges( xmlNodePtr dataNode, int point_dep, const std::vector<TSalonSeat> &seats,
                          bool with_pax, const std::map<int,SALONS2::TPaxList> &pax_lists );
  bool salonChangesToText( int point_id,
                           const std::vector<TPlaceList*> &oldlist, bool oldpr_craft_lat,
                           const std::vector<TPlaceList*> &newlist, bool newpr_craft_lat,
                           const BitSet<ASTRA::TCompLayerType> &editabeLayers,
                           std::vector<std::string> &referStrs, bool pr_set_base, int line_len );
  void ParseCompSections( xmlNodePtr sectionsNode, std::vector<TCompSection> &CompSections );
  void getLayerPlacesCompSection( TSalons &NSalons, TCompSection &compSection,
                                  bool only_high_layer,
                                  std::map<ASTRA::TCompLayerType, TPlaces> &uselayers_places,
                                  int &seats_count );
  bool ChangeCfg( const std::vector<TPlaceList*> &list1,
                  const std::vector<TPlaceList*> &list2 );
  bool IsMiscSet( int point_id, int misc_type );
  bool point_dep_AND_layer_type_FOR_TRZT_SOM_PRL( int point_id, int &point_dep, ASTRA::TCompLayerType &layer_type );
  void check_diffcomp_alarm( int point_id );
  std::string getDiffCompsAlarmRoutes( int point_id );
  int CRC32_Comp( int point_id );
  bool compatibleLayer( ASTRA::TCompLayerType layer_type );
  void verifyValidRem( const std::string &className, const std::string &remCode );
  bool isBaseLayer( ASTRA::TCompLayerType layer_type, bool isComponCraft );
  int getCrsPaxPointArv( int crs_pax_id, int point_id_spp );
  void CreateSalonMenu( int point_dep, xmlNodePtr salonsNode );
  
  bool isTranzitSalons( int point_id );
} // END namespace SALONS2


int testsalons(int argc,char **argv);

#endif /*_SALONS2_H_*/

