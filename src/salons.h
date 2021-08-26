#ifndef _SALONS_H_
#define _SALONS_H_

#include <vector>
#include <map>
#include <set>
#include <libxml/tree.h>
#include "astra_utils.h"
#include "astra_misc.h"
#include "astra_consts.h"
#include "etick.h"
#include "date_time.h"
#include "images.h"
#include "base_tables.h"
#include "seats_utils.h"
#include "comp_props.h"
#include "seats_utils.h"
#include "crafts/ComponCreator.h"

using BASIC::date_time::TDateTime;

namespace SALONS2
{

class TSalonOpType {
    public:
        static const std::list< std::pair<std::string, std::string> >& pairs()
        {
            static std::list< std::pair<std::string, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair("LAYERSDISABLE",     "������㯭� ����"));
                l.push_back(std::make_pair("LAYERSBLOCK_CENT",  "�����஢�� 業�஢��"));
                l.push_back(std::make_pair("LAYERSPROTECT",     "����ࢨ஢����"));
                l.push_back(std::make_pair("LAYERSSMOKE",       "���� ��� ������"));
                l.push_back(std::make_pair("LAYERSUNCOMFORT",   "��㤮��� ����"));
                l.push_back(std::make_pair("WEB_TARIFF",        "Web-���"));
                l.push_back(std::make_pair("REMS",              "����ન"));
                l.push_back(std::make_pair("RFISC",             "�����⪠ RFISC"));
                l.push_back(std::make_pair("SEAT",              "��᫮"));
                l.push_back(std::make_pair("SALON",             "����������"));
            }
            return l;
        }

        static std::string strip_op_type(const std::string &op_type);
};

class TSalonOpTypes: public ASTRA::PairList<std::string, std::string>
{
    private:
        virtual std::string className() const { return "TSalonOpTypes"; }
    public:
        TSalonOpTypes() : ASTRA::PairList<std::string, std::string>(TSalonOpType::pairs(),
                boost::none,
                boost::none) {}
};

const TSalonOpTypes &SalonOpTypes();

const std::string PARTITION_ELEM_TYPE = "�";
const std::string ARMCHAIR_ELEM_TYPE = "�";
const std::string ARMCHAIR_EMERGENCY_EXIT_TYPE = "�";
const std::string TOILET_ELEM_TYPE = "W";

enum TReadStyle { rTripSalons, rComponSalons };

enum TModify { mNone, mDelete, mAdd, mChange };

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
                     ccRFISC,
                     ccDrawProps };
enum TRFISCMode { rTariff, rRFISC, rAll };

typedef BitSet<TCompareComps> TCompareCompsFlags;

bool isREM_SUBCLS( std::string rem );

struct TPaxCover {
  public:
    int crs_pax_id;
    int pax_id;
    TPaxCover() {
      crs_pax_id = ASTRA::NoExists;
      pax_id = ASTRA::NoExists;
    }
    TPaxCover( int vcrs_pax_id, int vpax_id ) {
      crs_pax_id = vcrs_pax_id;
      pax_id = vpax_id;
    }
};

typedef std::vector<TPaxCover> TPaxsCover;


struct TSalonPoint {
    int x;
    int y;
    int num;
    TSalonPoint() {
        x = ASTRA::NoExists;
        y = ASTRA::NoExists;
        num = ASTRA::NoExists;
    }
    TSalonPoint( int ax, int ay, int anum ) {
        x = ax;
        y = ay;
        num = anum;
    }
    bool operator == ( const TSalonPoint &value ) const {
    return ( x == value.x &&
             y == value.y &&
             num == value.num );
    }
    bool operator < ( const TSalonPoint &value ) const {
      if ( num != value.num ) {
        return ( num < value.num );
      }
      if ( y != value.y ) {
        return( y < value.y );
      }
      if ( x != value.x ) {
        return ( x < value.x );
      }
      return false;
    }
    bool isEmpty() const {
      return ( x == ASTRA::NoExists ||
               y == ASTRA::NoExists ||
               num == ASTRA::NoExists );
    }
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
    TDateTime time_create;
    TPlaceLayer( int vpax_id, int vpoint_dep, int vpoint_arv,
                 ASTRA::TCompLayerType vlayer_type, TDateTime vtime_create, int vpriority ) {
      pax_id = vpax_id;
      point_dep = vpoint_dep;
      point_arv = vpoint_arv;
      layer_type = vlayer_type;
      time_create = vtime_create;
      priority = vpriority;
    }
    std::string toString() const {
      std::ostringstream buf;
      buf << "layer: " << EncodeCompLayerType( layer_type );
      buf << ",p_dep: " << point_dep;
      if ( point_arv != ASTRA::NoExists ) {
        buf << ",p_arv: " << point_arv;
      }
      if ( pax_id != ASTRA::NoExists ) {
        buf << ",pax_id: " << pax_id;
      }
      return buf.str();
    }
};

struct TRFISC {
    std::string color;
    double rate;
    std::string currency_id;
    std::string code;
    TRFISC() {
      clear();
    }
    TRFISC(const std::string &color,
           const double &rate,
           const std::string &currency_id,
           const std::string &code) {
      this->color = color;
      this->rate = rate;
      this->currency_id = currency_id;
      this->code = code;
    }
    void clear() {
      color.clear();
      rate = 0.0;
      currency_id.clear();
      code.clear();
    }
    bool empty() const
    {
      return color.empty(); //price == 0.0;
    }
    bool isUnknownRate() const
    {
      return ( !color.empty() && rate == 0.0 && currency_id.empty() );
    }
    std::string rateView() const
    {
      std::ostringstream buf;
      buf << std::fixed << std::setprecision(2) << rate;
      return buf.str();
    }
    std::string currencyView(const std::string &lang) const
    {
      return ElemIdToPrefferedElem(etCurrency, currency_id, efmtCodeNative, lang);
    }
    std::string str() const
    {
      return color+rateView()+currency_id;
    }
    bool equal( const TRFISC &rfisc ) const
    {
      return ( color == rfisc.color &&
               rate == rfisc.rate &&
               currency_id == rfisc.currency_id );
    }
    bool operator != (const TRFISC &rfisc) const
    {
      return !equal( rfisc );
    }
    bool operator == (const TRFISC &rfisc) const
    {
      return equal( rfisc );
    }
    bool operator < (const TRFISC &rfisc) const
    {
      if (rate != rfisc.rate)
        return rate < rfisc.rate;
      if (color != rfisc.color)
        return color > rfisc.color;
      return currency_id < rfisc.currency_id;
    }
};

struct TExtRFISC: public TRFISC {
  bool pr_prot_ckin=false;
  boost::optional<int> brand_priority;
  boost::optional<int> airps_priority;
  boost::optional<std::string> brand_code;
  void clear() {
    TRFISC::clear();
    pr_prot_ckin = false;
    brand_priority = boost::none;
    brand_code = boost::none;
    airps_priority = boost::none;
  }
};

struct TSeatTariff {
    std::string color;
    double rate;
    std::string currency_id;
    TSeatTariff()
    {
      clear();
    }
    TSeatTariff(const std::string& _color,
                const double& _rate,
                const std::string& _currency_id)
    {
      clear();
      color=_color;
      rate=_rate;
      currency_id=_currency_id;
    }
    void clear()
    {
      color.clear();
      rate = 0.0;
      currency_id.clear();
    }
    bool empty() const
    {
      return color.empty(); //price == 0.0;
    }
    std::string rateView() const
    {
      std::ostringstream buf;
      buf << std::fixed << std::setprecision(2) << rate;
      return buf.str();
    }
    std::string currencyView(const std::string &lang) const
    {
      return ElemIdToPrefferedElem(etCurrency, currency_id, efmtCodeNative, lang);
    }
    std::string str() const
    {
      return color+rateView()+currency_id;
    }
    bool equal( const TSeatTariff &seatTarif ) const
    {
      return ( color == seatTarif.color &&
               rate == seatTarif.rate &&
               currency_id == seatTarif.currency_id );
    }
    bool operator != (const TSeatTariff &seatTarif) const
    {
      return !equal( seatTarif );
    }
    bool operator < (const TSeatTariff &seatTarif) const
    {
      if (rate != seatTarif.rate)
        return rate < seatTarif.rate;
      if (color != seatTarif.color)
        return color > seatTarif.color;
      return currency_id < seatTarif.currency_id;
    }
};

struct SeatTariffCompare {
  bool operator() ( const TSeatTariff &tariff1, const TSeatTariff &tariff2 ) const
  {
    return tariff1<tariff2;
  }
};

struct RFISCCompare {
  bool operator() ( const TRFISC &rfisc1, const TRFISC &rfisc2 ) const
  {
    return rfisc1<rfisc2;
  }
};


class TSeatTariffMapType : public std::map<std::string,TExtRFISC> { //color,rfisc
  public:
    std::string key() const {
      std::string res;
      for (auto i=this->cbegin(); i!=this->cend(); i++ ) {
         res += " " + i->second.str();
      }
      return res;
    }
};

class TSeatTariffMap : public TSeatTariffMapType
{
  public:
    enum TStatus
    {
      stNotFound,       //����-����� �� �������
      stNotRFISC,       //���� �孮����� - ��� RFISC � �����������
      //�� ��⠫�� ������ �⭮����� � �孮����� �ᯮ�짮����� RFISC
      stNotOperating,   //���ᠦ�� �� �������饣� ��ॢ��稪�
      stNotET,          //����� �㬠��� ��� �� �����
      stUnknownETDisp,  //��ᯫ�� ����� �������⥭
      stUseRFISC        //����� �孮����� - RFISC � �����������
    };

  private:
    int _potential_queries, _real_queries;
    TStatus _status;
    std::map<int/*point_id_oper*/, TAdvTripInfo> oper_flts;
    std::map<int/*point_id_mark*/, TSimpleMktFlight> mark_flts;
    std::map<std::string/*airline_oper*/, TSeatTariffMapType> rfisc_colors;
    std::map<int/*point_id_oper*/, TSeatTariffMapType > tariff_map; //��� stNotRFISC

    void get(DB::TQuery &Qry, const std::string &traceDetail);
    void get_rfisc_colors_internal(const std::string &airline_oper);
  public:
    TSeatTariffMap() : _potential_queries(0), _real_queries(0) { clear(); }
    //᪮॥ �ᥣ� �㤥� �ᯮ�짮������ ��। ��ࢮ��砫쭮� ॣ����樥�:
    void get(const TAdvTripInfo &operFlt, const TSimpleMktFlight &markFlt,
             const CheckIn::TPaxTknItem &tkn, const std::string& airp_arv);
    //�᫨ ���ᠦ�� ��ॣ����஢�� � �� �⨬ ��᮫�⭮ ��������஢��� ������ � ��:
    void get(const int point_id_oper,
             const int point_id_mark,
             const int grp_id,
             const int pax_id,
             const std::string& airp_arv);
    //�᫨ ���ᠦ�� ��ॣ����஢��:
    void get(const int pax_id);

    int potential_queries() const
    {
      return _potential_queries;
    }
    int real_queries() const
    {
      return _real_queries;
    }
    TStatus status() const
    {
      return _status;
    }
    bool is_rfisc_applied(const std::string &airline_oper);
    void get_rfisc_colors(const std::string &airline_oper);
    void clear()
    {
      TSeatTariffMapType::clear();
      _status=stNotFound;
    }
    void trace( TRACE_SIGNATURE ) const;
};


enum TDrawPropsType { dpInfantWoSeats, dpTranzitSeats, dpTypeNum };

struct TDrawPropInfo {
   std::string figure;
   std::string color;
   std::string name;
};

class TFilterLayer_SOM_PRL {
  private:
    int point_dep;
    ASTRA::TCompLayerType layer_type;
    void IntRead( int point_id, const std::vector<TTripRouteItem> &routes );
    void Clear() {
      point_dep = ASTRA::NoExists;
      layer_type = ASTRA::cltUnknown;
    }
  public:
    TFilterLayer_SOM_PRL() {
      Clear();
    }
    void Read( int point_id );
    void ReadOnTranzitRoutes( int point_id, const std::vector<TTripRouteItem> &routes ) {
      IntRead( point_id, routes );
    }
    bool Get( int &vpoint_dep, ASTRA::TCompLayerType &vlayer_type ) {
      vpoint_dep = point_dep;
      vlayer_type = layer_type;
      return ( vpoint_dep != ASTRA::NoExists );
    }
};

class TFilterLayers:public BitSet<ASTRA::TCompLayerType> {
    private:
      int point_dep;
    void getIntFilterLayers( int point_id,
                             const std::vector<TTripRouteItem> &routes,
                             bool only_compon_props );
    public:
    TFilterLayers() {
      point_dep = ASTRA::NoExists;
    }
    bool CanUseLayer( ASTRA::TCompLayerType layer_type,
                      int layer_point_dep, // �㭪� �뫥� ᫮�
                      int point_salon_departure, // �㭪� �⮡ࠦ���� ����������
                      bool pr_takeoff /*�ਧ��� 䠪� �뫥�*/ );
    void getFilterLayers( int point_id, bool only_compon_props=false );
    void getFilterLayersOnTranzitRoutes( int point_id,
                                         const std::vector<TTripRouteItem> &routes,
                                         bool only_compon_props=false ) {
      getIntFilterLayers( point_id, routes, only_compon_props );
    }
    int getSOM_PRL_Dep( ) {
      return point_dep;
    };
};

enum TPointDepNum { pdPrior, pdNext, pdCurrent };

class TLayerSeat {
  public:
    int point_id;
    int point_dep;
    int point_arv;
    ASTRA::TCompLayerType layer_type;
    int pax_id;
    int crs_pax_id;
    TDateTime time_create;
    bool inRoute;
    bool equal( const TLayerSeat &layerSeat ) const {
      return ( point_id == layerSeat.point_id &&
               point_dep == layerSeat.point_dep &&
               point_arv == layerSeat.point_arv &&
               layer_type == layerSeat.layer_type &&
               pax_id == layerSeat.pax_id &&
               crs_pax_id == layerSeat.crs_pax_id &&
               time_create == layerSeat.time_create &&
               inRoute == layerSeat.inRoute );
    }
    void Clear() {
      point_id = ASTRA::NoExists;
      point_dep = ASTRA::NoExists;
      point_arv = ASTRA::NoExists;
      layer_type = ASTRA::cltUnknown;
      pax_id = ASTRA::NoExists;
      crs_pax_id = ASTRA::NoExists;
      time_create = ASTRA::NoExists;
      inRoute = true;
    }
  public:
    bool operator == (const TLayerSeat &layerSeat) const {
      return equal( layerSeat );
    }
    bool operator != (const TLayerSeat &layerSeat) const {
      return !equal( layerSeat );
    }
    TLayerSeat() {
      Clear();
    }
    int getPaxId() const {
       if ( pax_id != ASTRA::NoExists )
         return pax_id;
       return crs_pax_id;
     }

};

class TLayerPrioritySeat: private TLayerSeat {
  private:
    int fpriority;
    TPointDepNum fpoint_dep_num;
  public:
    int priority() const {
      return fpriority;
    }
    TLayerPrioritySeat( const TLayerSeat& layerSeat, int _priority ):TLayerSeat(layerSeat), fpriority(_priority){
      fpoint_dep_num = pdCurrent;
    }
    static TLayerPrioritySeat emptyLayer() {
      return TLayerPrioritySeat( TLayerSeat(), 0 );
    }
    ASTRA::TCompLayerType layerType() const {
      return TLayerSeat::layer_type;
    }
    int point_id() const {
      return TLayerSeat::point_id;
    }
    int point_dep() const {
      return TLayerSeat::point_dep;
    }
    int point_arv() const {
      return TLayerSeat::point_arv;
    }
    int pax_id() const {
      return TLayerSeat::pax_id;
    }
    int crs_pax_id() const {
      return TLayerSeat::crs_pax_id;
    }
    TDateTime time_create() const {
      return TLayerSeat::time_create;
    }
    int getPaxId() const {
      return TLayerSeat::getPaxId();
    }
    bool inRoute() const {
      return TLayerSeat::inRoute;
    }
    TPointDepNum point_dep_num() const {
      return fpoint_dep_num;
    }
    bool operator == (const TLayerPrioritySeat &seatPriorityLayer) const {
      return TLayerSeat::equal( seatPriorityLayer );
    }
    bool operator != (const TLayerPrioritySeat &seatPriorityLayer) const {
      return !TLayerSeat::equal( seatPriorityLayer );
    }
    void setInRoute( bool _inRoute ) {
      TLayerSeat::inRoute = _inRoute;
    }
    void setCurrentDep() {
      fpoint_dep_num = pdCurrent;
    }
    void setPriorDep() {
      fpoint_dep_num = pdPrior;
    }
    void setNextDep() {
      fpoint_dep_num = pdNext;
    }
    std::string toString() const;
};

/*struct TSeatLayer {
  int point_id;
  int point_dep;
  TPointDepNum point_dep_num;
  int point_arv;
  ASTRA::TCompLayerType layer_type;
  int pax_id;
  int crs_pax_id;
  TDateTime time_create;
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
  }
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
  }
  int getPaxId() const {
    if ( pax_id != ASTRA::NoExists )
      return pax_id;
    if ( crs_pax_id != ASTRA::NoExists )
      return crs_pax_id;
    return ASTRA::NoExists;
  }
  std::string toString() const;
};*/

inline int SIGND( TDateTime a ) {
    return (a > 0.0) - (a < 0.0);
}

bool compareLayerSeat( const TLayerPrioritySeat &layer1, const TLayerPrioritySeat &layer2 );

struct LayerPrioritySeatCompare {
  bool operator() ( const TLayerPrioritySeat &layer1, const TLayerPrioritySeat &layer2 ) const {
    return compareLayerSeat( layer1, layer2 );
  }
};

struct TSalonPax;
class TPlace;

class TSalonPointNames {
  public:
    TSalonPoint point;
    TSeat seat;
    TSalonPointNames( const TSalonPoint &asalonPoint, const TSeat &aseat ) {
      point = asalonPoint;
      seat = aseat;
    }
};

class TLayersSeats:public std::map<TLayerPrioritySeat,TPassSeats,LayerPrioritySeatCompare > {};

class TSectionInfo:public SimpleProp {
  private:
    std::map<ASTRA::TCompLayerType,std::vector<TPlace*> > totalLayerSeats; //᫮�
    std::map<ASTRA::TCompLayerType,std::vector<std::pair<TLayerPrioritySeat,TPassSeats> > > currentLayerSeats;
    std::vector<TSalonPointNames> salonPoints;
    TLayersSeats layersPaxs; //seatLayer->pax_id ᯨ᮪ ���ᠦ�஢ � ���⠬�
    std::map<int,TSalonPax> paxs;
  public:
    TSectionInfo():SimpleProp() {
      clear();
      clearProps();
    }
    void clearProps();
    void operator = (const TSectionInfo &sectionInfo);
    bool inSection( int row ) const {
      return SimpleProp::inSection( row );
    }
    bool inSection( const TSalonPoint &salonPoint ) const;
    bool inSection( const TSeat &aseat ) const;
    bool inSectionPaxId( int pax_id );
    void AddSalonPoints( const TSalonPoint &asalonPoint, const TSeat &aseat ) {
      salonPoints.push_back( TSalonPointNames( asalonPoint, aseat ) );
    }
    void AddTotalLayerSeat( const ASTRA::TCompLayerType &layer_type, TPlace* seat ) {
      totalLayerSeats[ layer_type ].push_back( seat );
    }
    void AddCurrentLayerSeat( const TLayerPrioritySeat &layer, TPlace* seat );
    void AddLayerSeats( const TLayerPrioritySeat &layerPrioritySeat, const TSeat &seats );
    void AddPax( const TSalonPax &pax );
    void GetLayerSeats( TLayersSeats &value );
    void GetPaxs( std::map<int,TSalonPax> &value );
    void GetCurrentLayerSeat( const ASTRA::TCompLayerType &layer_type,
                              std::vector<std::pair<TLayerPrioritySeat,TPassSeats> > &layersSeats );
    void GetTotalLayerSeat( const ASTRA::TCompLayerType &layer_type,
                            TPassSeats &layerSeats );
    int seatsTotalLayerSeats( const ASTRA::TCompLayerType &layer_type );
    int seatsCurrentLayerSeats( const ASTRA::TCompLayerType &layer_type );
};

class TCompSection: public TSectionInfo {
  public:
    int seats;
    TCompSection():TSectionInfo() {
      seats = -1;
    }
    void operator = (const TCompSection &compSection) {
      TSectionInfo::operator = ( compSection );
      seats = compSection.seats;
    }
    void operator = (const SimpleProp &simpleProp) {
       SimpleProp::operator = ( simpleProp );
    }
};

struct TSeatRemark {
  std::string value;
  bool pr_denial;
  bool equal( const TSeatRemark &seatRemark ) const {
    return ( value == seatRemark.value && pr_denial == seatRemark.pr_denial );
  }
  bool operator == (const TSeatRemark &seatRemark) const {
    return equal( seatRemark );
  }
  bool operator != (const TSeatRemark &seatRemark) const {
    return !equal( seatRemark );
  }
  TSeatRemark( const std::string& _value, bool _pr_denial ) {
    value = _value;
    pr_denial = _pr_denial;
  }
  TSeatRemark() {
    value = "";
    pr_denial = true;
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

struct classcomp   {
  bool operator() (const int& lhs, const int& rhs) const
  {return lhs<rhs;}
};

bool isPropsLayer( ASTRA::TCompLayerType layer_type );

/* ᢮��⢠
 1. ��।������ ᥣ���� ��� ࠧ��⪨
 2. ��।������ ������⢠ �㭪⮢ �뫥�, ����� ������ �� ࠧ���� � ��襬 �㭪�
 3. 䨫����� ᢮��� ���� � ������� 1 ��室� �� ��㤠 � �㤠 ᢮��᢮ ����
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
    TTripInfo fltInfo;
    std::map<int,PointAirpNum> pointNum;
    std::set<int> takeoffPoints;
    int readNum( int point_id, bool in_use );
  public:
    //��।��塞 ������⢮ �㭪⮢ �뫥� �� ����� ���� ������ ���ଠ��
    FilterRoutesProperty( );
    FilterRoutesProperty( const std::string &airline ):FilterRoutesProperty( ) {
      this->fltInfo.airline = airline;
    }
    FilterRoutesProperty( const TTripInfo &fltInfo ):FilterRoutesProperty( ) {
      this->fltInfo = fltInfo;
    }
    void Clear();
    void Read( const TFilterRoutesSets &filterRoutesSets );
    //䨫��� �-�� ���� ��室� �� ��� ᥣ���� ࠧ��⪨
    //�஢���� �� ����祭�� ᥣ���⮢
    void operator = (const FilterRoutesProperty &filterRoutes) {
      point_dep = filterRoutes.point_dep;
      point_arv = filterRoutes.point_arv;
      crc_comp = filterRoutes.crc_comp;
      comp_id = filterRoutes.comp_id;
      pr_craft_lat = filterRoutes.pr_craft_lat;
      fltInfo = filterRoutes.fltInfo;
      pointNum = filterRoutes.pointNum;
      clear();
      insert( end(), filterRoutes.begin(), filterRoutes.end() );
      takeoffPoints = filterRoutes.takeoffPoints;
    }
    bool useRouteProperty( int vpoint_dep, int vpoint_arv = ASTRA::NoExists );
    bool IntersecRoutes( int point_dep1, int point_arv1,
                         int point_dep2, int point_arv2, bool pr_routes );
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
    std::string getAirline() const {
      return fltInfo.airline;
    }
    int getCompId() const {
      return comp_id;
    }
    bool isTakeoff( int point_id ) {
      return ( takeoffPoints.find( point_id ) != takeoffPoints.end() );
    }
    bool inTripRoutes( int vpoint_id ) {
      for ( FilterRoutesProperty::const_iterator item=begin();
            item!=end(); item++ ) {
        if ( item->point_id == vpoint_id ) {
          return true;
        }
      }
      return false;
    }
    TTripInfo& getfltInfo() {
      return fltInfo;
    }
    void Build( xmlNodePtr node );
};

enum TGetLayersMode { glAll, glBase, glNoBase };

typedef std::set<TLayerPrioritySeat,LayerPrioritySeatCompare> TSetOfLayerPriority;

class TPlace {
  private:
    std::map<int, TSetOfLayerPriority,classcomp > lrss, save_lrss;
    std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
    std::map<int, TSeatTariff,classcomp> tariffs;
    std::map<int,TLayerPrioritySeat> drop_blocked_layers;
    std::map<int, TRFISC,classcomp> rfiscs;
    bool CompareRems( const TPlace &seat ) const;
    bool CompareLayers( const TPlace &seat ) const;
    bool CompareTariffs( const TPlace &seat ) const;
    bool CompareRFISCs( const TPlace &seat ) const;
  public:
    std::string seatDescr;
    bool visible;
    int x, y, num;
    std::string elem_type;
    bool isplace;
    int xprior, yprior;
    int xnext, ynext;
    int agle;
    std::string clname;
    std::string xname, yname;
    std::vector<TRem> rems;
    std::vector<TPlaceLayer> layers;
    TSeatTariff SeatTariff;
    //TRFISC rfisc;
    BitSet<TDrawPropsType> drawProps;
    bool isPax;
    TPlace() {
      x = -1;
      y = -1;
      num = -1;
      visible = false;
      isplace = false;
      xprior = -1;
      yprior = -1;
      xnext = -1;
      ynext = -1;
      agle = 0;
      isPax = false;
    }
    bool isLayer( ASTRA::TCompLayerType layer, int pax_id = -1 ) const;
    static bool isCleanDoubleLayerType( ASTRA::TCompLayerType layer_type ) {
      return ( layer_type == ASTRA::cltSOMTrzt ||
               layer_type == ASTRA::cltPRLTrzt );
    }
    void AddLayer( int key, const TLayerPrioritySeat &seatLayer );
    void ClearLayers() {
      lrss.clear();
    }
    void ClearLayer( int key, const TLayerPrioritySeat &seatLayer ) {
      if ( lrss.find( key ) != lrss.end() &&
           lrss[ key ].find( seatLayer ) != lrss[ key ].end() ) {
         lrss[ key ].erase( seatLayer );
         if ( lrss[ key ].empty() ) {
           lrss.erase( key );
         }
      }
    }
    void GetLayers( std::map<int, TSetOfLayerPriority,classcomp > &vlayers, TGetLayersMode layersMode ) const {
      vlayers.clear();
      for ( std::map<int, TSetOfLayerPriority,classcomp >::const_iterator ilayers=lrss.begin();
            ilayers!=lrss.end(); ilayers++ ) {
        if ( layersMode == glAll ) {
          vlayers = lrss;
          return;
        }
        for ( const auto& layer : ilayers->second ) {
          bool pr_base = ( layer.layerType() == ASTRA::cltProtect ||
                           layer.layerType() == ASTRA::cltSmoke ||
                           layer.layerType() == ASTRA::cltUncomfort );
          if ( (pr_base && layersMode == glNoBase) ||
               (!pr_base && layersMode == glBase) ) {
              continue;
          }
          vlayers[ ilayers->first ].insert( layer );
        }
      }
    }
    TLayerPrioritySeat getCurrLayer( int key ) const {
      std::map<int, TSetOfLayerPriority,classcomp >::const_iterator cl = lrss.find( key );
      if ( cl == lrss.end() ) {
        return TLayerPrioritySeat::emptyLayer();
      }
      return *cl->second.begin();
    }
    void GetLayers( int point_dep, TSetOfLayerPriority &vlayers, TGetLayersMode layersMode ) const {
      vlayers.clear();
      std::map<int, TSetOfLayerPriority,classcomp > rlayers;
      GetLayers( rlayers, layersMode );
      std::map<int, std::set<SALONS2::TLayerPrioritySeat,SALONS2::LayerPrioritySeatCompare>,SALONS2::classcomp >::iterator ilayers = rlayers.find( point_dep );
      if ( ilayers != rlayers.end() ) {
        vlayers = ilayers->second;
      }
    }
    void AddDropBlockedLayer( const TLayerPrioritySeat &layer ) {
      drop_blocked_layers.emplace( layer.point_id(), layer );
    }
    TLayerPrioritySeat getDropBlockedLayer( int point_id ) const {
      std::map<int,TLayerPrioritySeat>::const_iterator idrop_layer = drop_blocked_layers.find( point_id );
      if ( idrop_layer != drop_blocked_layers.end() ) {
        return idrop_layer->second;
      }
      return TLayerPrioritySeat::emptyLayer();
    }
    void RollbackLayers( FilterRoutesProperty &filterRoutes,
                         std::map<int,TFilterLayers> &filtersLayers );
    void CommitLayers() {
      save_lrss = lrss;
    }
    void AddRemark( int key, const TSeatRemark &seatRemark ) {
      remarks[ key ].push_back( seatRemark );
    }
    void GetRemarks( std::map<int, std::vector<TSeatRemark>,classcomp > &vremarks ) const {
      vremarks = remarks;
    }
    void GetRemarks( int key, std::vector<TSeatRemark> &vremarks ) const {
      vremarks.clear();
      std::map<int, std::vector<TSeatRemark>,classcomp >::const_iterator point_rems = remarks.find( key );
      if ( point_rems != remarks.end() ) {
        vremarks = point_rems->second;
      }
    }
    void GetRemarks( int key, std::vector<std::string> &vremarks, bool pr_denial ) const {
      vremarks.clear();
      std::map<int, std::vector<TSeatRemark>,classcomp >::const_iterator point_rems = remarks.find( key );
      if ( point_rems != remarks.end() ) {
        for ( auto irem : point_rems->second ) {
          if ( irem.pr_denial == pr_denial ) {
             vremarks.push_back( irem.value );
          }
        }
      }
    }
    void clearRemarks( ) {
      remarks.clear();
    }
    void AddTariff( int key, const TSeatTariff &seatTariff ) {
      tariffs[ key ] = seatTariff;
    }
    void clearTariffs() {
      tariffs.clear();
    }
    void GetTariffs( std::map<int, TSeatTariff,classcomp> &vtariffs ) const {
      vtariffs = tariffs;
    }
    TSeatTariff GetTariff( int point_dep ) const {
      std::map<int, TSeatTariff,classcomp>::const_iterator itariff = tariffs.find( point_dep );
      if ( itariff != tariffs.end() ) {
        return itariff->second;
      }
      return TSeatTariff();
    }
    void AddRFISC( int key, const TRFISC &rfisc ) {
      rfiscs[ key ] = rfisc;
    }
    void GetRFISCs( std::map<int, TRFISC,classcomp> &vrfiscs ) const {
      vrfiscs = rfiscs;
    }
    void clearRFISCs() {
      rfiscs.clear();
    }
   TRFISC getRFISC( int point_id ) const;
    void SetTariffsByRFISCColor( int point_dep, const TSeatTariffMapType &salonTariffs, const TSeatTariffMap::TStatus &status );
    void SetTariffsByRFISC( int point_dep, const std::string& airline );
    void AddLayerToPlace( ASTRA::TCompLayerType l, TDateTime time_create, int pax_id,
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
    }
    void SetRFISC( int point_id, TSeatTariffMapType &tariffMap );
    //void SetRFICSRemarkByColor( int key, TSeatTariffMapType salonRFISCColor );
    //void DropRFISCRemarks( TSeatTariffMapType salonRFISCColor );
    void convertSeatTariffs( int point_dep );
    void convertSeatTariffs( bool pr_departure_tariff_only, int point_dep, const std::vector<int> &points );
    bool isChange( const TPlace &seat, BitSet<TCompareComps> &flags ) const;
    xmlNodePtr Build( xmlNodePtr node, bool pr_lat_seat, bool pr_update ) const;
    xmlNodePtr Build( xmlNodePtr node, int point_dep, bool pr_lat_seat,
                      TRFISCMode RFISCMode, bool pr_update,
                      bool with_pax, const std::map<int,TPaxList> &pax_lists ) const;

    std::string denorm_view(bool is_lat) const
    {
      return getTSeat().denorm_view(is_lat);
    }

    TSeat getTSeat() const { return TSeat(yname, xname); }
};

typedef std::vector<TPlace> TPlaces;
typedef TPlaces::iterator IPlace;
typedef TPlaces::const_iterator CIPlace;

struct TCompSectionLayers {
  TCompSection compSection;
  std::map<ASTRA::TCompLayerType,TPlaces> layersSeats;
};

class TPlaceList {
  private:
    std::map<std::string,TPoint> CacheSeatName;
    std::vector<std::string> xs, ys;
  public:
    TPlaces places;
    int num;
    TPlace *place( int idx );
    TPlace *place( const TPoint &p );
    int GetPlaceIndex( const TPoint &p );
    int GetPlaceIndex( int x, int y );
    int GetXsCount();
    int GetYsCount();
    bool ValidPlace( const TPoint &p );
    std::string GetPlaceName( const TPoint &p );
    std::string GetXsName( int x );
    std::string GetYsName( int y );
    bool GetIsPlaceXY( const std::string& placeName, TPoint &p );
    bool GetIsNormalizePlaceXY( const std::string& xname,
                                const std::string& yname,
                                TSalonPoint &p );
    int Add( TPlace &pl );
    void clearSeats() {
      places.clear();
      xs.clear();
      ys.clear();
      CacheSeatName.clear();
    }
    bool isEmpty() {
      return places.empty();
    }
};

enum TValidLayerType { layerMultiVerify, layerVerify, layerValid, layerLess, layerInvalid, layerNotRoute };

class TWaitListReason: public TLayerPrioritySeat {
  public:
    TValidLayerType status;
    TWaitListReason( const TValidLayerType &_layerStatus,
                     const TLayerPrioritySeat &_layer ):TLayerPrioritySeat(_layer),status(_layerStatus){}
    TWaitListReason( ) :TLayerPrioritySeat(TLayerPrioritySeat::emptyLayer()),status(layerInvalid){}
    void operator = ( const TLayerPrioritySeat& layer ) {
      dynamic_cast<TLayerPrioritySeat&>(*this) = layer;
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
  std::set<TPlace*,CompareSeats> seats; //㯮�冷祭�� ����
  TWaitListReason waitListReason;
};

typedef std::map<TLayerPrioritySeat,TPaxLayerSeats,LayerPrioritySeatCompare> TLayersPax;

struct TPass {
  int pax_id;
  int grp_id;
  std::string grp_status;
  int point_dep;
  int point_arv;
  int reg_no;
  std::string name;
  std::string surname;
  int is_female;
  int parent_pax_id;
  int temp_parent_id;
  bool pr_inf;
  bool pr_web;
  std::string orig_cl;
  std::string cabin_cl;
  int cabin_class_grp;
  int seats;
  bool is_jmp;
  ASTRA::TPerson pers_type;
  ASTRA::TCrewType::Enum crew_type;
  TPass() {
    pax_id = ASTRA::NoExists;
    grp_id = ASTRA::NoExists;
    reg_no = ASTRA::NoExists;
    is_female = ASTRA::NoExists;
    point_dep = ASTRA::NoExists;
    point_arv = ASTRA::NoExists;
    cabin_class_grp = ASTRA::NoExists;
    parent_pax_id = ASTRA::NoExists;
    temp_parent_id = ASTRA::NoExists;
    pr_inf = false;
    pr_web = false;
    crew_type = ASTRA::TCrewType::Unknown;
    is_jmp = false;
  }
};

typedef std::map<TLayerPrioritySeat,TSeatRanges,LayerPrioritySeatCompare> TTotalRanges;

struct TSalonPax {
  private:
    void int_get_seats( TWaitListReason &waitListReason,
                        TCompLayerType &pax_layer_type,
                        std::vector<TPlace*> &seats,
                        bool with_crs=false) const;
  public:
    int grp_id; //+ sort
    int pax_id; //+
    std::string grp_status; //+
    int point_dep;
    int point_arv;
    unsigned int seats; //+
    bool is_jmp;
    std::string orig_cl;
    std::string cabin_cl; //+
    int cabin_class_grp;
    int reg_no; //+
    ASTRA::TPerson pers_type; //+
    std::string surname; //+
    std::string name; //+
    int is_female;
    int pr_infant; //+
    int parent_pax_id;
    bool pr_web;
    ASTRA::TCrewType::Enum crew_type;
    TLayersPax layers; // ����� ⮫쪮 ������� + �� ������� ����� �ਮ���� commit-rollback �� � save_layers
    TLayersPax save_layers;
    TTotalRanges total_ranges;
    TSalonPax() {
      seats = 0;
      is_jmp = false;
      reg_no = ASTRA::NoExists;
      is_female = ASTRA::NoExists;
      pr_infant = ASTRA::NoExists;
      pax_id = ASTRA::NoExists;
      grp_id = ASTRA::NoExists;
      cabin_class_grp = ASTRA::NoExists;
      point_dep = ASTRA::NoExists;
      point_arv = ASTRA::NoExists;
      parent_pax_id = ASTRA::NoExists;
      pr_web = false;
      pers_type = ASTRA::NoPerson;
      crew_type = ASTRA::TCrewType::Unknown;
    }
    void operator = ( const TPass &pass ) {
      grp_id = pass.grp_id;
      grp_status = pass.grp_status;
      point_dep = pass.point_dep;
      point_arv = pass.point_arv;
      seats = pass.seats;
      is_jmp = pass.is_jmp;
      reg_no = pass.reg_no;
      pers_type = pass.pers_type;
      orig_cl = pass.orig_cl;
      cabin_cl = pass.cabin_cl;
      cabin_class_grp = pass.cabin_class_grp;
      name = pass.name;
      surname = pass.surname;
      is_female = pass.is_female;
      if ( pass.pr_inf ) {
        pr_infant = pass.parent_pax_id;
      }
      pr_web = pass.pr_web;
      crew_type = pass.crew_type;
    }
    void get_seats( TWaitListReason &waitListReason,
                    TCompLayerType &pax_layer_type,
                    TPassSeats &ranges) const;
    void get_seats( TWaitListReason &waitListReason,
                    TPassSeats &ranges) const;
    void get_seats( TWaitListReason &waitListReason,
                    TPassSeats &ranges,
                    std::map<TSeat,TPlace*,CompareSeat> &descrs,
                    bool with_crs=false ) const;
    std::string seat_no( const std::string &format, bool pr_lat_seat,
                         TWaitListReason &waitListReason,
                         TCompLayerType &pax_layer_type) const;
    std::string seat_no( const std::string &format, bool pr_lat_seat,
                         TWaitListReason &waitListReason) const;
/*    std::string crs_seat_no( const std::string &format, bool pr_lat_seat,
                             TWaitListReason &waitListReason,
                             TCompLayerType &pax_layer_type) const;*/
    std::string event_seat_no(bool pr_lat_seat, int point_dep, TWaitListReason &waitListReason, LEvntPrms &evntPrms) const;
    std::string prior_seat_no( const std::string &format, bool pr_lat_seat ) const;
    std::string prior_crs_seat_no( const std::string &format, bool pr_lat_seat,
                                   TCompLayerType& pax_layer_type ) const;
};
                                //pax_id,TSalonPax
class TPaxList: public std::map<int,TSalonPax> {
  public:
    std::map<int,TSalonPax> infants;
    std::set<int> refuseds; //pax_id � �⬥������ ॣ����樥�
    void InfantToSeatDrawProps();
    void TranzitToSeatDrawProps( int point_dep );
    void dumpValidLayers();
    void setPaxWithInfant( std::set<int> &paxs ) const {
      for ( TPaxList::const_iterator ipax=begin(); ipax!=end(); ++ipax ) {
        if ( ipax->second.pr_infant != ASTRA::NoExists ) {
          paxs.insert( ipax->first );
        }
      }
    }
    bool isSeatInfant( int pax_id ) const  {
      TPaxList::const_iterator ipax = find( pax_id );
      if ( ipax == end() )
        return false;
      return ipax->second.pr_infant != ASTRA::NoExists;
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

//enum TSalonReadVersion { rfNoTranzitVersion, rfTranzitVersion };

bool filterComponsForView( const std::string &airline, const std::string &airp );
bool filterComponsForEdit( const std::string &airline, const std::string &airp );

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
  static void CheckAirlAirp(xmlNodePtr reqNode, std::string &airline, std::string &airp );
};

struct getSeatKey {
  static int get( int num, int x, int y ) {
    return num*1000000 + y*1000 + x;
  }
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
    boost::optional<std::set<std::string>> ExistSubcls; //for passernger seats with subcls rem
    boost::optional<std::set<ASTRA::TCompLayerType>> ExistBaseLayers; // for auto seats
    boost::optional<std::set<int>> OccupySeats; // for auto seats
    //void Write( const TComponSets &compSets );
//    void Build( xmlNodePtr salonsNode );
    //void Read( bool drop_not_used_pax_layers=true );
    //void Parse( xmlNodePtr salonsNode );
  public:
    int trip_id;
    int comp_id;
    std::string FilterClass;
    std::vector<TPlaceList*> placelists;
    ~TSalons( );
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
    }
    bool getLatSeat() { return pr_lat_seat; }
    /*void BuildLayersInfo( xmlNodePtr salonsNode,
                          const BitSet<TDrawPropsType> &props );*/
    void SetTariffsByRFISCColor( int point_dep, const TSeatTariffMapType &tariffs, const TSeatTariffMap::TStatus &status );
    void SetTariffsByRFISC( int point_dep, const std::string& airline );
    void AddExistSubcls( const TRem &rem ) {
      if ( rem.pr_denial || !isREM_SUBCLS( rem.rem ) ) { //�� ࠧ�襭��� ��� �� ��������
        return;
      }
      if ( ExistSubcls == boost::none ) {
        ExistSubcls = std::set<std::string>();
      }
      ExistSubcls.get().insert( rem.rem );
    }
    bool isExistSubcls( const std::string &rem ) const {
      return ExistSubcls != boost::none && ExistSubcls.get().find( rem ) != ExistSubcls.get().end();
    }
    void AddExistBaseLayer( const ASTRA::TCompLayerType &layer_type ) {
      if ( ExistBaseLayers == boost::none ) {
        ExistBaseLayers = std::set<ASTRA::TCompLayerType>();
      }
      ExistBaseLayers.get().insert( layer_type );
    }
    bool isExistBaseLayer( const ASTRA::TCompLayerType &layer_type ) const {
      return ExistBaseLayers != boost::none && ExistBaseLayers.get().find( layer_type ) != ExistBaseLayers.get().end();
    }
    bool canAddOccupy( TPlace* p ) {
      return ( !p->isplace || !p->visible || !placeIsFree( p ) );
    }

    int OccupySeatsCount() {
      if ( OccupySeats != boost::none ) {
        return (int)OccupySeats.get().size();
      }
      return 0;
    }

    int AddOccupySeat( int num, int x, int y ) {
      if ( OccupySeats == boost::none ) {
        OccupySeats = std::set<int>();
      }
      return *OccupySeats.get().insert( getSeatKey::get(num, x, y) ).first;
    }
    void RemoveOccupySeat( int num, int x, int y ) {
      if ( OccupySeats == boost::none ) {
        return;
      }
      OccupySeats.get().erase( getSeatKey::get(num, x, y) );
      return;
    }
    bool isExistsOccupySeat( int num, int x, int y ) const {
      return OccupySeats != boost::none && OccupySeats.get().find( getSeatKey::get(num, x, y) ) != OccupySeats.get().end();
    }
};

/* ����� �� ���� �� ���� �� ��室� � �� ��室� ��� ���� */
struct TZoneBL:public std::vector<TPlace*> {
  private:
    std::vector<TPlace> savePlaces;
    int salon_num;
  public:
    TZoneBL( int vsalon_num ) {
      salon_num = vsalon_num;
    }
    void setDisabled( const TTripInfo& fltInfo, TSalons* salons = nullptr );
    void rollback( TSalons* salons = nullptr );
    std::string toString();
};

struct TEmergencySeats: public std::map<int,boost::optional<TZoneBL> >{
private:
  TTripInfo fltInfo;
  boost::optional<bool> FDeniedEmergencySeats = boost::none;
  void getTuneSection( int point_id );
public:
  enum DisableMode { dlayers, dlrss };
  void setDisabledEmergencySeats( TSalons* Salons, const TPaxsCover &grpPaxs );
  void setDisabledEmergencySeats( TSalons* Salons, TPlaceList* placeList, const TPaxsCover &grpPaxs, DisableMode mode );
  void rollbackDisabledEmergencySeats( TSalons* salons = nullptr );
  TEmergencySeats( int point_id ) {
     getTuneSection( point_id );
  }
  bool deniedEmergencySection() {
   return FDeniedEmergencySeats.get();
  }
  bool in( int num, TPlace *seat ) {
    bool res = false;
    for ( TEmergencySeats::const_iterator izone=begin(); izone!=end(); izone++ ) {
      if ( izone->first != num ) {
        continue;
      }
      if ( izone->second != boost::none && !izone->second.get().empty() ) {
        res = std::find( izone->second.get().begin(), izone->second.get().end(), seat ) != izone->second.get().end();
      }
      break;
    }
    return res;
  }
};

struct TBabyZones: public std::map<int,boost::optional<TZoneBL> >{
  private:
    TTripInfo fltInfo;
    boost::optional<bool> FUseInfantSection = boost::none;
    void getTuneSection( int point_id );
    TBabyZones::iterator getZoneBL( SALONS2::TPlaceList *placeList, const TPlace &seat );
  public:
    enum DisableMode { dlayers, dlrss };
    TBabyZones( int point_id ) {
       getTuneSection( point_id );
    }
    bool useInfantSection() {
     return FUseInfantSection.get();
    }
    int getFirstSeatLine( SALONS2::TPlaceList *placeList, const TPlace &seat );
    void setDisabledBabySection( TSalons* Salons, const TPaxsCover &grpPaxs, const std::set<int> &pax_lists );
    void setDisabledBabySection( int point_id, TSalons* Salons, TPlaceList* placeList, const TPaxsCover &grpPaxs, const std::set<int> &pax_lists, DisableMode mode );
    void rollbackDisabledBabySection( TSalons* salons = nullptr );
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
     const TGrpStatusTypesRow &row1 = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status1 );
     const TGrpStatusTypesRow &row2 = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status2 );
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
enum TGetPass { gpPassenger, gpWaitList, gpTranzits, gpInfants };

class TGetPassFlags: public BitSet<TGetPass> {};
                                        //grp_status,TSalonPax
class TIntStatusSalonPassengers: public std::map<std::string,std::set<TSalonPax,ComparePassenger>,CompareGrpStatus >{};
                                  //class,grp_status,TSalonPax
class TIntClassSalonPassengers: public std::map<std::string,TIntStatusSalonPassengers,CompareClass >{};
                                           //point_arv
class TIntArvSalonPassengers: public std::map<int,TIntClassSalonPassengers,CompareArv >{};

class _TSalonPassengers: public TIntArvSalonPassengers {
  private:
    TWaitList status_wait_list;
  public:
    TIntArvSalonPassengers infants;
    int point_dep;
    bool pr_craft_lat;
    _TSalonPassengers( int vpoint_dep, bool vpr_craft_lat ) {
      point_dep = vpoint_dep;
      pr_craft_lat = vpr_craft_lat;
      status_wait_list = wlNotInit;
    };
    void clear() {
      TIntArvSalonPassengers::clear();
      infants.clear();
      status_wait_list = wlNotInit;
    }
    bool BuildWaitList( bool prSeatDescription, xmlNodePtr dataNode );
    void SetStatus( TWaitList status ) {
      status_wait_list = status;
    }
    bool isWaitList( );

    void dump_pax_map(const TIntArvSalonPassengers &pax_map);
    void dump();
};
                                        //point_dep
class TSalonPassengers: public std::map<int, _TSalonPassengers> {
  public:
    bool BuildWaitList( int point_dep, bool prSeatDescription, xmlNodePtr dataNode ) {
      TSalonPassengers::iterator ipasses = find( point_dep );
      if ( ipasses != end() ) {
        return ipasses->second.BuildWaitList( prSeatDescription, dataNode );
      }
      return false;
    }
    bool isWaitList( int point_dep ) {
      TSalonPassengers::iterator ipasses = find( point_dep );
      if ( ipasses != end() ) {
        return ipasses->second.isWaitList( );
      }
      return false;
    }
    void dump();
};
                                               //point_dep
struct TAutoSeat {
  int pax_id;
  TSalonPoint point;
  std::vector<TSeat> ranges;
  std::map<TSeat,std::string,CompareSeat> descrs;
  int seats;
  bool pr_down;
};

class TAutoSeats: public std::vector<TAutoSeat>
{
  public:
    void WritePaxSeats( int point_dep, int pax_id );
};

enum TDropLayers { clDropNotWeb,
                   clDropBlockCentLayers };
typedef BitSet<TDropLayers> TDropLayersFlags;

enum TCreateSalonProp { clDepOnlyTariff };

typedef BitSet<TCreateSalonProp> TCreateSalonPropFlags;

struct TPointInRoute {
  int point_id;
  bool inRoute;
  TPointDepNum depNum;
  TPointInRoute( int vpoint_id, bool vinRoute, TPointDepNum vdepNum ) {
    point_id = vpoint_id;
    inRoute = vinRoute;
    depNum = vdepNum;
  }
  TPointInRoute() {
    point_id = ASTRA::NoExists;
    inRoute = false;
    depNum = pdCurrent;
  }
};

class TPropsPoints: public std::vector<TPointInRoute> {
  public:
    TPropsPoints( const FilterRoutesProperty &filterRoutes, int point_dep, int point_arv );
    bool getPropRoute( int point_id, TPointInRoute &point );
    bool getLastPropRouteDeparture( TPointInRoute &point );
};

typedef std::pair<int,TPlace> TSalonSeat;

class CraftSeats: public std::vector<TPlaceList*> {
  public:
    void Clear();
    void read( DB::TQuery &Qry, const std::string &cls );
    int basechecksum( );
};

typedef std::map<bool, std::vector<std::string>> TBuildMap;

struct TSalonListReadParams {
  std::string filterClass;
  int tariff_pax_id;
  bool for_calc_waitlist;
  int prior_compon_props_point_id;
  bool read_all_notPax_layers;
  bool for_get_seat_no;
  TSalonListReadParams() {
    tariff_pax_id = ASTRA::NoExists;
    for_calc_waitlist = false;
    prior_compon_props_point_id = ASTRA::NoExists;
    read_all_notPax_layers = false;
    for_get_seat_no = false;
  }
};

class TSalonList {
  private:
    TFilterSets filterSets;
    int comp_id;
    bool pr_craft_lat;
    TRFISCMode RFISCMode;
    bool prSeatDescription;
    bool useSeatsCache;
    void Clear();
    inline bool findSeat( std::map<int,TPlaceList*> &salons, TPlaceList** placelist,
                          const TSalonPoint &point_s );
    void ReadRemarks( DB::TQuery &Qry, FilterRoutesProperty &filterSegments,
                      int prior_compon_props_point_id );
    void ReadLayers( DB::TQuery &Qry, FilterRoutesProperty &filterSegments,
                     TFilterLayers &filterLayers, TPaxList &pax_list,
                     const TSalonListReadParams &params,
                     bool is_tlg_ranges=false);
    void ReadTariff( DB::TQuery &Qry, FilterRoutesProperty &filterSegments,
                     int prior_compon_props_point_id );
    void ReadRFISCColors( DB::TQuery &Qry, FilterRoutesProperty &filterRoutes,
                          int prior_compon_props_point_id );
    void SetRFISC( int point_id, TSeatTariffMap &tariffMap );
    //void AddRFISCRemarks( int key, TSeatTariffMap &tariffMap );
    //void DropRFISCRemarks( TSeatTariffMap &tariffMap );
    //void SetTariffsByRFICSColor( int point_dep, TSeatTariffMap &tariffMap, bool setPassengerTariffs );
    void ReadPaxs( DB::TQuery &Qry, TPaxList &pax_list );
    void ReadCrsPaxs( DB::TQuery &Qry, TPaxList &pax_list,
                      int pax_id, std::string &airp_arv );
    void validateLayersSeats( bool read_all_notPax_layers );
    void CommitLayers(); // ��࠭���� ⮫쪮 �������� ᫮�� � pax_lists[ point_id, pax_id ].save_layers
    void RollbackLayers(); // ����⠭������� � pax_lists[ point_id, pax_id ].layers �� pax_lists[ point_id, pax_id ].save_layers
  public:
    SALONS2::CraftSeats _seats; //private!!!
    std::map<int,TPaxList> pax_lists;
    bool getPax( int point_dep, int pax_id, TSalonPax& salonPax ) const {
      salonPax = TSalonPax();
      std::map<int,TPaxList>::const_iterator pxs = pax_lists.find( point_dep );
      if ( pxs == pax_lists.end() )
        return false;
      TPaxList::const_iterator ipax = pxs->second.find( pax_id );
      if ( ipax == pxs->second.end() )
        return false;
      salonPax = ipax->second;
      return true;
    }
    bool isRefused( int point_dep, int pax_id ) const;
    bool isCraftLat() const {
      return pr_craft_lat;
    }
    void setCraftLang( bool pr_craft_lat ) {
      this->pr_craft_lat = pr_craft_lat;
    }
    TRFISCMode getRFISCMode() const {
      return RFISCMode;
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
    std::string getAirline() const {
      return filterSets.filterRoutes.getAirline();
    }
    TTripInfo& getfltInfo() {
      return filterSets.filterRoutes.getfltInfo();
    }
    void getEditableFlightLayers( BitSet<ASTRA::TCompLayerType> &editabeLayers, bool isLibraRequest = false );
    bool getSeatDescription() {
      return prSeatDescription;
    }
    TSalonList( bool vuseSeatsCache = false ) {
      pr_craft_lat = false;
      prSeatDescription = false;
      comp_id = ASTRA::NoExists;
      RFISCMode = rTariff;
      useSeatsCache = vuseSeatsCache;
    }
    ~TSalonList() {
      Clear();
    }
    void ReadCompon( int vcomp_id, int point_id );
    void ReadFlight( const TFilterRoutesSets &filterRoutesSets,
                     const std::string &filterClass,
                     int tariff_pax_id,
                     bool for_calc_waitlist = false,  //!!!
                     int prior_compon_props_point_id = ASTRA::NoExists );
    void ReadFlight( const TFilterRoutesSets &filterRoutesSets,
                     const TSalonListReadParams &params );
//    void ReadSeats( TQuery &Qry, const std::string &FilterClass );
    void Build( TBuildMap &seats);
    void Build( xmlNodePtr salonsNode );
    void Parse( boost::optional<TTripInfo> fltInfo, const std::string &airline, xmlNodePtr salonsNode );
    void WriteFlight( int vpoint_id, bool saveContructivePlaces, bool isLibraRequest = false );
    void WriteCompon( int &vcomp_id, const TComponSets &componSets, bool saveContructivePlaces );
    void convertSeatTariffs( TPlace &iseat, bool pr_departure_tariff_only, int point_dep, int point_arv ) const;
    bool CreateSalonsForAutoSeats( TSalons &salons,
                                   TFilterRoutesSets &filterRoutes,
                                   TCreateSalonPropFlags propFlags,
                                   const std::vector<ASTRA::TCompLayerType> &grp_layers,
                                   TDropLayersFlags &dropLayersFlags ) {
      TPaxsCover paxs;
      return CreateSalonsForAutoSeats<TPaxCover>( salons,
                                                  filterRoutes,
                                                  propFlags,
                                                  grp_layers,
                                                  paxs,
                                                  dropLayersFlags );
    }
    template <typename T1>
    bool CreateSalonsForAutoSeats(TSalons &Salons,
                                   TFilterRoutesSets &filterRoutes,
                                   TCreateSalonPropFlags propFlags,
                                   const std::vector<ASTRA::TCompLayerType> &grp_layers,
                                   const std::vector<T1> &paxs,
                                   TDropLayersFlags &dropLayersFlags ) {
      TPaxsCover paxs1;
      for ( typename std::vector<T1>::const_iterator ip=paxs.begin(); ip!=paxs.end(); ip++ ) {
         paxs1.push_back( TPaxCover( ip->crs_pax_id, ip->pax_id ) );
      }
      return CreateSalonsForAutoSeats( Salons,
                                       filterRoutes,
                                       propFlags,
                                       grp_layers,
                                       paxs1,
                                       dropLayersFlags );
    }
    bool CreateSalonsForAutoSeats( TSalons &Salons,
                                   TFilterRoutesSets &filterRoutes,
                                   TCreateSalonPropFlags propFlags,
                                   const std::vector<ASTRA::TCompLayerType> &grp_layers,
                                   const TPaxsCover &paxs,
                                   TDropLayersFlags &dropLayersFlags );
    void JumpToLeg( const FilterRoutesProperty &filterRoutesNew );
    void JumpToLeg( const TFilterRoutesSets &routesSets );
    void getPassengers( TSalonPassengers &passengers, const TGetPassFlags &flags );
    bool getPaxLayer( int point_dep, int pax_id, ASTRA::TCompLayerType layer_type,
                      std::set<TPlace*,CompareSeats> &seats ) const;
    bool getPaxLayer( int point_dep, int pax_id,
                      TLayerPrioritySeat &seatLayer,
                      std::set<TPlace*,CompareSeats> &seats ) const;
    bool check_waitlist_alarm_on_tranzit_routes( const TAutoSeats &autoSeats );
    void check_waitlist_alarm_on_tranzit_routes( const std::set<int> &paxs_external_logged );

    void getSectionInfo( std::vector<TSectionInfo> &CompSections, const TGetPassFlags &flags );
    void getSectionInfo( TSectionInfo &sectionInfo, const TGetPassFlags &flags );
    void getLayerPlacesCompSection( TCompSection &compSection,
                                    std::map<ASTRA::TCompLayerType, TPlaces> &uselayers_places,
                                    int &seats_count );
};

class TSalonChanges: public std::vector<TSalonSeat> {
  private:
    TRFISCMode _RFISCMode;
    int _point_dep;
  public:
    TSalonChanges( int point_dep, TRFISCMode RFISCMode ) {
      _point_dep = point_dep;
      _RFISCMode = RFISCMode;
    }
    TSalonChanges( ) {
      _point_dep = ASTRA::NoExists;
      _RFISCMode = rTariff;
    }
    TRFISCMode RFISCMode() const {
      return _RFISCMode;
    }
    void setRFISCMode( TRFISCMode RFISCMode ) {
      _RFISCMode = RFISCMode;
    }
    //�᫨ false - � �� ᬮ��� �ࠢ����, �ॡ���� ����⪠ �ᥣ� ᠫ���
    TSalonChanges& get( const TSalonList& oldSalon,
                        const TSalonList& newSalon );
    void toXML(xmlNodePtr dataNode, bool pr_lat_seat,
               const std::optional<std::map<int,TPaxList> >& pax_list );
};


class TAdjustmentRows: public adjustmentIndexRow {
  public:
    const TAdjustmentRows &get( const TSalonList &salonList );
};


  class TSelfCkinSalonTariff {
    public:
      void setTariffMap( int point_id,
                         TSeatTariffMap &tariffMap );
      void setTariffMap( const std::string &airline,
                         const std::string &airp_dep,
                         const std::string &airp_arv,
                         const std::string &craft,
                         TSeatTariffMap &tariffMap );
  };

    void check_waitlist_alarm_on_tranzit_routes( int point_dep, const std::set<int> &paxs_external_logged,
                                                 const std::string &whence );
    void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm,
                                                 const std::set<int> &paxs_external_logged,
                                                 const std::string& whence );
    void check_waitlist_alarm_on_tranzit_routes( int point_dep,
                                                 const std::string& whence );
    void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm ,
                                                 const std::string& whence );
    void WritePaxSeats( int point_dep, int pax_id, const TSeatRanges &ranges );


//typedef std::map<TPlace*, std::vector<TPlaceLayer> > TPlacePaxs; // ���஢�� �� �ਮ��⠬ ᫮��

  void LoadCompRemarksPriority( std::map<std::string, int> &rems );
  bool Checkin( int pax_id );
  bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull );
  void GetTripParams( int trip_id, xmlNodePtr dataNode );
  void GetCompParams( int comp_id, xmlNodePtr dataNode );
  int GetCompId( const std::string craft, const std::string bort, const std::string airline,
                 std::vector<std::string> airps,  int f, int c, int y );
  bool getSalonChanges( const std::vector<TPlaceList*> &list1, bool pr_craft_lat1,
                        const std::vector<TPlaceList*> &list2, bool pr_craft_lat2,
                        TRFISCMode RFISCMode,
                        TSalonChanges &seats );
  void getSalonChanges( const TSalonList &salonList,
                        int tariff_pax_id,
                        TSalonChanges &seats );
  void getSalonChanges( TSalons &OldSalons, TRFISCMode RFISCMode, TSalonChanges &seats );
  void BuildSalonChanges( xmlNodePtr dataNode, const TSalonChanges &seats );
  void BuildSalonChanges( xmlNodePtr dataNode, int point_dep, const TSalonChanges &seats,
                          bool with_pax, const std::map<int,SALONS2::TPaxList> &pax_lists );
  void salonChangesToText( int point_id,
                           const std::vector<TPlaceList*> &oldlist, bool oldpr_craft_lat,
                           const std::vector<TPlaceList*> &newlist, bool newpr_craft_lat,
                           const BitSet<ASTRA::TCompLayerType> &editabeLayers,
                           LEvntPrms &params, bool pr_set_base );
  bool ChangeCfg( const std::vector<TPlaceList*> &list1,
                  const std::vector<TPlaceList*> &list2,
                  const TCompareCompsFlags &compareFlags );
  bool ChangeCfg( const std::vector<TPlaceList*> &list1,
                  const std::vector<TPlaceList*> &list2 );
  bool IsMiscSet( int point_id, int misc_type );
  bool compatibleLayer( ASTRA::TCompLayerType layer_type );
  void verifyValidRem( const std::string &className, const std::string &remCode );
  bool isBaseLayer( ASTRA::TCompLayerType layer_type, bool isComponCraft );
  int getCrsPaxPointArv( int crs_pax_id, int point_id_spp );
  void CreateSalonMenu( const TTripInfo &fltInfo, xmlNodePtr salonsNode );
  bool isFreeSeating( int point_id );
  bool isEmptySalons( int point_id );
  void DeleteSalons( int point_id );
  bool isUserProtectLayer( ASTRA::TCompLayerType layer_type );
  void resetLayers( int point_id, ASTRA::TCompLayerType layer_type,
                    const TSeatRanges &seatRanges, const std::string &reason,
                    bool isLibraRequest = false );
  bool selfckin_client();
/*  void addAirlineSelfCkinTariff( const std::string &airline, TSeatTariffMap &tariffMap );
                                 std::string getPointAirp(int point_id);*/
  typedef std::map<int,std::set<std::string> > TSalonDesrcs;
  void getSalonDesrcs( int point_id, TSalonDesrcs &descrs );
  void getPaxSeatsWL( int point_id, std::map< bool,std::map < int,TSeatRanges > > &seats );
  bool haveConstructiveCompon( int id, TReadStyle style );
  bool isConstructivePlace( const std::string &elem_type );

  template <typename T1>
  bool isOwnerFreePlace( int pax_id, const std::vector<T1> &paxs )
  {
    bool res = false;
    for ( typename std::vector<T1>::const_iterator i=paxs.begin(); i!=paxs.end(); i++ ) {
      if ( i->pax_id != ASTRA::NoExists )
          continue;
      if ( i->crs_pax_id == pax_id ) {
          res = true;
          break;
      }
    }
    return res;
  }

  template <class T1>
  bool isOwnerPlace( int pax_id, const std::vector<T1> &paxs )
  {
    bool res = false;
    for ( typename std::vector<T1>::const_iterator i=paxs.begin(); i!=paxs.end(); i++ ) {
      if ( i->pax_id != ASTRA::NoExists && pax_id == i->pax_id ) {
          res = true;
          break;
      }
    }
    return res;
  }

  struct CompCheckSum {
    int total_crc32;
    int base_crc32; //��� alarm, �� ���뢠�� ⨯ ����, � ⮫쪮 "���� ��� ��ᠤ�� ��� ���"
    CompCheckSum( int _total_crc32, int _base_crc32 ) {
       total_crc32 = _total_crc32;
       base_crc32 = _base_crc32;
    }
    static int calcCheckSum( const std::string& buf );
    static CompCheckSum calcFromDB( int point_id );
    static CompCheckSum keyFromDB( int point_id );
  };
  bool isComponSeatsNoChanges( const TTripInfo &info );
 } // END namespace SALONS2

 int testsalons(int argc,char **argv);

#endif /*_SALONS2_H_*/
