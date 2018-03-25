#include <stdlib.h>
#include <boost/crc.hpp>
#include "salons.h"
#include "date_time.h"
#include "misc.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_date_time.h"
#include "oralib.h"
#include "seats.h"
#include "images.h"
#include "convert.h"
#include "astra_misc.h"
#include "astra_locale.h"
#include "base_tables.h"
#include "passenger.h"
#include "term_version.h"
#include "alarms.h"
#include "points.h"
#include "rozysk.h"
#include "qrys.h"
#include "etick.h"
#include "counters.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace BASIC_SALONS;

namespace SALONS2
{

int getCRC_Comp( int point_id );

bool selfckin_client() {
  return ( TReqInfo::Instance()->client_type == ctWeb ||
           TReqInfo::Instance()->client_type == ctKiosk ||
           TReqInfo::Instance()->client_type == ctMobile );
}

bool isREM_SUBCLS( std::string rem )
{
    return ( rem.size() == 4 && rem.substr(1,3) == "CLS" &&
               *(rem.c_str()) >= 'A' && *(rem.c_str()) <= 'Z' );
}

void constructiveElemTypes( std::vector<std::string> &elem_types ) {
  elem_types.clear();
  elem_types.push_back( "B" );
  elem_types.push_back( "R" );
  elem_types.push_back( "L" );
  elem_types.push_back( "W" );
}

bool isConstructivePlace( const std::string &elem_type, const std::vector<std::string> &elem_types ) {
  return ( std::find( elem_types.begin(), elem_types.end(), elem_type ) != elem_types.end() );
}

bool isConstructivePlace( const std::string &elem_type ) {
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  return isConstructivePlace( elem_type, elem_types );
}

bool haveConstructiveCompon( int id, TReadStyle style ) {
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  TQuery Qry( &OraSession );
  if ( style == rComponSalons ) {
    Qry.SQLText =
      "SELECT 1 FROM comp_elems WHERE comp_id=:comp_id AND elem_type IN " + GetSQLEnum( elem_types );
    Qry.CreateVariable( "comp_id", otInteger, id );
  }
  else {
      Qry.SQLText =
        "SELECT 1 FROM trip_comp_elems WHERE point_id=:point_id AND elem_type IN " + GetSQLEnum( elem_types );
      Qry.CreateVariable( "point_id", otInteger, id );
  }
  Qry.Execute();
  return !Qry.Eof;
}

bool forBuild( const TPlace &place, const std::vector<std::string> &elem_types ) {
  return ( !(!place.visible ||
             (!TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION ) &&
              isConstructivePlace( place.elem_type, elem_types ) ) ) );
}

void TSelfCkinSalonTariff::setTariffMap( int point_id,
                                         TSeatTariffMap &tariffMap ) {
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,craft,scd_out "
    " FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    return;
  }
  TTripInfo tripInfo( Qry );
  TTripRoute routes;
  if ( routes.GetRouteAfter( ASTRA::NoExists,
                             point_id,
                             trtNotCurrent,
                             trtNotCancelled ) &&
       !routes.empty() ) {
    setTariffMap( tripInfo.airline, tripInfo.airp, routes.begin()->airp, tripInfo.craft, tariffMap  );
  }
}
void TSelfCkinSalonTariff::setTariffMap( const std::string &airline,
                                         const std::string &airp_dep,
                                         const std::string &airp_arv,
                                         const std::string &craft,
                                         TSeatTariffMap &tariffMap ) {
  if ( !selfckin_client() ||
       tariffMap.empty() ||
       tariffMap.status() != TSeatTariffMap::stUseRFISC ||
       airline.empty() ||
       airp_dep.empty() ||
       airp_arv.empty()
        ) {
    return;
  }
  ProgTrace( TRACE5, "TSelfCkinSalonTariff::setTariffMapp: airline=%s, airp_dep=%s, airp_arv=%s, craft=%s",
             airline.c_str(), airp_dep.c_str(), airp_arv.c_str(), craft.c_str() );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT rfisc,rate,rate_cur, "
    "       DECODE(airp_dep,:airp_dep,100,NULL,50,0) + "
    "       DECODE(airp_arv,:airp_arv,100,NULL,50,0) + "
    "       DECODE(craft,:craft,10,NULL,5,0) as priority "
    " FROM rfisc_rates_self_ckin "
    " WHERE airline=:airline AND "
    "       rfisc=:rfisc AND "
    "       (airp_dep=:airp_dep OR airp_dep IS NULL) AND "
    "       (airp_arv=:airp_arv OR airp_arv IS NULL) AND "
    "       (craft=:craft OR craft IS NULL) "
    "ORDER BY priority DESC";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "airp_dep", otString, airp_dep );
  Qry.CreateVariable( "airp_arv", otString, airp_arv );
  Qry.CreateVariable( "craft", otString, craft );
  Qry.DeclareVariable( "rfisc", otString );
  for ( TSeatTariffMapType::iterator itariff=tariffMap.begin(); itariff!=tariffMap.end(); itariff++ ) {
    Qry.SetVariable( "rfisc", itariff->second.code );
    Qry.Execute();
    if ( !Qry.Eof ) {
      itariff->second.rate = Qry.FieldAsFloat("rate");
      itariff->second.currency_id = Qry.FieldAsString("rate_cur");
    }
    else {
      itariff->second.clear();
      itariff->second.rate = INT_MAX;
    }
  }
}

void TSeatTariffMap::get(TQuery &Qry, const std::string &traceDetail)
{
  clear();

  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TRFISC rfisc;
    rfisc.color=Qry.FieldAsString("rate_color");
    rfisc.rate=Qry.FieldAsFloat("rate");
    rfisc.currency_id=Qry.FieldAsString("rate_cur");
    rfisc.code=Qry.FieldAsString("rfisc");

    pair<TSeatTariffMapType::iterator, bool> i=insert(make_pair(rfisc.color, rfisc));
    if (!i.second && rfisc!=i.first->second)
    {
      ProgError(STDLOG, "TSeatTariffMap::get: color=%s duplicated (%s)", rfisc.color.c_str(), traceDetail.c_str());
      trace(TRACE5);
      if (rfisc.rate<i.first->second.rate)
      {
        i.first->second=rfisc;
        trace(TRACE5);
      };
    };
  };
}

void TSeatTariffMap::get_rfisc_colors(const std::string &airline_oper)
{
  get_rfisc_colors_internal(airline_oper);
  _status=empty()?stNotRFISC:stUseRFISC;
}

bool TSeatTariffMap::is_rfisc_applied(const std::string &airline_oper)
{
  get_rfisc_colors(airline_oper);
  return _status==stUseRFISC;
}

void TSeatTariffMap::get_rfisc_colors_internal(const std::string &airline_oper)
{
  clear();

  _potential_queries++;
  std::map<std::string/*airline_oper*/, TSeatTariffMapType>::iterator iRFISColors=rfisc_colors.find(airline_oper);
  if (iRFISColors==rfisc_colors.end())
  {
    _real_queries++;
    TCachedQuery Qry("SELECT rate_color, 0 AS rate, NULL AS rate_cur, code AS rfisc "
                     "FROM rfisc_comp_props "
                     "WHERE airline=:airline",
                     QParams() << QParam("airline", otString, airline_oper));

    ostringstream s;
    s << "airline_oper=" << airline_oper;
    get(Qry.get(), s.str());

    rfisc_colors.insert(make_pair(airline_oper, *this));
  }
  else static_cast<TSeatTariffMapType&>(*this)=iRFISColors->second;
}

void TSeatTariffMap::get(const TAdvTripInfo &operFlt, const TTripInfo &markFlt, const CheckIn::TPaxTknItem &tkn)
{
  clear();

  if (is_rfisc_applied(operFlt.airline))
  {
    //�������� ������ ���-�� ���� ����������� RFISC
    if (operFlt.airline!=markFlt.airline)
    {
      _status=stNotOperating;
      return;
    }

    if (!tkn.validET())
    {
      _status=stNotET;
      return;
    }

    _potential_queries++;
    _real_queries++;
    TETickItem etick;
    etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false);
    if (etick.empty())
    {
      _status=stUnknownETDisp;
      return;
    }

    _potential_queries++;
    _real_queries++;
    TCachedQuery Qry(
      "SELECT rfisc_comp_props.rate_color, rfisc_rates.rate, rfisc_rates.rate_cur, rfisc_rates.rfisc "
      "FROM brand_fares, rfisc_rates, rfisc_comp_props "
      "WHERE brand_fares.airline=rfisc_rates.airline AND "
      "      brand_fares.brand=rfisc_rates.brand AND "
      "      rfisc_rates.airline=rfisc_comp_props.airline AND "
      "      rfisc_rates.rfisc=rfisc_comp_props.code AND "
      "      brand_fares.airline=:airline AND "
      "      :fare_basis LIKE REPLACE(brand_fares.fare_basis,'*','%') AND "
      "      :issue_date>=rfisc_rates.sale_first_date AND "
      "      (rfisc_rates.sale_last_date IS NULL OR :issue_date<rfisc_rates.sale_last_date)",
      QParams() << QParam("airline", otString, operFlt.airline)
                << QParam("fare_basis", otString, etick.fare_basis)
                << QParam("issue_date", otDate, etick.issue_date)
    );

    ostringstream s;
    s << "status=stUseRFISC, "
      << "airline=" << operFlt.airline << ", "
      << "fare_basis=" << etick.fare_basis << ", "
      << "issue_date=" << DateTimeToStr(etick.issue_date, "dd.mm.yy");
    get(Qry.get(), s.str());

    if (empty()) get_rfisc_colors_internal(operFlt.airline);

    _status=stUseRFISC;
  }
  else
  {
    _potential_queries++;
    std::map<int/*point_id_oper*/, TSeatTariffMapType>::const_iterator iTariffMap=tariff_map.find(operFlt.point_id);
    if (iTariffMap==tariff_map.end())
    {
      _real_queries++;
      TCachedQuery Qry(
        "SELECT DISTINCT color AS rate_color, rate, rate_cur, NULL AS rfisc "
        "FROM trip_comp_rates "
        "WHERE point_id=:point_id",
        QParams() << QParam("point_id", otInteger, operFlt.point_id)
      );

      ostringstream s;
      s << "status=stNotRFISC, "
        << "point_id=" << operFlt.point_id;
      get(Qry.get(), s.str());

      tariff_map.insert(make_pair(operFlt.point_id, *this));
    }
    else static_cast<TSeatTariffMapType&>(*this)=iTariffMap->second;

    _status=stNotRFISC;
  }
}

void TSeatTariffMap::get(const int point_id_oper,
                         const int point_id_mark,
                         const int grp_id,
                         const int pax_id)
{
  clear();

  _potential_queries++;
  std::map<int/*point_id_oper*/, TAdvTripInfo>::const_iterator iOper=oper_flts.find(point_id_oper);
  if (iOper==oper_flts.end())
  {
    _real_queries++;
    TAdvTripInfo operFlt;
    if (!operFlt.getByPointId(point_id_oper))
    {
      _status=stNotFound;
      return;
    }
    iOper=oper_flts.insert(make_pair(point_id_oper, operFlt)).first;
  }
  if (iOper==oper_flts.end())
    throw EXCEPTIONS::Exception("TSeatTariffMap::get: iOper==oper_flts.end()!");

  _potential_queries++;
  std::map<int/*point_id_mark*/, TTripInfo>::const_iterator iMark=mark_flts.find(point_id_mark);
  if (iMark==mark_flts.end())
  {
    _real_queries++;
    TGrpMktFlight grpMarkFlt;
    if (!grpMarkFlt.getByGrpId(grp_id))
    {
      _status=stNotFound;
      return;
    }
    TTripInfo markFlt;
    markFlt.Init(grpMarkFlt);
    iMark=mark_flts.insert(make_pair(point_id_mark, markFlt)).first;
  }
  if (iMark==mark_flts.end())
    throw EXCEPTIONS::Exception("TSeatTariffMap::get: iMark==mark_flts.end()!");

  CheckIn::TPaxTknItem tkn;
  if (is_rfisc_applied(iOper->second.airline))
  {
    _potential_queries++;
    _real_queries++;
    CheckIn::LoadPaxTkn(pax_id, tkn);
  };

  get(iOper->second, iMark->second, tkn);
}

void TSeatTariffMap::get(const int pax_id)
{
  clear();

  _potential_queries++;
  _real_queries++;
  TCachedQuery Qry("SELECT pax_grp.point_dep AS point_id_oper, pax_grp.point_id_mark, pax_grp.grp_id "
                   "FROM pax_grp, pax "
                   "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id",
                   QParams() << QParam("pax_id", otInteger, pax_id));
  Qry.get().Execute();
  if (Qry.get().Eof)
  {
    _status=stNotFound;
    return;
  }
  int point_id_oper=Qry.get().FieldAsInteger("point_id_oper");
  int point_id_mark=Qry.get().FieldAsInteger("point_id_mark");
  int grp_id=Qry.get().FieldAsInteger("grp_id");

  get(point_id_oper, point_id_mark, grp_id, pax_id);
}

void TSeatTariffMap::trace( TRACE_SIGNATURE ) const
{
  ProgTrace(TRACE_PARAMS, "============ TSeatTariffMap ============");
  for(TSeatTariffMap::const_iterator i=begin(); i!=end(); ++i)
  {
    ostringstream s;
    s << right << setw(10) << i->second.color << ": "
      << right << setw(12) << i->second.rateView()
      << left << setw(3) << i->second.currency_id << " " << i->second.code;
    ProgTrace(TRACE_PARAMS, "%s", s.str().c_str());
  };
  string st;
  switch(_status)
  {
    case stNotFound:      st="stNotFound";      break;
    case stNotOperating:  st="stNotOperating";  break;
    case stNotET:         st="stNotET";         break;
    case stUnknownETDisp: st="stUnknownETDisp"; break;
    case stNotRFISC:      st="stNotRFISC";  break;
    case stUseRFISC:      st="stUseRFISC";  break;
  }
  ProgTrace(TRACE_PARAMS, "status=%s, potential_queries=%d, real_queries=%d",
                          st.c_str(), _potential_queries, _real_queries);
}



const int REM_VIP_F = 1;
const int REM_VIP_C = 1;
const int REM_VIP_Y = 3;




TDrawPropInfo getDrawProps( TDrawPropsType proptype )
{
  TDrawPropInfo res;
  if ( proptype == dpInfantWoSeats ) {
    res.figure = "framework";
    res.color = "$0005A5FA";
    res.name = "����� � ������楬";
  }
  if ( proptype == dpTranzitSeats ) {
    res.figure = "lurect";
    res.color = "$00800000";
    res.name = "�࠭���� ���ᠦ��";
  }
  return res;
};

struct CompRoute {
  int point_id;
  string airline;
  string airp;
  string craft;
  string bort;
  bool pr_reg;
  bool pr_alarm;
  bool auto_comp_chg;
  bool inRoutes;
  CompRoute() {
     point_id = NoExists;
     pr_alarm = false;
     pr_reg = false;
     auto_comp_chg = false;
     inRoutes = false;
  };
};

typedef vector<CompRoute> TCompsRoutes;

static std::map<std::string,std::string> SUBCLS_REMS;
void verifyValidRem( const std::string &className, const std::string &remCode );
void check_diffcomp_alarm( TCompsRoutes &routes );

std::string TSeatLayer::toString() const
{
  string res;
  res += "(p_id=" + IntToString( point_id );
  res += ",p_dep=";
  if ( point_dep != ASTRA::NoExists )
    res += IntToString( point_dep );
  res += ",p_arv=";
  if ( point_arv != ASTRA::NoExists )
    res += IntToString( point_arv );
  if ( pax_id != NoExists )
    res += ",pax_id=" + IntToString( pax_id );
  if ( crs_pax_id != NoExists )
    res += ",crs_id=" + IntToString( crs_pax_id );
  res += string(",type=") + EncodeCompLayerType( layer_type );
  res += ",time=" + DateTimeToStr( time_create );
  res += ",in=" + IntToString( inRoute ) + ")";
  return res;
}

void getMenuBaseLayers( std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers, bool isTripCraft )
{
  menuLayers.clear();
  for ( int ilayer=0; ilayer<ASTRA::cltTypeNum; ilayer++ ) {
    menuLayers[ (ASTRA::TCompLayerType)ilayer ].editable = false;
    menuLayers[ (ASTRA::TCompLayerType)ilayer ].notfree = false;
  }
    menuLayers[ cltUncomfort ].editable = true;
  menuLayers[ cltUncomfort ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltUncomfort, TReqInfo::Instance()->desk.lang );
    menuLayers[ cltSmoke ].editable = true;
  menuLayers[ cltSmoke ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltSmoke, TReqInfo::Instance()->desk.lang );
    menuLayers[ cltDisable ].editable = true;
    menuLayers[ cltDisable ].notfree = isTripCraft;
  menuLayers[ cltDisable ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltDisable, TReqInfo::Instance()->desk.lang );
  menuLayers[ cltProtect ].editable = true;
  menuLayers[ cltProtect ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltProtect, TReqInfo::Instance()->desk.lang );
}

bool isEditableMenuLayers( ASTRA::TCompLayerType layer, const std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers )
{
  std::map<ASTRA::TCompLayerType,TMenuLayer>::const_iterator iMenuLayer = menuLayers.find( layer );
  return ( iMenuLayer != menuLayers.end() && iMenuLayer->second.editable );
}

void getMenuLayers( bool isTripCraft,
                    TFilterLayers &FilterLayers,
                    std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers )
{
//!log  ProgTrace( TRACE5, "getMenuLayers: isTripCraft=%d", isTripCraft );
  menuLayers.clear();
  for ( int ilayer=0; ilayer<ASTRA::cltTypeNum; ilayer++ ) {
    menuLayers[ (ASTRA::TCompLayerType)ilayer ].editable = false;
    menuLayers[ (ASTRA::TCompLayerType)ilayer ].notfree = false;
  }
  getMenuBaseLayers( menuLayers, isTripCraft );

  if ( isTripCraft ) {
    menuLayers[ cltBlockCent ].editable = true;
    menuLayers[ cltBlockCent ].notfree = true;
    if ( FilterLayers.isFlag( cltProtTrzt ) )
      menuLayers[ cltProtTrzt ].editable = true;
    else
        menuLayers[ cltProtTrzt ].notfree = true;
    if ( FilterLayers.isFlag( cltBlockTrzt ) )
      menuLayers[ cltBlockTrzt ].editable = true;
    else
      menuLayers[ cltBlockTrzt ].notfree = true;
    menuLayers[ cltTranzit ].notfree = true;
    menuLayers[ cltCheckin ].notfree = true;
    menuLayers[ cltTCheckin ].notfree = true;
    menuLayers[ cltGoShow ].notfree = true;
    menuLayers[ cltSOMTrzt ].notfree = true;
    menuLayers[ cltPRLTrzt ].notfree = true;
    // �� �⮡ࠧ��� � help Ctrl+F4 - ����� �� ������
    menuLayers[ cltBlockCent ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltBlockCent, TReqInfo::Instance()->desk.lang );
    if ( FilterLayers.isFlag( cltBlockCent ) )
      menuLayers[ cltBlockCent ].func_key = "Shift+F2";
    if ( FilterLayers.isFlag( cltTranzit ) ||
         FilterLayers.isFlag( cltSOMTrzt ) ||
         FilterLayers.isFlag( cltPRLTrzt ) ) {
      menuLayers[ cltBlockTrzt ].name_view = AstraLocale::getLocaleText("�࠭���");
    }
    menuLayers[ cltCheckin ].name_view = AstraLocale::getLocaleText("���������");
    if ( FilterLayers.isFlag( cltProtTrzt ) ) {
        menuLayers[ cltProtTrzt ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltProtTrzt, TReqInfo::Instance()->desk.lang );
      menuLayers[ cltProtTrzt ].func_key = "Shift+F3";
    }
    if ( FilterLayers.isFlag( cltBlockTrzt ) ) {
      menuLayers[ cltBlockTrzt ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltBlockTrzt, TReqInfo::Instance()->desk.lang );
      menuLayers[ cltBlockTrzt ].func_key = "Shift+F3";
    }
    menuLayers[ cltPNLCkin ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltPNLCkin, TReqInfo::Instance()->desk.lang );
    menuLayers[ cltProtCkin ].name_view = BASIC_SALONS::TCompLayerTypes::Instance()->getName( cltProtCkin, TReqInfo::Instance()->desk.lang );

    if ( FilterLayers.isFlag( cltProtBeforePay ) ||
         FilterLayers.isFlag( cltProtAfterPay ) ||
         FilterLayers.isFlag( cltPNLBeforePay ) ||
         FilterLayers.isFlag( cltProtSelfCkin ) )
      menuLayers[ cltProtBeforePay ].name_view = AstraLocale::getLocaleText("����ࢨ஢���� ���� ���ᠦ��");

    if ( FilterLayers.isFlag( cltProtect ) )
      menuLayers[ cltProtect ].func_key = "Shift+F4";
    if ( FilterLayers.isFlag( cltUncomfort ) )
      menuLayers[ cltUncomfort ].func_key = "Shift+F5";
    if ( FilterLayers.isFlag( cltSmoke ) )
      menuLayers[ cltSmoke ].func_key = "Shift+F6";
    if ( FilterLayers.isFlag( cltDisable ) )
      menuLayers[ cltDisable ].func_key = "Shift+F1";
  } //end isTripCraft
}

void buildMenuLayers( bool isTripCraft,
                      const std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers,
                      const BitSet<TDrawPropsType> &props,
                      xmlNodePtr salonsNode, int point_id )
{
  int max_priority = -1;
  int id = 0;
  xmlNodePtr editNode = GetNode( "layers_prop", salonsNode );
  if ( editNode ) {
    ProgTrace( TRACE5, "buildMenuLayers - recreate isTripCraft=%d", isTripCraft );
    xmlUnlinkNode( editNode );
    xmlFreeNode( editNode );
  }
  else {
    ProgTrace( TRACE5, "buildMenuLayers - create isTripCraft=%d", isTripCraft );
  }
  editNode = NewTextChild( salonsNode, "layers_prop" );
  TReqInfo *r = TReqInfo::Instance();
  for( map<ASTRA::TCompLayerType,TMenuLayer>::const_iterator ilayer=menuLayers.begin(); ilayer!=menuLayers.end(); ilayer++ ) {
    if ( !compatibleLayer( ilayer->first ) )
      continue;
    if ( !isTripCraft &&
         !isBaseLayer( ilayer->first, !isTripCraft ) )
      continue;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( !BASIC_SALONS::TCompLayerTypes::Instance()->getElem( ilayer->first, layer_elem ) )
      continue;
    xmlNodePtr n = NewTextChild( editNode, "layer", layer_elem.getCode( ) );
    SetProp( n, "id", id );
    SetProp( n, "name", BASIC_SALONS::TCompLayerTypes::Instance()->getName( ilayer->first, TReqInfo::Instance()->desk.lang ) );
    SetProp( n, "priority", layer_elem.getPriority() );
    if ( max_priority < layer_elem.getPriority() )
      max_priority = layer_elem.getPriority();
    if ( ilayer->second.editable ) { // ���� �� �஢���� �� �ࠢ� ।���஢���� ⮣� ��� ����� ᫮�
      bool pr_edit = true;
      if ( (ilayer->first == cltBlockTrzt || ilayer->first == cltProtTrzt )&&
           !r->user.access.check_profile(point_id, 430) )
        pr_edit = false;
      if ( ilayer->first == cltBlockCent &&
           !r->user.access.check_profile(point_id, 420) )
        pr_edit = false;
      if ( (ilayer->first == cltUncomfort || ilayer->first == cltProtect || ilayer->first == cltSmoke) &&
           !r->user.access.check_profile(point_id, 410) )
        pr_edit = false;
      if ( ilayer->first == cltDisable &&
           (!r->user.access.check_profile(point_id, 425)) ) {
        pr_edit = false;
      }
      if ( pr_edit ) {
        SetProp( n, "edit", 1 );
        if ( isBaseLayer( ilayer->first, !isTripCraft ) )
          SetProp( n, "base_edit", 1 );
      }
    }
    if ( ilayer->second.notfree )
        SetProp( n, "notfree", 1 );
    if ( !ilayer->second.name_view.empty() ) {
        SetProp( n, "name_view_help", ilayer->second.name_view );
        if ( !ilayer->second.func_key.empty() )
            SetProp( n, "func_key", ilayer->second.func_key );
    }
   id++;
  }
  xmlNodePtr n = NewTextChild( editNode, "layer",  EncodeCompLayerType( cltUnknown ) );
  SetProp( n, "id", id );
    SetProp( n, "name", "LAYER_CLEAR_ALL" );
    SetProp( n, "priority", 10000 );
    SetProp( n, "edit", 1 );
  SetProp( n, "name_view_help", AstraLocale::getLocaleText("������ �� ������ ����") );
  SetProp( n, "func_key", "Shift+F8" );
  xmlNodePtr propNode = NewTextChild( salonsNode, "draw_props" );
  max_priority++;
  id++;
  //!log ProgTrace( TRACE5, "max_priority=%d", max_priority );
  for ( int i=0; i<dpTypeNum; i++ ) {
    if ( !props.isFlag( (TDrawPropsType)i ) )
      continue;
    TDrawPropInfo p = getDrawProps( (TDrawPropsType)i );
    //!log ProgTrace( TRACE5, "priority=%d, name=%s", max_priority, p.name.c_str() );
    n = NewTextChild( propNode, "draw_item", p.name );
    SetProp( n, "id", id );
    SetProp( n, "figure", p.figure );
    SetProp( n, "color", p.color );
    SetProp( n, "priority", max_priority );
    max_priority++;
  }
}

void CreateSalonMenu( int point_dep, xmlNodePtr salonsNode )
{
  //!log ProgTrace( TRACE5, "CreateSalonMenu" );
  TFilterLayers filterLayers;
  filterLayers.getFilterLayers( point_dep );
  std::map<ASTRA::TCompLayerType,SALONS2::TMenuLayer> menuLayers;
  getMenuLayers( true, filterLayers, menuLayers );
  BitSet<TDrawPropsType> props;
  buildMenuLayers( true, menuLayers, props, salonsNode, point_dep );
}

bool compatibleLayer( ASTRA::TCompLayerType layer_type )
{
  if ( layer_type == cltDisable &&
       !TReqInfo::Instance()->desk.compatible( DISABLE_LAYERS ) )
    return false;
  if ( layer_type == cltProtSelfCkin &&
       !TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) )
    return false;
  return true;
}

bool isPropsLayer( ASTRA::TCompLayerType layer_type ) {
  return ( layer_type == ASTRA::cltDisable ||
           layer_type == ASTRA::cltProtect ||
           layer_type == ASTRA::cltSmoke ||
           layer_type == ASTRA::cltUncomfort );
};

bool isBaseLayer( ASTRA::TCompLayerType layer_type, bool isComponCraft )
{
  return ( layer_type == cltSmoke ||
           layer_type == cltUncomfort ||
           ( ( layer_type == cltDisable || layer_type == cltProtect ) && isComponCraft ) );
}

void TFilterLayer_SOM_PRL::IntRead( int point_id, bool pr_tranzit_salons, const std::vector<TTripRouteItem> &routes )
{
  Clear();
    TQuery Qry( &OraSession );
  Qry.SQLText=
  "SELECT move_id, point_num, "
  "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
  " FROM points "
  " WHERE points.point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgTrace( TRACE5, "TFilterLayer_SOM_PRL::Read point_id=%d, layer not found", point_id );
    return;
  }
  int move_id = Qry.FieldAsInteger( "move_id" );
  int point_num = Qry.FieldAsInteger( "point_num" );
  int first_point = Qry.FieldAsInteger( "first_point" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT points.point_id AS point_dep, points.point_num AS point_num_dep,"
    "       DECODE(tlgs_in.type,'PRL',:prl_layer,:som_layer) AS layer_type "
    "FROM tlg_binding,tlg_source,tlgs_in, "
    "     (SELECT point_id,point_num FROM points "
    "      WHERE :first_point IN (first_point,point_id) AND point_num<:point_num AND pr_del=0 "
    "      ORDER BY point_num "
    "     ) points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
    "      NVL(tlg_source.has_errors,0)=0 AND "
    "      tlgs_in.id=tlg_source.tlg_id AND tlgs_in.num=1 AND tlgs_in.type IN ('PRL','SOM') "
    "ORDER BY point_num DESC,DECODE(tlgs_in.type,'PRL',1,0)";
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.CreateVariable( "som_layer", otString, EncodeCompLayerType( cltSOMTrzt ) );
  Qry.CreateVariable( "prl_layer", otString, EncodeCompLayerType( cltPRLTrzt ) );
  Qry.Execute();
  if ( pr_tranzit_salons ) {
    TQuery PaxQry( &OraSession );
    PaxQry.SQLText =
      "SELECT pax_grp.point_dep FROM pax_grp "
      " WHERE pax_grp.point_dep=:point_id AND "
      "       pax_grp.status NOT IN ('E') AND "
      "       rownum<2";
    PaxQry.DeclareVariable( "point_id", otInteger );
    TQuery TranzQry( &OraSession );
    TranzQry.SQLText =
      "SELECT NVL(pr_tranz_reg,0) pr_tranz_reg FROM points, trip_sets "
       " WHERE move_id=:move_id AND "
       "      point_num BETWEEN :point_num_dep+1 AND :point_num AND "
       "      points.point_id=trip_sets.point_id AND pr_del=0 AND pr_reg<>0 "
       " ORDER BY 1 DESC";
    TranzQry.CreateVariable( "move_id", otInteger, move_id );
    TranzQry.CreateVariable( "point_num", otInteger, point_num );
    TranzQry.DeclareVariable( "point_num_dep", otInteger );
    for ( ; !Qry.Eof; Qry.Next() ) {
      if ( pr_tranzit_salons ) {
        TranzQry.SetVariable( "point_num_dep", Qry.FieldAsInteger( "point_num_dep" ) );
        TranzQry.Execute();
        ProgTrace( TRACE5, "move_id=%d, point_num=%d, point_num_dep=%d",
                   move_id, point_num, Qry.FieldAsInteger( "point_num_dep" ) );
        if ( !TranzQry.Eof && TranzQry.FieldAsInteger( "pr_tranz_reg" ) ) { //���� ���ॣ������ �࠭��� �� �������� - �� �ᯮ��㥬 ⥫��ࠬ��
          tst();
          continue;
        }
        bool pr_find = false;
        int point_dep1 = Qry.FieldAsInteger( "point_dep" );
        //�஡�� �� ��������, �᫨ � �㭪� ���� ��ॣ����. ���ᠦ��� � �饬 �����
        for ( std::vector<TTripRouteItem>::const_iterator item=routes.begin();
              item!=routes.end(); item++ ) {
          if ( item->point_id == point_dep1 ) {
            PaxQry.SetVariable( "point_id", point_dep1 );
            PaxQry.Execute();
            pr_find = !PaxQry.Eof;
            ProgTrace( TRACE5, "point_dep1=%d, pr_find=%d", point_dep1, pr_find );
            break;
          }
        }
        if ( pr_find ) {
          continue;
        }
      }
      break;
    }
  }
  if ( Qry.Eof ) {
    ProgTrace( TRACE5, "TFilterLayer_SOM_PRL::Read point_id=%d, layer not found", point_id );
    return;
  }
  point_dep = Qry.FieldAsInteger( "point_dep" );
  layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
  ProgTrace( TRACE5, "TFilterLayer_SOM_PRL::Read point_id=%d, point_dep=%d, layer_type=%s",
             point_id, point_dep, EncodeCompLayerType( layer_type ) );
  return;
}

void TFilterLayer_SOM_PRL::ReadOnTranzitRoutes( int point_id,
                                                const std::vector<TTripRouteItem> &routes )
{
  IntRead( point_id, SALONS2::isTranzitSalons( point_id ), routes );
}

void TFilterLayer_SOM_PRL::Read( int point_id )
{
  std::vector<TTripRouteItem> routes;
  bool pr_tranzit_salons = SALONS2::isTranzitSalons( point_id );
  if ( pr_tranzit_salons ) {
    FilterRoutesProperty filterRoutes;
    filterRoutes.Read( TFilterRoutesSets( point_id ) );
    routes.insert( routes.end(), filterRoutes.begin(), filterRoutes.end() );
  }
  IntRead( point_id, pr_tranzit_salons, routes );
}

void TFilterLayers::getIntFilterLayers( int point_id,
                                        bool pr_tranzit_salons,
                                        const std::vector<TTripRouteItem> &routes,
                                        bool only_compon_props=false )
{
  point_dep = ASTRA::NoExists;
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  if ( only_compon_props ) {
    getMenuBaseLayers( menuLayers, true );
  }
    clearFlags();
    for ( int l=0; l!=cltTypeNum; l++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)l;
    if ( layer_type == cltTranzit ||
         layer_type == cltProtTrzt ||
         layer_type == cltBlockTrzt ||
         layer_type == cltTranzit ||
         layer_type == cltSOMTrzt ||
         layer_type == cltPRLTrzt ||
         layer_type == cltProtBeforePay ||
         layer_type == cltProtAfterPay ||
         layer_type == cltPNLBeforePay ||
         layer_type == cltPNLAfterPay ||
         layer_type == cltUnknown )
      continue;
    if ( only_compon_props && !isEditableMenuLayers( layer_type, menuLayers ) ) {
      continue;
    }
      setFlag( layer_type );
  }
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
    "SELECT pr_permit FROM trip_paid_ckin WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();

    if ( !Qry.Eof && Qry.FieldAsInteger( "pr_permit" )!=0 ) {
      setFlag( cltProtBeforePay );
      setFlag( cltProtAfterPay );
      setFlag( cltPNLBeforePay );
      setFlag( cltPNLAfterPay );
    }

  Qry.Clear();
    Qry.SQLText =
    "SELECT pr_tranz_reg,pr_block_trzt,ckin.get_pr_tranzit(:point_id) as pr_tranzit "
    "FROM trip_sets "
    "WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();

    if ( !Qry.Eof && Qry.FieldAsInteger( "pr_tranzit" ) ) { // �� �࠭���� ३�
        if ( Qry.FieldAsInteger( "pr_tranz_reg" ) ) {
      setFlag( cltProtTrzt );
      setFlag( cltTranzit );
        }
        else {
            if ( Qry.FieldAsInteger( "pr_block_trzt" ) ) {
              setFlag( cltBlockTrzt );
      }
            else {
                 ASTRA::TCompLayerType layer_tlg;
         TFilterLayer_SOM_PRL FilterLayer_SOM_PRL;
         FilterLayer_SOM_PRL.ReadOnTranzitRoutes( point_id, pr_tranzit_salons, routes );
         if ( FilterLayer_SOM_PRL.Get( point_dep, layer_tlg ) ) {
                   setFlag( layer_tlg );
                 }
            }
        }
    }
    for ( int l=0; l!=cltTypeNum; l++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)l;
    if ( isFlag( layer_type ) ) {
        ProgTrace( TRACE5, "TFilterLayers::getFilterLayers(%d,%s), point_dep=%d", point_id, EncodeCompLayerType( layer_type ), point_dep );
    }
  }
}

void TFilterLayers::getFilterLayers( int point_id, bool only_compon_props )
{
  std::vector<TTripRouteItem> routes;
  bool pr_tranzit_salons = SALONS2::isTranzitSalons( point_id );
  if ( pr_tranzit_salons ) {
    FilterRoutesProperty filterRoutes;
    filterRoutes.Read( TFilterRoutesSets( point_id ) );
    routes.insert( routes.end(), filterRoutes.begin(), filterRoutes.end() );
  }
  getIntFilterLayers( point_id, pr_tranzit_salons, routes, only_compon_props );
}

bool TFilterLayers::CanUseLayer( ASTRA::TCompLayerType layer_type,
                                 int layer_point_dep,
                                 int point_salon_departure,
                                 bool pr_takeoff )
{
  if ( pr_takeoff &&
       ( layer_type == cltProtBeforePay ||
         layer_type == cltProtAfterPay ||
         layer_type == cltPNLBeforePay ||
         layer_type == cltPNLAfterPay ||
         layer_type == cltProtSelfCkin ||
         layer_type == cltPNLCkin ||
         layer_type == cltBlockCent ||
         layer_type == cltProtect ||
         layer_type == cltProtTrzt ||
         layer_type == cltProtCkin ||
         layer_type == cltUncomfort ||
         layer_type == cltSmoke ) ) {
      return false;
  }
    switch( layer_type ) {
        case cltSOMTrzt:
            return ( isFlag( layer_type ) && point_dep == layer_point_dep && layer_point_dep != point_salon_departure );
        case cltPRLTrzt:
            return ( isFlag( layer_type ) && point_dep == layer_point_dep && layer_point_dep != point_salon_departure );
        default:
            return isFlag( layer_type );
    }
}

void TSalons::Clear( )
{
  FCurrPlaceList = NULL;
  if ( pr_owner ) {
    for ( std::vector<TPlaceList*>::iterator i = placelists.begin(); i != placelists.end(); i++ ) {
      delete *i;
    }
  }
  placelists.clear();
  ExistSubcls = boost::none;
  ExistBaseLayers = boost::none;
  OccupySeats = boost::none;
}

bool TPlace::CompareRems( const TPlace &seat ) const
{
  if ( remarks.size() != seat.remarks.size() ) {
    return false;
  }
    for ( std::map<int, std::vector<TSeatRemark> >::const_iterator p1=remarks.begin(),
            p2=seat.remarks.begin();
            p1!=remarks.end(),
            p2!=seat.remarks.end();
            p1++, p2++ ) {
        if ( p1->first != p2->first || p1->second.size() != p2->second.size() ) {
      return false;
        }
        for ( std::vector<TSeatRemark>::const_iterator lp1=p1->second.begin(),
          lp2=p2->second.begin();
          lp1!=p1->second.end(),
          lp2!=p2->second.end();
          lp1++, lp2++ ) {
      if ( *lp1 != *lp2 ) {
             return false;
      }
    }
  }
  return true;
}

bool TPlace::CompareLayers( const TPlace &seat ) const
{
  if ( lrss.size() != seat.lrss.size() ) {
//!log    ProgTrace( TRACE5, "TPlace::CompareLayers: lrss1.size()=%zu, lrss2.size()=%zu",
//!log               lrss.size(), seat.lrss.size() );
    return false;
  }
  for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare> >::const_iterator l1=lrss.begin(),
        l2=seat.lrss.begin();
        l1!=lrss.end(),
        l2!=seat.lrss.end();
        l1++, l2++ ) {
    if ( l1->first != l2->first || l1->second.size() != l2->second.size() ) {
      return false;
    }
    for ( std::set<TSeatLayer,SeatLayerCompare>::const_iterator lp1=l1->second.begin(),
          lp2=l2->second.begin();
          lp1!=l1->second.end(),
          lp2!=l2->second.end();
          lp1++, lp2++ ) {
      if ( *lp1 != *lp2 ) {
        //!logProgTrace( TRACE5, "TPlace::CompareLayers: %s!=%s",
//!log                   lp1->toString().c_str(), lp2->toString().c_str() );
        return false;
      }
    }
  }
  return true;
}

bool TPlace::CompareTariffs( const TPlace &seat ) const
{
  if ( tariffs.size() != seat.tariffs.size() ) {
    return false;
  }
  for ( std::map<int, TSeatTariff>::const_iterator p1=tariffs.begin(),
        p2=seat.tariffs.begin();
        p1!=tariffs.end(),
        p2!=seat.tariffs.end();
        p1++, p2++ ) {
    if ( p1->first != p2->first ||
         p1->second != p2->second ) {
      return false;
    }
  }
  return true;
}

bool TPlace::CompareRFISCs( const TPlace &seat ) const
{
//  ProgTrace( TRACE5, "rfiscs.size()=%zu, seat.rfiscs.size()=%zu",rfiscs.size(),seat.rfiscs.size());
  if ( rfiscs.size() != seat.rfiscs.size() ) {
    return false;
  }
  for ( std::map<int, TRFISC>::const_iterator p1=rfiscs.begin(),
        p2=seat.rfiscs.begin();
        p1!=rfiscs.end(),
        p2!=seat.rfiscs.end();
        p1++, p2++ ) {
    ProgTrace( TRACE5, "point_id1=%d, point_id2=%d, val1=%s, val2=%s", p1->first, p2->first, p1->second.str().c_str(), p2->second.str().c_str() );
    if ( p1->first != p2->first ||
         p1->second != p2->second ) {
      tst();
      return false;
    }
  }
  return true;
}


bool TPlace::isChange( const TPlace &seat, BitSet<TCompareComps> &compare ) const
{
  if ( compare.isFlag( ccXY ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( x != seat.x ||
             y != seat.y ) ) ) {
      //!logProgTrace( TRACE5, "TPlace::isChange(ccXY), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d",
//!log                 x, y, visible, seat.x, seat.y, seat.visible );
      return true;
    }
  }
  if ( compare.isFlag( ccXYVisible ) ) {
    if ( visible && seat.visible &&
         ( x != seat.x ||
           y != seat.y ) ) {
//!log        ProgTrace( TRACE5, "TPlace::isChange(ccXYVisible), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d",
//!log                   x, y, visible, seat.x, seat.y, seat.visible );
          return true;
    }
  }
  if ( compare.isFlag( ccName ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( xname != seat.xname ||
             yname != seat.yname ) ) ) {
      //!logProgTrace( TRACE5, "TPlace::isChange(ccName), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccNameVisible ) ) {
    if ( visible && seat.visible &&
         ( xname != seat.xname ||
           yname != seat.yname ) ) {
//!log        ProgTrace( TRACE5, "TPlace::isChange(ccNameVisible), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                   x, y, visible, seat.x, seat.y, seat.visible, string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
          return true;
    }
  }
  if ( compare.isFlag( ccElemType ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( elem_type != seat.elem_type ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccElemType), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                 "seat1.elem_type=%s, seat2.elem_type=%s, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, elem_type.c_str(),
//!log                 seat.elem_type.c_str(),
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccElemTypeVisible ) ) {
    if ( visible && seat.visible &&
         ( elem_type != seat.elem_type ) ) {
//!log        ProgTrace( TRACE5, "TPlace::isChange(ccElemTypeVisible), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                   "seat1.elem_type=%s, seat2.elem_type=%s, seat1.name=%s, seat2.name=%s",
//!log                   x, y, visible, seat.x, seat.y, seat.visible, elem_type.c_str(),
//!log                   seat.elem_type.c_str(),
//!log                   string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
          return true;
    }
  }

  if ( compare.isFlag( ccClass ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( clname != seat.clname ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccClass), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                 "seat1.clname=%s, seat2.clname=%s, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, clname.c_str(),
//!log                 seat.clname.c_str(),
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccXYPrior ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( xprior != seat.xprior ||
             yprior != seat.yprior ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccXYPrior), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                 "seat1.prior(%d,%d), seat2.prior(%d,%d), seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, xprior, yprior,
//!log                 seat.xprior, seat.yprior,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccXYNext ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( xnext != seat.xnext ||
             ynext != seat.ynext ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccXYNext), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                 "seat1.next(%d,%d), seat2.next(%d,%d), seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, xnext, ynext,
//!log                 seat.xnext, seat.ynext,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccAgle ) ) {
    if ( visible != seat.visible ||
         ( visible &&
           ( agle != seat.agle ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccAgle), seat1(%d,%d).visible=%d, seat2(%d,%d).visible=%d, "
//!log                 "seat1.agle=%d, seat2.agle=%d, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible, agle, seat.agle,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccRemarks ) ) {
    if ( visible != seat.visible ||
         ( visible && !CompareRems( seat ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccRemarks), seat1(%d,%d).visible=%d, "
//!log                 "seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccLayers ) ) {
    if ( visible != seat.visible ||
         ( visible && !CompareLayers( seat ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccLayers), seat1(%d,%d).visible=%d, "
//!log                 "seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccTariffs ) ) {
    if ( visible != seat.visible ||
         ( visible && !CompareTariffs( seat ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccTariffs), seat1(%d,%d).visible=%d, "
//!log                "seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                x, y, visible, seat.x, seat.y, seat.visible,
//!log                string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccRFISC ) ) {
    if ( visible != seat.visible ||
         ( visible && !CompareRFISCs( seat ) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccTariffs), seat1(%d,%d).visible=%d, "
//!log                "seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                x, y, visible, seat.x, seat.y, seat.visible,
//!log                string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }
  if ( compare.isFlag( ccDrawProps ) ) {
    if ( visible != seat.visible ||
         ( visible && !(drawProps == seat.drawProps) ) ) {
//!log      ProgTrace( TRACE5, "TPlace::isChange(ccDrawProps), seat1(%d,%d).visible=%d,"
//!log                 " seat2(%d,%d).visible=%d, seat1.name=%s, seat2.name=%s",
//!log                 x, y, visible, seat.x, seat.y, seat.visible,
//!log                 string(yname+xname).c_str(), string(seat.yname+seat.xname).c_str() );
      return true;
    }
  }

  return false;
}

void TPlace::AddLayer( int key, const TSeatLayer &seatLayer ) { //���஢�� �� �������� point_id, ���砫� �맮� � �������� �㭪⠬�, ����� ����� ���쭨��
  if ( isCleanDoubleLayerType( seatLayer.layer_type ) ) {
    for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=lrss.begin();
          ilayers!=lrss.end(); ilayers++ ) {
      if ( key == ilayers->first ) { //��諨 �� �㭪� ࠧ��⪨
        break; //��祣� �� ��諨
      }
      for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer= ilayers->second.begin();
            ilayer!=ilayers->second.end();  ilayer++ ){
        if ( ilayer->layer_type == seatLayer.layer_type &&
             ilayer->point_dep == seatLayer.point_dep &&
             ilayer->point_arv == seatLayer.point_arv ) {
          //�ॡ���� ��������� ������� ᫮�, �.�. �� ������ �ਮ��⥭
          ClearLayer( ilayer->point_id, *ilayer );
          break;
          //return;
        }
      }
    }
  }
  lrss[ key ].insert( seatLayer );
}

void TPlace::Build( xmlNodePtr node, int point_dep, bool pr_lat_seat,
                    TRFISCMode RFISCMode, bool pr_update,
                    bool with_pax, const std::map<int,SALONS2::TPaxList> &pax_lists ) const
{
   xmlNodePtr propsNode;
   xmlNodePtr propNode;
   xmlNodePtr remsNode;
   std::map<int,vector<TSeatRemark>,classcomp > remarks;
   std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
   NewTextChild( node, "x", x );
   NewTextChild( node, "y", y );
   GetRemarks( remarks );
   if ( RFISCMode != rTariff &&
        !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) ) {
     std::map<int, TRFISC,classcomp> rfiscs;
     GetRFISCs(rfiscs);
     for ( std::map<int, TRFISC,classcomp>::iterator irfisc=rfiscs.begin();
           irfisc!=rfiscs.end(); irfisc++ ) {
       if ( !irfisc->second.code.empty() ) {
         TSeatRemark remark;
         remark.pr_denial = 0;
         remark.value = irfisc->second.code;
         remarks[ irfisc->first ].push_back( remark );
       }
     }
   }
   set<TSeatRemark,SeatRemarkCompare> uniqueReamarks;
   propsNode = NULL;
   //!!!���� ᤥ���� ���஢�� �� �㭪⠬ � ���砫� �뢮���� ६�ન ᠬ� ������� � �㭪�� �뫥� - ��稭��� � ��砫� ������� - ������� � ⠪ ��!!!
   if ( !remarks.empty() ) {
     for ( std::map<int,vector<TSeatRemark> >::iterator iremarks = remarks.begin(); iremarks != remarks.end(); iremarks++ ) {
       if ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
            iremarks->first != point_dep ) {
         continue;
       }
       for ( std::vector<TSeatRemark>::iterator irem=iremarks->second.begin(); irem!=iremarks->second.end(); irem++ ) {
         if ( uniqueReamarks.find( *irem ) != uniqueReamarks.end() ) {
           continue;
         }
         uniqueReamarks.insert( *irem );
         if ( !propsNode ) {
           propsNode = NewTextChild( node, "rems" );
         }
         propNode = NewTextChild( propsNode, "rem" );
         if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
           if ( iremarks->first != ASTRA::NoExists ) {
             NewTextChild( propNode, "point_id", iremarks->first );
           }
           NewTextChild( propNode, "code", irem->value );
           if ( irem->pr_denial ) {
             NewTextChild( propNode, "pr_denial" );
           }
         }
         else { //prior varsion
           NewTextChild( propNode, "rem", irem->value );
           if ( irem->pr_denial ) {
             NewTextChild( propNode, "pr_denial" );
           }
         }
       }
     }
   }
   remsNode = propsNode;
   GetLayers( layers, glAll );
/*   TSeatLayer tmp_layer = getDropBlockedLayer( point_dep );
   if ( tmp_layer.layer_type != cltUnknown ) {
     layers[ point_dep ].insert( tmp_layer );
   } ???*/
   set<TSeatLayer,SeatLayerCompare> uniqueLayers;
   propsNode = NULL;
   //���� ᤥ���� ���஢�� �� �㭪⠬ � ���砫� �뢮���� ᫮� ᠬ� ������� � �㭪�� �뫥�
   //�᫨ �� ᮢ������ point_id c point_dep, � �ᮢ��� 㣮��� - ����砥�, �� ᠬ� �ਮ���� ᫮� �� �ਭ������� �㭪��
   if ( !layers.empty() ) {
     for( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin(); ilayers!=layers.end(); ilayers++ ) {
       for ( std::set<TSeatLayer>::iterator ilayer=ilayers->second.begin(); ilayer!=ilayers->second.end(); ilayer++ ) {
         if ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
              ilayers->first != point_dep &&
              isPropsLayer( ilayer->layer_type ) ) {
           continue;
         }
         TSeatLayer tmp_layer;
         tmp_layer.layer_type = ilayer->layer_type;
         if ( uniqueLayers.find( tmp_layer ) != uniqueLayers.end() ) {
           continue;
         }
         if ( tmp_layer.layer_type == ASTRA::cltProtSelfCkin &&
              !TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) ) {
            tmp_layer.layer_type = ASTRA::cltProtBeforePay;
         }
         uniqueLayers.insert( tmp_layer );
         if ( propsNode == NULL ) {
             propsNode = NewTextChild( node, "layers" );
         }
         if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
           propNode = NewTextChild( propsNode, "layer" );
           if ( ilayers->first != ASTRA::NoExists ) {
             NewTextChild( propNode, "point_id", ilayers->first );
           }
           if ( ilayer->point_dep != ASTRA::NoExists ) {
             NewTextChild( propNode, "point_dep", ilayer->point_dep );
           }
           if ( ilayer->point_arv != ASTRA::NoExists ) {
             NewTextChild( propNode, "point_arv", ilayer->point_arv );
           }
           if ( ilayer->pax_id != ASTRA::NoExists ) {
             NewTextChild( propNode, "pax_id", ilayer->pax_id );
             if ( with_pax ) {
               xmlNodePtr passNode = NewTextChild( propNode, "passenger" );
               TPaxList::const_iterator ipax = pax_lists.find( ilayer->point_id )->second.find( ilayer->pax_id );
               NewTextChild( passNode, "reg_no", ipax->second.reg_no );
               NewTextChild( passNode, "pers_type", EncodePerson( ipax->second.pers_type ) );
               NewTextChild( passNode, "seats", (int)ipax->second.seats );
               NewTextChild( passNode, "cl", ipax->second.cl );
               NewTextChild( passNode, "surname", ipax->second.surname );
               NewTextChild( passNode, "pr_infant", ipax->second.pr_infant != ASTRA::NoExists );
             }
           }
           if ( ilayer->crs_pax_id != ASTRA::NoExists ) {
             NewTextChild( propNode, "crs_pax_id", ilayer->crs_pax_id );
             //���ᠦ���
             if ( with_pax ) {
               xmlNodePtr passNode = NewTextChild( propNode, "passenger" );
               TPaxList::const_iterator ipax = pax_lists.find( ilayer->point_id )->second.find( ilayer->crs_pax_id );
               NewTextChild( passNode, "pers_type", EncodePerson( ipax->second.pers_type ) );
               NewTextChild( passNode, "seats", (int)ipax->second.seats );
               NewTextChild( passNode, "cl", ipax->second.cl );
               NewTextChild( passNode, "surname", ipax->second.surname );
               NewTextChild( passNode, "pr_infant", ipax->second.pr_infant != ASTRA::NoExists );
             }
           }
           if ( ilayer->time_create != ASTRA::NoExists ) {
             NewTextChild( propNode, "time_create", DateTimeToStr( ilayer->time_create ) );
           }
           ProgTrace( TRACE5, "TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN )=%d", TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) );
           if ( ilayer->layer_type == cltProtSelfCkin &&
                !TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) ) {
              NewTextChild( propNode, "layer_type", EncodeCompLayerType( cltProtBeforePay ) );
           }
           else {
             NewTextChild( propNode, "layer_type", EncodeCompLayerType( ilayer->layer_type ) );
         }
         }
         else { //prior version
           //�������� ᫮�, ����� �ਭ������� point_dep ��� �� �ਭ������� point_dep � �� ������
           if ( ilayer->layer_type  == cltDisable && !compatibleLayer( ilayer->layer_type ) ) {
             if ( !remsNode ) {
               remsNode = NewTextChild( node, "rems" );
             }
             propNode = NewTextChild( remsNode, "rem" );
             NewTextChild( propNode, "rem", "X" );    //!!!
             continue;
           }
           propNode = NewTextChild( propsNode, "layer" );
           if ( ilayer->layer_type == cltProtSelfCkin &&
                !TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) ) {
              NewTextChild( propNode, "layer_type", EncodeCompLayerType( cltProtBeforePay ) );
           }
           else {
             NewTextChild( propNode, "layer_type", EncodeCompLayerType( ilayer->layer_type ) );
           }
       }
     }
   }
   }
   if ( RFISCMode != rRFISC ||
        !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) ) {
     std::map<int,TSeatTariff,classcomp> tariffs;
     GetTariffs( tariffs );
     if ( RFISCMode != rTariff &&
          !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) ) { //����� ����⭮
       tariffs.clear();
       std::map<int, TRFISC,classcomp> rfiscs;
       GetRFISCs(rfiscs);
       for ( std::map<int, TRFISC,classcomp>::iterator irfisc=rfiscs.begin();
             irfisc!=rfiscs.end(); irfisc++ ) {
         TSeatTariff seatTariff;
         seatTariff.color = irfisc->second.color;
         seatTariff.rate = 1; //!!!
         seatTariff.currency_id = "���"; //!!!
         tariffs.insert( make_pair( irfisc->first, seatTariff ) );
       }
     }
     set<TSeatTariff,SeatTariffCompare> uniqueTariffs;
     if ( !tariffs.empty() ) {
       propsNode = NewTextChild( node, "tariffs" );
       for ( std::map<int,TSeatTariff>::iterator itariff=tariffs.begin(); itariff!=tariffs.end(); itariff++ ) {
         if ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
               itariff->first != point_dep ) {
           continue;
         }
         if ( uniqueTariffs.find( itariff->second ) != uniqueTariffs.end() ) {
             continue;
         }
         uniqueTariffs.insert( itariff->second );
         if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
           propNode = NewTextChild( propsNode, "tariff" );
           NewTextChild( propNode, "point_id", itariff->first );
           NewTextChild( propNode, "value", itariff->second.rate );
           NewTextChild( propNode, "color", itariff->second.color );
           NewTextChild( propNode, "currency_id", itariff->second.currency_id );
           /*!!!if ( !itariff->second.code.empty() ) {
             NewTextChild( propNode, "rfics", itariff->second.code );
           }*/
         }
         else {
           if ( !pr_update ) {
             xmlNodePtr n = NewTextChild( node, "tariff",itariff->second.rate );
             SetProp( n, "color", itariff->second.color );
             SetProp( n, "currency_id", itariff->second.currency_id );
             /*!!!if ( !itariff->second.code.empty() ) {
               SetProp( n, "rfics", itariff->second.code );
             }*/
           }
           else { //!!! �� ��஭� ������ ���� �訡�� ����� ⥣� XMLParseUpdateSalons: remsNode := GetNode( 'tariff', n ); ���� �ᯮ�짮���� GetNodeFast
                  //������ ������� ��� ⮣�, �⮡� ࠡ�⠫�
             xmlNodePtr n = NewTextChild( propsNode, "tariff",itariff->second.rate );
             SetProp( n, "color", itariff->second.color );
             SetProp( n, "currency_id", itariff->second.currency_id );
             /*!!!if ( !itariff->second.code.empty() ) {
               SetProp( n, "rfics", itariff->second.code );
             }*/
           }
         }
       }
     }
   }
   if ( RFISCMode != rTariff ) {
     set<TRFISC,RFISCCompare> uniqueRFISCS;
     std::map<int, TRFISC,classcomp> rfiscs;
     GetRFISCs( rfiscs );
     if ( !rfiscs.empty() ) {
       propsNode = NewTextChild( node, "rfiscs" );
       for ( std::map<int,TRFISC>::iterator irfisc=rfiscs.begin(); irfisc!=rfiscs.end(); irfisc++ ) {
         if ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
               irfisc->first != point_dep ) {
           continue;
         }
         if ( uniqueRFISCS.find( irfisc->second ) != uniqueRFISCS.end() ) {
             continue;
         }
         uniqueRFISCS.insert( irfisc->second );
         if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
           propNode = NewTextChild( propsNode, "rfisc" );
           NewTextChild( propNode, "point_id", irfisc->first );
           if ( irfisc->second.rate != 0.0 ) {
             NewTextChild( propNode, "value", irfisc->second.rate );
           }
           NewTextChild( propNode, "color", irfisc->second.color );
           if ( !irfisc->second.currency_id.empty() ) {
             NewTextChild( propNode, "currency_id", irfisc->second.currency_id );
           }
           if ( !irfisc->second.code.empty() ) {
             NewTextChild( propNode, "code", irfisc->second.code );
           }

           NewTextChild( propNode, "currency_id", irfisc->second.currency_id );
           NewTextChild( propNode, "code", irfisc->second.code );
         }
         else {
           //ProgTrace( TRACE5, "rfisc %s", irfisc->second.str().c_str() );
           propNode = NewTextChild( node, "rfisc" );
           if ( irfisc->second.rate != 0.0 ) {
             NewTextChild( propNode, "value", irfisc->second.rate );
           }
           NewTextChild( propNode, "color", irfisc->second.color );
           if ( !irfisc->second.currency_id.empty() ) {
             NewTextChild( propNode, "currency_id", irfisc->second.currency_id );
           }
           if ( !irfisc->second.code.empty() ) {
             NewTextChild( propNode, "code", irfisc->second.code );
           }
         }
       }
     }
   }
   //���� ���ᮢ��� ��⥩ � �� ⮫쪮
   if ( !drawProps.emptyFlags() ) {
     xmlNodePtr n = NewTextChild( node, "drawProps" );
     for ( int i=0; i<dpTypeNum; i++ ) {
       if ( !drawProps.isFlag( (TDrawPropsType)i ) ) {
         continue;
       }
       propNode = NewTextChild( n, "drawProp" );
       TDrawPropInfo pinfo = getDrawProps( (TDrawPropsType)i );
       SetProp( propNode, "figure", pinfo.figure );
       SetProp( propNode, "color", pinfo.color );
     }
   }
   if ( pr_update )
     return;
   NewTextChild( node, "elem_type", elem_type );
   if ( xprior != -1 ) {
     NewTextChild( node, "xprior", xprior );
   }
   if ( yprior != -1 ) {
     NewTextChild( node, "yprior", yprior );
   }
   if ( agle ) {
     NewTextChild( node, "agle", agle );
   }
   NewTextChild( node, "class", clname );
   NewTextChild( node, "xname", denorm_iata_line( xname, pr_lat_seat ) );
   NewTextChild( node, "yname", denorm_iata_row( yname ) );
}

void TPlace::SetTariffsByRFISCColor( int point_dep, const TSeatTariffMapType &salonTariffs, const TSeatTariffMap::TStatus &status )
{
  if ( salonTariffs.empty() || !visible || !isplace || status == TSeatTariffMap::stNotRFISC ) {
    return;
  }
  tariffs.clear();
  TSeatTariffMapType::const_iterator colorItem;
  std::map<int, TRFISC,classcomp>::iterator dep_rfisc = rfiscs.end();
  for ( std::map<int, TRFISC,classcomp>::iterator irfisc=rfiscs.begin();
        irfisc!=rfiscs.end();  ) {
    if ( irfisc->second.color.empty() ) {
      std::map<int, TRFISC,classcomp>::iterator next=irfisc;
      ++next;
      rfiscs.erase( irfisc );
      irfisc=next;
      continue;
    }
    colorItem = salonTariffs.find( irfisc->second.color );
    if ( colorItem != salonTariffs.end() ) {
      TSeatTariff tariff;
      tariff.color = colorItem->second.color;
      tariff.currency_id = colorItem->second.currency_id;
      if ( status != TSeatTariffMap::stUseRFISC ) {
      //०�� ࠡ��� � RFISC, �� �業�� �� ��諠, ���� 㪠���� ��� ࠧ��⪨ ����. �⮨�����, �⮡� ���ᠦ�� ᥫ �� ࠧ��祭�� ���� � ��᫥���� ��।�
        tariff.rate = INT_MAX;
      }
      else {
        tariff.rate = colorItem->second.rate;
      }
      if ( selfckin_client() ) {
         tariff.rate = irfisc->second.rate;
      }
      AddTariff( irfisc->first, tariff );
      ProgTrace( TRACE5, "1 place(%d,%d) set rate=%s",
                         x, y,
                         irfisc->second.str().c_str() );
      if ( irfisc->first == point_dep ) {
        SeatTariff = tariff;
        ProgTrace( TRACE5, "set rate %s",SeatTariff.str().c_str() );
      }
      ++irfisc;
    }
    else {
      ProgTrace( TRACE5, "1 place(%d,%d) delete rate=%s", x, y, irfisc->second.str().c_str() );
      if ( irfisc->first == point_dep ) {
        ProgTrace( TRACE5, "clear rate %s",SeatTariff.str().c_str() );
        SeatTariff.clear();
      }
      std::map<int, TRFISC,classcomp>::iterator next=irfisc;
      ++next;
      rfiscs.erase( irfisc );
      irfisc=next;
    }
  }
}

TRFISC TPlace::getRFISC( int point_id ) const
{
  TRFISC rfisc;
  if ( !visible || !isplace ) {
    return rfisc;
  }
  std::map<int, TRFISC,classcomp>::const_iterator irfisc = rfiscs.find( point_id );
  if ( irfisc!=rfiscs.end() ) {
    if ( !irfisc->second.color.empty() ) {
      rfisc = irfisc->second;
    }
  }
  return rfisc;
}

void TPlace::SetTariffsByRFISC( int point_dep )
{
  if ( !visible || !isplace ) {
    return;
  }
  tariffs.clear();
  std::map<int, TRFISC,classcomp>::iterator irfisc = rfiscs.find( point_dep );
  if ( irfisc!=rfiscs.end() ) {
    if ( !irfisc->second.color.empty() ) {
      TSeatTariff tariff;
      tariff.color = irfisc->second.color;
      tariff.currency_id = irfisc->second.currency_id;
      tariff.rate = irfisc->second.rate;
      SeatTariff = tariff;
      AddTariff( irfisc->first, tariff );

      ProgTrace( TRACE5, "SetTariffsByRFICS: place(%d,%d) set tarif=%s",
                 x, y, irfisc->second.str().c_str() );
    }
    else {
      SeatTariff.clear();
      AddLayerToPlace( cltDisable, NowUTC(), NoExists, point_dep, NoExists, BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltDisable ) );
      ProgTrace( TRACE5, "SetTariffsByRFICS: place(%d,%d) set unavailable", x, y );
    }
  }
}


/*void TPlace::SetRFICSRemarkByColor( int key, TSeatTariffMapType salonRFISCColor )
{
  if ( salonRFISCColor.empty() || !visible || !isplace || tariffs.find( key ) == tariffs.end() ) {
    return;
  }
  TSeatRemark remark;
  remark.pr_denial = 0;

  std::map<int, TSeatTariff,classcomp>::iterator itariff=tariffs.find( key );
  if ( !itariff->second.color.empty() ) {
    if ( salonRFISCColor.find( itariff->second.color ) != salonRFISCColor.end() ) {
      remark.value = salonRFISCColor[ itariff->second.color ].code;
      AddRemark( key, remark );
      return;
    }
  }
  if ( salonRFISCColor.find( SeatTariff.color ) != salonRFISCColor.end() ) {
    remark.value = salonRFISCColor[ SeatTariff.color ].code;
    AddRemark( key, remark );
  }
}*/

void TPlace::SetRFISC( int point_id, TSeatTariffMapType &tariffMap )
{
  std::map<int, TRFISC,classcomp> vrfiscs;
  std::map<int, TRFISC,classcomp>::iterator irfisc;
  GetRFISCs( vrfiscs );
  clearRFISCs();
  irfisc = vrfiscs.find( point_id );
  if ( irfisc != vrfiscs.end() ) {
    string color = irfisc->second.color;
    irfisc->second.clear();
    irfisc->second.color = color;
    if ( !tariffMap.empty() ) {
      std::map<std::string,TRFISC>::iterator vrfisc = tariffMap.find( color );
      if ( vrfisc != tariffMap.end() && !color.empty() ) {
        AddRFISC( point_id, vrfisc->second );
        ProgTrace( TRACE5, "SetRFISC point_id %d, %s", point_id, vrfisc->second.str().c_str() );
      }
    }
  }
}

/*void TPlace::DropRFISCRemarks( TSeatTariffMapType salonRFISCColor )
{
  if ( salonRFISCColor.empty() ) {
    return;
  }
  std::map<int, std::vector<TSeatRemark>,classcomp > vremarks;
  GetRemarks( vremarks );
  clearRemarks();
  for ( std::map<int, std::vector<TSeatRemark>,classcomp >::iterator iremarks=vremarks.begin();
        iremarks!=vremarks.end(); iremarks++ ) {
    for ( std::vector<TSeatRemark>::iterator ir=iremarks->second.begin();
          ir!=iremarks->second.end(); ir++ ) {
      bool prFind = false;
      for ( std::map<std::string,TRFISC>::iterator  ic=salonRFISCColor.begin();
            ic!=salonRFISCColor.end(); ic++ ) {
        if ( ir->value == ic->second.code ) {
          prFind = true;
          break;
        }
      }
      if ( !prFind ) {
        AddRemark( iremarks->first, *ir );
      }
    }
  }
}*/

void TPlace::convertSeatTariffs( int point_dep )
{
  vector<int> points;
  points.push_back( point_dep );
  convertSeatTariffs( true, point_dep, points );
}

void TPlace::convertSeatTariffs( bool pr_departure_tariff_only, int point_dep, const std::vector<int> &points )
{
  std::map<int, TSeatTariff,classcomp> tariffs;
  set<TSeatTariff,SeatTariffCompare> uniqueTariffs;
  GetTariffs( tariffs );
  for ( vector<int>::const_iterator ipoint=points.begin(); ipoint!=points.end(); ipoint++ ) {
  //  ProgTrace( TRACE5, " point_id=%d", *ipoint );
    if ( pr_departure_tariff_only && *ipoint != point_dep ) {
      continue;
    }
    if ( tariffs.find( *ipoint ) == tariffs.end() ) {
      continue;
    }
    //!logProgTrace( TRACE5, "ipoint->point_id=%d", *ipoint );
    if ( uniqueTariffs.find( tariffs[ *ipoint ] ) != uniqueTariffs.end() ) {
      //!log tst();
      continue;
    }
    uniqueTariffs.insert( tariffs[ *ipoint ] );
    //!logProgTrace( TRACE5, "*ipoint=%d, color=%s,", *ipoint, tariffs[ *ipoint ].color.c_str() );
    SeatTariff=TSeatTariff(tariffs[ *ipoint ].color,
                           tariffs[ *ipoint ].rate,
                           tariffs[ *ipoint ].currency_id);
    ProgTrace( TRACE5, "SeatTariff=%s", SeatTariff.str().c_str() );
    break;
  }
}

void TPlace::Build( xmlNodePtr node, bool pr_lat_seat, bool pr_update ) const
{
   NewTextChild( node, "x", x );
   NewTextChild( node, "y", y );
   xmlNodePtr remsNode = NULL;
   xmlNodePtr remNode;
   for ( vector<TRem>::const_iterator rem = rems.begin(); rem != rems.end(); rem++ ) {
     if ( !remsNode ) {
       remsNode = NewTextChild( node, "rems" );
     }
     remNode = NewTextChild( remsNode, "rem" );
     NewTextChild( remNode, "rem", rem->rem );
     if ( rem->pr_denial ) {
       NewTextChild( remNode, "pr_denial" );
     }
   }
   if ( !layers.empty() ) {
     xmlNodePtr layersNode = NewTextChild( node, "layers" );
     for ( std::vector<TPlaceLayer>::const_iterator l=layers.begin(); l!=layers.end(); l++ ) {
       if ( l->layer_type  == cltDisable && !compatibleLayer( l->layer_type ) ) {
         if ( !remsNode ) {
           remsNode = NewTextChild( node, "rems" );
         }
        remNode = NewTextChild( remsNode, "rem" );
        NewTextChild( remNode, "rem", "X" );    //!!!
        continue;
       }
       remNode = NewTextChild( layersNode, "layer" );
       if ( l->layer_type == cltProtSelfCkin &&
            !TReqInfo::Instance()->desk.compatible( LAYER_PROT_SELF_CKIN ) ) {
          NewTextChild( remNode, "layer_type", EncodeCompLayerType( cltProtBeforePay ) );
       }
       else {
       NewTextChild( remNode, "layer_type", EncodeCompLayerType( l->layer_type ) );
     }
   }
   }
   if ( !SeatTariff.empty() ) {
     xmlNodePtr n = NewTextChild( node, "tariff", SeatTariff.rate );
     SetProp( n, "color", SeatTariff.color );
     SetProp( n, "currency_id", SeatTariff.currency_id );
/*!!!     if ( !SeatTariff.RFISC.empty() ) {
       SetProp( n, "rfics", SeatTariff.RFISC );
     }*/
   }

   if ( !drawProps.emptyFlags() ) {
     xmlNodePtr n = NewTextChild( node, "drawProps" );
     for ( int i=0; i<dpTypeNum; i++ ) {
       if ( !drawProps.isFlag( (TDrawPropsType)i ) ) {
         continue;
       }
       remNode = NewTextChild( n, "drawProp" );
       TDrawPropInfo pinfo = getDrawProps( (TDrawPropsType)i );
       SetProp( remNode, "figure", pinfo.figure );
       SetProp( remNode, "color", pinfo.color );
       //props.setFlag( (TDrawPropsType)i );
     }
   }
   //read
   if ( pr_update )
     return;
   NewTextChild( node, "elem_type", elem_type );
   if ( xprior != -1 ) {
     NewTextChild( node, "xprior", xprior );
   }
   if ( yprior != -1 ) {
     NewTextChild( node, "yprior", yprior );
   }
   if ( agle ) {
     NewTextChild( node, "agle", agle );
   }
   NewTextChild( node, "class", clname );
   NewTextChild( node, "xname", denorm_iata_line( xname, pr_lat_seat ) );
   NewTextChild( node, "yname", denorm_iata_row( yname ) );
}

void TSalons::BuildLayersInfo( xmlNodePtr salonsNode,
                               const BitSet<TDrawPropsType> &props )
{
  int max_priority = -1;
  int id = 0;
  xmlNodePtr editNode = GetNode( "layers_prop", salonsNode );
  if ( editNode ) {
    ProgTrace( TRACE5, "TSalons::BuildLayersInfo - recreate" );
    xmlUnlinkNode( editNode );
    xmlFreeNode( editNode );
  }
  else
    ProgTrace( TRACE5, "TSalons::BuildLayersInfo - create" );
  editNode = NewTextChild( salonsNode, "layers_prop" );
  TReqInfo *r = TReqInfo::Instance();
  for( std::map<ASTRA::TCompLayerType,TMenuLayer>::iterator i=menuLayers.begin(); i!=menuLayers.end(); i++ ) {
    if ( !compatibleLayer( i->first ) )
      continue;
    if ( readStyle == rComponSalons &&
         !isBaseLayer( i->first, readStyle == rComponSalons ) )
      continue;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( !BASIC_SALONS::TCompLayerTypes::Instance()->getElem( i->first, layer_elem ) )
      continue;
    xmlNodePtr n = NewTextChild( editNode, "layer", EncodeCompLayerType( i->first ) );
    SetProp( n, "id", id );
    SetProp( n, "name", i->second.name_view );
    int layer_priority = BASIC_SALONS::TCompLayerTypes::Instance()->priority( i->first );
    SetProp( n, "priority", layer_priority );
    if ( max_priority < layer_priority )
      max_priority = layer_priority;
    if ( i->second.editable ) { // ���� �� �஢���� �� �ࠢ� ।���஢���� ⮣� ��� ����� ᫮�
      bool pr_edit = true;
      if ( (i->first == cltBlockTrzt || i->first == cltProtTrzt )&&
           !r->user.access.check_profile(trip_id, 430) )
        pr_edit = false;
      if ( i->first == cltBlockCent &&
           !r->user.access.check_profile(trip_id, 420) )
        pr_edit = false;
      if ( (i->first == cltUncomfort || i->first == cltProtect || i->first == cltSmoke) &&
           !r->user.access.check_profile(trip_id, 410) )
        pr_edit = false;
      if ( i->first == cltDisable &&
           !r->user.access.check_profile(trip_id, 425) ) {
        pr_edit = false;
      }
      if ( pr_edit ) {
        SetProp( n, "edit", 1 );
        if ( isBaseLayer( i->first, readStyle == rComponSalons ) )
          SetProp( n, "base_edit", 1 );
      }
    }
    if ( i->second.notfree )
        SetProp( n, "notfree", 1 );
    if ( !i->second.name_view.empty() ) {
        SetProp( n, "name_view_help", i->second.name_view );
        if ( !i->second.func_key.empty() )
            SetProp( n, "func_key", i->second.func_key );
    }
   id++;
  }
  xmlNodePtr n = NewTextChild( editNode, "layer",  EncodeCompLayerType( cltUnknown ) );
  SetProp( n, "id", id );
  SetProp( n, "name", "LAYER_CLEAR_ALL" );
  SetProp( n, "priority", 10000 );
  SetProp( n, "edit", 1 );
  SetProp( n, "name_view_help", AstraLocale::getLocaleText("������ �� ������ ����") );
  SetProp( n, "func_key", "Shift+F8" );
  xmlNodePtr propNode = NewTextChild( salonsNode, "draw_props" );
  max_priority++;
  id++;
//!log  ProgTrace( TRACE5, "max_priority=%d", max_priority );
  for ( int i=0; i<dpTypeNum; i++ ) {
    if ( !props.isFlag( (TDrawPropsType)i ) )
      continue;
    TDrawPropInfo p = getDrawProps( (TDrawPropsType)i );
//!log    ProgTrace( TRACE5, "priority=%d, name=%s", max_priority, p.name.c_str() );
    n = NewTextChild( propNode, "draw_item", p.name );
    SetProp( n, "id", id );
    SetProp( n, "figure", p.figure );
    SetProp( n, "color", p.color );
    SetProp( n, "priority", max_priority );
    max_priority++;
  }
}

TSalons::TSalons()
{
  readStyle = rTripSalons;
  comp_id = ASTRA::NoExists;
  trip_id = ASTRA::NoExists;
  pr_owner = false;
  FCurrPlaceList = NULL;
}

TSalons::TSalons( int id, TReadStyle vreadStyle )
{
  pr_owner = false;
    readStyle = vreadStyle;
    if ( readStyle == rComponSalons )
        comp_id = id;
    else
      trip_id = id;
    pr_lat_seat = false;
  FCurrPlaceList = NULL;

  if ( readStyle == rTripSalons ) {
    std::vector<TTripRouteItem> routes;
    FilterLayers.getFilterLayersOnTranzitRoutes( trip_id, false, routes ); //prior version
  }
  getMenuLayers( readStyle == rTripSalons,
                 FilterLayers,
                 menuLayers );
}

TSalons::~TSalons()
{
  Clear( );
}

TPlaceList *TSalons::CurrPlaceList()
{
  return FCurrPlaceList;
}

void TSalons::SetCurrPlaceList( TPlaceList *newPlaceList )
{
  FCurrPlaceList = newPlaceList;
}

void TSalons::Build( xmlNodePtr salonsNode )
{
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  BitSet<TDrawPropsType> props;
    SetProp( salonsNode, "pr_lat_seat", pr_lat_seat );
  for( vector<TPlaceList*>::iterator placeList = placelists.begin();
       placeList != placelists.end(); placeList++ ) {
    xmlNodePtr placeListNode = NewTextChild( salonsNode, "placelist" );
    SetProp( placeListNode, "num", (*placeList)->num );
    int xcount=0, ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      if ( !forBuild( *place, elem_types ) ) {
        continue;
      }
      if ( place->x > xcount )
        xcount = place->x;
      if ( place->y > ycount )
        ycount = place->y;
      place->Build( NewTextChild( placeListNode, "place" ), pr_lat_seat, false );
      props += place->drawProps;
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
    BuildLayersInfo( salonsNode, props );
}

void TSalons::Write( const TComponSets &compSets )
{
  if ( readStyle == rTripSalons )
    ProgTrace( TRACE5, "TSalons::Write TripSalons with params trip_id=%d",
               trip_id );
  else {
    if ( compSets.modify == mNone )
      return;  //???
    FilterClass.clear();
    ProgTrace( TRACE5, "TSalons::Write ComponSalons with params comp_id=%d",
               comp_id );
  }
  TQuery Qry( &OraSession );
  TQuery QryLayers( &OraSession );
  QryLayers.SQLText =
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create) "
    "  VALUES "
    "    (:range_id,:point_id,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:crs_pax_id,:pax_id,system.UTCSYSDATE); "
    "END; ";
  QryLayers.CreateVariable( "range_id", otInteger, FNull );
  QryLayers.CreateVariable( "point_id", otInteger, trip_id );
  QryLayers.CreateVariable( "crs_pax_id", otInteger, FNull );
  QryLayers.CreateVariable( "pax_id", otInteger, FNull );
  QryLayers.DeclareVariable( "layer_type", otString );
  QryLayers.DeclareVariable( "first_xname", otString );
  QryLayers.DeclareVariable( "last_xname", otString );
  QryLayers.DeclareVariable( "first_yname", otString );
  QryLayers.DeclareVariable( "last_yname", otString );
  if ( readStyle == rTripSalons ) {
    TFlights flights;
        flights.Get( trip_id, ftTranzit );
        flights.Lock(__FUNCTION__);
    Qry.SQLText = "BEGIN "\
                  " UPDATE trip_sets SET pr_lat_seat=:pr_lat_seat WHERE point_id=:point_id; "
                  " DELETE trip_comp_rem WHERE point_id=:point_id; "
                  " DELETE trip_comp_baselayers WHERE point_id=:point_id; "
                  " DELETE trip_comp_rates WHERE point_id=:point_id; "
                  " DELETE trip_comp_rfisc WHERE point_id=:point_id; "
                  " DELETE trip_comp_elems WHERE point_id=:point_id; "
                  "END;";

    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText =
      "DELETE trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type=:layer_type";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.DeclareVariable( "layer_type", otString );
    for( map<ASTRA::TCompLayerType,TMenuLayer>::iterator i=menuLayers.begin(); i!=menuLayers.end(); i++ ) {
        if ( i->second.editable ) {
            Qry.SetVariable( "layer_type", EncodeCompLayerType( i->first ) );
            Qry.Execute();
        }
    }
  }
  else { /* ��࠭���� ���������� */
    if ( compSets.modify == mAdd ) {
      Qry.Clear();
      Qry.SQLText = "SELECT id__seq.nextval as comp_id FROM dual";
      Qry.Execute();
      comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    Qry.Clear();
    switch ( (int)compSets.modify ) {
      case mChange:
         Qry.SQLText = "BEGIN "\
                       " UPDATE comps SET airline=:airline,airp=:airp,craft=:craft,bort=:bort,descr=:descr, "
                       "        time_create=system.UTCSYSDATE,classes=:classes,pr_lat_seat=:pr_lat_seat "
                       "  WHERE comp_id=:comp_id; "
                       " DELETE comp_rem WHERE comp_id=:comp_id; "
                       " DELETE comp_baselayers WHERE comp_id=:comp_id; "
                       " DELETE comp_rates WHERE comp_id=:comp_id; "
                       " DELETE comp_elems WHERE comp_id=:comp_id; "
                       " DELETE comp_classes WHERE comp_id=:comp_id; "
                       "END; ";
         break;
      case mAdd:
         Qry.SQLText = "INSERT INTO comps(comp_id,airline,airp,craft,bort,descr,time_create,classes,pr_lat_seat) "
                       " VALUES(:comp_id,:airline,:airp,:craft,:bort,:descr,system.UTCSYSDATE,:classes,:pr_lat_seat) ";
         break;
      case mDelete:
         Qry.SQLText = "BEGIN "
                       " UPDATE trip_sets SET comp_id=NULL WHERE comp_id=:comp_id; "
                       " DELETE comp_rem WHERE comp_id=:comp_id; "
                       " DELETE comp_baselayers WHERE comp_id=:comp_id; "
                       " DELETE comp_rates WHERE comp_id=:comp_id; "
                       " DELETE comp_elems WHERE comp_id=:comp_id; "
                       " DELETE comp_sections WHERE comp_id=:comp_id; "
                       " DELETE comp_classes WHERE comp_id=:comp_id; "
                       " DELETE comps WHERE comp_id=:comp_id; "
                       "END; ";
         break;
    }
    Qry.DeclareVariable( "comp_id", otInteger );
    Qry.SetVariable( "comp_id", comp_id );
    if ( compSets.modify != mDelete ) {
      Qry.CreateVariable( "airline", otString, compSets.airline );
      Qry.CreateVariable( "airp", otString, compSets.airp );
      Qry.CreateVariable( "craft", otString, compSets.craft );
      Qry.CreateVariable( "descr", otString, compSets.descr );
      Qry.CreateVariable( "bort", otString, compSets.bort );
      Qry.CreateVariable( "classes", otString, compSets.classes );
      Qry.CreateVariable( "pr_lat_seat", otString, pr_lat_seat );
    }
    Qry.Execute();
  }
  if ( readStyle == rComponSalons && compSets.modify == mDelete )
    return; /* 㤠���� ���������� */

  TQuery QryWebTariff( &OraSession );
  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
      " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
    LQry.SQLText =
      "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
      " VALUES(:point_id,:num,:x,:y,:layer_type)";
    LQry.CreateVariable( "point_id", otInteger, trip_id );
    QryWebTariff.SQLText =
      "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
      " VALUES(:point_id,:num,:x,:y,:color,:rate,:rate_cur)";
    QryWebTariff.CreateVariable( "point_id", otInteger, trip_id );
  }
  else {
    RQry.SQLText = "INSERT INTO comp_rem(comp_id,num,x,y,rem,pr_denial) "
                   " VALUES(:comp_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
    LQry.SQLText =
      "INSERT INTO comp_baselayers(comp_id,num,x,y,layer_type) "
      " VALUES(:comp_id,:num,:x,:y,:layer_type)";
    LQry.CreateVariable( "comp_id", otInteger, comp_id );
    QryWebTariff.SQLText =
      "INSERT INTO comp_rates(comp_id,num,x,y,color,rate,rate_cur) "
      " VALUES(:comp_id,:num,:x,:y,:color,:rate,:rate_cur)";
    QryWebTariff.CreateVariable( "comp_id", otInteger, comp_id );
  }

  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "x", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "rem", otString );
  RQry.DeclareVariable( "pr_denial", otInteger );

  QryWebTariff.DeclareVariable( "num", otInteger );
  QryWebTariff.DeclareVariable( "x", otInteger );
  QryWebTariff.DeclareVariable( "y", otInteger );
  QryWebTariff.DeclareVariable( "color", otString );
  QryWebTariff.DeclareVariable( "rate", otFloat );
  QryWebTariff.DeclareVariable( "rate_cur", otString );

  LQry.DeclareVariable( "num", otInteger );
  LQry.DeclareVariable( "x", otInteger );
  LQry.DeclareVariable( "y", otInteger );
  LQry.DeclareVariable( "layer_type", otString );

  Qry.Clear();
  if ( readStyle == rTripSalons ) {
    Qry.SQLText =
      "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
      " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, :xname,:yname)";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
  }
  else {
    Qry.SQLText =
      "INSERT INTO comp_elems(comp_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
      " VALUES(:comp_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class,:xname,:yname) ";
    Qry.DeclareVariable( "comp_id", otInteger );
    Qry.SetVariable( "comp_id", comp_id );
  }
  Qry.DeclareVariable( "num", otInteger );
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );
  Qry.DeclareVariable( "elem_type", otString );
  Qry.DeclareVariable( "xprior", otInteger );
  Qry.DeclareVariable( "yprior", otInteger );
  Qry.DeclareVariable( "agle", otInteger );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );

  vector<TPlaceList*>::iterator plist;
  map<TClass,int> countersClass;
  TClass cl;

  for ( plist = placelists.begin(); plist != placelists.end(); plist++ ) {
    Qry.SetVariable( "num", (*plist)->num );
    RQry.SetVariable( "num", (*plist)->num );
    LQry.SetVariable( "num", (*plist)->num );
    for ( TPlaces::iterator place = (*plist)->places.begin(); place != (*plist)->places.end(); place++ ) {
      if ( !place->visible )
       continue;
      Qry.SetVariable( "x", place->x );
      Qry.SetVariable( "y", place->y );
      Qry.SetVariable( "elem_type", place->elem_type );
      if ( place->xprior == -1 )
        Qry.SetVariable( "xprior", FNull );
      else
        Qry.SetVariable( "xprior", place->xprior );
      if ( place->yprior == -1 )
        Qry.SetVariable( "yprior", FNull );
      else
        Qry.SetVariable( "yprior", place->yprior );
      Qry.SetVariable( "agle", place->agle );
      if ( place->clname.empty() || !TCompElemTypes::Instance()->isSeat( place->elem_type ) )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", place->clname );
        cl = DecodeClass( place->clname.c_str() );
        if ( cl != NoClass )
          countersClass[ cl ] = countersClass[ cl ] + 1;
      }
      Qry.SetVariable( "xname", place->xname );
      Qry.SetVariable( "yname", place->yname );
      Qry.Execute();
      if ( !place->rems.empty() ) {
        RQry.SetVariable( "x", place->x );
        RQry.SetVariable( "y", place->y );
        for( vector<TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
          RQry.SetVariable( "rem", rem->rem );
          if ( !rem->pr_denial )
            RQry.SetVariable( "pr_denial", 0 );
          else
            RQry.SetVariable( "pr_denial", 1 );
          RQry.Execute();
        }
      }
      if ( !place->layers.empty() ) {
        QryLayers.SetVariable( "first_xname", place->xname );
        QryLayers.SetVariable( "last_xname", place->xname );
        QryLayers.SetVariable( "first_yname", place->yname );
        QryLayers.SetVariable( "last_yname", place->yname );
        for ( vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
//!log      	  ProgTrace( TRACE5, "write layers_priority.empty()=%d, layer=%s, editable=%d",
//!log                     menuLayers.empty(), EncodeCompLayerType( l->layer_type ), menuLayers[ l->layer_type ].editable );
          if ( !menuLayers[ l->layer_type ].editable )
            continue;
              if ( isBaseLayer( l->layer_type, readStyle == rComponSalons ) ) {
            LQry.SetVariable( "x", place->x );
            LQry.SetVariable( "y", place->y );
            LQry.SetVariable( "layer_type", EncodeCompLayerType( l->layer_type ) );
            ProgTrace( TRACE5, "(%d,%d)=%s", place->x, place->y, EncodeCompLayerType( l->layer_type ) );
            LQry.Execute();
                continue;
          }
              QryLayers.SetVariable( "layer_type", EncodeCompLayerType( l->layer_type ) );
              QryLayers.Execute();
        }
      }
      if ( !place->SeatTariff.empty() ) {
        QryWebTariff.SetVariable( "num", (*plist)->num );
        QryWebTariff.SetVariable( "x", place->x );
        QryWebTariff.SetVariable( "y", place->y );
        QryWebTariff.SetVariable( "color", place->SeatTariff.color );
        QryWebTariff.SetVariable( "rate", place->SeatTariff.rate );
        QryWebTariff.SetVariable( "rate_cur", place->SeatTariff.currency_id );
        QryWebTariff.Execute();
      }
    } //for place
  }
  // ��࠭塞 ���䨣���� ����
  if ( readStyle != rTripSalons ) {
    Qry.Clear();
    Qry.SQLText =
     "INSERT INTO comp_classes(comp_id,class,cfg) VALUES(:comp_id,:class,:cfg)";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.DeclareVariable( "class", otString );
    Qry.DeclareVariable( "cfg", otInteger );
    for ( map<TClass,int>::iterator i=countersClass.begin(); i!=countersClass.end(); i++ ) {
      if ( i->second > 999 )
        throw UserException( "MSG.SALONS.MATCH_PLACES" );
      Qry.SetVariable( "class", EncodeClass( i->first ) );
      Qry.SetVariable( "cfg", i->second );
      Qry.Execute();
    }
  }
}

struct TPaxLayer {
    ASTRA::TCompLayerType layer_type;
    TDateTime time_create;
    int priority;
    int valid; // 0 - ok
    vector<TSalonPoint>	places;
    TPaxLayer( ASTRA::TCompLayerType vlayer_type, TDateTime vtime_create, int vpriority, TSalonPoint p ) {
        priority = vpriority;
        layer_type = vlayer_type;
        time_create = vtime_create;
        valid = 0;
        places.push_back( p );
    }
};

struct TPaxLayerRec {
    unsigned int seats;
    string cl;
    vector<TPaxLayer> paxLayers;
    TPaxLayerRec() {
    seats = 0;
    }
  void AddLayer( TPaxLayer paxLayer ) {
    std::vector<TPaxLayer>::iterator i;
    for (i=paxLayers.begin(); i!=paxLayers.end(); i++) {
      if ( paxLayer.priority < i->priority ||
             ( paxLayer.priority == i->priority &&
               paxLayer.time_create > i->time_create ) )
        break;
    }
    paxLayers.insert( i, paxLayer );

  };
};

typedef map< int, TPaxLayerRec > TPaxLayers;

void SetValidPaxLayer( TPaxLayers::iterator ipax, vector<TPaxLayer>::iterator ivalid_pax_layer )
{
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // �஡�� �� ᫮� ���ᠦ��
    if ( ivalid_pax_layer != ipax_layer ) {
      if ( ipax_layer->valid != -1 ) {
        ipax_layer->valid = -1;
        ProgTrace( TRACE5, "SetValidPaxLayer: pax_id=%d, layer_type=%s -- not ok!", ipax->first, EncodeCompLayerType( ipax_layer->layer_type ) );
      }
    }
  }
}

void ClearInvalidPaxLayers( TSalons *CSalon, TPaxLayers::iterator ipax )
{
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // �஡�� �� ᫮� ���ᠦ��
    if ( ipax_layer->valid != -1 ) {
      continue;
    }
    for( vector<TSalonPoint>::iterator icoord=ipax_layer->places.begin(); icoord!=ipax_layer->places.end(); icoord++ ) { // �஡�� �� ���⠬
      for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {  // �஡�� �� ᠫ����
          if ( (*it)->num == icoord->num ) {
              TPlace* place = (*it)->place( (*it)->GetPlaceIndex( icoord->x, icoord->y ) );
              for ( vector<TPlaceLayer>::iterator iplace_layer=place->layers.begin(); iplace_layer!=place->layers.end(); iplace_layer++ ) { // �஡�� �� ᫮� ����
          if ( iplace_layer->layer_type == ipax_layer->layer_type &&
                 iplace_layer->time_create == ipax_layer->time_create &&
               iplace_layer->pax_id == ipax->first ) {
            place->layers.erase( iplace_layer );
            break;
          }
        }
        break;
          }
      }
    }
  }
}

void GetValidPaxLayer( TSalons *CSalon, TPaxLayers &pax_layers, TPaxLayers::iterator ipax )
{
//  ProgTrace( TRACE5, "GetValidPaxLayer1: pax_id=%d", ipax->first );
  TPlace *place = 0;
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // �஡�� �� ᫮� ���ᠦ��
    int vfirst_x = NoExists;
    int vfirst_y = NoExists;
    int vlast_x = NoExists;
    int vlast_y = NoExists;
  //  ProgTrace( TRACE5, "GetValidPaxLayer2: layer_type=%s", EncodeCompLayerType( ipax_layer->layer_type ) );
    if ( ipax_layer->places.size() != ipax->second.seats )
      ipax_layer->valid = -1;
    if ( ipax_layer->valid == -1 ) {
      continue;
    }
    for( vector<TSalonPoint>::iterator icoord=ipax_layer->places.begin(); icoord!=ipax_layer->places.end(); icoord++ ) { // �஡�� �� ���⠬ ᫮�
      for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {
            if ( (*it)->num == icoord->num ) {
                place = (*it)->place( (*it)->GetPlaceIndex( icoord->x, icoord->y ) );
          if ( ipax->second.cl != place->clname || !place->isplace || ipax_layer->places.begin()->num != place->num ) {
            ipax_layer->valid = -1;
          }
                break;
            }
      }
      if ( ipax_layer->valid == -1 ) {
        break;
      }
    //  ProgTrace( TRACE5, "GetValidPaxLayer3: seat_no=%s", string( place->yname+place->xname).c_str() );
      for ( vector<TPlaceLayer>::iterator iplace_layer=place->layers.begin(); iplace_layer!=place->layers.end(); iplace_layer++ ) { // �஡�� �� ᫮� ����
      //  ProgTrace( TRACE5, "GetValidPaxLayer4: seat layer_type=%s, pax_id=%d", EncodeCompLayerType( iplace_layer->layer_type ), iplace_layer->pax_id );
          if ( iplace_layer->layer_type == ipax_layer->layer_type &&
               iplace_layer->time_create == ipax_layer->time_create &&
             iplace_layer->pax_id == ipax->first ) // �� ��� ᫮� - � �� ᠬ� �ਮ����
              break;
          if ( iplace_layer->pax_id <= 0 ) { // ᫮� ����� �ਮ���� � �� �� �ਭ������� ���ᠦ���
          ipax_layer->valid = -1;
          break;
        }
        //���� ����� �ਮ���� ᫮� �ਭ������騩 ��㣮�� ���ᠦ���
        // ��室�� �⮣� ���ᠦ�� � �஡����� �� ��� ᫮� ��� ���᭥��� ���� ᫮��
        TPaxLayers::iterator inext_pax = pax_layers.find( iplace_layer->pax_id );
        if ( inext_pax == pax_layers.end() ) {
          //!logProgTrace( TRACE5, "GetValidPaxLayerError, iplace_layer->pax_id=%d", iplace_layer->pax_id );
          throw EXCEPTIONS::Exception( "not found iplace_layer->pax_id" );;
        }
        GetValidPaxLayer( CSalon, pax_layers, inext_pax );
        // �஢��塞 ᫮� � ���� ᠫ��� �� �, �� �� ��訩 � ���ᠦ��
        for ( vector<TPaxLayer>::iterator jpax_layer=inext_pax->second.paxLayers.begin(); jpax_layer!=inext_pax->second.paxLayers.end(); jpax_layer++ ) { // �஡�� �� ᫮� ���ᠦ��
            if ( iplace_layer->layer_type == jpax_layer->layer_type &&
                 iplace_layer->time_create == jpax_layer->time_create &&
               iplace_layer->pax_id == inext_pax->first ) {
            if ( jpax_layer->valid != -1 ) {
              ipax_layer->valid = -1;
        //      ProgTrace( TRACE5, "GetValidPaxLayer5: invalid layer_type=%s, pax_id=%d", EncodeCompLayerType( ipax_layer->layer_type ), ipax->first );
            }
                break;
          }
        }
        if ( ipax_layer->valid == -1 )
          break;
      }
      if ( ipax_layer->valid == -1 )
        break;

      if ( vfirst_x == NoExists || vfirst_y == NoExists ||
        vfirst_y*1000+vfirst_x > place->y*1000+place->x ) {
        vfirst_x = place->x;
        vfirst_y = place->y;
      }
      if ( vlast_x == NoExists || vlast_y == NoExists ||
        vlast_y*1000+vlast_x<place->y*1000+place->x ) {
        vlast_x=place->x;
        vlast_y=place->y;
      }
    }  // ����� �஡��� �� ���⠬
    if ( ipax_layer->valid == -1 ) {
      continue;
    }
    if ( !( ( vfirst_x == vlast_x && vfirst_y+(int)ipax_layer->places.size()-1 == vlast_y ) ||
            ( vfirst_y == vlast_y && vfirst_x+(int)ipax_layer->places.size()-1 == vlast_x ) ) ) {
      ipax_layer->valid = -1;
      continue;
    }
    // �᫨ �� �����, � ᫮� ��訩. ���� ������� �� ��⠫�� ᫮� � ���ᠦ�� ��� ���娥
    //!logProgTrace( TRACE5, "GetValidPaxLayer5: ipax->pax_id=%d, layer_type=%s -- ok", ipax->first, EncodeCompLayerType( ipax_layer->layer_type ) );
    SetValidPaxLayer( ipax, ipax_layer );
    break;
  }
}

/*
  1. �����⪠ "������㯭� ����" �� ���뢠���� � ⠡��� trip_comp_ranges
  �ਣ���:
  BEFORE INSERT OR UPDATE OR DELETE
  OF point_id, num, x, y, xname, yname, class
  ON trip_comp_elems
  2. trip_comp_ranges - ������ ࠧ���� �⭮�⥫쭮 point_id, ��� �㦭� ࠧ��⪠ point_dep, point_arv

  ���� ��� ��魮��:
  1. �����⪠ ᫮� � �㭪� ��ᠤ�� point_id � 㪠������ ��砫� � ����砭�� ����⢨� �� �������� - trip_comp_layers
  2. �����⪠ ᫮� � �㭪� ��ᠤ�� point_id ��� ��� ������� - trip_comp_ranges



*/

void TSalonList::Clear()
{
  for ( vector<TPlaceList*>::iterator i=begin(); i!=end(); i++ ) {
    delete *i;
  }
  clear();
}

void TSalonList::ReadSeats( TQuery &Qry, const string &FilterClass )
{
  Clear();
  pax_lists.clear();
  string ClassName = ""; /* ����᫥��� ��� ����ᮢ, ����� ���� � ᠫ��� */
  TPlaceList *placeList = NULL;
  int num = -1;
  TPoint point_p;
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_elem_type = Qry.FieldIndex( "elem_type" );
  int col_xprior = Qry.FieldIndex( "xprior" );
  int col_yprior = Qry.FieldIndex( "yprior" );
  int col_agle = Qry.FieldIndex( "agle" );
  int col_xname = Qry.FieldIndex( "xname" );
  int col_yname = Qry.FieldIndex( "yname" );
  int col_class = Qry.FieldIndex( "class" );
  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( num != Qry.FieldAsInteger( col_num ) ) { //���� ᠫ��
      if ( placeList && !FilterClass.empty() && ClassName.find( FilterClass ) == string::npos ) {
        //���� ᠫ�� � ����� 䨫�� �� ����ᠬ � �� �� ��� �����
        placeList->clearSeats();
      }
      else {
        //���室�� �� ���� ᠫ��
        placeList = new TPlaceList();
        push_back( placeList );
      }
      ClassName.clear();
      num = Qry.FieldAsInteger( col_num );
      placeList->num = num;
    }
    // ����७�� ����! - ࠧ�� ᫮�
    TPlace place;
    point_p.x = Qry.FieldAsInteger( col_x );
    point_p.y = Qry.FieldAsInteger( col_y );
    // �᫨ ���� �� �� ��।����� ��� ���� ����, �� �� �ந��樠����஢���
    if ( !placeList->ValidPlace( point_p ) || placeList->place( point_p )->x == -1 ) {
        place.x = point_p.x;
        place.y = point_p.y;
        place.num = num;
      place.elem_type = Qry.FieldAsString( col_elem_type );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( Qry.FieldIsNULL( col_xprior ) )
        place.xprior = -1;
      else
        place.xprior = Qry.FieldAsInteger( col_xprior );
      if ( Qry.FieldIsNULL( col_yprior ) )
        place.yprior = -1;
      else
        place.yprior = Qry.FieldAsInteger( col_yprior );
      place.agle = Qry.FieldAsInteger( col_agle );
      place.clname = Qry.FieldAsString( col_class );
      place.xname = Qry.FieldAsString( col_xname );
      place.yname = Qry.FieldAsString( col_yname );
      if ( ClassName.find( Qry.FieldAsString( col_class ) ) == string::npos )
        ClassName += Qry.FieldAsString( col_class );
    }
    else { // �� ���� �ந��樠����஢��� - �� ���� ᫮�
      throw EXCEPTIONS::Exception( "Read trip_comp_elems: doublicate coord: x=%d, y=%d", point_p.x, point_p.y );
    }
    place.visible = true;
    placeList->Add( place );
  }	/* end for */
  if ( placeList && !FilterClass.empty() && ClassName.find( FilterClass ) == string::npos ) {
    ProgTrace( TRACE5, "Read trip_comp_elems: delete empty placeList->num=%d", placeList->num );
    pop_back( );
    delete placeList; // ��� ��� �����/ᠫ�� �� �㦥�
  }
}

inline bool TSalonList::findSeat( std::map<int,TPlaceList*> &salons, TPlaceList** placelist,
                                  const TSalonPoint &point_s )
{
  *placelist = NULL;
  if ( salons.find( point_s.num ) != salons.end() ) {
    *placelist = salons[ point_s.num ];
  }
  else {
    for ( std::vector<TPlaceList*>::iterator isalon=begin(); isalon!=end(); isalon++ ) {
      if ( (*isalon)->num == point_s.num ) {
        *placelist = *isalon;
        salons[ point_s.num ] = *placelist;
        break;
      }
    }
  }
  return ( *placelist != NULL );
}

void TSalonList::ReadRemarks( TQuery &Qry, FilterRoutesProperty &filterRoutes,
                              int prior_compon_props_point_id )
{
  int col_point_id = Qry.GetFieldIndex( "point_id" );
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_rem = Qry.FieldIndex( "rem" );
  int col_pr_denial = Qry.FieldIndex( "pr_denial" );
  map<int,TPlaceList*> salons; // ��� ����ன ����樨 � ᠫ���
  for ( ; !Qry.Eof; Qry.Next() ) {
    TPlaceList* placelist = NULL;
    TSalonPoint point_s;
    point_s.num = Qry.FieldAsInteger( col_num );
    point_s.x = Qry.FieldAsInteger( col_x );
    point_s.y = Qry.FieldAsInteger( col_y );
    if ( !findSeat( salons, &placelist, point_s ) ) {
      if ( filterSets.filterClass.empty() ) {
        ProgError( STDLOG, "TSalonList::ReadRemarks: placelist not found num=%d", point_s.num );
      }
      continue;
    }
    //��諨 �㦭� ᠫ��
    TPoint seat_p( point_s.x, point_s.y );
    if ( !placelist->ValidPlace( seat_p ) ) {
      //ProgError( STDLOG, "TSalonList::ReadRemarks: seat not found num=%d, x=%d, y=%d", point_s.num, point_s.x, point_s.y );
      continue;
    }
    TSeatRemark remark;
    remark.value = Qry.FieldAsString( col_rem );
    remark.pr_denial = Qry.FieldAsInteger( col_pr_denial );
    if ( col_point_id >= 0 && !filterRoutes.useRouteProperty( Qry.FieldAsInteger( col_point_id ) ) ) {
      continue;
    }
    if ( col_point_id >= 0 ) {
      if ( prior_compon_props_point_id != ASTRA::NoExists ) {
        placelist->place( seat_p )->AddRemark( prior_compon_props_point_id, remark );
      }
      else {
        placelist->place( seat_p )->AddRemark( Qry.FieldAsInteger( col_point_id ), remark );
      }
    }
    else {
      placelist->place( seat_p )->AddRemark( NoExists, remark );
    }
  //!log  ProgTrace( TRACE5, "TSalonList::ReadRemarks: AddRemark(%d,%d): value=%s, pr_denial=%d",
//!log               placelist->place( seat_p )->x, placelist->place( seat_p )->y, remark.value.c_str(), remark.pr_denial );
  }
}

// pax_list - ������ ���� �������� � �⮬� �������
void TSalonList::ReadLayers( TQuery &Qry, FilterRoutesProperty &filterRoutes,
                             TFilterLayers &filterLayers, TPaxList &pax_list,
                             int prior_compon_props_point_id )
{
  int col_point_id = Qry.GetFieldIndex( "point_id" );
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_layer_type = Qry.FieldIndex( "layer_type" );
  int col_time_create = Qry.GetFieldIndex( "time_create" );
  int col_pax_id = Qry.GetFieldIndex( "pax_id" );
  int col_crs_pax_id = Qry.GetFieldIndex( "crs_pax_id" );
  int col_point_dep = Qry.GetFieldIndex( "point_dep" );
  int col_point_arv = Qry.GetFieldIndex( "point_arv" );
  int idx_first_xname = Qry.GetFieldIndex( "first_xname" );
  int idx_first_yname = Qry.GetFieldIndex( "first_yname" );
  int idx_last_xname = Qry.GetFieldIndex( "last_xname" );
  int idx_last_yname = Qry.GetFieldIndex( "last_yname" );
  map<int,TPlaceList*> salons; // ��� ����ன ����樨 � ᠫ���
  for ( ; !Qry.Eof; Qry.Next() ) {
    TSeatLayer layer;
        if ( col_point_id < 0 || Qry.FieldIsNULL( col_point_id ) )
      layer.point_id = NoExists;
    else {
      layer.point_id = Qry.FieldAsInteger( col_point_id );
    }
        if ( col_point_dep < 0 ) {
      layer.point_dep = NoExists;
    }
    else {
      if ( Qry.FieldIsNULL( col_point_dep ) ) { // ��� �����⬠ ��易⥫쭮 ������ ���� ����� point_dep
        layer.point_dep = layer.point_id;
      }
      else {
        layer.point_dep = Qry.FieldAsInteger( col_point_dep );
      }
    }
        if ( col_point_arv < 0 || Qry.FieldIsNULL( col_point_arv ) )
      layer.point_arv = NoExists;
    else {
      layer.point_arv = Qry.FieldAsInteger( col_point_arv );
    }
    if ( col_pax_id < 0 || Qry.FieldIsNULL( col_pax_id ) )
      layer.pax_id = NoExists;
    else
      layer.pax_id = Qry.FieldAsInteger( col_pax_id );
    if ( col_crs_pax_id < 0 || Qry.FieldIsNULL( col_crs_pax_id ) )
      layer.crs_pax_id = NoExists;
    else
      layer.crs_pax_id = Qry.FieldAsInteger( col_crs_pax_id );
    layer.layer_type = DecodeCompLayerType( Qry.FieldAsString( col_layer_type ) );
    if ( col_time_create < 0 || Qry.FieldIsNULL( col_time_create ) )
      layer.time_create = NoExists;
    else
      layer.time_create = Qry.FieldAsDateTime( col_time_create );
    if ( col_point_id < 0 ||
         filterLayers.CanUseLayer( layer.layer_type,
                                   layer.point_dep,
                                   filterRoutes.getDepartureId(),
                                   filterRoutes.isTakeoff( layer.point_id ) ) ) { // ᫮� �㦭� ��������
      bool inRoute = ( col_point_id < 0 ||
                       layer.point_id == filterRoutes.getDepartureId() ||
                       filterRoutes.useRouteProperty( layer.point_dep, layer.point_arv ) );
      if ( prior_compon_props_point_id != ASTRA::NoExists ) { // �������
        if ( !inRoute ) {
          continue;
        }
        if ( layer.point_id != ASTRA::NoExists ) {
           layer.point_id = prior_compon_props_point_id;
        }
        if ( layer.point_dep != ASTRA::NoExists ) {
           layer.point_dep = prior_compon_props_point_id;
        }
        if ( layer.point_arv != ASTRA::NoExists ) { //???
           layer.point_arv = prior_compon_props_point_id;
        }
        layer.time_create = ASTRA::NoExists;
      }
      TPlaceList* placelist = NULL;
      TSalonPoint point_s;
      point_s.num = Qry.FieldAsInteger( col_num );
      point_s.x = Qry.FieldAsInteger( col_x );
      point_s.y = Qry.FieldAsInteger( col_y );
      TPoint seat_p( point_s.x, point_s.y );
      if ( layer.getPaxId() != NoExists &&
           pax_list.find( layer.getPaxId() ) != pax_list.end() ) { //᫮� �ਭ������� ���ᠦ���
        TSeatRange seatRange( TSeat(Qry.FieldAsString( idx_first_yname ),
                                    Qry.FieldAsString( idx_first_xname ) ),
                              TSeat(Qry.FieldAsString( idx_last_yname ),
                                    Qry.FieldAsString( idx_last_xname ) ) );
        std::map<TSeatLayer,TInvalidRange >::iterator iranges = pax_list[ layer.getPaxId() ].invalid_ranges.find( layer );
        if ( iranges != pax_list[ layer.getPaxId() ].invalid_ranges.end() ) {
          iranges->second.insert( seatRange );
        }
        else {
          TInvalidRange ranges;
          ranges.insert( seatRange );
          pax_list[ layer.getPaxId() ].invalid_ranges.insert( make_pair( layer, ranges ) );
        }
      }

      if ( Qry.FieldIsNULL( col_num ) ||
           !findSeat( salons, &placelist, point_s ) ||
           !placelist->ValidPlace( seat_p ) ) {
        ProgTrace( TRACE5, ">>>TSalonList::ReadLayers: seat not found num=%d, x=%d, y=%d %s",
                   point_s.num, point_s.x, point_s.y, layer.toString().c_str() );
        if ( layer.getPaxId() != NoExists &&
             pax_list.find( layer.getPaxId() ) != pax_list.end() ) {
          //!log tst();
          if ( pax_list[ layer.getPaxId() ].layers.find( layer ) != pax_list[ layer.getPaxId() ].layers.end() ) {
            pax_list[ layer.getPaxId() ].layers[ layer ].waitListReason = TWaitListReason();
          }
        }
        continue;
      }

//log      ProgTrace( TRACE5, "placelist=%p, point_s.num=%d, point_s.x=%d, point_s.y=%d %s",
//log                 placelist, point_s.num, point_s.x, point_s.y, layer.toString().c_str() );
      //��諨 �㦭� ᠫ��
      TPlace *place = placelist->place( seat_p );
      int id = layer.getPaxId();
      if ( id != NoExists ) { // ���� ᫮� ���ᠦ��
        if ( pax_list.find( id ) == pax_list.end() ) {
          ProgError( STDLOG, "TSalonList::ReadLayers: layer_type=%s, num=%d, x=%d, y=%d, but pass not found pax_id=%d",
                     BASIC_SALONS::TCompLayerTypes::Instance()->getCode( layer.layer_type ).c_str(),
                     point_s.num, point_s.x, point_s.y, id );
          continue;
        }
        /*�஢�ન �� �ਣ������� ᫮�
          1. ᮢ������� ����� + isSeat
          2. �� ���� ������ ᫮� � ����� ᠫ���
          3. �� ���� ������ ᫮� ��室���� �冷�
        */
        if ( pax_list[ id ].cl != place->clname || !place->isplace || !place->visible ) {
          if ( pax_list[ id ].layers.find( layer ) != pax_list[ id ].layers.end() ) {
            pax_list[ layer.getPaxId() ].layers[ layer ].waitListReason = TWaitListReason();
          }
          continue;
        }
        // �஡�� �� �ᥬ ���⠬ � ����� � ⥬ �� ⨯�� ᫮� ���ᠦ��
        std::set<SALONS2::TPlace*,SALONS2::CompareSeats> &layer_places = pax_list[ id ].layers[ layer ].seats;
        TWaitListReason &waitListReason = pax_list[ id ].layers[ layer ].waitListReason;
        if ( layer_places.empty() ) {
          waitListReason.layerStatus = layerVerify;
        }
        //!logProgTrace( TRACE5, "id=%d, layerStatus=%d", id, waitListReason.layerStatus );
        // �������� ����
        layer_places.insert( place );
        // ���� � ࠧ��� ᠫ����
        if ( (*layer_places.begin())->num != place->num ) {
          waitListReason.layerStatus = layerInvalid;
        }
        // �᫨ ��諨 �㦭�� ���-�� ����, � ᤥ���� �஢��� �� �, �� �� ���� �冷�
        if ( waitListReason.layerStatus == layerVerify && pax_list[ id ].seats == layer_places.size() ) {
          int first_x = NoExists;
          int first_y = NoExists;
          int last_x = NoExists;
          int last_y = NoExists;
          for ( std::set<TPlace*,CompareSeats>::iterator iseat = layer_places.begin(); iseat != layer_places.end(); iseat++ ) {
            //���� �������쭮� � ���ᨬ��쭮� ���न����
            if ( first_x == NoExists || first_y == NoExists ) {
              first_x = (*iseat)->x;
              first_y = (*iseat)->y;
              last_x = (*iseat)->x;
              last_y = (*iseat)->y;
            }
            if ( first_x > (*iseat)->x ) {
              first_x = (*iseat)->x;
            }
            if ( first_y > (*iseat)->y ) {
              first_y = (*iseat)->y;
            }
            if ( last_x < (*iseat)->x ) {
              last_x = (*iseat)->x;
            }
            if ( last_y < (*iseat)->y ) {
              last_y = (*iseat)->y;
            }
          }
          if ( layer_places.size() > 1 ) {
            ProgTrace( TRACE5, "SalonList::ReadLayers: invalid coord pax_id=%d, first_x=%d,last_x=%d,first_y=%d,last_y=%d, layer_places.size()=%zu",
                       id, first_x, last_x, first_y, last_y, layer_places.size() );
          }
          if ( !( ( first_x == last_x && first_y+(int)layer_places.size()-1 == last_y ) ||
                  ( first_y == last_y && first_x+(int)layer_places.size()-1 == last_x ) ) ) {
            ProgTrace( TRACE5, "SalonList::ReadLayers: invalid coord pax_id=%d", id );
            waitListReason.layerStatus = layerInvalid;
          }
          else
            waitListReason.layerStatus = layerMultiVerify;
        }
        // �᫨ �� �஢�ન ��諨, � ��諮�� �� ����
        if ( waitListReason.layerStatus == layerMultiVerify && pax_list[ id ].seats != layer_places.size() ) {
          waitListReason.layerStatus = layerInvalid;
        }
      } // end if id �᫨ ᫮� �ਭ������� ���ᠦ���
      //!logProgTrace( TRACE5, "id=%d", id );
      bool pr_add = true;
      if ( place->isCleanDoubleLayerType( layer.layer_type ) ) {
         pr_add = filterRoutes.useRouteProperty( layer.point_id, layer.point_arv ); //ࠧ��⪠ ������ ���� ����� �������
         //� ��� �� �� �������� ᫮� �ਧ���� ���ॣ����樨 �࠭���?
      }
      if ( pr_add ) {
        place->AddLayer( layer.point_id, layer ); //!!!����� ���஢�� point_id ��� addLayer
//log        ProgTrace( TRACE5, "TSalonList::ReadLayers:AddLayer %s",
//log                   layer.toString().c_str() );
      }
    }
  }
  for ( TPaxList::iterator ipax=pax_list.begin(); ipax!=pax_list.end(); ipax++ ) {
    for ( TLayersPax::iterator ilayer=ipax->second.layers.begin();
          ilayer!=ipax->second.layers.end(); ilayer++ ) {
      if ( ipax->second.seats != ilayer->second.seats.size() ) {
        ilayer->second.waitListReason.layerStatus = layerInvalid;
      }
      else {
        if ( ilayer->second.waitListReason.layerStatus == layerVerify ||
             ilayer->second.waitListReason.layerStatus == layerMultiVerify ) {
          ilayer->second.waitListReason.layerStatus = layerValid;
        }
      }
    }
  }
}

void TPaxList::InfantToSeatDrawProps()
{
  for ( std::map<int,TSalonPax>::iterator ipax=begin(); ipax!=end(); ipax++ ) {
    if ( !isSeatInfant( ipax->first ) ||
         ipax->second.layers.empty() )
      continue;
    for ( std::map<TSeatLayer,TPaxLayerSeats>::iterator ilayer=ipax->second.layers.begin();
          ilayer!=ipax->second.layers.end(); ilayer++ ) {
      if ( ilayer->second.waitListReason.layerStatus != layerValid ) {
        continue;
      }
      for ( std::set<TPlace*,CompareSeats>::iterator iseat=ilayer->second.seats.begin(); iseat!=ilayer->second.seats.end(); iseat++ ) {
        (*iseat)->drawProps.setFlag( dpInfantWoSeats );
        //!logProgTrace( TRACE5, "InfantToSeatDrawProps: %s, %s", string((*iseat)->yname+(*iseat)->xname).c_str(), ilayer->first.toString().c_str() );
      }
    }
  }
}

void TPaxList::TranzitToSeatDrawProps( int point_dep )
{
  for ( std::map<int,TSalonPax>::iterator ipax=begin(); ipax!=end(); ipax++ ) {
    if ( ipax->second.layers.empty() )
      continue;
    for ( std::map<TSeatLayer,TPaxLayerSeats>::iterator ilayer=ipax->second.layers.begin();
          ilayer!=ipax->second.layers.end(); ilayer++ ) {
      if ( ilayer->second.waitListReason.layerStatus != layerValid ) {
        continue;
      }
      if ( ilayer->first.point_id == point_dep ) {
        continue;
      }
      for ( std::set<TPlace*,CompareSeats>::iterator iseat=ilayer->second.seats.begin(); iseat!=ilayer->second.seats.end(); iseat++ ) {
        (*iseat)->drawProps.setFlag( dpTranzitSeats );
      }
    }
  }
}

void TLayersPax::dumpPaxLayers( const TSeatLayer &seatLayer, const TPaxLayerSeats &seats,
                                const TPlace *seat )
{
  string str;
  switch ( seats.waitListReason.layerStatus ) {
    case layerMultiVerify:
      str += "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!>>>>>layerMultiVerify";
      break;
    case layerInvalid:
      str += "Invalid";
      break;
    case layerLess:
      str += string("Less reason=") + seats.waitListReason.layer.toString();
      break;
    case layerNotRoute:
      str += string("NotRoute reason=") + seats.waitListReason.layer.toString();
      break;
    case layerVerify:
      str += "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!>>>>>layerVerify";
      break;
    case layerValid:
      str += "Valid";
      break;
    default:
      str += "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!>>>>>";
  };
  if ( seatLayer.pax_id != ASTRA::NoExists ) {
    str += " pax_id=" + IntToString( seatLayer.pax_id );
  }
  if ( seatLayer.crs_pax_id != ASTRA::NoExists ) {
    str += " crs_id=" + IntToString( seatLayer.crs_pax_id );
  }
  str += string(" type=") + EncodeCompLayerType( seatLayer.layer_type );
  str += " (";
  for ( std::set<TPlace*,CompareSeats>::const_iterator iseat=seats.seats.begin(); iseat!=seats.seats.end(); iseat++ ) {
    TPlace *place = *iseat;
    if ( seat == NULL || seat == place ) {
      if ( iseat != seats.seats.begin() )
        str += ",";
      str += place->yname + place->xname;
    }
  }
  str += ")";
  if ( seatLayer.point_dep != ASTRA::NoExists ) {
    str += " p_dep=" + IntToString( seatLayer.point_dep );
  }
  if ( seatLayer.point_id != ASTRA::NoExists ) {
    str += " p_id=" + IntToString( seatLayer.point_id );
  }
  if ( seatLayer.point_arv != ASTRA::NoExists ) {
    str += " p_arv=" + IntToString( seatLayer.point_arv );
  }
  str += string(" time=") + DateTimeToStr( seatLayer.time_create );
  str += string( " in=" ) + IntToString( seatLayer.inRoute );
  ProgTrace( TRACE5, "dumpLayers: %s", str.c_str() );
}

void TPaxList::dumpValidLayers()
{
  for ( std::map<int,TSalonPax>::iterator ipax=begin(); ipax!=end(); ipax++ ) {
    for ( std::map<TSeatLayer,TPaxLayerSeats>::iterator ilayer = ipax->second.layers.begin();
          ilayer != ipax->second.layers.end(); ++ilayer ) {
      TLayersPax::dumpPaxLayers( ilayer->first, ilayer->second );
    }
  }
}

void TSalonList::SetRFISC( int point_id, TSeatTariffMap &tariffMap )
{
  for ( std::vector<TPlaceList*>::iterator iplacelist=begin(); iplacelist!=end(); iplacelist++ ) {
    for ( TPlaces::iterator iseat=(*iplacelist)->places.begin(); iseat!=(*iplacelist)->places.end(); iseat++ ) {
      iseat->SetRFISC( point_id, tariffMap );
    }
  }
}

void TSalonList::ReadRFISCColors( TQuery &Qry, FilterRoutesProperty &filterRoutes,
                                  int prior_compon_props_point_id )
{
  ProgTrace( TRACE5, "TSalonList::ReadRFISCColors, prior_compon_props_point_id=%d", prior_compon_props_point_id );
  int col_point_id = Qry.GetFieldIndex( "point_id" );
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_color = Qry.FieldIndex( "color" );
  map<int,TPlaceList*> salons; // ��� ����ன ����樨 � ᠫ���
  for ( ; !Qry.Eof; Qry.Next() ) {
    TPlaceList* placelist = NULL;
    TSalonPoint point_s;
    point_s.num = Qry.FieldAsInteger( col_num );
    point_s.x = Qry.FieldAsInteger( col_x );
    point_s.y = Qry.FieldAsInteger( col_y );
    if ( !findSeat( salons, &placelist, point_s ) ) {
      if ( filterSets.filterClass.empty() ) {
        ProgError( STDLOG, "TSalonList::ReadTariff: placelist not found num=%d", point_s.num );
      }
      continue;
    }
    //��諨 �㦭� ᠫ��
    TPoint seat_p( point_s.x, point_s.y );
    if ( !placelist->ValidPlace( seat_p ) ) {
      //ProgError( STDLOG, "TSalonList::ReadTariff: seat not found num=%d, x=%d, y=%d", point_s.num, point_s.x, point_s.y );
      continue;
    }
    if ( col_point_id >= 0 && !filterRoutes.useRouteProperty( Qry.FieldAsInteger( col_point_id ) ) ) {
      continue;
    }
    TRFISC rfisc;
    rfisc.color = Qry.FieldAsString( col_color );
    if ( col_point_id >= 0 ) {
      if ( prior_compon_props_point_id != ASTRA::NoExists ) {
        placelist->place( seat_p )->AddRFISC( prior_compon_props_point_id, rfisc );
        //ProgTrace( TRACE5, "add rfisc %s", rfisc.color.c_str() );
      }
      else {
        placelist->place( seat_p )->AddRFISC( Qry.FieldAsInteger( col_point_id ), rfisc );
        //ProgTrace( TRACE5, "add rfisc %s", rfisc.color.c_str() );
      }
    }
    else {
      placelist->place( seat_p )->AddRFISC( NoExists, rfisc );
      //ProgTrace( TRACE5, "add rfisc %s", rfisc.color.c_str() );
    }
  }
}

void TSalonList::ReadTariff( TQuery &Qry, FilterRoutesProperty &filterRoutes,
                             int prior_compon_props_point_id )
{
  ProgTrace( TRACE5, "TSalonList::ReadTariff, prior_compon_props_point_id=%d", prior_compon_props_point_id );
  int col_point_id = Qry.GetFieldIndex( "point_id" );
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_color = Qry.FieldIndex( "color" );
  int col_rate = Qry.FieldIndex( "rate" );
  int col_rate_cur = Qry.FieldIndex( "rate_cur" );
  map<int,TPlaceList*> salons; // ��� ����ன ����樨 � ᠫ���
  for ( ; !Qry.Eof; Qry.Next() ) {
    TPlaceList* placelist = NULL;
    TSalonPoint point_s;
    point_s.num = Qry.FieldAsInteger( col_num );
    point_s.x = Qry.FieldAsInteger( col_x );
    point_s.y = Qry.FieldAsInteger( col_y );
    if ( !findSeat( salons, &placelist, point_s ) ) {
      if ( filterSets.filterClass.empty() ) {
        //ProgError( STDLOG, "TSalonList::ReadTariff: placelist not found num=%d", point_s.num );
      }
      continue;
    }
    //��諨 �㦭� ᠫ��
    TPoint seat_p( point_s.x, point_s.y );
    if ( !placelist->ValidPlace( seat_p ) ) {
      //ProgError( STDLOG, "TSalonList::ReadTariff: seat not found num=%d, x=%d, y=%d", point_s.num, point_s.x, point_s.y );
      continue;
    }
    if ( col_point_id >= 0 && !filterRoutes.useRouteProperty( Qry.FieldAsInteger( col_point_id ) ) ) {
      continue;
    }
    TSeatTariff tariff;
    tariff.color = Qry.FieldAsString( col_color );
    tariff.rate = Qry.FieldAsFloat( col_rate );
    tariff.currency_id = Qry.FieldAsString( col_rate_cur );
    if ( col_point_id >= 0 ) {
      if ( prior_compon_props_point_id != ASTRA::NoExists ) {
        placelist->place( seat_p )->AddTariff( prior_compon_props_point_id, tariff );
      }
      else {
        placelist->place( seat_p )->AddTariff( Qry.FieldAsInteger( col_point_id ), tariff );
      }
    }
    else {
      placelist->place( seat_p )->AddTariff( NoExists, tariff );
    }
  }
}

void TSalonList::ReadPaxs( TQuery &Qry, TPaxList &pax_list )
{
  pax_list.clear();
  int idx_pax_id = Qry.FieldIndex( "pax_id" );
  int idx_grp_id = Qry.FieldIndex( "grp_id" );
  int idx_status = Qry.FieldIndex( "status" );
  int idx_parent_pax_id = Qry.FieldIndex( "parent_pax_id" );
  int idx_reg_no = Qry.FieldIndex( "reg_no" );
  int idx_seats = Qry.FieldIndex( "seats" );
  int idx_is_jmp = Qry.FieldIndex( "is_jmp" );
  int idx_pers_type = Qry.FieldIndex( "pers_type" );
  int idx_name = Qry.FieldIndex( "name" );
  int idx_surname = Qry.FieldIndex( "surname" );
  int idx_is_female = Qry.FieldIndex( "is_female" );
  int idx_class = Qry.FieldIndex( "class" );
  int idx_class_grp = Qry.FieldIndex( "class_grp" );
  int idx_point_dep = Qry.FieldIndex( "point_dep" );
  int idx_point_arv = Qry.FieldIndex( "point_arv" );
  int idx_pr_web = Qry.FieldIndex( "pr_web" );
  int idx_crew_type = Qry.FieldIndex( "crew_type" );
  vector<TPass> InfItems, AdultItems;
  //TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  for ( ; !Qry.Eof; Qry.Next() ) {
    TPass pass;
    pass.pax_id = Qry.FieldAsInteger( idx_pax_id );
    pass.grp_id = Qry.FieldAsInteger( idx_grp_id );
    pass.grp_status = Qry.FieldAsString( idx_status );
    pass.point_dep = Qry.FieldAsInteger( idx_point_dep );
    pass.point_arv = Qry.FieldAsInteger( idx_point_arv );
    pass.reg_no = Qry.FieldAsInteger( idx_reg_no );
    pass.name = Qry.FieldAsString( idx_name );
    pass.surname = Qry.FieldAsString( idx_surname );
    pass.is_female = Qry.FieldIsNULL( idx_is_female )?ASTRA::NoExists:Qry.FieldAsInteger( idx_is_female );
    pass.parent_pax_id = Qry.FieldAsInteger( idx_parent_pax_id );
    pass.pers_type = DecodePerson( Qry.FieldAsString( idx_pers_type ) );
    pass.pr_web = ( Qry.FieldAsInteger( idx_pr_web ) != 0 );
    if(not Qry.FieldIsNULL(idx_crew_type))
        pass.crew_type = TCrewTypes().decode(Qry.FieldAsString(idx_crew_type));
    pass.seats = Qry.FieldAsInteger( idx_seats );
    pass.is_jmp = Qry.FieldAsInteger( idx_is_jmp )!=0;
    pass.cl =  Qry.FieldAsString( idx_class );
    pass.class_grp = Qry.FieldAsInteger( idx_class_grp );
    //!logProgTrace( TRACE5, "ReadPaxs: pax_id=%d, grp_status=%s, point_arv=%d, pass.pr_web=%d",
//!log               pass.pax_id, pass.grp_status.c_str(), pass.point_arv, pass.pr_web );
    if ( pass.seats == 0 ) {
      InfItems.push_back( pass );
    }
    else {
      if ( pass.pers_type == ASTRA::adult ) {
        AdultItems.push_back( pass );
      }
      else {  //children with seats
        TSalonPax pax;
        pax = pass;
        pax.pr_infant = ASTRA::NoExists;
        pax_list.insert( make_pair( pass.pax_id, pax ) );
      }
    }
  }
  //�ਢ易�� ��⥩ � �����
  ProgTrace( TRACE5, "TSalonList::ReadPaxs: SetInfantsToAdults: InfItems.size()=%zu, AdultItems.size()=%zu", InfItems.size(), AdultItems.size() );
  SetInfantsToAdults<TPass,TPass>( InfItems, AdultItems );
  pax_list.infants.clear();
  for ( vector<TPass>::iterator inf=InfItems.begin(); inf!=InfItems.end(); inf++ ) {
    //!logProgTrace( TRACE5, "Infant pax_id=%d", inf->pax_id );
    TSalonPax pax;
    pax = *inf;
    pax_list.infants.insert( make_pair( inf->pax_id, pax ) );
    for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
      if ( inf->parent_pax_id == j->pax_id ) {
        j->pr_inf = true;
        //!logProgTrace( TRACE5, "Infant to pax_id=%d", j->pax_id );
        break;
      }
    }
  }
  for ( vector<TPass>::iterator ipax=AdultItems.begin(); ipax!=AdultItems.end(); ipax++ ) {
    if ( pax_list.find( ipax->pax_id ) == pax_list.end() ) {
      TSalonPax pax;
      pax = *ipax;
      pax_list.insert( make_pair( ipax->pax_id, pax ) );
    }
  }
}
/* �᫨ ���ᠦ�� ��ॣ����஢��, � � �⮩ �롮થ �� �� ������*/
void TSalonList::ReadCrsPaxs( TQuery &Qry, TPaxList &pax_list )
{
  int idx_pax_id = Qry.FieldIndex( "pax_id" );
  int idx_seats = Qry.FieldIndex( "seats" );
  int idx_pers_type = Qry.FieldIndex( "pers_type" );
  int idx_name = Qry.FieldIndex( "name" );
  int idx_surname = Qry.FieldIndex( "surname" );
  int idx_class = Qry.FieldIndex( "class" );
  for ( ; !Qry.Eof; Qry.Next() ) {
    int id = Qry.FieldAsInteger( idx_pax_id );
    if ( pax_list.find( id ) != pax_list.end() ) {
      continue;
    }
    TSalonPax pax;
    pax.seats = Qry.FieldAsInteger( idx_seats );
    pax.reg_no = NoExists;
    pax.pers_type = DecodePerson( Qry.FieldAsString( idx_pers_type ) );
    pax.cl = Qry.FieldAsString( idx_class );
    pax.name = Qry.FieldAsString( idx_name );
    pax.surname = Qry.FieldAsString( idx_surname );
    pax.pr_infant = ASTRA::NoExists;
    pax.pr_web = false;
    pax_list.insert( make_pair( id, pax ) );
  }
}

void FilterRoutesProperty::Clear()
{
  point_dep = ASTRA::NoExists;
  point_arv = ASTRA::NoExists;
  comp_id = ASTRA::NoExists;
  crc_comp = 0;
  pr_craft_lat = false;
  airline = "";
}

FilterRoutesProperty::FilterRoutesProperty( )
{
  Clear();
}

FilterRoutesProperty::FilterRoutesProperty( const std::string &airline ) {
  Clear();
  this->airline = airline;
  ProgTrace( TRACE5, "getDepartureId=%d", getDepartureId() );
}

/* ���⪠ ������� */
void FilterRoutesProperty::Read( const TFilterRoutesSets &filterRoutesSets )
{
  //ProgTrace(TRACE5, "FilterRoutesProperty::Read::filterRoutesSets.point_dep=%d", filterRoutesSets.point_dep );
  clear();
  pointNum.clear();
  point_dep = filterRoutesSets.point_dep;
  point_arv = filterRoutesSets.point_arv;
  //�࠭���� �������
  //1. ����祭�� ������� ���� ������ � ᥣ���⮬ (�᫨ ����� point_dep, point_arv)
  //2. �஢���� ���� ������� �� �।���: ᮢ������� crc_comp
  //3. ����� ���� �㤥� ᤥ���� �����
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline, "
    "       act_out, "
    "       pr_lat_seat, "
    "       NVL(comp_id,-1) comp_id, "
    "       crc_comp, "
    "       airp, "
    "       pr_tranzit, "
    "       pr_tranz_reg "
    " FROM trip_sets, points "
    "WHERE points.point_id = :point_id AND "
    "      points.point_id = trip_sets.point_id";
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw UserException( "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" );
  }
  crc_comp = Qry.FieldAsInteger( "crc_comp" );
  pr_craft_lat = Qry.FieldAsInteger( "pr_lat_seat" );
  airline = Qry.FieldAsString( "airline" );
  comp_id = Qry.FieldAsInteger( "comp_id" );
  bool pr_tranzit = ( Qry.FieldAsInteger( "pr_tranzit" ) != 0 &&
                      Qry.FieldAsInteger( "pr_tranz_reg" ) == 0 );
  TTripRoute routes;
  if ( routes.GetRouteBefore( ASTRA::NoExists,
                              point_dep,
                              trtWithCurrent,
                              trtNotCancelled ) &&
       !routes.empty() ) {
    if ( routes.rbegin()->point_id != point_dep ) {
      ProgTrace( TRACE5, ">>>routes.rbegin()->point_id=%d, point_dep=%d", routes.rbegin()->point_id, point_dep );
      throw UserException( "MSG.FLIGHT.CANCELED.REFRESH_DATA" );
    }
    insert( begin(), *routes.rbegin() );
    pointNum[ routes.rbegin()->point_id ] = PointAirpNum( routes.rbegin()->point_num, routes.rbegin()->airp, true );
    for ( std::vector<TTripRouteItem>::reverse_iterator iroute=routes.rbegin() + 1;
          iroute!=routes.rend(); iroute++ ) {
      //!logProgTrace( TRACE5, "point_id=%d, pr_tranzit=%d", iroute->point_id,  pr_tranzit );
      Qry.SetVariable( "point_id", iroute->point_id );
      Qry.Execute();
      if ( Qry.Eof ||
           crc_comp != Qry.FieldAsInteger( "crc_comp" ) ||
           !pr_tranzit )
        break;
      //!logProgTrace( TRACE5, "point_id=%d, pr_tranzit=%d, act_out=%d",
//!log                 iroute->point_id,  Qry.FieldAsInteger( "pr_tranzit" ), !Qry.FieldIsNULL( "act_out" ) );
      insert( begin(), *iroute );
      pointNum[ iroute->point_id ] = PointAirpNum( iroute->point_num, iroute->airp, true );
      if ( !Qry.FieldIsNULL( "act_out" ) ) {
        takeoffPoints.insert( iroute->point_id );
      }
      pr_tranzit = ( Qry.FieldAsInteger( "pr_tranzit" ) != 0 &&
                     Qry.FieldAsInteger( "pr_tranz_reg" ) == 0 );
    }
  }
  if ( empty() ) {
    ProgTrace( TRACE5, ">>>FilterRoutesProperty::Read, point_id=%d", point_dep );
    throw UserException( "MSG.FLIGHT.CANCELED.REFRESH_DATA" );
  }
  routes.clear();
  if ( routes.GetRouteAfter( ASTRA::NoExists,
                             point_dep,
                             trtNotCurrent,
                             trtNotCancelled ) ) {
    //ProgTrace(TRACE5, "FilterRoutesProperty::Read::point_dep=%d", point_dep );
    for ( std::vector<TTripRouteItem>::iterator iroute=routes.begin();
          iroute!=routes.end(); iroute++ ) {
      if ( iroute!=routes.end() - 1 ) {
        // 㤠���� ��᫥���� �㭪� �ਫ��
        Qry.SetVariable( "point_id", iroute->point_id );
        Qry.Execute();
        if ( Qry.Eof ||
             crc_comp != Qry.FieldAsInteger( "crc_comp" )  ||
             Qry.FieldAsInteger( "pr_tranzit" ) == 0 ||
             Qry.FieldAsInteger( "pr_tranz_reg" ) != 0 )
          break;
        //!logProgTrace( TRACE5, "point_id=%d, pr_tranzit=%d, act_out=%d",
        //!log           iroute->point_id,  Qry.FieldAsInteger( "pr_tranzit" ), !Qry.FieldIsNULL( "act_out" ) );
        push_back( *iroute );
        if ( !Qry.FieldIsNULL( "act_out" ) ) {
          takeoffPoints.insert( iroute->point_id );
        }
      }
      pointNum[ iroute->point_id ] = PointAirpNum( iroute->point_num, iroute->airp, true );
    }
    if ( point_arv == ASTRA::NoExists && !routes.empty() ) {
      point_arv = routes.begin()->point_id;
    }
  }
  if ( routes.empty() && point_arv == ASTRA::NoExists ) { //��� �뫥�
      point_arv = point_dep;
  }
  if ( point_arv != ASTRA::NoExists ) {
    readNum( point_arv, true );
  }
  //!logProgTrace( TRACE5, "FilterRoutesProperty::Read(): point_dep=%d, point_arv=%d, FilterRoutesProperty.size()=%zu",
//!log             point_dep, point_arv, size() );
  for ( std::vector<TTripRouteItem>::iterator iroute=begin();
        iroute!=end(); iroute++ ) {
    ProgTrace( TRACE5, "point_id=%d, point_num=%d", iroute->point_id, iroute->point_num );
  }
}

int FilterRoutesProperty::readNum( int point_id, bool in_use )
{
  if ( pointNum.find( point_id ) == pointNum.end() ) {
    TQuery Qry( &OraSession );
    Qry.SQLText =
      "SELECT point_num, airp FROM points WHERE point_id = :point_id AND pr_del=0";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( Qry.Eof )
      throw EXCEPTIONS::Exception( "FilterRoutesProperty::readNum: point_id=%d not found!!!", point_id );
    pointNum[ point_id ] = PointAirpNum( Qry.FieldAsInteger( "point_num" ),
                                         Qry.FieldAsString( "airp" ),
                                         in_use );
  }
  return pointNum[ point_id ].num;
}

bool FilterRoutesProperty::useRouteProperty( int vpoint_dep, int vpoint_arv )
{
  int num_dep, num_arv;
  num_dep = readNum( vpoint_dep, false );
  if ( vpoint_arv == ASTRA::NoExists ) {
    num_arv = NoExists;
  }
  else {
    num_arv = readNum( vpoint_arv, false );
  }
  //����祭��
  if ( empty() ) {
    return false;
  }
  // int vpoint_dep, int vpoint_arv ������ ��室���� ����� ������������ �������
  bool ret = ( pointNum[ vpoint_dep ].in_use &&
             ( ( num_dep < pointNum[ point_dep ].num && num_arv > pointNum[ point_dep ].num ) ||
               ( num_dep >= pointNum[ point_dep ].num && num_dep < pointNum[ point_arv ].num ) ||
               ( num_dep < pointNum[ point_arv ].num && num_arv >= pointNum[ point_arv ].num ) ) );

  //!logProgTrace( TRACE5, "FilterRoutesProperty::useRouteProperty: vpoint_dep=%d, vpoint_arv=%d, num_dep=%d, num_arv=%d, range_dep=%d, range_arv=%d, ret=%d",
//!log             vpoint_dep, vpoint_arv, num_dep, num_arv, pointNum[ point_dep ].num, pointNum[ point_arv ].num, ret );
  return ret;
}

bool FilterRoutesProperty::IntersecRoutes( int point_dep1, int point_arv1,
                                           int point_dep2, int point_arv2, bool pr_routes )
{
  int num_dep1 = readNum( point_dep1, false );
  int num_dep2 = readNum( point_dep2, false );
  if ( num_dep1 == num_dep2 ) {
    return true;
  }
  int num_arv1 = point_arv1==NoExists?NoExists:readNum( point_arv1, false );
  int num_arv2 = point_arv2==NoExists?NoExists:readNum( point_arv2, false );

  bool result = false;

  if ( num_dep1 < num_dep2 ) {
    if ( num_arv1 == NoExists ) {
      result = false;
    }
    else {
      result = ( num_arv1 > num_dep2 );
    }
  }
  else {
    if ( num_dep1 > num_dep2 ) {
      if ( num_arv2 == NoExists ) {
        result = false;
      }
      else {
        result = ( num_arv2 > num_dep1 );
      }
    }
  }
  if ( result ) {
    return true;
  }
  //�� ���ᥪ�����, � � ������� ������⮬ A-B-C-D. 1: A->C, 2: C->D, ������� �⮡ࠦ���� B->D
  if ( pr_routes ) {
    return false;
  }
  bool res1, res2;
  return ( (res1=IntersecRoutes( point_dep1, point_arv1, getDepartureId(), getArrivalId(), true )) &&
           (res2=IntersecRoutes( point_dep2, point_arv2, getDepartureId(), getArrivalId(), true )) );
}

void FilterRoutesProperty::Build( xmlNodePtr node )
{
  NewTextChild( node, "point_dep", getDepartureId() );
  NewTextChild( node, "point_arv", getArrivalId() );
  node = NewTextChild( node, "items" );
  for ( std::map<int,PointAirpNum>::const_iterator idest=pointNum.begin();
        idest!=pointNum.end(); idest++ ) {
    if ( !idest->second.in_use ) {
      continue;
    }
    xmlNodePtr n = NewTextChild( node, "item" );
    NewTextChild( n, "point_id", idest->first );
    NewTextChild( n, "airp", idest->second.airp );
  }
}

/*void TSalonList::AddRFISCRemarks( int key, TSeatTariffMap &tariffMap )
{
  for ( std::vector<TPlaceList*>::iterator iplacelist=begin(); iplacelist!=end(); iplacelist++ ) {
    for ( TPlaces::iterator iseat=(*iplacelist)->places.begin(); iseat!=(*iplacelist)->places.end(); iseat++ ) {
      iseat->SetRFICSRemarkByColor( key, tariffMap );
    }
  }
}*/

/*void TSalonList::DropRFISCRemarks( TSeatTariffMap &tariffMap )
{
  for ( std::vector<TPlaceList*>::iterator iplacelist=begin(); iplacelist!=end(); iplacelist++ ) {
    for ( TPlaces::iterator iseat=(*iplacelist)->places.begin(); iseat!=(*iplacelist)->places.end(); iseat++ ) {
      iseat->DropRFISCRemarks( tariffMap );
    }
  }
}*/

/*void TSalonList::SetTariffsByRFICSColor( int point_dep, TSeatTariffMap &tariffMap, bool setPassengerTariffs )
{
  ProgTrace( TRACE5, "tariffMap.size()=%zu", tariffMap.size() );
  for( vector<TPlaceList*>::iterator placeList = begin();
       placeList != end(); placeList++ ) {
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      place->SetTariffsByRFICSColor( point_dep, tariffMap, setPassengerTariffs );
    }
  }
}
*/

void TSalonList::ReadCompon( int vcomp_id, int point_id )
{
  ProgTrace( TRACE5, "TSalonList::ReadCompon(), comp_id=%d", vcomp_id );
  Clear();
  comp_id = vcomp_id;
  TFilterLayers filterLayers;
  TPaxList pax_list;
  filterLayers.clearFlags();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_lat_seat,airline FROM comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.SALONS.NOT_FOUND.REFRESH_DATA");
  pr_craft_lat = Qry.FieldAsInteger( "pr_lat_seat" );
  string airline = Qry.FieldAsString( "airline" );
  if ( point_id != ASTRA::NoExists ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT airline FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( !Qry.Eof ) {
      airline = Qry.FieldAsString( "airline" );
    }
  }
  FilterRoutesProperty filterRoutes( airline );
  filterSets.filterRoutes = filterRoutes;
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,x,y,elem_type,xprior,yprior,agle,xname,yname,class "
    " FROM comp_elems "
    "WHERE comp_id=:comp_id "
    "ORDER BY num, x desc, y desc ";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  if ( Qry.Eof )
    throw UserException( "MSG.SALONS.NOT_FOUND" );
  ReadSeats( Qry, filterSets.filterClass );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,x,y,rem,pr_denial FROM comp_rem "
    " WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  ReadRemarks( Qry, filterRoutes, NoExists );
  //����뢠�� ���� ���� �� ��������
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,x,y,color,rate,rate_cur FROM comp_rates "
    " WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  ReadTariff( Qry, filterRoutes, ASTRA::NoExists );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,x,y,layer_type FROM comp_baselayers "
    " WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  ReadLayers( Qry, filterRoutes, filterLayers, pax_list, NoExists );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,x,y,color FROM comp_rfisc "
    " WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  ReadRFISCColors( Qry, filterRoutes, NoExists );
  RFISCMode = rAll;
  if ( !airline.empty() ) {
    TSeatTariffMap tariffMap;
    tariffMap.get_rfisc_colors( airline );
    if ( tariffMap.status() == TSeatTariffMap::stUseRFISC ) {
      SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
      if ( point_id != ASTRA::NoExists ) {
        SelfCkinSalonTariff.setTariffMap( point_id, tariffMap );
      }
      SetRFISC( ASTRA::NoExists, tariffMap );
      if ( point_id != ASTRA::NoExists ) {
        RFISCMode = rRFISC;
      }
    }
    else {
      if ( point_id != ASTRA::NoExists ) {
        RFISCMode = rTariff;
      }
    }
  }
}

//���� ᠬ��� �ਮ��⭮�� � ��������� ᫮� � �㭪� �뫥�
bool getTopSeatLayerOnRoute( const std::map<int,TPaxList> &pax_lists,
                             TPlace* pseat,
                             int point_id, TSeatLayer &layer,
                             bool useFilterRoute )
{
  layer = TSeatLayer();
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  pseat->GetLayers( layers, glNoBase );
  layer = TSeatLayer();
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::const_iterator isetSeatLayer;
  isetSeatLayer = layers.find( point_id );
/*  ProgTrace( TRACE5, "getTopSeatLayerOnRoute: point_id=%d, seat=%s, useFilterRoute=%d",
             point_id, string( pseat->yname+pseat->xname ).c_str(), useFilterRoute );*/
  if ( isetSeatLayer == layers.end() ) {
    //!logProgTrace( TRACE5, "getTopSeatLayerOnRoute: point_id=%d, layers empty, return false, cltUnknown, seat=%s",
//!log               point_id, string( pseat->yname + pseat->xname ).c_str() );
    return false;
  }
  for ( std::set<TSeatLayer,SeatLayerCompare>::const_iterator ilayer=isetSeatLayer->second.begin();
        ilayer!=isetSeatLayer->second.end(); ilayer++ ) {
    //!logProgTrace( TRACE5, "getTopSeatLayerOnRoute: %s, %s", string( pseat->yname+pseat->xname ).c_str(), ilayer->toString().c_str() );
    if ( useFilterRoute && !ilayer->inRoute ) {
      //!log tst();
      continue;
    }
    if ( ilayer->getPaxId() == ASTRA::NoExists ) {
      layer = *ilayer;
      //!logProgTrace( TRACE5, "getTopSeatLayerOnRoute: return %s", layer.toString().c_str() );
      return true;
    }
    // ᫮� �ਭ������� ���ᠦ���
    std::map<int,TPaxList>::const_iterator ipax_list = pax_lists.find( ilayer->point_id );
    if ( ipax_list == pax_lists.end() ) {
      ProgError( STDLOG, "getTopSeatLayerOnRoute: flight route not found %s", ilayer->toString().c_str() );
      continue;
    }
    TPaxList::const_iterator ipax = ipax_list->second.find( ilayer->getPaxId() );
    if ( ipax == ipax_list->second.end() ) {
      ProgError( STDLOG, "getTopSeatLayerOnRoute: pass not found %s", ilayer->toString().c_str() );
      continue;
    }
    std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::const_iterator ipax_curr_layer = ipax->second.layers.find( *ilayer );
    if ( ipax_curr_layer == ipax->second.layers.end() ) {
      ProgError( STDLOG, "getTopSeatLayerOnRoute: layer not found %s", ilayer->toString().c_str() );
      continue;
    }
    //᫮� ��������� - �� ������ � ���������
    if ( ipax_curr_layer->second.waitListReason.layerStatus != layerValid ) {
      continue;
    }
    layer = *ilayer;
    //logProgTrace( TRACE5, "getTopSeatLayerOnRoute: return true, %s, seat=%s",
    //log           layer.toString().c_str(), string( pseat->yname + pseat->xname ).c_str() );
    return true;
  }
//log  ProgTrace( TRACE5, "getTopSeatLayerOnRoute: return false, cltUnknown, seat=%s", string( pseat->yname + pseat->xname ).c_str() );
  return false;
}

void getTopSeatLayer( FilterRoutesProperty &filterRoutes,
                      std::map<int,TPaxList> &pax_lists,
                      const std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers,
                      TPlace* pseat,
                      TSeatLayer &max_priority_layer,
                      bool useFilterRoute );


/* ��।���� ���ᨬ���� �� �� ᫮�*/
inline bool isMaxPaxLayer( FilterRoutesProperty &filterRoutes,
                           std::map<int,TPaxList> &pax_lists,
                           const std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers,
                           const TSeatLayer &layer,
                           bool useFilterRoute )
{
  //!logProgTrace( TRACE5, "isMaxPaxLayer: %s, useFilterRoute=%d", layer.toString().c_str(), useFilterRoute );
  if ( layer.getPaxId() == NoExists ) {
    //logProgTrace( TRACE5, "layer is not pass" );
    return true;
  }
  //�ਭ������� ���ᠦ��� - �������� ��� ���� 㤠���� �� �᫮���, �� ���� ����� �ਮ���� ᫮� - ������ ���
  // �ਭ������� ���ᠦ���
  //�ॡ���� �஢���� �� ����� �ਮ���� ᫮� � ���ᠦ��
  std::map<int,TPaxList>::iterator ipax_list = pax_lists.find( layer.point_id );
  if ( ipax_list == pax_lists.end() ) {
    //logProgError( STDLOG, "isMaxPaxLayer: flight route not found %s", layer.toString().c_str() );
    return true;
  }
  TPaxList::iterator ipax = ipax_list->second.find( layer.getPaxId() );
  if ( ipax == ipax_list->second.end() ) {
    ProgError( STDLOG, "isMaxPaxLayer: pass not found%s", layer.toString().c_str() );
    return true;
  }
  std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::iterator ipax_curr_layer = ipax->second.layers.find( layer );
  if ( ipax_curr_layer == ipax->second.layers.end() ) {
    ProgError( STDLOG, "TSalonList::validateLayersSeats: pass layer not found %s", layer.toString().c_str() );
    return true;
  }
  bool pr_find = false;
  TWaitListReason waitListReason;
  //�஡�� �� �ᥬ ����� �ਮ���� ᫮� ���ᠦ��
  for ( std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::iterator ipax_layer=ipax->second.layers.begin();
        ipax_layer!=ipax->second.layers.end(); ipax_layer++ ) {
    if ( ipax_layer->second.waitListReason.layerStatus != layerValid ) {
      continue;
    }
    if ( useFilterRoute && !ipax_layer->first.inRoute ) {
      //!log tst();
      continue;
    }
    if ( pr_find ) { //�᫨ ������ ������� ᫮�, � ��⠫�� ���������
      ipax_layer->second.waitListReason = waitListReason;
      continue;
    }
    else {
      if ( ipax_layer == ipax_curr_layer ) {
        ipax_curr_layer->second.waitListReason.layerStatus = layerValid;
        break;
      }
    }
    if ( ipax_layer->second.waitListReason.layerStatus != layerValid ) {
      //�஡�� �� ���⠬ ࠧ��祭��� ����� �ਮ���� ᫮��
      for ( std::set<TPlace*,CompareSeats>::iterator nseat=ipax_layer->second.seats.begin();
            nseat!=ipax_layer->second.seats.end(); nseat++ ) {
        TSeatLayer seatLayer;
        getTopSeatLayer( filterRoutes,
                         pax_lists,
                         menuLayers,
                         *nseat,
                         seatLayer,
                         useFilterRoute );
        //�᫨ �� ���� ᠬ� �ਮ���� ��㣮� ᫮�, � ��� ���������
        if ( ipax_layer->first != seatLayer ) {
          //logProgTrace( TRACE5, "isMaxPaxLayer: not max %s, because max %s", ipax_layer->first.toString().c_str(), seatLayer.toString().c_str() );
          ipax_layer->second.waitListReason = TWaitListReason( layerLess, seatLayer );
          break;
        }
        else {
          ipax_layer->second.waitListReason.layerStatus = layerValid;
        }
      }
    }
    pr_find = ( ipax_layer->second.waitListReason.layerStatus == layerValid );
    waitListReason = TWaitListReason( layerLess, ipax_layer->first );
  }
  //logProgTrace( TRACE5, "isMaxPaxLayer: return %d, %s", !pr_find, layer.toString().c_str() );
  return !pr_find;
}

inline void setInvalidLayer( std::map<int,TPaxList> &pax_lists,
                             const TSeatLayer &layer,
                             const TWaitListReason &waitListReason )
{
  std::map<int,TPaxList>::iterator ipax_list = pax_lists.find( layer.point_id );
  if ( ipax_list == pax_lists.end() ) {
    ProgError( STDLOG, "setInvalidLayer: flight route not found %s", layer.toString().c_str() );
    return;
  }
  TPaxList::iterator ipax = ipax_list->second.find( layer.getPaxId() );
  if ( ipax == ipax_list->second.end() ) {
    ProgError( STDLOG, "setInvalidLayer: pass not found%s", layer.toString().c_str() );
    return;
  }
  std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::iterator ipax_curr_layer = ipax->second.layers.find( layer );
  if ( ipax_curr_layer == ipax->second.layers.end() ) {
    ProgError( STDLOG, "setInvalidLayer: pass layer not found %s", layer.toString().c_str() );
    return;
  }
  ipax_curr_layer->second.waitListReason = waitListReason;
  for ( std::set<TPlace*,CompareSeats>::iterator iseat=ipax_curr_layer->second.seats.begin();
        iseat!=ipax_curr_layer->second.seats.end(); iseat++ ) {
    TPlace *seat = *iseat;
    seat->ClearLayer( ipax_curr_layer->first.point_id, ipax_curr_layer->first );
  }
  //logProgTrace( TRACE5, "setInvalidLayer: drop %s", ipax_curr_layer->first.toString().c_str() );
}

bool isBlockedLayer( const ASTRA::TCompLayerType &layer_type )
{
  return ( layer_type == cltBlockCent ||
           layer_type == cltDisable );
}

inline void dropLayer( std::map<int,TPaxList> &pax_lists,
                       const TSeatLayer &layer,
                       TPlace* pseat,
                       const TWaitListReason &waitListReason )
{
  if ( layer.getPaxId() == ASTRA::NoExists ) {
    //!log tst();
    std::set<TSeatLayer,SeatLayerCompare> del_layers;
    if ( isBlockedLayer( layer.layer_type ) ) { //�� ��������騩 ᫮�, ���� 㤠���� �� ᫮� ��� ���
      std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > players;
      pseat->GetLayers( players, glNoBase );
      bool pr_find=false;
      for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=players[ layer.point_id ].begin();
            ilayer!=players[ layer.point_id ].end(); ilayer++ ) {
        if ( *ilayer == layer ) {
          pr_find = true;
        }
        if ( pr_find ) {
          del_layers.insert( *ilayer );
        }
      }
    }
    else {
      del_layers.insert( layer );
    }
    for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=del_layers.begin();
          ilayer!=del_layers.end(); ilayer++ ) {
      //ProgTrace( TRACE5, "dropLayer: %s", ilayer->toString().c_str() );
      if ( ilayer->getPaxId() == ASTRA::NoExists ) {
        if ( isBlockedLayer( ilayer->layer_type ) ) {
          pseat->AddDropBlockedLayer( *ilayer );
        }
        pseat->ClearLayer( ilayer->point_id, *ilayer );
      }
      else {
        //!log tst();
        setInvalidLayer( pax_lists, *ilayer, waitListReason );
      }
    }
  }
  else {
    //!log tst();
    setInvalidLayer( pax_lists, layer, waitListReason );
  }
}

//��।������ ��� ���� ᠬ��� �ਮ��⭮�� ᫮� �� ��������
void getTopSeatLayer( FilterRoutesProperty &filterRoutes,
                      std::map<int,TPaxList> &pax_lists,
                      const std::map<ASTRA::TCompLayerType,TMenuLayer> &menuLayers,
                      TPlace* pseat,
                      TSeatLayer &max_priority_layer,
                      bool useFilterRoute )
{
  max_priority_layer = TSeatLayer();
  TSeatLayer curr_layer, prior_layer;
  std::map<ASTRA::TCompLayerType,TMenuLayer>::const_iterator imenu;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::const_iterator isetSeatLayer;
/*  //�஡�� �� �㭪⠬ �뫥�, ��稭�� � ��᫥�����
  for ( std::vector<TTripRouteItem>::const_reverse_iterator iroute=filterRoutes.rbegin();
        iroute!=filterRoutes.rend(); ++iroute ) {*/
  //�஡�� �� �㭪⠬ �뫥�, ��稭�� � ��ࢮ��
  for ( std::vector<TTripRouteItem>::const_iterator iprior_route=filterRoutes.begin();
        iprior_route!=filterRoutes.end(); ++iprior_route ) {
    if ( !getTopSeatLayerOnRoute( pax_lists, pseat, iprior_route->point_id, prior_layer, useFilterRoute ) ) { //� �㭪� �뫥� ��� ᫮��
      continue;
    }
/*    ProgTrace( TRACE5, "getTopSeatLayer: iroute point_id=%d, airp=%s, seat=%s, getTopSeatLayerOnRoute return %s",
               iroute->point_id, iroute->airp.c_str(), string(pseat->yname+pseat->xname).c_str(), curr_layer.toString().c_str() );*/
/*    //�஡�� �� �।. �㭪⠬
    for ( std::vector<TTripRouteItem>::const_reverse_iterator iprior_route=iroute+1;
          iprior_route!=filterRoutes.rend(); ++iprior_route ) {*/
    //�஡�� �� ᫥�. �㭪⠬
    for ( std::vector<TTripRouteItem>::const_iterator iroute=iprior_route+1;
          iroute!=filterRoutes.end(); ++iroute ) {
      if ( !getTopSeatLayerOnRoute( pax_lists, pseat, iroute->point_id, curr_layer, useFilterRoute ) ) {
        continue;
      }
/*      ProgTrace( TRACE5, "getTopSeatLayer: iprior_route point_id=%d, airp=%s, seat=%s, getTopSeatLayerOnRoute return %s",
                 iprior_route->point_id, iprior_route->airp.c_str(), string(pseat->yname+pseat->xname).c_str(), prior_layer.toString().c_str() );*/
      prior_layer.point_dep_num = pdPrior;
      curr_layer.point_dep_num = pdNext;
//log      ProgTrace( TRACE5, "before IntersecRoutes filterRoutes.getDepartureId()=%d, filterRoutes.getArrivalId()=%d, %s, %s",
//log                 filterRoutes.getDepartureId(), filterRoutes.getArrivalId(),
//log                 prior_layer.toString().c_str(), curr_layer.toString().c_str() );
      if ( filterRoutes.IntersecRoutes( prior_layer.point_dep==NoExists?prior_layer.point_id:prior_layer.point_dep,
                                        prior_layer.point_arv,
                                        curr_layer.point_dep==NoExists?curr_layer.point_id:curr_layer.point_dep,
                                        curr_layer.point_arv, false ) ) { //���� ����祭��  - �ࠢ������ �।��騩 � ��᫥���騬
        bool pr_prior_layer = compareSeatLayer( prior_layer, curr_layer );
        prior_layer.point_dep_num = pdCurrent;
        curr_layer.point_dep_num = pdCurrent;
//log        ProgTrace( TRACE5, "IntersecRoutes %s, %s, pr_prior_layer=%d",
//log                   prior_layer.toString().c_str(), curr_layer.toString().c_str(), pr_prior_layer );
        TSeatLayer tmp_layer;
        TWaitListReason waitListReason;
        if ( isMaxPaxLayer( filterRoutes,
                            pax_lists,
                            menuLayers,
                            pr_prior_layer?prior_layer:curr_layer,
                            useFilterRoute ) ) {
          tmp_layer = pr_prior_layer?curr_layer:prior_layer;
          waitListReason.layerStatus = layerLess;
          waitListReason.layer = pr_prior_layer?prior_layer:curr_layer;
        }
        else {
          tmp_layer = pr_prior_layer?prior_layer:curr_layer;
          waitListReason.layerStatus = layerLess;
          waitListReason.layer = pr_prior_layer?curr_layer:prior_layer;
        }
        dropLayer( pax_lists, tmp_layer, pseat, waitListReason );
        getTopSeatLayer( filterRoutes,
                         pax_lists,
                         menuLayers,
                         pseat,
                         max_priority_layer,
                         useFilterRoute ); //����⪠, �.�. ᫥�. ᫮� ����� ���ᥪ����� � ��㣨��
      }
      else { //�� ���ᥪ�����
     //log   tst();
      }
    }
    if ( getTopSeatLayerOnRoute( pax_lists, pseat, iprior_route->point_id, curr_layer, useFilterRoute ) ) {
      if ( !curr_layer.inRoute ) {
        dropLayer( pax_lists, curr_layer, pseat, TWaitListReason( layerNotRoute, curr_layer ) );
        getTopSeatLayer( filterRoutes,
                         pax_lists,
                         menuLayers,
                         pseat,
                         max_priority_layer,
                         useFilterRoute ); //����⪠, �.�. ᫥�. ᫮� ����� ���ᥪ����� � ��㣨��
      }
    }

    //��᫥ ����祭�� �஢�ਬ ����祭�� ᫮� �� ���ᨬ����
    if ( getTopSeatLayerOnRoute( pax_lists, pseat, iprior_route->point_id, curr_layer, true ) ) { //� �㭪� �뫥� ���� ᫮�
      if ( isMaxPaxLayer( filterRoutes,
                          pax_lists,
                          menuLayers,
                          curr_layer,
                          useFilterRoute ) ) {
//log        ProgTrace( TRACE5, "isMaxPaxLayer: return curr_layer %s", curr_layer.toString().c_str() );
        if ( curr_layer.inRoute ) { //� ��襬 �������
          curr_layer.point_dep_num = pdPrior;
          max_priority_layer.point_dep_num = pdNext;
          if ( max_priority_layer.layer_type == cltUnknown ||
               compareSeatLayer( curr_layer, max_priority_layer ) ) {
            max_priority_layer = curr_layer;
//log            ProgTrace( TRACE5, "getTopSeatLayer: max_layer %s", max_priority_layer.toString().c_str() );
          }
          curr_layer.point_dep_num = pdCurrent;
          max_priority_layer.point_dep_num = pdCurrent;
        }
      }
    }
  }
  if ( max_priority_layer.layer_type != cltUnknown ) {
    ProgTrace( TRACE5, "max %s", max_priority_layer.toString().c_str() );
  }
}


struct TClearSeatLayer {
  TPlace *seat;
  TSeatLayer max_layer;
};

void TSalonList::CommitLayers()
{
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  //�஡�� �� ���ᠦ�ࠬ
  for ( std::map<int,TPaxList>::iterator iroute_pax_list=pax_lists.begin();
        iroute_pax_list!=pax_lists.end(); iroute_pax_list++ ) {
    for ( TPaxList::iterator ipax_list=iroute_pax_list->second.begin();
          ipax_list!=iroute_pax_list->second.end(); ipax_list++ ) {
      ipax_list->second.save_layers.clear();
      for ( TLayersPax::iterator ilayer=ipax_list->second.layers.begin();
            ilayer!=ipax_list->second.layers.end(); ilayer++ ) {
        if ( ilayer->second.waitListReason.layerStatus != layerValid ) {  //��࠭塞 ⮫쪮 ������� ᫮�
          for ( std::set<TPlace*,CompareSeats>::iterator iseat=ilayer->second.seats.begin();
                iseat!=ilayer->second.seats.end(); iseat++ ) { // �஡�� �� ���⠬ - 㤠�塞 ��������� ᫮� �� ����
            (*iseat)->ClearLayer( ilayer->first.point_id, ilayer->first );
          }
          //!logProgTrace( TRACE5, "TSalonList:: NOT CommitLayers: %s", ilayer->first.toString().c_str() );
          continue;
        }
        TSeatLayer layer = ilayer->first;
        TPaxLayerSeats paxlayer = ilayer->second;
        ipax_list->second.save_layers.insert( make_pair( layer, paxlayer ) );
      }
    }
  }
  for ( TSalonList::iterator isalonlist=begin();
        isalonlist!=end(); isalonlist++ ) {
    for ( TPlaces::iterator iseat=(*isalonlist)->places.begin();
          iseat!=(*isalonlist)->places.end(); iseat++ ) {
      //��࠭塞 ⮫쪮 ������� ᫮�
      iseat->CommitLayers();
    }
  }
}

void TPlace::RollbackLayers( FilterRoutesProperty &filterRoutes,
                             std::map<int,TFilterLayers> &filtersLayers ) {
  lrss.clear();
  drop_blocked_layers.clear();
  for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=save_lrss.begin();
        ilayers!=save_lrss.end(); ilayers++ ) {
    for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
          ilayer!=ilayers->second.end(); ilayer++ ) {
      TSeatLayer layer = *ilayer;
      if ( layer.point_dep != ASTRA::NoExists &&
           filtersLayers[ layer.point_id ].CanUseLayer( layer.layer_type,
                                                        layer.point_dep,
                                                        filterRoutes.getDepartureId(),
                                                        filterRoutes.isTakeoff( layer.point_id ) ) ) { // ᫮� �㦭� ��������
        layer.inRoute = ( ilayer->point_id == filterRoutes.getDepartureId() ||
                          filterRoutes.useRouteProperty( ilayer->point_dep, ilayer->point_arv ) );
//log        ProgTrace( TRACE5, "TPlace::RollbackLayers: %s, takeoff(%d)=%d",
//log                   layer.toString().c_str(), layer.point_id, filterRoutes.isTakeoff( layer.point_id ) );
        lrss[ layer.point_id ].insert( layer );
      }
    }
  }
}

void TSalonList::RollbackLayers( )
{
  FilterRoutesProperty &filterRoutes = filterSets.filterRoutes;
  for ( TSalonList::iterator isalonlist=begin();
        isalonlist!=end(); isalonlist++ ) {
    for ( TPlaces::iterator iseat=(*isalonlist)->places.begin();
          iseat!=(*isalonlist)->places.end(); iseat++ ) {
      iseat->RollbackLayers( filterRoutes, filterSets.filtersLayers );
      iseat->drawProps.clearFlags();
    }
  }
  for ( std::map<int,TPaxList>::iterator iroute_pax_list=pax_lists.begin();
        iroute_pax_list!=pax_lists.end(); iroute_pax_list++ ) {
    for ( TPaxList::iterator ipax_list=iroute_pax_list->second.begin();
          ipax_list!=iroute_pax_list->second.end(); ipax_list++ ) {
      ipax_list->second.layers.clear();
      for ( TLayersPax::iterator ilayer=ipax_list->second.save_layers.begin();
            ilayer!=ipax_list->second.save_layers.end(); ilayer++ ) {
        TSeatLayer layer = ilayer->first;
        if ( layer.point_dep != ASTRA::NoExists &&
             filterSets.filtersLayers[ layer.point_id ].CanUseLayer( layer.layer_type,
                                                                     layer.point_dep,
                                                                     filterRoutes.getDepartureId(),
                                                                     filterRoutes.isTakeoff( layer.point_id ) ) ) { // ᫮� �㦭� ��������
          layer.inRoute = ( layer.point_id == filterRoutes.getDepartureId() ||
                            filterRoutes.useRouteProperty( layer.point_dep, layer.point_arv ) );
          //!logProgTrace( TRACE5, "TSalonList::RollbackLayers: %s", layer.toString().c_str() );
          TPaxLayerSeats paxlayer = ilayer->second;
          paxlayer.waitListReason = TWaitListReason( layerValid, TSeatLayer() );
          ipax_list->second.layers.insert( make_pair( layer, paxlayer ) );
        }
      }
    }
  }
}

void TSalonList::validateLayersSeats( )
{
  RollbackLayers();
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuBaseLayers( menuLayers, true );

  TSeatLayer max_priority_layer;
  vector<TClearSeatLayer> clearSeatLayers;

  pax_lists[ getDepartureId() ].dumpValidLayers();

  for ( std::vector<TPlaceList*>::iterator iplacelist=begin(); iplacelist!=end(); iplacelist++ ) {
    for ( TPlaces::iterator iseat=(*iplacelist)->places.begin(); iseat!=(*iplacelist)->places.end(); iseat++ ) {
//log      ProgTrace( TRACE5, "TSalonList::validateLayersSeats: before validate %s",
//log                 string( iseat->yname + iseat->xname ).c_str() );
      getTopSeatLayer( filterSets.filterRoutes,
                       pax_lists,
                       menuLayers,
                       &(*iseat),
                       max_priority_layer,
                       false );
//log      ProgTrace( TRACE5, "TSalonList::validateLayersSeats: seat %s have max %s",
//log                 string( iseat->yname + iseat->xname ).c_str(), max_priority_layer.toString().c_str() );
      if ( max_priority_layer.layer_type != cltUnknown ) {  //???
        TClearSeatLayer seatLayer;
        seatLayer.max_layer = max_priority_layer;
        seatLayer.seat = &(*iseat);
        clearSeatLayers.push_back( seatLayer );
      }
    }
  }

  //����� ⥯��� �㤥� ���� 㤠���� �� �� �㦭� ᫮�???
  for ( vector<TClearSeatLayer>::iterator iseatLayer=clearSeatLayers.begin();
        iseatLayer!=clearSeatLayers.end(); iseatLayer++ ) {
    iseatLayer->seat->GetLayers( layers, glNoBase );
    for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin();
          ilayers!=layers.end(); ilayers++ ) {
      for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
            ilayer!=ilayers->second.end(); ilayer++ ) {
        if ( !ilayer->inRoute ) {
           iseatLayer->seat->ClearLayer( ilayer->point_id, *ilayer );
           continue;
        }
        if ( *ilayer != iseatLayer->max_layer ) {
          if ( ilayer->getPaxId() != ASTRA::NoExists ) { //�������� ���� 㤠���� ᫮� �ਭ������騥 ���ᠦ���???
            //!log tst();
            setInvalidLayer( pax_lists, *ilayer, TWaitListReason( layerLess, iseatLayer->max_layer ) );
          }
          else {
            if ( isBlockedLayer( ilayer->layer_type ) ) {
              iseatLayer->seat->AddDropBlockedLayer( *ilayer );
            }
            iseatLayer->seat->ClearLayer( ilayer->point_id, *ilayer );
          }
          continue;
        }
      }
    }
  }

  //㤠����� ��� ���������� ᫮�� + ᫮��, ����� �� � �������
  for ( std::map<int,TPaxList>::iterator ipax_list=pax_lists.begin(); ipax_list!=pax_lists.end(); ipax_list++ ) {
    for ( std::map<int,TSalonPax>::iterator ipax=ipax_list->second.begin(); ipax!=ipax_list->second.end(); ipax++ ) {
      bool pr_find = false;
      TWaitListReason waitListReason;
      for ( std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::iterator ilayers=ipax->second.layers.begin();
            ilayers!=ipax->second.layers.end(); ilayers++ ) {
        if ( pr_find ) {
          if ( !ilayers->first.inRoute ) {
            ilayers->second.waitListReason = TWaitListReason( layerNotRoute, ilayers->first );
          }
          else {
            ilayers->second.waitListReason = waitListReason;
          }
        }
        if ( ilayers->second.waitListReason.layerStatus == layerValid ) {
          pr_find = true;
          waitListReason = TWaitListReason( layerLess, ilayers->first );
          continue;
        }
        if ( !ilayers->first.inRoute ) {
          ilayers->second.waitListReason = TWaitListReason( layerNotRoute, ilayers->first );
          continue;
        }
        if ( ilayers->second.waitListReason.layerStatus != layerValid ) {
          for ( std::set<TPlace*,CompareSeats>::iterator iseat=ilayers->second.seats.begin();
                iseat!=ilayers->second.seats.end(); iseat++ ) {
            TPlace *seat = *iseat;
            seat->ClearLayer( ilayers->first.point_id, ilayers->first );
          }
        }
      }
    }
  }
  for ( std::vector<TPlaceList*>::iterator iplacelist=begin(); iplacelist!=end(); iplacelist++ ) {
    for ( TPlaces::iterator iseat=(*iplacelist)->places.begin(); iseat!=(*iplacelist)->places.end(); iseat++ ) {
      iseat->GetLayers( layers, glBase );
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin();
           ilayers!=layers.end(); ilayers++ ) {
       for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
             ilayer!=ilayers->second.end(); ilayer++ ) {
         if ( !ilayer->inRoute ) {
            iseat->ClearLayer( ilayer->point_id, *ilayer );
            continue;
         }
       }
      }
    }
  }

  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterSets.filterRoutes.begin();
        iseg!=filterSets.filterRoutes.end(); iseg++ ) {
    //����⨬ ��⥩
    pax_lists[ iseg->point_id ].InfantToSeatDrawProps();
    pax_lists[ iseg->point_id ].TranzitToSeatDrawProps( filterSets.filterRoutes.getDepartureId() );
    pax_lists[ iseg->point_id ].dumpValidLayers();
  }
}

void TSalonList::JumpToLeg( const FilterRoutesProperty &filterRoutesNew )
{
  if ( filterRoutesNew.getMaxRoute() != filterSets.filterRoutes.getMaxRoute() ) {
    throw EXCEPTIONS::Exception( "TSalonList::JumpToLeg: invalid filterRoutesNew(%d,%d)",
                                 filterRoutesNew.getDepartureId(), filterRoutesNew.getArrivalId() );
  }
  filterSets.filterRoutes = filterRoutesNew;
  ProgTrace( TRACE5, "TSalonList::JumpToLeg: getDepartureId=%d, getArrivalId=%d", getDepartureId(), getArrivalId() );
  validateLayersSeats( );
}

void TSalonList::JumpToLeg( const TFilterRoutesSets &routesSets )
{
  //!logProgTrace( TRACE5, "TSalonList::JumpToLeg: point_dep=%d, point_arv=%d",
  //!log           routesSets.point_dep, routesSets.point_arv );
  //�஢�ઠ, �� �ॡ㥬� ��ய��� �뫥� � �ਫ�� ��室���� ����� ���⠭���� �������
  FilterRoutesProperty filterRoutesTmp;
  filterRoutesTmp.Read( TFilterRoutesSets( routesSets.point_dep, routesSets.point_arv ) );
  JumpToLeg( filterRoutesTmp );
}

inline void __getpass( int point_dep,
                       FilterRoutesProperty &filterRoutes,
                       const std::map<int,TSalonPax> &pax_list,
                       TIntArvSalonPassengers &passes,
                       bool pr_waitlist,
                       bool pr_infants )
{
    TPassSeats seats;
    TWaitListReason waitListReason;
    for ( std::map<int,TSalonPax>::const_iterator ipax=pax_list.begin();
            ipax!=pax_list.end(); ipax++ ) {
        if ( ipax->second.point_arv == ASTRA::NoExists ||
                ipax->second.grp_status.empty() ||
                ipax->second.reg_no == ASTRA::NoExists ) {
            //!logProgTrace( TRACE5, "__getpass: pnl pass - ipax->second.layers.empty() pax_id=%d", ipax->first );
            continue;
        }
        if ( filterRoutes.useRouteProperty( ipax->second.point_dep, ipax->second.point_arv ) ) { //���ᠦ�� � ��襬 �������
            if ( !pr_infants ) {
                ipax->second.get_seats( waitListReason, seats );
            }
            if ( pr_infants ||
                    waitListReason.layerStatus == layerValid ||
                    pr_waitlist ) {
                TSalonPax salonPax = ipax->second;
                salonPax.pax_id = ipax->first;
                //!logProgTrace( TRACE5,
                //!log           "__getpass: add point_dep=%d, point_arv=%d,"
                //!log           " filterRoutes.getDepartureId=%d, filterRoutes.getArrivalId=%d,"
                //!log           "ipax->second.cl=%s, salonPax.grp_status=%s, pax_id=%d, salonPax.layers.size()=%zu, ipax->second.layers.size()=%zu",
                //!log           salonPax.point_dep, salonPax.point_arv,
                //!log           filterRoutes.getDepartureId(), filterRoutes.getArrivalId(),
                //!log           salonPax.cl.c_str(), salonPax.grp_status.c_str(),
                //!log           salonPax.pax_id, salonPax.layers.size(), ipax->second.layers.size() );
                passes[ salonPax.point_arv ][ salonPax.cl ][ salonPax.grp_status ].insert( salonPax );
            }
        }
    }
}

string getPointAirp(int point_id)
{
    TCachedQuery Qry(
            "select airp from points where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    string result;
    if(not Qry.get().Eof)
        result = Qry.get().FieldAsString(0);
    return result;
}

void _TSalonPassengers::dump_pax_map(const TIntArvSalonPassengers &pax_map)
{
    for(TIntArvSalonPassengers::const_iterator
            iArv = pax_map.begin();
            iArv != pax_map.end();
            iArv++) {
        for(TIntClassSalonPassengers::const_iterator
                iCls = iArv->second.begin();
                iCls != iArv->second.end();
                iCls++) {
            for(TIntStatusSalonPassengers::const_iterator
                    iStatus = iCls->second.begin();
                    iStatus != iCls->second.end();
                    iStatus++) {
                LogTrace(TRACE5)
                    << "[" << getPointAirp(iArv->first) << "]"
                    << "[" << iCls->first << "]"
                    << "[" << iStatus->first << "]"
                    << " = " << iStatus->second.size();
                for(set<TSalonPax,ComparePassenger>::const_iterator
                        iPax = iStatus->second.begin();
                        iPax != iStatus->second.end();
                        iPax++) {
                    LogTrace(TRACE5)
                        << iPax->pax_id << ": "
                        << iPax->surname << " " << iPax->name
                        << " seats: " << iPax->seats;
                }
            }
        }
    }
}

void _TSalonPassengers::dump()
{
    LogTrace(TRACE5) << "---_TSalonPassengers::dump---";
    LogTrace(TRACE5) << "infants.size(): " << infants.size();
    if(not infants.empty()) {
        LogTrace(TRACE5) << "---Infants map---";
        dump_pax_map(infants);
        LogTrace(TRACE5) << "-----------------";
    }
    dump_pax_map(*this);
    LogTrace(TRACE5) << "-----------------------------";
}
void TSalonPassengers::dump()
{
    LogTrace(TRACE5) << "---TSalonPassengers::dump---";
    for(TSalonPassengers::iterator iDep = begin(); iDep != end(); iDep++) {
        LogTrace(TRACE5) << "airp_dep: " << getPointAirp(iDep->first);
        iDep->second.dump();
    }
    LogTrace(TRACE5) << "----------------------------";
}

// point_dep, pont_arv, class, grp_status, pax_id
void TSalonList::getPassengers( TSalonPassengers &passengers, const TGetPassFlags &flags )
{
  ProgTrace( TRACE5, "flags: gpPassenger=%d, gpWaitList=%d, gpTranzits=%d, gpInfants=%d",
             flags.isFlag( gpPassenger ),
             flags.isFlag( gpWaitList ),
             flags.isFlag( gpTranzits ),
             flags.isFlag( gpInfants ) );
  passengers.clear();
  FilterRoutesProperty &filterRoutes = filterSets.filterRoutes;
  FilterRoutesProperty filterRoutesPrior = filterRoutes;
  std::map<int,FilterRoutesProperty> filterRoutesProps;
  try {
    for ( FilterRoutesProperty::const_reverse_iterator iroute=filterRoutesPrior.rbegin();
          iroute!=filterRoutesPrior.rend(); iroute++ ) {
//      ProgTrace( TRACE5, "TSalonList::getPassenger: point_id=%d, filterRoutesPrior.getDepartureId()=%d, filterRoutesPrior.getArrivalId()=%d",
//               iroute->point_id, filterRoutesPrior.getDepartureId(), filterRoutesPrior.getArrivalId() );
      _TSalonPassengers passes( iroute->point_id, filterSets.filterRoutes.isCraftLat() );  //!!!filterSets.filterRoutes.isCraftLat() - � ᠫ��� ����� ���� ��㣮�
      JumpToLeg( TFilterRoutesSets( iroute->point_id ) );
//      ProgTrace( TRACE5, "TSalonList::getPassenger: point_id=%d, filterRoutes.getDepartureId()=%d, filterRoutes.getArrivalId()=%d",
//                 iroute->point_id, filterRoutes.getDepartureId(), filterRoutes.getArrivalId() );
      bool pr_find = false;
      for ( FilterRoutesProperty::const_reverse_iterator jroute=filterRoutes.rbegin();
            jroute!=filterRoutes.rend(); jroute++ ) {
        if ( jroute->point_id == filterRoutes.getDepartureId() ) {
          pr_find = true;
        }
        if ( !pr_find ) {
          continue;
        }
        //!logProgTrace( TRACE5, "TSalonList::getPassenger: jroute->point_id=%d", jroute->point_id );
        if ( !flags.isFlag( gpTranzits ) &&
             jroute->point_id != filterRoutes.getDepartureId() ) {
          //!log tst();
          continue;
        }
        if ( !flags.isFlag( gpPassenger ) &&
             jroute->point_id == filterRoutes.getDepartureId() ) {
          //!log tst();
          continue;
        }
        std::map<int,TPaxList>::const_iterator ipax_list = pax_lists.find( jroute->point_id );
        if ( ipax_list == pax_lists.end() ) {
          //!log tst();
          continue;
        }
        //�롨ࠥ� ���ᠦ�஢
        __getpass( jroute->point_id, filterSets.filterRoutes, ipax_list->second, passes, flags.isFlag( gpWaitList ), false );
        if ( !passes.empty() ) {
          if ( flags.isFlag( gpInfants ) ) {
            __getpass( jroute->point_id, filterSets.filterRoutes, ipax_list->second.infants, passes.infants, flags.isFlag( gpWaitList ), true );
          }
          //!logProgTrace( TRACE5, "iroute->point_id=%d", iroute->point_id );
        }
      }
      passengers.insert( make_pair( iroute->point_id, passes ) );
    }
  }
  catch( ... ) {
    filterRoutes = filterRoutesPrior;
    throw;
  }
  filterRoutes = filterRoutesPrior;
  JumpToLeg( TFilterRoutesSets( filterRoutes.getDepartureId() ) );
}

void TSalonList::getPaxLayer( int point_dep, int pax_id, ASTRA::TCompLayerType layer_type,
                              std::set<TPlace*,CompareSeats> &seats ) const
{
  seats.clear();
  std::map<int,TPaxList>::const_iterator ipax_list = pax_lists.find( point_dep );
  if ( ipax_list == pax_lists.end() ) {
    return;
  }
  std::map<int,TSalonPax>::const_iterator ipax = ipax_list->second.find( pax_id );
  if ( ipax == ipax_list->second.end() ) {
    return;
  }

  for ( TLayersPax::const_iterator ilayers=ipax->second.layers.begin();
        ilayers!=ipax->second.layers.end(); ilayers++ ) {
    if ( ilayers->first.layer_type != layer_type ||
         !(ilayers->second.waitListReason.layerStatus == layerLess ||
           ilayers->second.waitListReason.layerStatus == layerValid) ||
         ilayers->first.getPaxId( ) != pax_id ) {
      continue;
    }
    seats = ilayers->second.seats;
    break;
  }
}


void TSalonList::getPaxLayer( int point_dep, int pax_id,
                              TSeatLayer &seatLayer,
                              std::set<TPlace*,CompareSeats> &seats ) const
{
  seatLayer = TSeatLayer();
  seats.clear();
  std::map<int,TPaxList>::const_iterator ipax_list = pax_lists.find( point_dep );
  if ( ipax_list == pax_lists.end() ) {
    return;
  }
  std::map<int,TSalonPax>::const_iterator ipax = ipax_list->second.find( pax_id );
  if ( ipax == ipax_list->second.end() ) {
    return;
  }
  for ( TLayersPax::const_iterator ilayers=ipax->second.layers.begin();
        ilayers!=ipax->second.layers.end(); ilayers++ ) {
    if ( ilayers->second.waitListReason.layerStatus != layerValid ||
         ilayers->first.getPaxId( ) != pax_id ) {
      continue;
    }
    seatLayer = ilayers->first;
    seats = ilayers->second.seats;
    break;
  }
}

void TSectionInfo::clearProps() {
    clear();
    salonPoints.clear();
    totalLayerSeats.clear();
    currentLayerSeats.clear();
    layersPaxs.clear();
    paxs.clear();
}

void TSectionInfo::operator = (const TSectionInfo &sectionInfo) {
    SimpleProp::operator = ( sectionInfo );
//    ProgTrace( TRACE5, "section=%s", sectionInfo.str().c_str() );
    salonPoints = sectionInfo.salonPoints;
    totalLayerSeats = sectionInfo.totalLayerSeats;
    currentLayerSeats = sectionInfo.currentLayerSeats;
    layersPaxs = sectionInfo.layersPaxs;
    paxs = sectionInfo.paxs;
}

bool TSectionInfo::inSection( const TSalonPoint &salonPoint ) const {
    for ( std::vector<TSalonPointNames>::const_iterator iseat=salonPoints.begin();
            iseat!=salonPoints.end(); iseat++ ) {
        if ( iseat->point == salonPoint ) {
            return true;
        }
    }
    return false;
}

bool TSectionInfo::inSection( const TSeat &aseat ) const {
    for ( std::vector<TSalonPointNames>::const_iterator iseat=salonPoints.begin();
            iseat!=salonPoints.end(); iseat++ ) {
        if ( iseat->seat == aseat ) {
            return true;
        }
    }
    return false;
}

bool TSectionInfo::inSection( int row ) const {
    return ( (row >= getFirstRow() || getFirstRow() == ASTRA::NoExists) &&
            (row <= getLastRow() || getLastRow() == ASTRA::NoExists) ); // ����� ᥪ樨 ��� ��� �࠭�� ᥪ樨
}

void TSectionInfo::AddLayerSeats( const TSeatLayer &seatLayer, const TSeat &seats ) {
    layersPaxs[ seatLayer ].insert( seats );
}

void TSectionInfo::GetLayerSeats( TLayersSeats &value ) {
    value = layersPaxs;
}

void TSectionInfo::GetPaxs( std::map<int,TSalonPax> &value ) {
    value = paxs;
}

int TSectionInfo::seatsTotalLayerSeats( const ASTRA::TCompLayerType &layer_type ) {
    if ( totalLayerSeats.find( layer_type ) != totalLayerSeats.end() ) {
        return (int)totalLayerSeats[ layer_type ].size();
    }
    return 0;
}

int TSectionInfo::seatsCurrentLayerSeats( const ASTRA::TCompLayerType &layer_type ) {
    if ( currentLayerSeats.find( layer_type ) != currentLayerSeats.end() ) {
        return (int)currentLayerSeats[ layer_type ].size();
    }
    return 0;
}

void TSectionInfo::AddPax( const TSalonPax &pax )
{
   paxs.insert( make_pair( pax.pax_id, pax ) );
}

bool TSectionInfo::inSectionPaxId( int pax_id )
{
  return paxs.find( pax_id ) != paxs.end();
}

void TSectionInfo::AddCurrentLayerSeat( const TSeatLayer &layer, TPlace* seat ) {
  for (std::vector<std::pair<TSeatLayer,TPassSeats> >::iterator ilayer=currentLayerSeats[ layer.layer_type ].begin();
       ilayer!=currentLayerSeats[ layer.layer_type ].end(); ilayer++ ) {
    if ( ilayer->first == layer ) {
      ilayer->second.insert(  TSeat( seat->yname, seat->xname ) );
      return;
    }
  }
  TPassSeats p;
  p.insert( TSeat( seat->yname, seat->xname ) );
  currentLayerSeats[ layer.layer_type ].push_back( make_pair( layer, p ) );
}

void TSectionInfo::GetCurrentLayerSeat( const ASTRA::TCompLayerType &layer_type,
                                        std::vector<std::pair<TSeatLayer,TPassSeats> > &layersSeats )
{
  layersSeats.clear();
  if ( currentLayerSeats.find( layer_type ) != currentLayerSeats.end() ) {
    layersSeats = currentLayerSeats[ layer_type ];
  }
}

void TSectionInfo::GetTotalLayerSeat( const ASTRA::TCompLayerType &layer_type,
                                      TPassSeats &layerSeats )
{
  layerSeats.clear();
  std::map<ASTRA::TCompLayerType,std::vector<TPlace*> >::iterator itotal=totalLayerSeats.find( layer_type );
  if ( itotal != totalLayerSeats.end() ) {
    for ( std::vector<TPlace*>::const_iterator iseat=itotal->second.begin();
          iseat!=itotal->second.end(); iseat++ ) {
      layerSeats.insert( TSeat( (*iseat)->yname, (*iseat)->xname ) );
    }
  }
}


/*  ���� ���������:
    std::map<ASTRA::TCompLayerType,std::vector<TPlace*> > totalLayerSeats; //᫮� + ����
    std::map<ASTRA::TCompLayerType,std::vector<TPlace*> > currentLayerSeats; //ᠬ� �ਮ���� ᫮� + ����
    std::set<TSalonPoint> salonPoints; ���� �ਭ������騥 ᥪ樨
    TLayersSeats layersPaxs; //seatLayer->pax_id ᯨ᮪ ���ᠦ�஢ � ���⠬�
    std::map<int,TSeatPax> paxs; //pax_id ���� �ਭ������騥 ���ᠦ��� � ᫮�
*/

void TSalonList::getSectionInfo( TSectionInfo &sectionInfo, const TGetPassFlags &flags )
{
  std::vector<TSectionInfo> salonsInfo;
  salonsInfo.push_back( sectionInfo );
  getSectionInfo( salonsInfo, flags );
  sectionInfo = *salonsInfo.begin();
}

void TSalonList::getSectionInfo( std::vector<TSectionInfo> &salonsInfo, const TGetPassFlags &flags )
{
  //std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > vlayers;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::const_iterator ilayer;
  SALONS2::TSeatLayer layer;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  for ( vector<SALONS2::TSectionInfo>::iterator icompSection=salonsInfo.begin(); icompSection!=salonsInfo.end(); icompSection++ ) {
    icompSection->clearProps();
    int Idx = 0;
    for ( vector<TPlaceList*>::const_iterator si=begin(); si!=end(); si++ ) {
      for ( int y=0; y<(*si)->GetYsCount(); y++ ) {
        if ( icompSection->inSection( Idx ) ) { // ����� ᥪ樨 ��� ��� �࠭�� ᥪ樨
          for ( int x=0; x<(*si)->GetXsCount(); x++ ) {
            TPlace *seat = (*si)->place( (*si)->GetPlaceIndex( x, y ) );
            if ( !seat->isplace || !seat->visible ) {
               continue;
            }
            icompSection->AddSalonPoints( TSalonPoint( seat->x, seat->y, (*si)->num ), TSeat( seat->yname, seat->xname ) );
            seat->GetLayers( layers, glAll );
            ilayer = layers.find( getDepartureId() );
            if ( ilayer != layers.end() ) {
              for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilr=ilayer->second.begin();
                    ilr != ilayer->second.end(); ilr++ ) {
                icompSection->AddTotalLayerSeat( ilr->layer_type, seat );
              }
              if ( !ilayer->second.empty() ) {
                icompSection->AddCurrentLayerSeat( *ilayer->second.begin(), seat );
              }
            }
          }
        }
        Idx++;
      }
    }
  }
  TPassSeats seats;
  TWaitListReason waitListReason;
  //�ନ�㥬 ᯨ᮪ ��� ���ᠦ�஢
  if ( !flags.emptyFlags() ) {
    TSalonPassengers passengers;
    getPassengers( passengers, flags );
    for ( TSalonPassengers::iterator ipass_dep=passengers.begin();
          ipass_dep!=passengers.end(); ipass_dep++ ) {
      if ( ipass_dep->first != getDepartureId() ) {
        continue;
      }
      for ( TIntArvSalonPassengers::iterator ipass_arv=ipass_dep->second.begin();
            ipass_arv!=ipass_dep->second.end(); ipass_arv++ ) {
        for ( TIntClassSalonPassengers::iterator ipass_class=ipass_arv->second.begin();
              ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
          for ( TIntStatusSalonPassengers::iterator ipass_status=ipass_class->second.begin();
                ipass_status!=ipass_class->second.end(); ipass_status++ ) {
            for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=ipass_status->second.begin();
                  ipass!=ipass_status->second.end(); ipass++ ) {
              ipass->get_seats( waitListReason, seats );
              //!logProgTrace( TRACE5, "pax_id=%d, seats.size()=%zu", ipass->pax_id, seats.size() );
              if ( waitListReason.layerStatus == layerValid ) {
                for ( TPassSeats::const_iterator iseat=seats.begin();
                      iseat!=seats.end(); iseat++ ) {
                  for ( vector<SALONS2::TSectionInfo>::iterator icompSection=salonsInfo.begin(); icompSection!=salonsInfo.end(); icompSection++ ) {
                    if ( icompSection->inSection( *iseat ) ) {
                      //!logProgTrace( TRACE5, "pax_id=%d, seat_no=%s, layer=%s",
                      //!log           ipass->pax_id, string(string(iseat->row)+iseat->line).c_str(), waitListReason.layer.toString().c_str() );
                      icompSection->AddPax( *ipass );
                      icompSection->AddLayerSeats( waitListReason.layer, *iseat );
                      break;
                    }
                  }
                }
              }
            }
          }
        }
        if ( flags.isFlag( gpInfants ) ) { //infants
          //!log tst();
          for ( TIntArvSalonPassengers::iterator ipass_arv=ipass_dep->second.infants.begin();
                ipass_arv!=ipass_dep->second.infants.end(); ipass_arv++ ) {
            for ( TIntClassSalonPassengers::iterator ipass_class=ipass_arv->second.begin();
                  ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
              for ( TIntStatusSalonPassengers::iterator ipass_status=ipass_class->second.begin();
                    ipass_status!=ipass_class->second.end(); ipass_status++ ) {
                for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=ipass_status->second.begin();
                      ipass!=ipass_status->second.end(); ipass++ ) {
                  for ( vector<SALONS2::TSectionInfo>::iterator icompSection=salonsInfo.begin(); icompSection!=salonsInfo.end(); icompSection++ ) {
                    if ( icompSection->inSectionPaxId( ipass->pax_id ) ) {
                      icompSection->AddPax( *ipass );
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  } //end pass
}

/*
  filterRoutes - ᯨ᮪ �㭪⮢ � ������ �������� ���� ����, ���. ���� ������ �� ���� ࠧ����
*/
void TSalonList::ReadFlight( const TFilterRoutesSets &filterRoutesSets,
                             TSalonReadVersion version,
                             const std::string &filterClass,
                             int tariff_pax_id,
                             bool for_calc_waitlist,
                             int prior_compon_props_point_id )
{
  if ( !for_calc_waitlist && SALONS2::isFreeSeating( filterRoutesSets.point_dep ) ) {
    throw EXCEPTIONS::Exception( "MSG.SALONS.FREE_SEATING" );
  }
  bool only_compon_props = ( prior_compon_props_point_id != ASTRA::NoExists );
  ProgTrace( TRACE5, "TSalonList::ReadFlight(): version=%d, filterClass=%s, prior_compon_props_point_id=%d",
             version==SALONS2::rfTranzitVersion, filterClass.c_str(), prior_compon_props_point_id );
  Clear();
  filterSets.version = version;
  filterSets.filterClass = filterClass;
  FilterRoutesProperty &filterRoutes = filterSets.filterRoutes;
  filterRoutes.Read( filterRoutesSets );
  //!logProgTrace( TRACE5, "filterRoutes.getCompId=%d", filterRoutes.getCompId() );
  TQuery Qry( &OraSession );
  pax_lists.clear();
  // ���⠥� �࠭���� �������
  comp_id = filterSets.filterRoutes.getCompId();
  pr_craft_lat = filterSets.filterRoutes.isCraftLat();
  ProgTrace( TRACE5, "TSalonList::ReadFlight(): vcomp_id=%d, vpr_lat_seat=%d filterRoutes.size()=%zu, FilterClass=%s",
             comp_id, pr_craft_lat, filterSets.filterRoutes.size(), filterSets.filterClass.c_str() );
  std::map<int,TFilterLayers> &filtersLayers = filterSets.filtersLayers;
  filtersLayers.clear();
  //���⪠ 䨫�⮢ ᫮�� �� ��������
  int Max_SOM_PRL_Departure_id = ASTRA::NoExists;
  int Max_SOM_PRL_Num = ASTRA::NoExists;
  vector<TTripRouteItem> routes;
  routes.insert( routes.end(), filterRoutes.begin(), filterRoutes.end() );
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    filtersLayers[ iseg->point_id ].getFilterLayersOnTranzitRoutes( iseg->point_id,
                                                                    SALONS2::isTranzitSalons( iseg->point_id ),
                                                                    routes,
                                                                    only_compon_props );
    if ( filtersLayers[ iseg->point_id ].isFlag( cltSOMTrzt ) ||
         filtersLayers[ iseg->point_id ].isFlag( cltPRLTrzt ) ) { //���� ࠧ��⪠ �� ⥫��ࠬ�� SOM
      for ( std::vector<TTripRouteItem>::const_iterator  iprior_seg=filterRoutes.begin(); //������ ��㤠 ��諠 ⥫��ࠬ��
            iprior_seg!=iseg; iprior_seg++ ) {
        //!logProgTrace( TRACE5, "iprior_seg->point_id=%d", iprior_seg->point_id );
        if ( iprior_seg->point_id == filtersLayers[ iseg->point_id ].getSOM_PRL_Dep( ) ) { //��諨 �㭪� �� ���ண� ��諠 ⥫��ࠬ��
          if ( Max_SOM_PRL_Num < iprior_seg->point_num ) {
            Max_SOM_PRL_Num = iprior_seg->point_num;
            Max_SOM_PRL_Departure_id = iprior_seg->point_id;
            ProgTrace( TRACE5, "Max_SOM_PRL_Num=%d, Max_SOM_PRL_Departure_id=%d",
                       Max_SOM_PRL_Num, Max_SOM_PRL_Departure_id );
          }
          break;
        }
      }
    }
  }
  Qry.Clear();
  //����뢠�� ���������� ⮫쪮 �� ��襬� �㭪�� ��ᠤ��
  Qry.SQLText =
    "SELECT num, x, y, elem_type, xprior, yprior, agle,"
    "       xname, yname, class "
    " FROM trip_comp_elems "
    "WHERE point_id = :point_id "
    "ORDER BY num, x desc, y desc";
  Qry.CreateVariable( "point_id", otInteger, filterRoutes.getDepartureId() );
  Qry.Execute();
  bool empty_salons = Qry.Eof;
  if ( empty_salons && !for_calc_waitlist ) {
    ProgTrace( TRACE5, "point_id=%d", filterRoutes.getDepartureId() );
    throw UserException( "MSG.FLIGHT_WO_CRAFT_CONFIGURE" );
  }
  ReadSeats( Qry, filterSets.filterClass );
  //����뢠�� ६�ન �� ��������
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id, num, x, y, rem, pr_denial "
    " FROM trip_comp_rem "
    " WHERE point_id = :point_id ";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    Qry.SetVariable( "point_id", iseg->point_id );
    Qry.Execute();
    ReadRemarks( Qry, filterRoutes, prior_compon_props_point_id );
  }
  //����뢠�� ���� ���� �� ��������
  Qry.Clear();
  Qry.SQLText =
  "SELECT point_id,num,x,y,color,rate,rate_cur "
  " FROM trip_comp_rates "
  " WHERE point_id=:point_id ";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    if ( filtersLayers[ iseg->point_id ].CanUseLayer( cltProtBeforePay, -1, -1, filterRoutes.isTakeoff( iseg->point_id ) ) ||
         filtersLayers[ iseg->point_id ].CanUseLayer( cltProtAfterPay, -1, -1, filterRoutes.isTakeoff( iseg->point_id ) ) ||
         filtersLayers[ iseg->point_id ].CanUseLayer( cltPNLBeforePay, -1, -1, filterRoutes.isTakeoff( iseg->point_id ) ) ||
         filtersLayers[ iseg->point_id ].CanUseLayer( cltPNLAfterPay, -1, -1, filterRoutes.isTakeoff( iseg->point_id ) ) ) {
      Qry.SetVariable( "point_id", iseg->point_id );
      Qry.Execute();
      ReadTariff( Qry, filterRoutes, prior_compon_props_point_id );
    }
  }
  Qry.SQLText =
    "SELECT point_id,num,x,y,color FROM trip_comp_rfisc "
    " WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    Qry.SetVariable( "point_id", iseg->point_id );
    Qry.Execute();
    ReadRFISCColors( Qry, filterRoutes, prior_compon_props_point_id );
  }

  if ( !only_compon_props ) {
    // ����뢠�� ᯨ᮪ ��ॣ����஢����� ���ᠦ�஢ �� ��������  pax_list
    Qry.Clear();
    Qry.SQLText =
      " SELECT pax.grp_id, pax.pax_id, pax.pers_type, pax.seats, pax.is_jmp, class, class_grp, "
      "        reg_no, pax.name, pax.surname, pax.is_female, pax_grp.status, "
      "        pax_grp.point_dep, pax_grp.point_arv, "
      "        crs_inf.pax_id AS parent_pax_id, "
      "        DECODE(client_type,:web_client,1,:mobile_client,1,0) pr_web, "
      "        crew_type "
      "    FROM pax_grp, pax, crs_inf "
      "   WHERE pax.grp_id=pax_grp.grp_id AND "
      "         pax_grp.point_dep=:point_dep AND "
      "         pax.pax_id=crs_inf.inf_id(+) AND "
      "         pax_grp.status NOT IN ('E') AND "
      "         pax.refuse IS NULL ";
    Qry.DeclareVariable( "point_dep", otInteger );
    Qry.CreateVariable( "web_client", otString, EncodeClientType( ASTRA::ctWeb ) );
    Qry.CreateVariable( "mobile_client", otString, EncodeClientType( ASTRA::ctMobile ) );
    for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
          iseg!=filterRoutes.end(); iseg++ ) {
      if ( filterSets.version==rfNoTranzitVersion && iseg->point_id != filterRoutesSets.point_dep ) {
        continue;
      }
      Qry.SetVariable( "point_dep", iseg->point_id );
      Qry.Execute();
      ReadPaxs( Qry,  pax_lists[ iseg->point_id ] );
//      ProgTrace( TRACE5, "TSalonList::ReadFlight: pax_lists[ %d ].size()=%zu", iseg->point_id, pax_lists[ iseg->point_id ].size() );
    }
    // ����뢠�� ᯨ᮪ ���஭�஢����� ���ᠦ�஢ �� ३��  pax_list
    Qry.Clear();
    Qry.SQLText =
      "SELECT pax_id, seats, pers_type, name, surname, class "
      "    FROM crs_pax, crs_pnr, tlg_binding "
      "   WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "         crs_pnr.point_id=tlg_binding.point_id_tlg AND "
      "         tlg_binding.point_id_spp=:point_dep AND "
      "         crs_pnr.system='CRS' AND "
      "         crs_pax.pr_del=0 ";
    Qry.DeclareVariable( "point_dep", otInteger );
    for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
          iseg!=filterRoutes.end(); iseg++ ) {
      if ( filterSets.version==rfNoTranzitVersion && iseg->point_id != filterRoutesSets.point_dep ) {
        continue;
      }
      Qry.SetVariable( "point_dep", iseg->point_id );
      Qry.Execute();
      ReadCrsPaxs( Qry, pax_lists[ iseg->point_id ] );
//      ProgTrace( TRACE5, "TSalonList::ReadFlight: crs_pax_lists[ %d ].size()=%zu", iseg->point_id, pax_lists[ iseg->point_id ].size() );
    }
  }
  ProgTrace( TRACE5, "prior_compon_props_point_id=%d", prior_compon_props_point_id );
  //����뢠�� ������ ᫮� �� ��������
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id,num,x,y,layer_type,NULL as time_create, "
    "       NULL as pax_id, NULL as crs_pax_id, NULL as point_dep, "
    "       NULL as point_arv "
    " FROM trip_comp_baselayers "
    " WHERE point_id=:point_id ";
  Qry.DeclareVariable( "point_id", otInteger );
  //!!!����� ���஢�� ��� addLayer �� �������� point_id
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    Qry.SetVariable( "point_id", iseg->point_id );
    Qry.Execute();
    ReadLayers( Qry, filterRoutes, filtersLayers[ iseg->point_id ],
                pax_lists[ iseg->point_id ],
                prior_compon_props_point_id );
  }
  //����뢠�� ᫮� �� ��������, � �⮬� ������� ������ ���� �������� pax_list
  Qry.Clear();
    Qry.SQLText =
    "SELECT num, x, y, trip_comp_layers.layer_type, crs_pax_id, pax_id, time_create, "
    "       trip_comp_layers.point_id, point_dep, point_arv, "
    "       first_xname, first_yname, last_xname, last_yname "
    " FROM trip_comp_ranges, trip_comp_layers "
    " WHERE trip_comp_layers.point_id = :point_id AND "
    "       trip_comp_layers.range_id = trip_comp_ranges.range_id(+)";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( std::vector<TTripRouteItem>::const_iterator iseg=filterRoutes.begin();
        iseg!=filterRoutes.end(); iseg++ ) {
    if ( (only_compon_props || filterSets.version==rfNoTranzitVersion) && iseg->point_id != filterRoutesSets.point_dep ) {
      continue;
    }
    Qry.SetVariable( "point_id", iseg->point_id );
    Qry.Execute();
    ReadLayers( Qry, filterRoutes, filtersLayers[ iseg->point_id ],
                pax_lists[ iseg->point_id ],
                prior_compon_props_point_id );
  }
  CommitLayers();

  TSeatTariffMap tariffMap;
  bool paxTariff = false;
  if ( tariff_pax_id != NoExists ) {
    if ( SALONS2::Checkin( tariff_pax_id ) ) {
      tariffMap.get( tariff_pax_id );
      paxTariff = ( tariffMap.status() == TSeatTariffMap::stUseRFISC );
      ProgTrace( TRACE5, "RFIS CMode=%d, paxTariff=%d, tariff_pax_id=%d", RFISCMode, paxTariff, tariff_pax_id );
      tariffMap.trace(TRACE5);
    }
    else {
      TAdvTripInfo operFlt;
      operFlt.getByPointId( filterRoutesSets.point_dep );
      TMktFlight flight;
      flight.getByCrsPaxId( tariff_pax_id );
      ProgTrace( TRACE5, "tariff_pax_id=%d, flight.empty()=%d", tariff_pax_id, flight.empty() );
      if ( !flight.empty() ) {
        TTripInfo markFlt;
        markFlt.airline = flight.airline;
        CheckIn::TPaxTknItem tkn;
        CheckIn::LoadCrsPaxTkn( tariff_pax_id, tkn);
        tariffMap.get( operFlt, markFlt, tkn );
        paxTariff = (tariffMap.status() == TSeatTariffMap::stUseRFISC);
      }
    }
  }
  if ( !paxTariff ) { // �᫨ ���ᠦ�� �� ������, � �� ������������ �⮡ࠦ��� rfics + color
    if ( !filterRoutes.getAirline().empty() ) {
      tariffMap.is_rfisc_applied( filterRoutes.getAirline() );
    }
  }

  RFISCMode = ( tariffMap.status() == TSeatTariffMap::stUseRFISC )?rRFISC:rTariff;
  ProgTrace( TRACE5, "RFISCMode=%d, paxTariff=%d, tariff_pax_id=%d", RFISCMode, paxTariff, tariff_pax_id );
  tariffMap.trace(TRACE5);
  //������ ����
  if ( RFISCMode != rTariff ) {
    SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
    SelfCkinSalonTariff.setTariffMap( filterRoutesSets.point_dep, tariffMap );
    SetRFISC( filterRoutesSets.point_dep, tariffMap );
  }
  //AddRFISCRemarks( filterRoutesSets.point_dep, tariffMap );
/*  if ( !tariffMap.empty() ) {
    SetTariffsByColor( tariffMap, true );
  }*/



  //����� �� ��室� ������⢮ ᫮�� � ��⮬ 䨫���
  //������⢮ ���ᠦ�஢ ��� ��� 䨫��� - ����� 㤠���� ��, � ������ ��� ᫮�
  /* ����� ������⢮ ���� � ᫮ﬨ ���ᠦ�஢ � �ਧ����� �������� (vlInvalid,vlMultiVerify)
     ����⪠ ᫮�� vlInvalid �� �᫮���:
     ���� ����� �ਮ���� ᫮� �� ����� ������� ���ᠦ�஬, �� ���砫� ��� ᫮� ���� �஢���� �� �ਧ��� vlMultiVerify.
     �᫨ ��� ᫮� �ਭ������� ��㣮�� ���ᠦ���, � ���� 㡥������ � ⮬, �� �� ᠬ� �ਮ���� � ��� ��㣨� � ᠫ���
     ����� �ਮ����� ��� ������� ���ᠦ��

     ���� 㤠���� �� ᫮� � �ਧ����� vlInvalid
  */
  if ( !only_compon_props ) {
    validateLayersSeats( );
  }
}

void TSalonList::Build( bool with_pax,
                        xmlNodePtr salonsNode )
{         //compon
  BitSet<TDrawPropsType> props;
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  SetProp( salonsNode, "pr_lat_seat", isCraftLat() );
  int comp_crc = 0;
  ProgTrace( TRACE5, "getDepartureId=%d", getDepartureId() );
  if ( getDepartureId() != ASTRA::NoExists ) {
    comp_crc = getCRC_Comp( filterSets.filterRoutes.getDepartureId() );
  }
  if ( comp_crc != 0 ) {
    SetProp( salonsNode, "comp_crc", comp_crc );
  }
  SetProp( salonsNode, "RFISCMode", (int)getRFISCMode() );
  filterSets.filterRoutes.Build( NewTextChild( salonsNode, "filterRoutes" ) );
    //!logProgTrace( TRACE5, "TSalonList::Build: size()=%zu", size() );

  for( vector<TPlaceList*>::iterator placeList = begin(); placeList != end(); placeList++ ) {
    xmlNodePtr placeListNode = NewTextChild( salonsNode, "placelist" );
    SetProp( placeListNode, "num", (*placeList)->num );
    int xcount=0, ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      if ( !forBuild( *place, elem_types ) ) {
        continue;
      }
      if ( place->x > xcount )
        xcount = place->x;
      if ( place->y > ycount )
        ycount = place->y;
      place->Build( NewTextChild( placeListNode, "place" ),
                    getDepartureId(),
                    isCraftLat(),
                    getRFISCMode(), false,
                    true, pax_lists );
      props += place->drawProps;
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuLayers( getDepartureId() != ASTRA::NoExists,
                 filterSets.filtersLayers[ getDepartureId() ],
                 menuLayers );
  buildMenuLayers( getDepartureId() != ASTRA::NoExists,
                   menuLayers, props, salonsNode, getDepartureId() );
  TSeatTariffMap tariffMap;
  ProgTrace( TRACE5, "airline=%s", getAirline().c_str() );
  if ( !this->getAirline().empty() ) {
    if ( tariffMap.is_rfisc_applied( this->getAirline() ) ) {
      tst();
      xmlNodePtr tariffsNode = NewTextChild( salonsNode, "rfisc_colors" );
      for( std::map<std::string,TRFISC>::iterator i=tariffMap.begin(); i!=tariffMap.end(); i++ ) {
        xmlNodePtr n = NewTextChild( tariffsNode, "item" );
        SetProp( n, "color", i->first );
        SetProp( n, "figure", "rurect" );
      }
    }
  }
/*???  else { // base compon?
    tst();
    std::map<std::string,std::string> colors;
    getTariffColors( colors );
    if ( !colors.empty() ) {
      xmlNodePtr tariffsNode = NewTextChild( salonsNode, "rfisc_colors" );
      for ( map<string,string>::iterator i=colors.begin(); i!=colors.end(); i++ ) {
        xmlNodePtr n = NewTextChild( tariffsNode, "item" );
        SetProp( n, "color", i->first );
        SetProp( n, "figure", i->second );
      }
    }
  }*/
}

void checkRFISC( const TPlace &seat, const string  &color,
                 bool pr_lat, const TSeatTariffMap &tariffMap,
                 bool is_rfisc_applied )
{
  if ( is_rfisc_applied &&
       tariffMap.find( color ) == tariffMap.end() ) {
    throw UserException( "MSG.TARIFFCOLOR_NOTINUSE",
                         LParams()<<LParam("color",ElemIdToNameLong( etRateColor, color ))
                         <<LParam("seat1",seat.denorm_view(pr_lat)) );
  }

}

void checkTariffs( const TPlace &seat, const TSeatTariff &seatTariff,
                   std::map<std::string,pair<TSeatTariff, TPlace> > &uniqTariffs,
                   bool pr_lat )
{
  //�� ���� �஢�ઠ �� �, �� ��� ����� 梥� ���ᠭ�� � ����ன��� �� RFISC'��
  ProgTrace( TRACE5, "seatTariff.color=%s", seatTariff.color.c_str());
  std::map<std::string,pair<TSeatTariff, TPlace> >::iterator itariff;
  if ( !uniqTariffs.empty() ) {
    itariff = uniqTariffs.begin();
    if ( itariff->second.first.currency_id != seatTariff.currency_id ) {
      throw UserException( "MSG.DIFFERENTE_CURRENCY",
                            LParams()<<LParam("seat1", seat.denorm_view(pr_lat))
                                     <<LParam("currency1", ElemIdToNameShort( etCurrency, seatTariff.currency_id))
                                     <<LParam("seat2", itariff->second.second.denorm_view(pr_lat))
                                     <<LParam("currency2", ElemIdToNameShort( etCurrency, itariff->second.first.currency_id) ) );
    }
  }
  itariff = uniqTariffs.find( seatTariff.color );
  if ( itariff == uniqTariffs.end() ) {
    uniqTariffs.insert( make_pair( seatTariff.color, make_pair(seatTariff,seat) ) );
    return;
  }
  if ( itariff->second.first.rate != seatTariff.rate ) {
    throw UserException( "MSG.DIFFERENTE_PRICE",
                          LParams()<<LParam("color",ElemIdToNameLong( etRateColor, seatTariff.color ))
                                   <<LParam("seat1",seat.denorm_view(pr_lat))
                                   <<LParam("tariff1",seatTariff.rateView())
                                   <<LParam("seat2",itariff->second.second.denorm_view(pr_lat))
                                   <<LParam("tariff2",itariff->second.first.rateView()) );
  }
}

void TSalonList::Parse( int vpoint_id, const std::string &airline, xmlNodePtr salonsNode )
{
  ProgTrace( TRACE5, "TSalonList::Parse, point_id=%d, airlne=%s", vpoint_id, airline.c_str() );
  Clear();
  if ( salonsNode == NULL )
    return;
  xmlNodePtr node;
  bool pr_lat_seat_init = false;
  node = GetNode( "@pr_lat_seat", salonsNode );
  if ( node ) {
    pr_craft_lat = NodeAsInteger( node );
    pr_lat_seat_init = true;
  }
  RFISCMode = rTariff;
  node = GetNode( "@RFISCMode", salonsNode );
  if ( node ) {
    tst();
    RFISCMode =  (TRFISCMode)NodeAsInteger( node );
  }
  node = salonsNode->children;
  xmlNodePtr salonNode = NodeAsNodeFast( "placelist", node );
  TSeatRemark seatRemark;
  int lat_count = 0, rus_count = 0;
  string rus_lines = rus_seat, lat_lines = lat_seat;
  TElemFmt fmt;
  std::map<std::string,pair<TSeatTariff, TPlace> > uniqTariffs;
  TSeatTariffMap tariffMap;
  tariffMap.get_rfisc_colors( airline );
  if ( !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) ) {
    if ( tariffMap.status() != TSeatTariffMap::stUseRFISC ) {
      tariffMap.clear();
    }
    else {
      RFISCMode = rRFISC;
    }
  }
  tariffMap.trace( TRACE5 );
  std::map<std::string, int> remarks;
  LoadCompRemarksPriority( remarks );
  while ( salonNode ) {
    TPlaceList *placeList = new TPlaceList();
    placeList->num = NodeAsInteger( "@num", salonNode );
    xmlNodePtr placeNode = salonNode->children;
    while ( placeNode &&
            ( ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
                string( (const char*)placeNode->name ) == "seat" ) ||
              ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
                string( (const char*)placeNode->name ) == "place" ) ) ) {
      node = placeNode->children;
      TPlace place;
      place.x = NodeAsIntegerFast( "x", node );
      place.y = NodeAsIntegerFast( "y", node );
      place.elem_type = NodeAsStringFast( "elem_type", node );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( !GetNodeFast( "xprior", node ) )
        place.xprior = -1;
      else
        place.xprior = NodeAsIntegerFast( "xprior", node );
      if ( !GetNodeFast( "yprior", node ) )
        place.yprior = -1;
      else
        place.yprior = NodeAsIntegerFast( "yprior", node );
      if ( !GetNodeFast( "agle", node ) )
        place.agle = 0;
      else
        place.agle = NodeAsIntegerFast( "agle", node );
      place.clname = NodeAsStringFast( "class", node );
      if ( !place.clname.empty() ) {
        place.clname = ElemToElemId( etClass, place.clname, fmt );
        if ( fmt == efmtUnknown )
          throw UserException( "MSG.INVALID_CLASS" );
      }

      place.xname = NodeAsStringFast( "xname", node );

      if ( !pr_lat_seat_init ) {
        if ( rus_lines.find( place.xname ) != string::npos ) {
          rus_count++;
        }
          if ( lat_lines.find( place.xname ) != string::npos ) {
          lat_count++;
        }
        }
      place.xname = norm_iata_line( place.xname );
      place.yname = norm_iata_row( NodeAsStringFast( "yname", node ) );

      xmlNodePtr n1, n2;
      bool pr_disable_layer = false;
      if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
        n1 = GetNodeFast( "remarks", node );
        if ( n1 ) {
            n1 = n1->children;
            while ( n1 && string( (const char*)n1->name ) == "remark" ) {
              n2 = n1->children;
              int point_id = NodeAsIntegerFast( "point_id", n2, vpoint_id );
              seatRemark.value = NodeAsStringFast( "code", n2, "" );
              seatRemark.pr_denial = GetNodeFast( "pr_denial", n2 );
              verifyValidRem( place.clname, seatRemark.value );
              if ( remarks.find( seatRemark.value ) != remarks.end() ) { //�⮡� rfisc �� ��⠫�� ��������� ��� ६�ઠ �� ����஢���� ���������� ३� � �������, � ���ன ��� ����஥� rfisc
                place.AddRemark( point_id, seatRemark );
              }
              n1 = n1->next;
          }
        }
      }
      else { //prior version
        n1 = GetNodeFast( "rems", node );
        if ( n1 ) {
          n1 = n1->children;
          while ( n1 ) {
            n2 = n1->children;
            seatRemark.value = NodeAsStringFast( "rem", n2 );
            seatRemark.pr_denial = GetNodeFast( "pr_denial", n2 );
            if ( seatRemark.value == "X" ) {
              if ( !pr_disable_layer && !compatibleLayer( cltDisable ) && !seatRemark.pr_denial ) {
                pr_disable_layer = true;
              }
            }
            else {
              verifyValidRem( place.clname, seatRemark.value );
              if ( remarks.find( seatRemark.value ) != remarks.end() ) { //�⮡� rfisc �� ��⠫�� ��������� ��� ६�ઠ �� ����஢���� ���������� ३� � �������, � ���ன ��� ����஥� rfisc
                place.AddRemark( vpoint_id, seatRemark );
              }
            }
            n1 = n1->next;
          }
        }
      }
      set<TSeatLayer,SeatLayerCompare> uniqueLayers;
      n1 = GetNodeFast( "layers", node );
      if ( n1 ) {
        n1 = n1->children; //layer
        while( n1 && string( (const char*)n1->name ) == "layer" ) {
            n2 = n1->children;
            TSeatLayer seatlayer;
            seatlayer.layer_type = DecodeCompLayerType( NodeAsStringFast( "layer_type", n2, "" ) );
            if ( seatlayer.layer_type != cltUnknown ) {
            seatlayer.point_id = NodeAsIntegerFast( "point_id", n2, vpoint_id );
            seatlayer.point_dep = NodeAsIntegerFast( "point_dep", n2, vpoint_id ); // �㦭� ��� ������
            seatlayer.point_arv = NodeAsIntegerFast( "point_arv", n2, NoExists );
            seatlayer.pax_id = NodeAsIntegerFast( "pax_id", n2, NoExists );
            seatlayer.crs_pax_id = NodeAsIntegerFast( "crs_pax_id", n2, NoExists );
            if ( uniqueLayers.find( seatlayer ) == uniqueLayers.end() ) {
              uniqueLayers.insert( seatlayer ); // ��� ��� �६���
              seatlayer.time_create = NodeAsDateTimeFast( "time_create", n2, NoExists );
              place.AddLayer( seatlayer.point_id, seatlayer );
              //!logProgTrace( TRACE5, "seatlayer=%s", seatlayer.toString().c_str() );
            }
            }
            n1 = n1->next;
        }
      }
      if ( !TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) &&
           !compatibleLayer( cltDisable ) &&
           pr_disable_layer ) {
        TSeatLayer seatlayer;
        seatlayer.layer_type = cltDisable;
        seatlayer.point_id = vpoint_id;
        seatlayer.point_dep = vpoint_id;
        place.AddLayer( seatlayer.point_id, seatlayer );
      }

      if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
        n1 = GetNodeFast( "tariffs", node );
        if ( n1 ) {
          n1 = n1->children;
          while ( n1 && string( (const char*)n1->name ) == "tariff" ) {
            n2 = n1->children;
            TSeatTariff seatTariff;
            int point_id = NodeAsIntegerFast( "point_id", n2, vpoint_id );
            seatTariff.color = NodeAsStringFast( "color", n2, "" );
            if ( seatTariff.color != "clBtnFace" ) { // �訡�� �� ��஭� �ନ���� � ��砥 ���. ०���
              seatTariff.rate = NodeAsFloatFast( "value", n2, NoExists );
              seatTariff.currency_id = NodeAsStringFast( "currency_id", n2, "" );
              checkTariffs( place, seatTariff, uniqTariffs, pr_craft_lat );
              place.AddTariff( point_id, seatTariff );
              if ( !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) &&
                   tariffMap.status() == TSeatTariffMap::stUseRFISC ) {
                //if ( tariffMap.find( seatTariff.color ) != tariffMap.end() ) {
                  TRFISC rfisc;
                  rfisc.color = seatTariff.color;
                  checkRFISC(  place, rfisc.color, pr_craft_lat, tariffMap, tariffMap.is_rfisc_applied( airline ) );
                  place.AddRFISC( point_id, rfisc );
                //}
                if ( point_id != ASTRA::NoExists ) { // ᠫ�� ३� - � ०��� rFISC ���� 㤠���� ���� (।���஢���� ��� ��� rFISC)
                  place.clearTariffs();
                }
              }
            }
            n1 = n1->next;
          }
        }
      }
      else { //prior version
        n1 = GetNodeFast( "tarif", node );
        if ( n1 ) {
          TSeatTariff seatTariff;
          seatTariff.color = NodeAsString( "@color", n1 );
          if ( seatTariff.color != "clBtnFace" ) { // �訡�� �� ��஭� �ନ���� � ��砥 ���. ०���
            seatTariff.rate = NodeAsFloat( n1 );
            seatTariff.currency_id = NodeAsString( "@currency_id", n1 );
            checkTariffs( place, seatTariff, uniqTariffs, pr_craft_lat );
            place.AddTariff( vpoint_id, seatTariff );
            if ( !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) &&
                 tariffMap.status() == TSeatTariffMap::stUseRFISC ) {
              //if ( tariffMap.find( seatTariff.color ) != tariffMap.end() ) {
                TRFISC rfisc;
                rfisc.color = seatTariff.color;
                checkRFISC(  place, rfisc.color, pr_craft_lat, tariffMap, tariffMap.is_rfisc_applied( airline ) );
                place.AddRFISC( vpoint_id, rfisc );
              //}
              if ( vpoint_id != ASTRA::NoExists ) { // ᠫ�� ३� - � ०��� rFISC ���� 㤠���� ���� (।���஢���� ��� ��� rFISC)
                place.clearTariffs();
              }
            }
          }
        }
      }
      if ( TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION ) ) {
        n1 = GetNodeFast( "rfiscs", node );
        if ( n1 ) {
          n1 = n1->children;
          while ( n1 && string( (const char*)n1->name ) == "rfisc" ) {
            n2 = n1->children;
            TRFISC rfisc;
            int point_id = NodeAsIntegerFast( "point_id", n2, vpoint_id );
            rfisc.color = NodeAsStringFast( "color", n2, "" );

            if ( rfisc.color != "clBtnFace" ) { // �訡�� �� ��஭� �ନ���� � ��砥 ���. ०���
              checkRFISC(  place, rfisc.color, pr_craft_lat, tariffMap, tariffMap.is_rfisc_applied( airline ) );
              place.AddRFISC( point_id, rfisc );
            }
            n1 = n1->next;
          }
        }
      }
      else { //prior version
        n1 = GetNodeFast( "rfisc", node );
        if ( n1 ) {
          TRFISC rfisc;
          rfisc.color = NodeAsString( "@color", n1 );
          if ( rfisc.color != "clBtnFace" ) { // �訡�� �� ��஭� �ନ���� � ��砥 ���. ०���
            checkRFISC(  place, rfisc.color, pr_craft_lat, tariffMap, tariffMap.is_rfisc_applied( airline ) );
            place.AddRFISC( vpoint_id, rfisc );
          }
        }
      }
      place.visible = true;
      placeList->Add( place );
      placeNode = placeNode->next;
    }
    push_back( placeList );
    salonNode = salonNode->next;
  }
  if ( !pr_lat_seat_init ) {
    pr_craft_lat = ( lat_count >= rus_count );
  }
/*  if ( !TReqInfo::Instance()->desk.compatible( RFISC_VERSION ) &&
       tariffMap.is_rfisc_applied( airline ) ) {
    DropRFISCRemarks( tariffMap );
  }*/
}

void getEditableFlightLayers1( TFilterLayers &FilterLayers,
                              BitSet<ASTRA::TCompLayerType> &editabeLayers ) {
  editabeLayers.clearFlags();
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuLayers( true, FilterLayers, menuLayers );
  for ( std::map<ASTRA::TCompLayerType,TMenuLayer>::iterator imenulayer=menuLayers.begin();
        imenulayer!=menuLayers.end(); imenulayer++ ) {
    if ( imenulayer->second.editable ) {
      editabeLayers.setFlag( imenulayer->first );
    }
  }
}

void TSalonList::getEditableFlightLayers( BitSet<ASTRA::TCompLayerType> &editabeLayers ) {
  getEditableFlightLayers1( filterSets.filtersLayers[ getDepartureId() ], editabeLayers );
}

void TSalonList::WriteFlight( int vpoint_id, bool saveContructivePlaces )
{
  ProgTrace( TRACE5, "TSalonList::WriteFlight: point_id=%d, RFISCMode=%d, saveContructivePlaces=%d", vpoint_id, getRFISCMode(), saveContructivePlaces );
  TFlights flights;
  flights.Get( vpoint_id, ftTranzit );
  flights.Lock(__FUNCTION__);
  TQuery Qry( &OraSession );
  TQuery QryLayers( &OraSession );
  QryLayers.SQLText =
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,point_dep,point_arv,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create) "
    "  VALUES "
    "    (:range_id,:point_id,:point_dep,:point_arv,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:crs_pax_id,:pax_id,:time_create); "
    "END; ";
  QryLayers.CreateVariable( "range_id", otInteger, FNull );
  QryLayers.CreateVariable( "point_id", otInteger, vpoint_id );
  QryLayers.DeclareVariable( "point_dep", otInteger );
  QryLayers.DeclareVariable( "point_arv", otInteger );
  QryLayers.CreateVariable( "crs_pax_id", otInteger, FNull );
  QryLayers.CreateVariable( "pax_id", otInteger, FNull );
  QryLayers.DeclareVariable( "layer_type", otString );
  QryLayers.DeclareVariable( "first_xname", otString );
  QryLayers.DeclareVariable( "last_xname", otString );
  QryLayers.DeclareVariable( "first_yname", otString );
  QryLayers.DeclareVariable( "last_yname", otString );
  QryLayers.DeclareVariable( "time_create", otDate );
  TQuery QryReadX( &OraSession );
  FilterRoutesProperty filterRoutes;
  filterRoutes.Read( TFilterRoutesSets( vpoint_id ) );
  if ( getRFISCMode() == rRFISC ) { //!!!
    QryReadX.Clear();
    QryReadX.SQLText =
      "SELECT point_id,num,x,y,color,rate,rate_cur "
      " FROM trip_comp_rates "
      " WHERE point_id=:point_id ";
    QryReadX.CreateVariable( "point_id", otInteger, vpoint_id );
    QryReadX.Execute();
    ReadTariff( QryReadX, filterRoutes, ASTRA::NoExists );
  }
  if ( getRFISCMode() == rTariff )  { //!!!
    QryReadX.Clear();
    QryReadX.SQLText =
      "SELECT point_id,num,x,y,color "
      " FROM trip_comp_rfisc "
      " WHERE point_id=:point_id";
    QryReadX.CreateVariable( "point_id", otInteger, vpoint_id  );
    QryReadX.Execute();
    ReadRFISCColors( QryReadX, filterRoutes, ASTRA::NoExists );
  }
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  string sqlStr =
      "BEGIN "
      " UPDATE trip_sets SET pr_lat_seat=:pr_lat_seat WHERE point_id=:point_id; "
      " DELETE trip_comp_rem WHERE point_id=:point_id; "
      " DELETE trip_comp_rfisc WHERE point_id=:point_id; "
      " DELETE trip_comp_baselayers WHERE point_id=:point_id; "
      " DELETE trip_comp_rates WHERE point_id=:point_id; "
      " DELETE trip_comp_rfisc WHERE point_id=:point_id; ";
  if ( saveContructivePlaces ) {
      sqlStr +=
      " DELETE trip_comp_elems WHERE point_id=:point_id AND elem_type NOT IN ";
      sqlStr += GetSQLEnum( elem_types );
  }
  else {
    sqlStr +=
      " DELETE trip_comp_elems WHERE point_id=:point_id ";
  }
  sqlStr += ";";
  sqlStr += "END;";
  Qry.SQLText = sqlStr;
  Qry.CreateVariable( "point_id", otInteger, vpoint_id );
  Qry.CreateVariable( "pr_lat_seat", otInteger, isCraftLat() );
  Qry.Execute();
  //���⪠ 䨫��� ᫮�� �� ��襬� �㭪�� ��ᠤ��
  std::map<int,TFilterLayers> &filtersLayers = filterSets.filtersLayers;
  BitSet<TDrawPropsType> props;
  filtersLayers.clear();
  filtersLayers[ vpoint_id ].getFilterLayers( vpoint_id );
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuLayers( true, filterSets.filtersLayers[ vpoint_id ], menuLayers );
  //㤠����� �� ।����㥬�� ᫮��
  Qry.Clear();
  Qry.SQLText =
      "DELETE trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type=:layer_type";
  Qry.CreateVariable( "point_id", otInteger, vpoint_id );
  Qry.DeclareVariable( "layer_type", otString );
  for ( int ilayer=0; ilayer<ASTRA::cltTypeNum; ilayer++ ) {
    if ( isEditableMenuLayers( (ASTRA::TCompLayerType)ilayer, menuLayers ) ) {
        Qry.SetVariable( "layer_type", EncodeCompLayerType( (ASTRA::TCompLayerType)ilayer ) );
        Qry.Execute();
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
    " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, :xname,:yname)";
  Qry.CreateVariable( "point_id", otInteger, vpoint_id );
  Qry.DeclareVariable( "num", otInteger );
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );
  Qry.DeclareVariable( "elem_type", otString );
  Qry.DeclareVariable( "xprior", otInteger );
  Qry.DeclareVariable( "yprior", otInteger );
  Qry.DeclareVariable( "agle", otInteger );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );
  TQuery QryRemarks( &OraSession );
  QryRemarks.SQLText =
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
  QryRemarks.CreateVariable( "point_id", otInteger, vpoint_id );
  QryRemarks.DeclareVariable( "num", otInteger );
  QryRemarks.DeclareVariable( "x", otInteger );
  QryRemarks.DeclareVariable( "y", otInteger );
  QryRemarks.DeclareVariable( "rem", otString );
  QryRemarks.DeclareVariable( "pr_denial", otInteger );
  TQuery QryTariffs( &OraSession );
  QryTariffs.SQLText =
    "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
    " VALUES(:point_id,:num,:x,:y,:color,:rate,:rate_cur)";
  QryTariffs.CreateVariable( "point_id", otInteger, vpoint_id );
  QryTariffs.DeclareVariable( "num", otInteger );
  QryTariffs.DeclareVariable( "x", otInteger );
  QryTariffs.DeclareVariable( "y", otInteger );
  QryTariffs.DeclareVariable( "color", otString );
  QryTariffs.DeclareVariable( "rate", otFloat );
  QryTariffs.DeclareVariable( "rate_cur", otString );
  TQuery QryRFISC( &OraSession );
  QryRFISC.SQLText =
    "INSERT INTO trip_comp_rfisc(point_id,num,x,y,color) "
    " VALUES(:point_id,:num,:x,:y,:color)";
  QryRFISC.CreateVariable( "point_id", otInteger, vpoint_id );
  QryRFISC.DeclareVariable( "num", otInteger );
  QryRFISC.DeclareVariable( "x", otInteger );
  QryRFISC.DeclareVariable( "y", otInteger );
  QryRFISC.DeclareVariable( "color", otString );
  TQuery QryBaseLayers( &OraSession );
  QryBaseLayers.SQLText =
    "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
    " VALUES(:point_id,:num,:x,:y,:layer_type)";
  QryBaseLayers.CreateVariable( "point_id", otInteger, vpoint_id );
  QryBaseLayers.DeclareVariable( "num", otInteger );
  QryBaseLayers.DeclareVariable( "x", otInteger );
  QryBaseLayers.DeclareVariable( "y", otInteger );
  QryBaseLayers.DeclareVariable( "layer_type", otString );
  vector<TPlaceList*>::iterator plist;
  map<TClass,int> countersClass;
  TClass cl;
  std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
  std::map<int, TSeatTariff,classcomp> tariffs;
  std::map<int, TRFISC,classcomp> rfiscs;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  TDateTime layer_time_create = NowUTC();
  for ( vector<TPlaceList*>::iterator plist = begin(); plist != end(); plist++ ) {
    Qry.SetVariable( "num", (*plist)->num );
    QryRFISC.SetVariable( "num", (*plist)->num );
    QryTariffs.SetVariable( "num", (*plist)->num );
    QryRemarks.SetVariable( "num", (*plist)->num );
    QryBaseLayers.SetVariable( "num", (*plist)->num );
    for ( TPlaces::iterator iseat = (*plist)->places.begin(); iseat != (*plist)->places.end(); iseat++ ) {
      if ( !iseat->visible )
        continue;
      if ( saveContructivePlaces &&
           isConstructivePlace( iseat->elem_type, elem_types ) ) {
        continue;
      }
      Qry.SetVariable( "x", iseat->x );
      Qry.SetVariable( "y", iseat->y );
      Qry.SetVariable( "elem_type", iseat->elem_type );
      if ( iseat->xprior < 0 )
        Qry.SetVariable( "xprior", FNull );
      else
        Qry.SetVariable( "xprior", iseat->xprior );
      if ( iseat->yprior < 0 )
        Qry.SetVariable( "yprior", FNull );
      else
        Qry.SetVariable( "yprior", iseat->yprior );
      Qry.SetVariable( "agle", iseat->agle );
      if ( iseat->clname.empty() || !TCompElemTypes::Instance()->isSeat( iseat->elem_type ) )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", iseat->clname );
        cl = DecodeClass( iseat->clname.c_str() );
        if ( cl != NoClass ) {
          countersClass[ cl ]++;
        }
      }
      Qry.SetVariable( "xname", iseat->xname );
      Qry.SetVariable( "yname", iseat->yname );
      Qry.Execute();
      iseat->GetRemarks( remarks );
      if ( !remarks.empty() ) {
        QryRemarks.SetVariable( "x", iseat->x );
        QryRemarks.SetVariable( "y", iseat->y );
      }
      for ( std::map<int, std::vector<TSeatRemark>,classcomp >::iterator iremarks=remarks.begin(); iremarks!=remarks.end(); iremarks++ ) {
        if ( iremarks->first != vpoint_id ) {
          ProgError( STDLOG, "invalid remark.point_id=%d", iremarks->first );
          continue;
        }
        for ( std::vector<TSeatRemark>::iterator iremark=iremarks->second.begin(); iremark!=iremarks->second.end(); iremark++ ) {
          QryRemarks.SetVariable( "rem", iremark->value );
          if ( !iremark->pr_denial )
            QryRemarks.SetVariable( "pr_denial", 0 );
          else
            QryRemarks.SetVariable( "pr_denial", 1 );
          QryRemarks.Execute();
        }
      }
      iseat->GetTariffs( tariffs );
      if ( !tariffs.empty() ) {
        QryTariffs.SetVariable( "x", iseat->x );
        QryTariffs.SetVariable( "y", iseat->y );
      }
      for ( std::map<int, TSeatTariff,classcomp>::iterator itariff=tariffs.begin(); itariff!=tariffs.end(); itariff++) {
        if ( itariff->first != vpoint_id ) {
          ProgError( STDLOG, "invalid tariff.point_id=%d", itariff->first );
          continue;
        }
        QryTariffs.SetVariable( "color", itariff->second.color );
        QryTariffs.SetVariable( "rate", itariff->second.rate );
        QryTariffs.SetVariable( "rate_cur", itariff->second.currency_id );
        QryTariffs.Execute();
      }
      iseat->GetRFISCs( rfiscs );
      if ( !rfiscs.empty() ) {
        QryRFISC.SetVariable( "x", iseat->x );
        QryRFISC.SetVariable( "y", iseat->y );
      }
      for ( std::map<int, TRFISC,classcomp>::iterator irfisc=rfiscs.begin(); irfisc!=rfiscs.end(); irfisc++) {
        if ( irfisc->first != vpoint_id ) {
          ProgError( STDLOG, "invalid tariff.point_id=%d", irfisc->first );
          continue;
        }
        QryRFISC.SetVariable( "color", irfisc->second.color );
        QryRFISC.Execute();
      }
      iseat->GetLayers( layers, glAll );
      bool pr_baselayers_init = false, pr_otherlayers_init = false;
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin(); ilayers!=layers.end(); ilayers++ ) {
        for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin(); ilayer!=ilayers->second.end(); ilayer++ ) {
           if ( !isEditableMenuLayers( ilayer->layer_type, menuLayers ) ) {
             continue;
           }
           if ( isBaseLayer( ilayer->layer_type, false ) ) {
             //baselayers
             if ( !pr_baselayers_init ) {
               QryBaseLayers.SetVariable( "x", iseat->x );
               QryBaseLayers.SetVariable( "y", iseat->y );
               pr_baselayers_init = true;
             }
             QryBaseLayers.SetVariable( "layer_type", EncodeCompLayerType( ilayer->layer_type ) );
             QryBaseLayers.Execute();
             //!logProgTrace( TRACE5, "baselayers(%d,%d)=%s", iseat->x, iseat->y, EncodeCompLayerType( ilayer->layer_type ) );
             continue;
           }
           if ( ilayers->first != vpoint_id ) {
             ProgError( STDLOG, "invalid layer.point_id=%d, %s", ilayers->first, ilayer->toString().c_str() );
             continue;
           }
           //otherlayers
           if ( !pr_otherlayers_init ) {
             QryLayers.SetVariable( "first_xname", iseat->xname );
               QryLayers.SetVariable( "last_xname", iseat->xname );
               QryLayers.SetVariable( "first_yname", iseat->yname );
               QryLayers.SetVariable( "last_yname", iseat->yname );
             pr_otherlayers_init = true;
           }
           QryLayers.SetVariable( "layer_type", EncodeCompLayerType( ilayer->layer_type ) );
           if ( ilayer->point_dep != NoExists ) {
             QryLayers.SetVariable( "point_dep", ilayer->point_dep );
           }
           else {
             QryLayers.SetVariable( "point_dep", FNull );
           }
           if ( ilayer->point_arv != NoExists ) {
             QryLayers.SetVariable( "point_arv", ilayer->point_arv );
           }
           else {
             QryLayers.SetVariable( "point_arv", FNull );
           }
           if ( ilayer->time_create != NoExists ) {
             QryLayers.SetVariable( "time_create", ilayer->time_create );
           }
           else {
             QryLayers.SetVariable( "time_create", layer_time_create );
           }
               QryLayers.Execute();
               //!logProgTrace( TRACE5, "otherlayers: x=%d,y=%d,layer_type=%s,point_id=%d, point_dep=%d,point_arv=%d",
           //!log           iseat->x, iseat->y, EncodeCompLayerType( ilayer->layer_type ),
           //!log           ilayer->point_id, ilayer->point_dep, ilayer->point_arv );
        } // for layers
      }
    } //for place
  }
}

void TSalonList::WriteCompon( int &vcomp_id, const TComponSets &componSets, bool saveContructivePlaces )
{
  ProgTrace( TRACE5, "TSalonList::WriteCompon: comp_id=%d, modify=%d", vcomp_id, (int)componSets.modify );
  if ( componSets.modify == mNone ) {
    return;
  }
  /* ��࠭���� ���������� */
  TQuery Qry( &OraSession );
  if ( componSets.modify == mAdd ) {
    Qry.SQLText = "SELECT id__seq.nextval as comp_id FROM dual";
    Qry.Execute();
    vcomp_id = Qry.FieldAsInteger( "comp_id" );
  }
  Qry.Clear();
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  string sqlStr;
  switch ( (int)componSets.modify ) {
    case mChange:
      sqlStr =
        "BEGIN "
        " UPDATE comps SET airline=:airline,airp=:airp,craft=:craft,bort=:bort,descr=:descr, "
        "        time_create=system.UTCSYSDATE,classes=:classes,pr_lat_seat=:pr_lat_seat "
        "  WHERE comp_id=:comp_id; "
        " DELETE comp_rem WHERE comp_id=:comp_id; "
        " DELETE comp_baselayers WHERE comp_id=:comp_id; "
        " DELETE comp_rfisc WHERE comp_id=:comp_id; "
        " DELETE comp_rates WHERE comp_id=:comp_id; ";
      if ( saveContructivePlaces ) {
        sqlStr +=
        " DELETE comp_elems WHERE comp_id=:comp_id AND elem_type NOT IN ";
        sqlStr += GetSQLEnum( elem_types );
      }
      else {
        sqlStr +=
        " DELETE comp_elems WHERE comp_id=:comp_id ";
      }
      sqlStr += ";";
      sqlStr +=
        " DELETE comp_classes WHERE comp_id=:comp_id; "
        "END; ";
      ProgTrace( TRACE5, "sql=%s", sqlStr.c_str() );
      Qry.SQLText = sqlStr;
      break;
    case mAdd:
      Qry.SQLText =
        "INSERT INTO comps(comp_id,airline,airp,craft,bort,descr,time_create,classes,pr_lat_seat) "
        " VALUES(:comp_id,:airline,:airp,:craft,:bort,:descr,system.UTCSYSDATE,:classes,:pr_lat_seat) ";
      break;
    case mDelete:
      Qry.SQLText =
        "BEGIN "
        " UPDATE trip_sets SET comp_id=NULL WHERE comp_id=:comp_id; "
        " DELETE comp_rem WHERE comp_id=:comp_id; "
        " DELETE comp_baselayers WHERE comp_id=:comp_id; "
        " DELETE comp_rates WHERE comp_id=:comp_id; "
        " DELETE comp_rfisc WHERE comp_id=:comp_id; "
        " DELETE comp_elems WHERE comp_id=:comp_id; "
        " DELETE comp_sections WHERE comp_id=:comp_id; "
        " DELETE comp_classes WHERE comp_id=:comp_id; "
        " DELETE comps WHERE comp_id=:comp_id; "
        "END; ";
      break;
  }
  Qry.CreateVariable( "comp_id", otInteger, vcomp_id );
  if ( componSets.modify != mDelete ) {
    Qry.CreateVariable( "airline", otString, componSets.airline );
    Qry.CreateVariable( "airp", otString, componSets.airp );
    Qry.CreateVariable( "craft", otString, componSets.craft );
    Qry.CreateVariable( "descr", otString, componSets.descr );
    Qry.CreateVariable( "bort", otString, componSets.bort );
    Qry.CreateVariable( "classes", otString, componSets.classes );
    Qry.CreateVariable( "pr_lat_seat", otString, isCraftLat() );
  }
  Qry.Execute();
  if ( componSets.modify == mDelete )
    return; /* 㤠���� ���������� */

  TQuery QryRemarks( &OraSession );
  QryRemarks.SQLText =
    "INSERT INTO comp_rem(comp_id,num,x,y,rem,pr_denial) "
    " VALUES(:comp_id,:num,:x,:y,:rem,:pr_denial)";
  QryRemarks.CreateVariable( "comp_id", otInteger, vcomp_id );
  QryRemarks.DeclareVariable( "num", otInteger );
  QryRemarks.DeclareVariable( "x", otInteger );
  QryRemarks.DeclareVariable( "y", otInteger );
  QryRemarks.DeclareVariable( "rem", otString );
  QryRemarks.DeclareVariable( "pr_denial", otInteger );
  TQuery QryBaseLayers( &OraSession );
  QryBaseLayers.SQLText =
    "INSERT INTO comp_baselayers(comp_id,num,x,y,layer_type) "
    " VALUES(:comp_id,:num,:x,:y,:layer_type)";
  QryBaseLayers.CreateVariable( "comp_id", otInteger, vcomp_id );
  QryBaseLayers.DeclareVariable( "num", otInteger );
  QryBaseLayers.DeclareVariable( "x", otInteger );
  QryBaseLayers.DeclareVariable( "y", otInteger );
  QryBaseLayers.DeclareVariable( "layer_type", otString );
  TQuery QryTariffs( &OraSession );
  QryTariffs.SQLText =
    "INSERT INTO comp_rates(comp_id,num,x,y,color,rate,rate_cur) "
    " VALUES(:comp_id,:num,:x,:y,:color,:rate,:rate_cur)";
  QryTariffs.CreateVariable( "comp_id", otInteger, vcomp_id );
  QryTariffs.DeclareVariable( "num", otInteger );
  QryTariffs.DeclareVariable( "x", otInteger );
  QryTariffs.DeclareVariable( "y", otInteger );
  QryTariffs.DeclareVariable( "color", otString );
  QryTariffs.DeclareVariable( "rate", otFloat );
  QryTariffs.DeclareVariable( "rate_cur", otString );

  TQuery QryRFISC( &OraSession );
  QryRFISC.SQLText =
    "INSERT INTO comp_rfisc(comp_id,num,x,y,color) "
    " VALUES(:comp_id,:num,:x,:y,:color)";
  QryRFISC.CreateVariable( "comp_id", otInteger, vcomp_id );
  QryRFISC.DeclareVariable( "num", otInteger );
  QryRFISC.DeclareVariable( "x", otInteger );
  QryRFISC.DeclareVariable( "y", otInteger );
  QryRFISC.DeclareVariable( "color", otString );

  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO comp_elems(comp_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
    " VALUES(:comp_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class,:xname,:yname) ";
  Qry.DeclareVariable( "comp_id", otInteger );
  Qry.SetVariable( "comp_id", vcomp_id );
  Qry.DeclareVariable( "num", otInteger );
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );
  Qry.DeclareVariable( "elem_type", otString );
  Qry.DeclareVariable( "xprior", otInteger );
  Qry.DeclareVariable( "yprior", otInteger );
  Qry.DeclareVariable( "agle", otInteger );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );

  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuBaseLayers( menuLayers, true );
  map<TClass,int> countersClass;
  TClass cl;
  std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
  std::map<int, TSeatTariff,classcomp> tariffs;
  std::map<int, TRFISC,classcomp> rfiscs;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  for ( vector<TPlaceList*>::iterator plist=begin(); plist!=end(); plist++ ) {
    Qry.SetVariable( "num", (*plist)->num );
    QryRemarks.SetVariable( "num", (*plist)->num );
    QryBaseLayers.SetVariable( "num", (*plist)->num );
    QryTariffs.SetVariable( "num", (*plist)->num );
    QryRFISC.SetVariable( "num", (*plist)->num );
    for ( TPlaces::iterator iseat=(*plist)->places.begin(); iseat!=(*plist)->places.end(); iseat++ ) {
      if ( !iseat->visible ) {
        continue;
      }
      if ( componSets.modify == mChange &&
           saveContructivePlaces &&
           isConstructivePlace( iseat->elem_type, elem_types ) ) {
        continue;
      }
      Qry.SetVariable( "x", iseat->x );
      Qry.SetVariable( "y", iseat->y );
      Qry.SetVariable( "elem_type", iseat->elem_type );
      if ( iseat->xprior < 0 )
        Qry.SetVariable( "xprior", FNull );
      else
        Qry.SetVariable( "xprior", iseat->xprior );
      if ( iseat->yprior < 0 )
        Qry.SetVariable( "yprior", FNull );
      else
        Qry.SetVariable( "yprior", iseat->yprior );
      Qry.SetVariable( "agle", iseat->agle );
      if ( iseat->clname.empty() || !TCompElemTypes::Instance()->isSeat( iseat->elem_type ) )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", iseat->clname );
        cl = DecodeClass( iseat->clname.c_str() );
        if ( cl != NoClass )
          countersClass[ cl ]++;
      }
      Qry.SetVariable( "xname", iseat->xname );
      Qry.SetVariable( "yname", iseat->yname );
      Qry.Execute();

      iseat->GetRemarks( remarks );
      if ( !remarks.empty() ) {
        QryRemarks.SetVariable( "x", iseat->x );
        QryRemarks.SetVariable( "y", iseat->y );
        for( std::map<int, std::vector<TSeatRemark>,classcomp >::iterator iremarks=remarks.begin(); iremarks!=remarks.end(); iremarks++ ) {
          if ( iremarks->first != NoExists ) {
            ProgError( STDLOG, "invalid remark.point_id=%d", iremarks->first );
            continue;
          }
          for ( std::vector<TSeatRemark>::iterator iremark=iremarks->second.begin(); iremark!=iremarks->second.end(); iremark++ ) {
            QryRemarks.SetVariable( "rem", iremark->value );
            if ( !iremark->pr_denial )
              QryRemarks.SetVariable( "pr_denial", 0 );
            else
              QryRemarks.SetVariable( "pr_denial", 1 );
            QryRemarks.Execute();
          }
        }
      }
      iseat->GetTariffs( tariffs );
      if ( !tariffs.empty() ) {
        QryTariffs.SetVariable( "x", iseat->x );
        QryTariffs.SetVariable( "y", iseat->y );
      }
      for ( std::map<int, TSeatTariff,classcomp>::iterator itariff=tariffs.begin(); itariff!=tariffs.end(); itariff++) {
        if ( itariff->first != NoExists ) {
          ProgError( STDLOG, "invalid tariff.point_id=%d", itariff->first );
          continue;
        }
        QryTariffs.SetVariable( "color", itariff->second.color );
        QryTariffs.SetVariable( "rate", itariff->second.rate );
        QryTariffs.SetVariable( "rate_cur", itariff->second.currency_id );
        QryTariffs.Execute();
      }
      iseat->GetRFISCs( rfiscs );
      if ( !rfiscs.empty() ) {
        QryRFISC.SetVariable( "x", iseat->x );
        QryRFISC.SetVariable( "y", iseat->y );
      }
      for ( std::map<int, TRFISC,classcomp>::iterator irfisc=rfiscs.begin(); irfisc!=rfiscs.end(); irfisc++) {
        if ( irfisc->first != NoExists ) {
          ProgError( STDLOG, "invalid tariff.point_id=%d", irfisc->first );
          continue;
        }
        QryRFISC.SetVariable( "color", irfisc->second.color );
        QryRFISC.Execute();
      }
      iseat->GetLayers( layers, glAll );
      bool pr_init = false;
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin(); ilayers!=layers.end(); ilayers++ ) {
        if ( ilayers->first != NoExists ) {
            ProgError( STDLOG, "invalid ilayers.point_id=%d", ilayers->first );
            continue;
        }
        for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin(); ilayer!=ilayers->second.end(); ilayer++ ) {
          if ( !isEditableMenuLayers( ilayer->layer_type, menuLayers ) ) {
            continue;
          }
          if ( !isBaseLayer( ilayer->layer_type, true ) ) {
            continue;
          }
          if ( !pr_init ) {
            QryBaseLayers.SetVariable( "x", iseat->x );
            QryBaseLayers.SetVariable( "y", iseat->y );
            pr_init = true;
          }
          QryBaseLayers.SetVariable( "layer_type", EncodeCompLayerType( ilayer->layer_type ) );
          //!logProgTrace( TRACE5, "baselayers(%d,%d)=%s", iseat->x, iseat->y, EncodeCompLayerType( ilayer->layer_type ) );
          QryBaseLayers.Execute();
        }
      }
    } //for place
  }
  // ��࠭塞 ���䨣���� ����
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO comp_classes(comp_id,class,cfg) VALUES(:comp_id,:class,:cfg)";
  Qry.CreateVariable( "comp_id", otInteger, vcomp_id );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "cfg", otInteger );
  for ( map<TClass,int>::iterator i=countersClass.begin(); i!=countersClass.end(); i++ ) {
    if ( i->second > 999 )
      throw UserException( "MSG.SALONS.MATCH_PLACES" );
    Qry.SetVariable( "class", EncodeClass( i->first ) );
    Qry.SetVariable( "cfg", i->second );
    Qry.Execute();
  }
}

TPropsPoints::TPropsPoints( const FilterRoutesProperty &filterRoutes, int point_dep, int point_arv )
{
  bool inRoute = true;
  bool beforeDeparture = true;
  //!logProgTrace( TRACE5, "TPropsPoints: point_arv=%d", point_arv );
  for ( std::vector<TTripRouteItem>::const_iterator iroute=filterRoutes.begin();
        iroute!=filterRoutes.end(); ++iroute ) {
    if ( iroute->point_id == point_arv ) {
      inRoute = false;
    }
    if ( iroute->point_id == point_dep ) {
      beforeDeparture = false;
    }
    push_back( TPointInRoute( iroute->point_id, inRoute, beforeDeparture ) );
    //!logProgTrace( TRACE5, "TPropsPoints: points.push_back(%d,%d,%d)", iroute->point_id, inRoute, beforeDeparture );
  }
}

bool TPropsPoints::getPropRoute( int point_id, TPointInRoute &point )
{
  for ( vector<TPointInRoute>::iterator ipoint=begin(); ipoint!=end(); ipoint++ ) {
    if ( ipoint->point_id == point_id ) {
      point.point_id = ipoint->point_id;
      point.inRoute = ipoint->inRoute;
      point.beforeDeparture = ipoint->beforeDeparture;
      return true;
    }
  }
  return false;
}

bool TPropsPoints::getLastPropRouteDeparture( TPointInRoute &point )
{
  point = TPointInRoute();
  for( vector<TPointInRoute>::const_reverse_iterator iroute=rbegin();
       iroute!=rend(); iroute++ ) {
    if ( iroute->inRoute ) {
      point = *iroute;
      return true;
    }
  }
  return false;
}

void TSalonList::convertSeatTariffs( TPlace &iseat, bool pr_departure_tariff_only, int point_dep, int point_arv ) const
{
  TPropsPoints points( filterSets.filterRoutes, point_dep, point_arv );
  vector<int> pnts;
  for ( vector<TPointInRoute>::const_iterator ipoint=points.begin(); ipoint!=points.end(); ipoint++ ) {
    pnts.push_back( ipoint->point_id );
  }
  iseat.convertSeatTariffs( pr_departure_tariff_only, point_dep, pnts );
}

bool TSalonList::CreateSalonsForAutoSeats( TSalons &Salons,
                                           TFilterRoutesSets &filterRoutes,
                                           bool pr_departure_tariff_only,
                                           const std::vector<ASTRA::TCompLayerType> &grp_layers,
                                           const TPaxsCover &paxs,
                                           TDropLayersFlags &dropLayersFlags )
{
  bool pr_web_terminal = ( TReqInfo::Instance()->client_type != ASTRA::ctTerm &&
                           TReqInfo::Instance()->client_type != ASTRA::ctPNL );

  Salons.Clear();
  if ( filterRoutes.point_arv == filterRoutes.point_dep ) {
    tst();
    return false;
  }
  ProgTrace( TRACE5, "filterRoutes.point_dep=%d, filterRoutes.point_arv=%d, drop_not_web_passes=%d, pr_web_terminal=%d",
             filterRoutes.point_dep, filterRoutes.point_arv, dropLayersFlags.isFlag( clDropNotWeb ), pr_web_terminal );
  TPropsPoints points( filterSets.filterRoutes, filterRoutes.point_dep, filterRoutes.point_arv );
  Salons.Clear();
  Salons.trip_id = getDepartureId();
  Salons.comp_id = getCompId();
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  getMenuLayers( true,
                 filterSets.filtersLayers[ getDepartureId() ],
                 menuLayers );
  Salons.SetProps( filterSets.filtersLayers[ getDepartureId() ],
                   rTripSalons,
                   isCraftLat(),
                   filterSets.filterClass,
                   menuLayers );
  //���� ᮧ���� ᠫ�� �� �᭮�� filterRoutes.point_dep, filterRoutes.point_arv
  for ( std::vector<TPlaceList*>::iterator iseatlist=begin(); iseatlist!=end(); iseatlist++ ) {
    Salons.placelists.push_back( *iseatlist );
  }
  //⥯��� 䨫��� �� ��譨� ᢮��⢠� � ��࠭���� �� � ����� TPlace
  std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
  set<TSeatRemark,SeatRemarkCompare> uniqueRemarks;
  std::map<int, TSeatTariff,classcomp> tariffs;
  set<TSeatTariff,SeatTariffCompare> uniqueTariffs;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
  set<TSeatLayer,SeatLayerCompare> uniqueLayers;
  vector<TSeatLayer> currLayers;
  for ( vector<ASTRA::TCompLayerType>::const_iterator ilayer=grp_layers.begin();
        ilayer!=grp_layers.end(); ilayer++ ) {
    TSeatLayer currLayer;
    currLayer.point_dep = getDepartureId();
    currLayer.point_id =  getDepartureId();
    currLayer.point_arv =  getArrivalId();
    currLayer.inRoute = true;
    currLayer.time_create = NowUTC();
    currLayer.layer_type = *ilayer;
    currLayers.push_back( currLayer );
  }
  TRem rem;
  TPointInRoute point;
  for ( std::vector<TPlaceList*>::iterator iseatlist=Salons.placelists.begin();
        iseatlist!=Salons.placelists.end(); iseatlist++ ) {
    for ( IPlace iseat=(*iseatlist)->places.begin(); iseat!=(*iseatlist)->places.end(); iseat++ ) {
      //���������� ६�ப
      iseat->rems.clear();
      remarks.clear();
      iseat->GetRemarks( remarks );
      uniqueRemarks.clear();
      //����� ����� ���� ������⢮ ६�ப
      for ( std::map<int,vector<TSeatRemark> >::iterator iremarks = remarks.begin(); iremarks != remarks.end(); iremarks++ ) {
        for ( std::vector<TSeatRemark>::iterator irem=iremarks->second.begin(); irem!=iremarks->second.end(); irem++ ) {
          if ( !points.getPropRoute( iremarks->first, point ) ||
               !point.inRoute ) {
            continue;
          }
          if ( uniqueRemarks.find( *irem ) != uniqueRemarks.end() ) {
            continue;
          }
          //!logProgTrace( TRACE5, "CreateSalonsForAutoSeats: add remark(%s) value=%s, pr_denial=%d",
          //!log           string(iseat->yname+iseat->xname).c_str(), irem->value.c_str(), irem->pr_denial );
          uniqueRemarks.insert( *irem );
          rem.rem = irem->value;
          rem.pr_denial = irem->pr_denial;
          iseat->rems.push_back( rem );
          Salons.AddExistSubcls( rem );
        }
      }
      // ���������� �������ᮢ

      //���������� ��䮢
//      tariffs.clear();
//      iseat->GetTariffs( tariffs );
//      uniqueTariffs.clear();
      if ( getRFISCMode() != rRFISC ) { // ���� ०�� ࠡ��� � ��䠬�
        convertSeatTariffs( *iseat, pr_departure_tariff_only, filterRoutes.point_dep, filterRoutes.point_arv );
      }

      //�᫨ ���ᠦ�� ��� ������� ���⭮� ����, � ���� �᪠�� ��� ⮫쪮 � ��襬 �㭪�
      //�᫨ ���ᠦ�� ॣ��������� �� �� ���⭮� ����, � ���� ���� ���⭮� �� ��������
      /*for ( vector<TPointInRoute>::iterator ipoint=points.begin(); ipoint!=points.end(); ipoint++ ) {
        if ( pr_departure_tariff_only && ipoint->point_id != getDepartureId() ) {
          continue;
        }
        if ( tariffs.find( ipoint->point_id ) == tariffs.end() ) {
          continue;
        }
        //!logProgTrace( TRACE5, "ipoint->point_id=%d", ipoint->point_id );
        if ( uniqueTariffs.find( tariffs[ ipoint->point_id ] ) != uniqueTariffs.end() ) {
          //!log tst();
          continue;
        }
        uniqueTariffs.insert( tariffs[ ipoint->point_id ] );
        //!logProgTrace( TRACE5, "ipoint->point_id=%d, color=%s,", ipoint->point_id, tariffs[ ipoint->point_id ].color.c_str() );
        iseat->AddTariff( tariffs[ ipoint->point_id ].color,
                          tariffs[ ipoint->point_id ].value,
                          tariffs[ ipoint->point_id ].currency_id );
        ProgTrace( TRACE5, "SeatTariff=%s", iseat->SeatTariff.tariffStr().c_str() );
        break;
      }*/
      //���������� ᫮��: 㤠�塞 ⮫쪮 ����� �ਮ����
      layers.clear();
      iseat->layers.clear();
      iseat->GetLayers( layers, glAll );
      uniqueLayers.clear();
      bool pr_blocked_layer = false;

      TSeatLayer tmp_layer = iseat->getDropBlockedLayer( getDepartureId() );
      if ( tmp_layer.layer_type != cltUnknown ) {
        iseat->AddLayerToPlace( tmp_layer.layer_type, tmp_layer.time_create, tmp_layer.getPaxId(),
                                  tmp_layer.point_dep, tmp_layer.point_arv,
                                BASIC_SALONS::TCompLayerTypes::Instance()->priority( tmp_layer.layer_type ) );
        //logProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s add %s",
        //log           string(iseat->yname+iseat->xname).c_str(), tmp_layer.toString().c_str() );
        pr_blocked_layer = true;
      }

      for( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin(); ilayers!=layers.end(); ilayers++ ) {
        if ( pr_blocked_layer ) {
          break;
        }
        for ( std::set<TSeatLayer>::iterator ilayer=ilayers->second.begin(); ilayer!=ilayers->second.end(); ilayer++ ) {
          if ( pr_blocked_layer ) {
            break;
          }
          tmp_layer = *ilayer;
          if ( ilayers->first != getDepartureId() ) { //�᫨ �� �� ��� �㭪� �뫥�
            if ( !points.getPropRoute( ilayers->first, point ) ) { //᫮� �� ������ - ⠪��� �� ����� ����
              //logProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s, not add %s", string(iseat->yname+iseat->xname).c_str(),
              //log           ilayer->toString().c_str() );
              continue;
            }
            //�஢���� �ਮ��� ᫮� �⭮�⥫쭮 ��襩 ��㯯�
            TPointDepNum currLayerDepNum;
            if ( point.beforeDeparture ) {
              tmp_layer.point_dep_num = pdPrior;
              currLayerDepNum = pdNext;
            }
            else {
              tmp_layer.point_dep_num = pdNext;
              currLayerDepNum = pdPrior;
            }
            for ( vector<TSeatLayer>::iterator icurrLayer=currLayers.begin(); //ᯨ᮪ ᫮��, ����묨 ������㥬 ࠧ����� ����
                  icurrLayer!=currLayers.end(); icurrLayer++ ) {
              icurrLayer->point_dep_num = currLayerDepNum;
              if ( !compareSeatLayer( *icurrLayer, tmp_layer ) ) {
                iseat->AddLayerToPlace( cltDisable, icurrLayer->time_create, icurrLayer->getPaxId(),
                                          icurrLayer->point_dep, NoExists,
                                        BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltDisable ) );
                ProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s add cltDisable because %s", string(iseat->yname+iseat->xname).c_str(),
                           ilayer->toString().c_str() );
                pr_blocked_layer = true;
                break;
              }
            }
          }
                if ( pr_web_terminal && !paxs.empty() ) { //�ॡ㥬 ���������� ᯨ᪠ ���ᠦ�஢
                  //!logProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s %s, paxs.empty=%d, isOwnerFreePlace=%d",
            //!log           ilayer->toString().c_str(), tmp_layer.toString().c_str(), paxs.empty(),
            //!log           AstraWeb::isOwnerFreePlace( tmp_layer.getPaxId(), paxs ) );
                    if ( ( tmp_layer.layer_type == cltPNLCkin ||
                   isUserProtectLayer( tmp_layer.layer_type ) )  && !isOwnerFreePlace<TPaxCover>( tmp_layer.getPaxId(), paxs ) ) {
                iseat->AddLayerToPlace( cltDisable, tmp_layer.time_create, tmp_layer.getPaxId(),
                                          tmp_layer.point_dep, NoExists,
                                        BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltDisable ) );
                ProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s add cltDisable because %s", string(iseat->yname+iseat->xname).c_str(),
                           ilayer->toString().c_str() );
                pr_blocked_layer = true;
            }
          }
          if ( pr_blocked_layer ) {
            break;
          }
          if ( !points.getPropRoute( ilayers->first, point ) ||
               !point.inRoute ) { //�뫥� ᫮� ��᫥ ��१���� �㭪�
            ProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s, not add %s", string(iseat->yname+iseat->xname).c_str(),
                       ilayer->toString().c_str() );
            continue;
          }
          TPointInRoute point;
          if ( dropLayersFlags.isFlag( clDropBlockCentLayers ) &&
               ilayer->point_id != getDepartureId() &&
               ilayer->layer_type == cltBlockCent ) { //㤠�塞 ᫮� �����஢�� 業�஢�� �� ��� ������ �㭪��
            //!logProgTrace( TRACE5, "drop not web pass %s", ilayer->toString().c_str() );
            continue;
          }
          if ( dropLayersFlags.isFlag( clDropNotWeb ) &&
               points.getLastPropRouteDeparture( point ) &&
               ilayer->point_id == point.point_id &&
               ilayer->getPaxId() != ASTRA::NoExists ) { //㤠�塞 ��� ���ᠦ�஢, ����� �� web_client
            std::map<int,TPaxList>::iterator ipax_list = pax_lists.find( ilayers->first );
            if ( ipax_list != pax_lists.end() &&
                 !ipax_list->second.isWeb( ilayer->getPaxId() ) ) {
              //!logProgTrace( TRACE5, "drop not web pass %s", ilayer->toString().c_str() );
              continue;
            }
          }
          tmp_layer = TSeatLayer();
          tmp_layer.layer_type = ilayer->layer_type;
          if ( uniqueLayers.find( tmp_layer ) != uniqueLayers.end() ) {
            continue;
          }
          uniqueLayers.insert( tmp_layer );
          //logProgTrace( TRACE5, "CreateSalonsForAutoSeats: %s add %s",
          //log           string(iseat->yname+iseat->xname).c_str(), ilayer->toString().c_str() );
          iseat->AddLayerToPlace( ilayer->layer_type, ilayer->time_create, ilayer->getPaxId(),
                                  ilayer->point_dep, ilayer->point_arv,
                                  BASIC_SALONS::TCompLayerTypes::Instance()->priority( ilayer->layer_type ) );
          if ( isBaseLayer( tmp_layer.layer_type, false ) ) {
            Salons.AddExistBaseLayer( tmp_layer.layer_type );
          }
        }
      }
      if ( Salons.canAddOccupy( &(*iseat) ) ) {
        Salons.AddOccupySeat( (*iseatlist)->num, iseat->x, iseat->y );
        //ProgTrace(TRACE5, "AddOccupySeat %d, %d, %d, %d",(*iseatlist)->num, iseat->x, iseat->y, salons.c() );
      }
    } // end for iseat
  }
  bool res = ( filterRoutes.point_arv != filterRoutes.point_dep );
  bool pr_lastRoute = points.getLastPropRouteDeparture( point );
  if ( !dropLayersFlags.isFlag( clDropBlockCentLayers ) &&
       pr_lastRoute &&
       point.point_id != filterRoutes.point_dep ) { //�� ᢮��� �㭪�� �� 㤠�塞 �����஢�� 業�஢��
    dropLayersFlags.setFlag( clDropBlockCentLayers );
  }
  else
    if ( !dropLayersFlags.isFlag( clDropNotWeb ) &&
         pr_lastRoute &&
         point.point_id != filterRoutes.point_dep ) {
      dropLayersFlags.setFlag( clDropNotWeb );
    }
    else {
      //��१���� ��᫥����� �㭪�
      if ( pr_lastRoute ) {
        filterRoutes.point_arv = point.point_id;
      }
      else {
        filterRoutes.point_arv = filterRoutes.point_dep;
      }
      dropLayersFlags.clearFlags( );
    }
  //!logProgTrace( TRACE5, "filterRoutes.point_dep=%d, filterRoutes.point_arv=%d,drop_web_passes=%d",
  //!log           filterRoutes.point_dep, filterRoutes.point_arv, drop_not_web_passes );
  tst();
  return res;
}

void check_waitlist_alarm_on_tranzit_routes( int point_dep )
{
  std::set<int> paxs_external_logged;
  check_waitlist_alarm_on_tranzit_routes( point_dep, paxs_external_logged );
}

void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm )
{
  std::set<int> paxs_external_logged;
  check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm, paxs_external_logged );
}

void check_waitlist_alarm_on_tranzit_routes( int point_dep, const std::set<int> &paxs_external_logged )
{
  std::vector<int> points_tranzit_check_wait_alarm( 1, point_dep );
  check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm, paxs_external_logged );
}

void check_waitlist_alarm_on_tranzit_routes( const std::vector<int> &points_tranzit_check_wait_alarm,
                                             const std::set<int> &paxs_external_logged )
{
  TFlights flights;
  flights.Get( points_tranzit_check_wait_alarm, ftAll );
  flights.Lock(__FUNCTION__);

  TSalonList salonList;
  TSalonPassengers passengers;
  FilterRoutesProperty filterRoutes;
  //bool pr_exists_salons;
  for ( TFlights::iterator iflights=flights.begin(); iflights!=flights.end(); iflights++ ) { //�஡�� �� ३ᠬ
    for ( FlightPoints::iterator iroute=iflights->begin(); iroute!=iflights->end()-1; iroute++ ) { //�஡�� �� �㭪⠬
      //!logProgTrace( TRACE5, "check_waitlist_alarm_on_tranzit_routes: point_id=%d", iroute->point_id );
      FilterRoutesProperty filterRoutesTmp;
      try {
        filterRoutesTmp.Read( TFilterRoutesSets( iroute->point_id, ASTRA::NoExists ) ); //�⥭�� ������� ३�
      }
      catch( UserException &e ) {
        //!log tst();
        if ( e.getLexemaData().lexema_id != "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" &&
             e.getLexemaData().lexema_id != "MSG.FLIGHT.CANCELED.REFRESH_DATA" )
          throw;
        continue;
      }
      if ( filterRoutes.getMaxRoute() != filterRoutesTmp.getMaxRoute() ) { //����. ���� �� ࠢ�� �।��饬�
//        ProgTrace( TRACE5, "check_waitlist_alarm_on_tranzit_routes: point_id=%d, filterRoutesSets.point_dep=%d,%d, filterRoutesTmp.getMaxRoute()=%d,%d, filterRoutesTmp.getArrivalId()=%d",
//                   iroute->point_id, filterRoutes.getMaxRoute().point_dep, filterRoutes.getMaxRoute().point_arv,
//                   filterRoutesTmp.getMaxRoute().point_dep, filterRoutesTmp.getMaxRoute().point_arv,filterRoutesTmp.getArrivalId()  );
        filterRoutes = filterRoutesTmp;
        if ( iroute->point_id == filterRoutes.getArrivalId() ) { //��� �뫥�
          //pr_exists_salons = false;
//          tst();
          continue;
        }
//        ProgTrace( TRACE5, "iroute->point_id=%d", iroute->point_id );
        salonList.ReadFlight( TFilterRoutesSets( iroute->point_id, filterRoutes.getArrivalId() ), rfTranzitVersion, "", ASTRA::NoExists, true );
        salonList.check_waitlist_alarm_on_tranzit_routes( paxs_external_logged );
      }
    }
  }
}

void getStrWaitListReason( const std::string &fullname,
                                   const std::string &seat_no,
                                   const std::string &airp_dep,
                                   const std::string &airp_arv,
                                   int regNo,
                                   LEvntPrms &params)
{
  params << PrmSmpl<std::string>("fullname", fullname);
  if ( !seat_no.empty() ) {
    PrmLexema lexema("seat", "EVT.SEAT_NO");
    lexema.prms << PrmSmpl<std::string>("seat_no", seat_no);
    params << lexema;
  }
  else params << PrmSmpl<std::string>("seat", "");
  if ( !airp_dep.empty() && !airp_arv.empty() ) {
    PrmLexema lexema("route", "EVT.ROUTE");
    lexema.prms << PrmElem<std::string>("airp_dep", etAirp, airp_dep)
                   << PrmElem<std::string>("airp_arv", etAirp, airp_arv);
    params << lexema;
  }
  else params << PrmSmpl<std::string>("route", "");
  if ( regNo != ASTRA::NoExists ) {
    PrmLexema lexema("reg_no", "EVT.REG_NO");
    lexema.prms << PrmSmpl<int>("reg_no", regNo);
    params << lexema;
  }
  else params << PrmSmpl<std::string>("reg_no", "");
}

void CheckWaitListToLog( TQuery &QryAirp,
                         bool pr_exists,
                         int pax_id,
                         int point_dep,
                         const std::map<int,TPaxList> &pax_lists,
                         const std::map<int,TSalonPax> &passes,
                         bool pr_craft_lat )
{
  std::map<int,TSalonPax>::const_iterator ipass = passes.find( pax_id );
  if ( ipass == passes.end() ) {
    ProgError( STDLOG, "CheckWaitListToLog: invalid pax_id=%d", pax_id );
    return;
  }
  string fullname = ipass->second.surname;
  fullname = TrimString( fullname )  + (ipass->second.name.empty()?"":" ") + ipass->second.name;
  TWaitListReason waitListReason;
  PrmEnum seatPrmEnum("seat", "");
  QryAirp.SetVariable( "point_id", point_dep );
  QryAirp.Execute();
  string airline;
  if ( !QryAirp.Eof ) {
    airline = QryAirp.FieldAsString( "airline" );
  }
  string new_seat_no = ipass->second.event_seat_no( pr_craft_lat, point_dep, waitListReason, seatPrmEnum.prms );
  if ( waitListReason.layerStatus == layerValid ) {
    if ( pr_exists ) {
      TReqInfo::Instance()->LocaleToLog("EVT.PASSENGER_CHANGE_SEAT_AUTOMATICALLY",
                                        LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                                    << seatPrmEnum,
                                        evtPax, point_dep, ipass->second.reg_no, ipass->second.grp_id);
    }
    else {
      TReqInfo::Instance()->LocaleToLog("EVT.PASSENGER_SEATED_AUTOMATICALLY",
                                        LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                                    << seatPrmEnum,
                                        evtPax, point_dep, ipass->second.reg_no, ipass->second.grp_id );
    }
    return;
  }
  if ( new_seat_no.empty() ) {
    new_seat_no = ipass->second.prior_seat_no( "list", pr_craft_lat );
  }
  LEvntPrms params;
  string airp_dep, airp_arv;
  int regNo = ASTRA::NoExists;
  switch( waitListReason.layerStatus ) {
    case layerInvalid:
      params << PrmLexema("reason", "EVT.REASON_LAYER_INVALID");
      getStrWaitListReason( fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
      break;
    case layerLess:
      airp_dep.clear();
      airp_arv.clear();
      if ( waitListReason.layer.getPaxId() != ASTRA::NoExists ) {
        std::map<int,TPaxList>::const_iterator ipax_list = pax_lists.find( waitListReason.layer.point_id );
        if ( ipax_list != pax_lists.end() ) {
          regNo = ipax_list->second.getRegNo( waitListReason.layer.getPaxId() );
        }
        QryAirp.SetVariable( "point_id", waitListReason.layer.point_dep );
        QryAirp.Execute();
        if ( !QryAirp.Eof ) {
          airp_dep = QryAirp.FieldAsString( "airp" );
        }
        QryAirp.SetVariable( "point_id", waitListReason.layer.point_arv );
        QryAirp.Execute();
        if ( !QryAirp.Eof ) {
          airp_arv = QryAirp.FieldAsString( "airp" );
        }
      }
      switch ( waitListReason.layer.layer_type ) {
        case cltBlockCent:
          params << PrmLexema("reason", "EVT.REASON_BLOCK_CENT");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltDisable:
          params << PrmLexema("reason", "EVT.REASON_DISABLE");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltProtBeforePay:
        case cltProtAfterPay:
        case cltPNLBeforePay:
        case cltPNLAfterPay:
          params << PrmLexema("reason", "EVT.REASON_PAYMENT");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltBlockTrzt:
        case cltSOMTrzt:
        case cltPRLTrzt:
        case cltProtTrzt:
          params << PrmLexema("reason", "EVT.REASON_TRZT");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltPNLCkin:
          params << PrmLexema("reason", "EVT.REASON_PNL_CKIN");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltProtCkin:
          params << PrmLexema("reason", "EVT.REASON_PROT_CKIN");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
          break;
        case cltProtect:
        case cltProtSelfCkin:
          params << PrmLexema("reason", "EVT.REASON_PROTECT");
          getStrWaitListReason(fullname, airp_dep, new_seat_no, airp_arv, regNo, params);
          break;
        case cltCheckin:
        case cltTCheckin:
        case cltGoShow:
        case cltTranzit:
          params << PrmLexema("reason", "EVT.REASON_COOUPIED");
          getStrWaitListReason(fullname, new_seat_no, airp_dep, airp_arv, regNo, params);
        default:;
      };
      break;
    default:
      break;
  };
  TReqInfo::Instance()->LocaleToLog("EVT.WAIT_LIST_REASON", params, evtPax,
                                  point_dep, ipass->second.reg_no, ipass->second.grp_id );
}

void TSalonList::check_waitlist_alarm_on_tranzit_routes( const std::set<int> &paxs_external_logged )
{
  ProgTrace( TRACE5, "TSalonPassengers::check_waitlist_alarm" );
  TSalonPassengers passengers;
  TGetPassFlags flgGetPass;
  flgGetPass.setFlag( gpWaitList );
  flgGetPass.setFlag( gpPassenger );
  getPassengers( passengers, flgGetPass );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pax_id, xname, yname from pax_seats "
    " WHERE point_id=:point_id AND NVL(pr_wl,0)=0";
  Qry.DeclareVariable( "point_id", otInteger );
  TQuery DelQry( &OraSession );
  DelQry.SQLText =
    "BEGIN "
    " IF :pr_update = 1 THEN "
    " DELETE pax_seats WHERE point_id=:point_id AND pax_id=:pax_id AND pr_wl=1; "
    " UPDATE pax_seats SET pr_wl=1 WHERE point_id=:point_id AND pax_id=:pax_id AND NVL(pr_wl,0)=0; "
    " :pr_update := SQL%ROWCOUNT;"
    " END IF;"
    " DELETE pax_seats WHERE point_id=:point_id AND pax_id=:pax_id AND NVL(pr_wl,0)=0; "
    "END;";
  DelQry.DeclareVariable( "point_id", otInteger );
  DelQry.DeclareVariable( "pax_id", otInteger );
  DelQry.DeclareVariable( "pr_update", otInteger );
  TQuery InsQry( &OraSession );
  InsQry.Clear();
  InsQry.SQLText =
    "INSERT INTO pax_seats(point_id,pax_id,xname,yname,pr_wl) "
    "VALUES(:point_id,:pax_id,:xname,:yname,0)";
  InsQry.DeclareVariable( "point_id", otInteger );
  InsQry.DeclareVariable( "pax_id", otInteger );
  InsQry.DeclareVariable( "xname", otString );
  InsQry.DeclareVariable( "yname", otString );
  TQuery QryAirp( &OraSession );
  QryAirp.SQLText =
    "SELECT airline,airp FROM points WHERE point_id=:point_id";
  QryAirp.DeclareVariable( "point_id", otInteger );
  FilterRoutesProperty &filterRoutes = filterSets.filterRoutes;
  TWaitListReason waitListReason;
  TPassSeats seats;
  map<int,TSalonPax> passes;

  for ( FilterRoutesProperty::iterator ipoint=filterRoutes.begin();
        ipoint!=filterRoutes.end(); ipoint++ ) {  //�ࠧ� �� �ᥬ� ��������
    DelQry.SetVariable( "point_id", ipoint->point_id );
    InsQry.SetVariable( "point_id", ipoint->point_id );
    //!logProgTrace( TRACE5, "TSalonPassengers::check_waitlist_alarm: point_dep=%d, pr_craft_lat=%d",
    //!log           ipoint->point_id, pr_craft_lat );
    Qry.SetVariable( "point_id", ipoint->point_id );
    Qry.Execute();
    int idx_pax_id = Qry.FieldIndex( "pax_id" );
    int idx_xname = Qry.FieldIndex( "xname" );
    int idx_yname = Qry.FieldIndex( "yname" );
    std::map<int,TPassSeats> old_seats, new_seats;
    for ( ; !Qry.Eof; Qry.Next() ) {
      old_seats[ Qry.FieldAsInteger( idx_pax_id ) ].insert( TSeat( Qry.FieldAsString( idx_yname ),
                                                                   Qry.FieldAsString( idx_xname ) ) );
    }
    //!log tst();
    bool pr_is_sync_paxs = is_sync_paxs( ipoint->point_id );
    TSalonPassengers::iterator idep_pass = passengers.find( ipoint->point_id );
    passes.clear();
    if ( idep_pass != passengers.end() ) {  //� �⮬ �㭪� ���� ���ᠦ���
      idep_pass->second.SetStatus( wlNo );
      for ( _TSalonPassengers::iterator iarv_pass=idep_pass->second.begin(); iarv_pass!=idep_pass->second.end(); iarv_pass++ ) {
        //class
        for ( TIntClassSalonPassengers::iterator iclass=iarv_pass->second.begin();
              iclass!=iarv_pass->second.end(); iclass++ ) {
         //grp_status
          for ( TIntStatusSalonPassengers::iterator igrp_layer=iclass->second.begin();
                igrp_layer!=iclass->second.end(); igrp_layer++ ) {
            for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=igrp_layer->second.begin();
                  ipass!=igrp_layer->second.end(); ipass++ ) {
              if ( ipass->is_jmp ) {
                continue;
              }
              passes[ ipass->pax_id ] = *ipass;
              ipass->get_seats( waitListReason, seats );
              if ( waitListReason.layerStatus != layerValid ) {
                idep_pass->second.SetStatus( wlYes );
                ProgTrace( TRACE5, "pax_id=%d - waitlist", ipass->pax_id );
                continue;
              }
              for ( TPassSeats::iterator ipass_seat=seats.begin();
                    ipass_seat!=seats.end(); ipass_seat++ ) {
                new_seats[ ipass->pax_id ].insert( *ipass_seat );
              }
            }
          }
        }
      }
    }
    //�஡�� �� ���� ���ᠦ�ࠬ-���⠬
    std::set<int> change_pax_seats;
    for ( std::map<int,TPassSeats>::iterator iold=old_seats.begin(); iold!=old_seats.end(); iold++ ) {
      std::map<int,TPassSeats>::iterator inew = new_seats.find( iold->first );
      if ( inew != new_seats.end() ) { //���ᠦ�� ������
        if ( iold->second == inew->second ) {
          continue;
        }
        //���������� ���� - 㤠�塞 ����, �����뢠�� ����
        DelQry.SetVariable( "pax_id", inew->first );
        DelQry.SetVariable( "pr_update", 0 );
        DelQry.Execute();
        change_pax_seats.insert( inew->first );
      }
      else { //���ᠦ�� �� ������ � ����� ᯨ᪥ - ࠧॣ������ �� �訡�� ����� ��� ��
        DelQry.SetVariable( "pax_id", iold->first );
        DelQry.SetVariable( "pr_update", 1 );
        DelQry.Execute();
        bool pr_exists = DelQry.GetVariableAsInteger( "pr_update") != 0;
        if ( pr_exists ) {
          if ( passes.find( iold->first ) != passes.end() ) { //������� ⠪�� ���ᠦ��
            if ( paxs_external_logged.find( iold->first ) == paxs_external_logged.end() ) {
              CheckWaitListToLog( QryAirp,
                                  pr_exists,
                                  iold->first,
                                  ipoint->point_id,
                                  pax_lists,
                                  passes,
                                  pr_craft_lat );
            }
            rozysk::sync_pax( iold->first, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr  );
            if ( pr_is_sync_paxs ) {
              update_pax_change( ipoint->point_id, iold->first, passes[ iold->first ].reg_no, "�" );
            }
          }
        }
      } //end else not find
    }
    //�஡�� �� ���� ���ᠦ�ࠬ-���⠬
    for ( std::map<int,TPassSeats>::iterator inew=new_seats.begin(); inew!=new_seats.end(); inew++ ) {
      std::map<int,TPassSeats>::iterator iold = old_seats.find( inew->first );
      if ( iold == old_seats.end() ||
           change_pax_seats.find( inew->first ) != change_pax_seats.end() ) { //���ᠦ�� �� ������ ��� � ���ᠦ�� ���������� ����
        for ( TPassSeats::iterator iseat=inew->second.begin();
              iseat!=inew->second.end(); iseat++ ) {
          InsQry.SetVariable( "pax_id", inew->first );
          InsQry.SetVariable( "xname", iseat->line );
          InsQry.SetVariable( "yname", iseat->row );
          InsQry.Execute();
        }
        if ( paxs_external_logged.find( inew->first ) == paxs_external_logged.end() ) {
          CheckWaitListToLog( QryAirp,
                              change_pax_seats.find( inew->first ) != change_pax_seats.end(),
                              inew->first,
                              ipoint->point_id,
                              pax_lists,
                              passes,
                              pr_craft_lat );
        }
        rozysk::sync_pax( inew->first, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr  );
        if ( pr_is_sync_paxs ) {
          update_pax_change( ipoint->point_id, inew->first, passes[ inew->first ].reg_no, "�" );
        }
      }
    }
    set_alarm( ipoint->point_id, Alarm::Waitlist,
               idep_pass != passengers.end() &&
               idep_pass->second.isWaitList() &&
               !isFreeSeating( ipoint->point_id ) );
  }
}


bool TSalonList::check_waitlist_alarm_on_tranzit_routes( const TAutoSeats &autoSeats )
{
  ProgTrace( TRACE5, "TSalonList::check_waitlist_alarm_on_tranzit_routes: point_dep=%d", getDepartureId() );
  map<int,TPlaceList*> salons; // ��� ����ன ����樨 � ᠫ���
  TPlaceList* placelist = NULL;
  std::map<ASTRA::TCompLayerType,TMenuLayer> menuLayers;
  std::map<int,TFilterLayers> &filtersLayers = filterSets.filtersLayers;
  getMenuLayers( true, filtersLayers[ getDepartureId() ], menuLayers );
  std::map<int,TPaxList>::iterator ipaxList = pax_lists.find( getDepartureId() );
  if ( ipaxList == pax_lists.end() ) {
    ProgError( STDLOG, "TSalonList::check_waitlist_alarm_on_tranzit_routes: paxlist not found point_dep=%d, return true", getDepartureId() );
    return true;
  }
  //���� ����, ����� �뫨 �� ��ᠤ�� �����祭� ���ᠦ�ࠬ
  for ( TAutoSeats::const_iterator ipass=autoSeats.begin();
        ipass!=autoSeats.end(); ipass++ ) {
    TPaxList::iterator ipax = ipaxList->second.find( ipass->pax_id );
    if ( ipax == ipaxList->second.end() ) {
      ProgTrace( TRACE5, "TSalonList::check_waitlist_alarm_on_tranzit_routes: pnl pax not found pax_id=%d", ipass->pax_id );
      continue;
    }
    for ( TLayersPax::iterator ilayer=ipax->second.layers.begin();
          ilayer!=ipax->second.layers.end(); ilayer++ ) {
      if ( ilayer->second.waitListReason.layerStatus != layerValid ) { //��������� ᫮� �� ����� ������???
        //!logProgTrace( TRACE5, "%s", ilayer->first.toString().c_str() );
        continue;
      }
      //���� ⠪
      //!logProgTrace( TRACE5, "before checkin layer %s, return true", ilayer->first.toString().c_str() );
      return true;
    }
  }
  //���� ����, ����� �����祭� ���ᠦ�ࠬ
  for ( TAutoSeats::const_iterator ipass=autoSeats.begin();
        ipass!=autoSeats.end(); ipass++ ) {
    if ( !findSeat( salons, &placelist, ipass->point ) ) {
      ProgError( STDLOG, "TSalonList::check_waitlist_alarm: placelist not found num=%d, return true", ipass->point.num );
      return true;
    }
    TPoint seat_p( ipass->point.x, ipass->point.y );
    for ( int j=0; j<ipass->seats; j++ ) {
      TPlace *place = placelist->place( seat_p );
      std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
      std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers;
      place->GetLayers( layers, glAll );
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin();
            ilayers!=layers.end(); ilayers++ ) {
        for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
              ilayer!=ilayers->second.end(); ilayer++ ) {
          if ( (ilayer->getPaxId() != ASTRA::NoExists && /* ᫮� �ਭ������� ��㣮�� ���ᠦ��� */
                ilayer->getPaxId() != ipass->pax_id) ||
                (ilayer->getPaxId() == ASTRA::NoExists &&
                 menuLayers[ ilayer->layer_type ].notfree) /*᫮� �� �ਭ������� ���ᠦ��� � ������㯥� */
             ) {
            //!logProgTrace( TRACE5, "check_waitlist_alarm_on_tranzit_routes: not free %s, return true",
            //!log           ilayer->toString().c_str() );
            return true;
          }
        }
      }
      if ( ipass->pr_down ) {
        seat_p.y++;
      }
      else {
        seat_p.x++;
      }
    }
  }
  ProgTrace( TRACE5, "check_waitlist_alarm_on_tranzit_routes: return false" );
  return false;
}

/////////////////////////////////////////////////////////
void TSalons::Read( bool drop_not_used_pax_layers )
{
  pr_owner = true;
  switch ( readStyle ) {
    case rTripSalons:
      break;
    case rComponSalons:
      FilterClass.clear();
      break;
  }
  Clear();
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );
  TQuery QryWebTariff( &OraSession );
  TQuery PaxQry( &OraSession );


  if ( readStyle == rTripSalons ) {
    if ( SALONS2::isFreeSeating( trip_id ) ) {
      throw EXCEPTIONS::Exception( "MSG.SALONS.FREE_SEATING" );
    }
    Qry.SQLText =
     "SELECT pr_lat_seat, NVL(comp_id,-1) comp_id FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
    comp_id = Qry.FieldAsInteger( "comp_id" );
  }
  else {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("MSG.SALONS.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  Qry.Clear();

  if ( readStyle == rTripSalons ) {
    PaxQry.Clear();
    PaxQry.SQLText =
      " SELECT pax.grp_id, pax.pax_id, pax.pers_type, pax.seats, class, "
      "        reg_no, pax.surname, pax.is_female, crs_inf.pax_id AS parent_pax_id, 1 priority "
      "    FROM pax_grp, pax, crs_inf "
      "   WHERE pax.grp_id=pax_grp.grp_id AND "
      "         pax_grp.point_dep=:point_dep AND "
      "         pax.pax_id=crs_inf.inf_id(+) AND "
      "         pax_grp.status NOT IN ('E') AND "
      "         pax.refuse IS NULL "
      " UNION "
      " SELECT NULL, pax_id, crs_pax.pers_type, crs_pax.seats, crs_pnr.class, "
      "        NULL, crs_pax.surname, NULL, NULL, 2 priority "
      "    FROM crs_pax, crs_pnr, tlg_binding "
      "   WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "         crs_pnr.point_id=tlg_binding.point_id_tlg AND "
      "         tlg_binding.point_id_spp=:point_dep AND "
      "         crs_pnr.system='CRS' AND "
      "         crs_pax.pr_del=0 "
      " ORDER BY priority ";
    PaxQry.CreateVariable( "point_dep", otInteger, trip_id );
    // ����뢠�� �� ᫮� � ᠫ���
    Qry.SQLText =
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.xname,t.yname,t.class,r.layer_type, "
      "                NVL(l.pax_id, l.crs_pax_id) pax_id, l.point_dep, l.point_arv, l.time_create "
      " FROM trip_comp_elems t, trip_comp_ranges r, trip_comp_layers l "
      "WHERE t.point_id=:point_id AND "
      "      t.point_id=r.point_id(+) AND "
      "      t.num=r.num(+) AND "
      "      t.x=r.x(+) AND "
      "      t.y=r.y(+) AND "
      "      r.range_id=l.range_id(+) "
      "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
  }
  else {
    Qry.SQLText =
      "SELECT num,x,y,elem_type,xprior,yprior,agle,xname,yname,class "
      " FROM comp_elems "
      "WHERE comp_id=:comp_id "
      "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
  }
  Qry.Execute();
  if ( Qry.RowCount() == 0 ) {
    if ( readStyle == rTripSalons ) {
      throw UserException( "MSG.FLIGHT_WO_CRAFT_CONFIGURE" );
    }
    else {
      throw UserException( "MSG.SALONS.NOT_FOUND" );
    }
  }
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_elem_type = Qry.FieldIndex( "elem_type" );
  int col_xprior = Qry.FieldIndex( "xprior" );
  int col_yprior = Qry.FieldIndex( "yprior" );
  int col_agle = Qry.FieldIndex( "agle" );
  int col_xname = Qry.FieldIndex( "xname" );
  int col_yname = Qry.FieldIndex( "yname" );
  int col_class = Qry.FieldIndex( "class" );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    RQry.CreateVariable( "point_id", otInteger, trip_id );
    LQry.SQLText =
      "SELECT num,x,y,layer_type FROM trip_comp_baselayers "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    LQry.CreateVariable( "point_id", otInteger, trip_id );


    if ( FilterLayers.CanUseLayer( cltProtBeforePay, -1, -1, false ) ||
         FilterLayers.CanUseLayer( cltProtAfterPay, -1, -1, false ) ||
         FilterLayers.CanUseLayer( cltPNLBeforePay, -1, -1, false ) ||
         FilterLayers.CanUseLayer( cltPNLAfterPay, -1, -1, false ) ) {
      QryWebTariff.SQLText =
        "SELECT num,x,y,color,rate,rate_cur FROM trip_comp_rates "
        " WHERE point_id=:point_id "
        "ORDER BY num,x desc, y desc ";
      QryWebTariff.CreateVariable( "point_id", otInteger, trip_id );
    }
  }
  else {
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM comp_rem "
                   " WHERE comp_id=:comp_id "
                   "ORDER BY num, x desc, y desc ";
    RQry.CreateVariable( "comp_id", otInteger, comp_id );
    LQry.SQLText = "SELECT num,x,y,layer_type FROM comp_baselayers "
                   " WHERE comp_id=:comp_id "
                   "ORDER BY num, x desc, y desc ";
    LQry.CreateVariable( "comp_id", otInteger, comp_id );
    QryWebTariff.SQLText =
      "SELECT num,x,y,color,rate,rate_cur FROM comp_rates "
      " WHERE comp_id=:comp_id "
      "ORDER BY num,x desc, y desc ";
    QryWebTariff.CreateVariable( "comp_id", otInteger, comp_id );
  }
  RQry.Execute();
  int rem_col_num = RQry.FieldIndex( "num" );
  int rem_col_x = RQry.FieldIndex( "x" );
  int rem_col_y = RQry.FieldIndex( "y" );
  int rem_col_rem = RQry.FieldIndex( "rem" );
  int rem_col_pr_denial = RQry.FieldIndex( "pr_denial" );

  LQry.Execute();
  int baselayer_col_num = LQry.FieldIndex( "num" );
  int baselayer_col_x = LQry.FieldIndex( "x" );
  int baselayer_col_y = LQry.FieldIndex( "y" );
  int baselayer_col_layer_type = LQry.FieldIndex( "layer_type" );


  int webtariff_col_num = 0;
  int webtariff_col_x = 0;
  int webtariff_col_y = 0;
  int webtariff_col_color = 0;
  int webtariff_col_rate = 0;
  int webtariff_col_rate_cur = 0;

  if ( readStyle != rTripSalons ||
       FilterLayers.CanUseLayer( cltProtBeforePay, -1, -1, false ) ||
       FilterLayers.CanUseLayer( cltProtAfterPay, -1, -1, false ) ||
       FilterLayers.CanUseLayer( cltPNLBeforePay, -1, -1, false ) ||
       FilterLayers.CanUseLayer( cltPNLAfterPay, -1, -1, false ) ) {
    QryWebTariff.Execute();
    webtariff_col_num = QryWebTariff.FieldIndex( "num" );
    webtariff_col_x = QryWebTariff.FieldIndex( "x" );
    webtariff_col_y = QryWebTariff.FieldIndex( "y" );
    webtariff_col_color = QryWebTariff.FieldIndex( "color" );
    webtariff_col_rate = QryWebTariff.FieldIndex( "rate" );
    webtariff_col_rate_cur = QryWebTariff.FieldIndex( "rate_cur" );
  }
  string ClName = ""; /* ����᫥��� ��� ����ᮢ, ����� ���� � ᠫ��� */
  TPlaceList *placeList = NULL;
  int num = -1;
  TPoint point_p;
  int pax_id;
  TPlaceLayer PlaceLayer( 0, 0, 0, cltUnknown, ASTRA::NoExists, 10000 );
  TPaxLayers pax_layers;
  //PaxsOnPlaces.clear();

  vector<TPass> InfItems, AdultItems;
  if ( readStyle == rTripSalons ) { // ������塞 ���� �� ���ᠦ���
    PaxQry.Execute();
    int idx_pax_id = PaxQry.FieldIndex( "pax_id" );
    int idx_grp_id = PaxQry.FieldIndex( "grp_id" );
    int idx_parent_pax_id = PaxQry.FieldIndex( "parent_pax_id" );
    int idx_reg_no = PaxQry.FieldIndex( "reg_no" );
    int idx_seats = PaxQry.FieldIndex( "seats" );
    int idx_pers_type = PaxQry.FieldIndex( "pers_type" );
    int idx_surname = PaxQry.FieldIndex( "surname" );
    int idx_is_female = PaxQry.FieldIndex( "is_female" );
    int idx_class = PaxQry.FieldIndex( "class" );
    int idx_priority = PaxQry.FieldIndex( "priority" );
    while ( !PaxQry.Eof ) {
      if ( PaxQry.FieldAsInteger( idx_priority ) == 1 ) {
        TPass pass;
        pass.grp_id = PaxQry.FieldAsInteger( idx_grp_id );
        pass.pax_id = PaxQry.FieldAsInteger( idx_pax_id );
        pass.reg_no = PaxQry.FieldAsInteger( idx_reg_no );
        pass.surname = PaxQry.FieldAsString( idx_surname );
        pass.is_female = PaxQry.FieldIsNULL( idx_is_female )?ASTRA::NoExists:PaxQry.FieldAsInteger( idx_is_female );
        pass.parent_pax_id = PaxQry.FieldAsInteger( idx_parent_pax_id );
        if ( PaxQry.FieldAsInteger( idx_seats ) == 0 ) {
          InfItems.push_back( pass );
        }
        else {
          //!logProgTrace( TRACE5, "PaxQry.FieldAsString( idx_pers_type )=%s", PaxQry.FieldAsString( idx_pers_type ) );
          if ( string(PaxQry.FieldAsString( idx_pers_type )) == string("��") ) {
            //tst();
            AdultItems.push_back( pass );
          }
        }
      }
      if ( PaxQry.FieldAsInteger( idx_seats ) >= 1 &&
           pax_layers.find( PaxQry.FieldAsInteger( idx_pax_id ) ) == pax_layers.end() ) {
        pax_layers[ PaxQry.FieldAsInteger( idx_pax_id ) ].seats = PaxQry.FieldAsInteger( idx_seats );
        pax_layers[ PaxQry.FieldAsInteger( idx_pax_id ) ].cl = PaxQry.FieldAsString( idx_class );
      }
      PaxQry.Next();
    }
    //�ਢ易�� ��⥩ � �����
    //!logProgTrace( TRACE5, "SetInfantsToAdults: InfItems.size()=%zu, AdultItems.size()=%zu", InfItems.size(), AdultItems.size() );
    SetInfantsToAdults<TPass,TPass>( InfItems, AdultItems );
    for ( vector<TPass>::iterator i=InfItems.begin(); i!=InfItems.end(); i++ ) {
      //!logProgTrace( TRACE5, "Infant pax_id=%d", i->pax_id );
      for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
        if ( i->parent_pax_id == j->pax_id ) {
          j->pr_inf = true;
          //!logProgTrace( TRACE5, "Infant to pax_id=%d", j->pax_id );
          break;
        }
      }
    }
  }

  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( num != Qry.FieldAsInteger( col_num ) ) {
      if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
        placeList->places.clear();
      }
      else {
        placeList = new TPlaceList();
        placelists.push_back( placeList );
      }
      ClName.clear();
      num = Qry.FieldAsInteger( col_num );
      placeList->num = num;
    }
    // ����७�� ����! - ࠧ�� ᫮�
    TPlace place;
    point_p.x = Qry.FieldAsInteger( col_x );
    point_p.y = Qry.FieldAsInteger( col_y );
    if ( readStyle != rTripSalons || Qry.FieldIsNULL( "pax_id" ) )
        pax_id = -1;
    else
        pax_id = Qry.FieldAsInteger( "pax_id" );
    // �᫨ ���� �� �� ��।����� ��� ���� ����, �� �� �ந��樠����஢���
    if ( !placeList->ValidPlace( point_p ) || placeList->place( point_p )->x == -1 ) {
        place.x = point_p.x;
        place.y = point_p.y;
        place.num = num;
      place.elem_type = Qry.FieldAsString( col_elem_type );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( Qry.FieldIsNULL( col_xprior ) )
        place.xprior = -1;
      else
        place.xprior = Qry.FieldAsInteger( col_xprior );
      if ( Qry.FieldIsNULL( col_yprior ) )
        place.yprior = -1;
      else
        place.yprior = Qry.FieldAsInteger( col_yprior );
      place.agle = Qry.FieldAsInteger( col_agle );
      place.clname = Qry.FieldAsString( col_class );
      place.xname = Qry.FieldAsString( col_xname );
      place.yname = Qry.FieldAsString( col_yname );
      while ( !RQry.Eof && RQry.FieldAsInteger( rem_col_num ) == num &&
              RQry.FieldAsInteger( rem_col_x ) == place.x &&
              RQry.FieldAsInteger( rem_col_y ) == place.y ) {
        TRem rem;
        rem.rem = RQry.FieldAsString( rem_col_rem );
        rem.pr_denial = RQry.FieldAsInteger( rem_col_pr_denial );
        place.rems.push_back( rem );
        AddExistSubcls( rem );
        RQry.Next();
      }
      while ( !LQry.Eof && LQry.FieldAsInteger( baselayer_col_num ) == num &&
              LQry.FieldAsInteger( baselayer_col_x ) == place.x &&
              LQry.FieldAsInteger( baselayer_col_y ) == place.y ) {
        ASTRA::TCompLayerType layer_type = DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) );
        place.AddLayerToPlace( layer_type,
                               0, 0, NoExists, NoExists,
                               BASIC_SALONS::TCompLayerTypes::Instance()->priority( DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) ) ) );
        if ( isBaseLayer( layer_type, false ) ) {
          AddExistBaseLayer( layer_type );
        }
        LQry.Next();
      }
      if ( readStyle != rTripSalons ||
           FilterLayers.CanUseLayer( cltProtBeforePay, -1, -1, false ) ||
           FilterLayers.CanUseLayer( cltProtAfterPay, -1, -1, false ) ||
           FilterLayers.CanUseLayer( cltPNLBeforePay, -1, -1, false ) ||
           FilterLayers.CanUseLayer( cltPNLAfterPay, -1, -1, false ) ) {
        if ( !QryWebTariff.Eof &&
              QryWebTariff.FieldAsInteger( webtariff_col_num ) == num &&
              QryWebTariff.FieldAsInteger( webtariff_col_x ) == place.x &&
              QryWebTariff.FieldAsInteger( webtariff_col_y ) == place.y ) {
          place.SeatTariff=TSeatTariff( QryWebTariff.FieldAsString( webtariff_col_color ),
                                        QryWebTariff.FieldAsFloat( webtariff_col_rate ),
                                        QryWebTariff.FieldAsString( webtariff_col_rate_cur ) );
          QryWebTariff.Next();
        }
      }
      if ( ClName.find( Qry.FieldAsString( col_class ) ) == string::npos )
        ClName += Qry.FieldAsString(col_class );
    }
    else { // �� ���� �ந��樠����஢��� - �� ���� ᫮�
        place = *placeList->place( point_p );
      if ( place.x != point_p.x || place.y != point_p.y )
        throw EXCEPTIONS::Exception( "invalid x, y" );
    }
    PlaceLayer.pax_id = -1;
    if ( readStyle == rTripSalons ) { // ����� ࠡ�� � �ᥬ� ᫮ﬨ ��� 㤠����� ����� �ਮ����� ᫮�� �� ���ᠦ�ࠬ
        PlaceLayer.layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
        if ( Qry.FieldIsNULL( "point_dep" ) )
            PlaceLayer.point_dep = NoExists;
        else
          PlaceLayer.point_dep = Qry.FieldAsInteger( "point_dep" );
        if ( Qry.FieldIsNULL( "point_arv" ) )
            PlaceLayer.point_arv = NoExists;
        else
          PlaceLayer.point_arv = Qry.FieldAsInteger( "point_arv" );
        PlaceLayer.time_create = Qry.FieldAsDateTime( "time_create" );
      if ( FilterLayers.CanUseLayer( PlaceLayer.layer_type, Qry.FieldAsInteger( "point_dep" ), -1, false ) ) { // ��� ᫮� �ᯮ��㥬
//      	ProgTrace( TRACE5, "seat_no=%s, pax_id=%d", string(string(Qry.FieldAsString("yname"))+Qry.FieldAsString("xname")).c_str(), pax_id );
        if ( PlaceLayer.layer_type != cltUnknown ) { // ᫮� ���஢��� �� �ਮ����, ���� - ᠬ� �ਮ���� ᫮� � �����
          place.AddLayerToPlace( PlaceLayer.layer_type, PlaceLayer.time_create, pax_id,
                                 PlaceLayer.point_dep, PlaceLayer.point_arv,
                                 BASIC_SALONS::TCompLayerTypes::Instance()->priority( PlaceLayer.layer_type ) );// ����� ���� ����७�� ᫮��
          PlaceLayer.pax_id = pax_id;
        } // ����� ᫮� � ����
      }
    }
    place.visible = true;
    placeList->Add( place );
    if ( PlaceLayer.pax_id > 0 ) {
      TPaxLayers::iterator ipl = pax_layers.find( PlaceLayer.pax_id );
      if ( ipl != pax_layers.end() ) { // ��諨 ����� ᫮�� ���९������ �� pax_id
        vector<TPaxLayer>::iterator ip=ipl->second.paxLayers.end();
        for( ip=ipl->second.paxLayers.begin(); ip!=ipl->second.paxLayers.end(); ip++ ) {
            if ( ip->layer_type == PlaceLayer.layer_type && ip->time_create == PlaceLayer.time_create )
                break;
        }
        if ( ip != ipl->second.paxLayers.end() ) { // ��諨 ᫮�, �� ���� ���� � 祫����� � �⮬ ᫮�
            ip->places.push_back( TSalonPoint( point_p.x, point_p.y, placeList->num ) );
        }
        else {
            ipl->second.AddLayer( TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
                                             BASIC_SALONS::TCompLayerTypes::Instance()->priority( PlaceLayer.layer_type ),
                                             TSalonPoint( point_p.x, point_p.y, placeList->num ) )); // ᫮� �� ������, ᮧ���� ���� ᫮�
         }
      }
      else { // ���ᠦ�� �� ������, ᮧ���� ���ᠦ�� � ᫮�
        pax_layers[ pax_id ].AddLayer( TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
                                                  BASIC_SALONS::TCompLayerTypes::Instance()->priority( PlaceLayer.layer_type ),
                                                  TSalonPoint( point_p.x, point_p.y, placeList->num ) ));  //??? � ���� �� djek
      }
    }
  }	/* end for */
  if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
    //!logProgTrace( TRACE5, "placeList->num=%d", placeList->num );
    placelists.pop_back( );
    delete placeList; // ��� ��� �����/ᠫ�� �� �㦥�
  }

  if ( drop_not_used_pax_layers ) {
    for( TPaxLayers::iterator ipax=pax_layers.begin(); ipax!=pax_layers.end(); ipax++ ) { // �஡�� �� ���ᠦ�ࠬ
      GetValidPaxLayer( this, pax_layers, ipax );
    }
    for( TPaxLayers::iterator ipax=pax_layers.begin(); ipax!=pax_layers.end(); ipax++ ) { // �஡�� �� ���ᠦ�ࠬ
      ClearInvalidPaxLayers( this, ipax );
    }
  }

  if ( readStyle == rTripSalons ) { // ������� ���ᠦ�஢ � ���쬨
    for( vector<TPlaceList*>::iterator placeList = placelists.begin();
      placeList != placelists.end(); placeList++ ) {
      for ( TPlaces::iterator place = (*placeList)->places.begin();
            place != (*placeList)->places.end(); place++ ) {
        if ( canAddOccupy( &(*place) ) ) {
          AddOccupySeat( (*placeList)->num, place->x, place->y );
        }
        if ( !place->visible || !place->isplace ||
             place->layers.empty() || place->layers.begin()->pax_id == NoExists )
         continue;
        for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
          if ( !j->pr_inf || place->layers.begin()->pax_id != j->pax_id )
            continue;
          //!logProgTrace( TRACE5, "TDrawProp: seat_no=%s", string(place->yname+place->xname).c_str() );
          place->drawProps.setFlag( dpInfantWoSeats );
          break;
        }
      }
    }
  }
}

void TSalons::Parse( xmlNodePtr salonsNode )
{
  if ( salonsNode == NULL )
    return;
  xmlNodePtr node;
  bool pr_lat_seat_init = false;
  node = GetNode( "@pr_lat_seat", salonsNode );
  if ( node ) {
    tst();
    pr_lat_seat = NodeAsInteger( node );
    pr_lat_seat_init = true;
  }
  Clear();
  node = salonsNode->children;
  xmlNodePtr salonNode = NodeAsNodeFast( "placelist", node );
  TRem rem;
  int lat_count=0, rus_count=0;
  string rus_lines = rus_seat, lat_lines = lat_seat;
  TElemFmt fmt;
  while ( salonNode ) {
    TPlaceList *placeList = new TPlaceList();
    placeList->num = NodeAsInteger( "@num", salonNode );
    xmlNodePtr placeNode = salonNode->children;
    while ( placeNode ) {
      node = placeNode->children;
      TPlace place;
      place.x = NodeAsIntegerFast( "x", node );
      place.y = NodeAsIntegerFast( "y", node );
      place.elem_type = NodeAsStringFast( "elem_type", node );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( !GetNodeFast( "xprior", node ) )
        place.xprior = -1;
      else
        place.xprior = NodeAsIntegerFast( "xprior", node );
      if ( !GetNodeFast( "yprior", node ) )
        place.yprior = -1;
      else
        place.yprior = NodeAsIntegerFast( "yprior", node );
      if ( !GetNodeFast( "agle", node ) )
        place.agle = 0;
      else
        place.agle = NodeAsIntegerFast( "agle", node );
      place.clname = NodeAsStringFast( "class", node );
      if ( !place.clname.empty() ) {
        place.clname = ElemToElemId( etClass, place.clname, fmt );
        if ( fmt == efmtUnknown )
          throw UserException( "MSG.INVALID_CLASS" );
      }

      place.xname = NodeAsStringFast( "xname", node );

      if ( !pr_lat_seat_init ) {
        if ( rus_lines.find( place.xname ) != string::npos ) {
          rus_count++;
        }
          if ( lat_lines.find( place.xname ) != string::npos ) {
          lat_count++;
        }
        }
      place.xname = norm_iata_line( place.xname );
      place.yname = norm_iata_row( NodeAsStringFast( "yname", node ) );

      xmlNodePtr remsNode = GetNodeFast( "rems", node );
      xmlNodePtr remNode;
      bool pr_disable_layer = false;
      if ( remsNode ) {
        remsNode = remsNode->children;
        while ( remsNode ) {
          remNode = remsNode->children;
          rem.rem = NodeAsStringFast( "rem", remNode );
          rem.pr_denial = GetNodeFast( "pr_denial", remNode );
          if ( rem.rem == "X" ) {
            if ( !pr_disable_layer && !compatibleLayer( cltDisable ) && !rem.pr_denial ) {
              pr_disable_layer = true;
            }
          }
          else {
            place.rems.push_back( rem );
            verifyValidRem( place.clname, rem.rem );
          }
          remsNode = remsNode->next;
        }
      }
      remsNode = GetNodeFast( "layers", node );
      if ( remsNode ) {
        remsNode = remsNode->children; //layer
        while( remsNode ) {
            remNode = remsNode->children;
            ASTRA::TCompLayerType l = DecodeCompLayerType( NodeAsStringFast( "layer_type", remNode ) );
            if ( l != cltUnknown && !place.isLayer( l ) )
                 place.AddLayerToPlace( l, 0, 0, NoExists, NoExists, BASIC_SALONS::TCompLayerTypes::Instance()->priority( l ) );
            remsNode = remsNode->next;
        }
      }
      if ( !compatibleLayer( cltDisable ) && pr_disable_layer ) {
        place.AddLayerToPlace( cltDisable, 0, 0, NoExists, NoExists, BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltDisable ) );
      }
      remNode = GetNodeFast( "tarif", node );
      if ( remNode ) {
        place.SeatTariff.color = NodeAsString( "@color", remNode );
        if ( place.SeatTariff.color != "clBtnFace" ) { // �訡�� �� ��஭� �ନ���� � ��砥 ���. ०���
          place.SeatTariff.color.clear();
        }
        else {
          place.SeatTariff.rate = NodeAsFloat( remNode );
          place.SeatTariff.currency_id = NodeAsString( "@currency_id", remNode );
        }
      }
      place.visible = true;
      placeList->Add( place );
      placeNode = placeNode->next;
    }
    placelists.push_back( placeList );
    salonNode = salonNode->next;
  }
  if ( !pr_lat_seat_init ) {
    pr_lat_seat = ( lat_count>=rus_count );
  }
}

void TSalons::SetTariffsByRFISCColor( int point_dep, const TSeatTariffMapType &tariffs, const TSeatTariffMap::TStatus &status )
{
  if ( tariffs.empty() ) {
    return;
  }
  for( vector<TPlaceList*>::iterator placeList = placelists.begin();
       placeList != placelists.end(); placeList++ ) {
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      place->SetTariffsByRFISCColor( point_dep, tariffs, status );
    }
  }
}

void TSalons::SetTariffsByRFISC( int point_dep )
{
  for( vector<TPlaceList*>::iterator placeList = placelists.begin();
       placeList != placelists.end(); placeList++ ) {
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      place->SetTariffsByRFISC( point_dep );
      if ( canAddOccupy( &(*place) ) ) {
        AddOccupySeat( (*placeList)->num, place->x, place->y );
      }
    }
  }
}


void verifyValidRem( const std::string &className, const std::string &remCode )
{
  if ( SUBCLS_REMS.empty() ) {
    SUBCLS_REMS.insert( make_pair( string("MCLS"), string("�") ) );
    SUBCLS_REMS.insert( make_pair( string("SCLS"), string("�") ) );
    SUBCLS_REMS.insert( make_pair( string("YCLS"), string("�") ) );
    SUBCLS_REMS.insert( make_pair( string("LCLS"), string("�") ) );
    //!logProgTrace( TRACE5, "verifyValidRem: init SUBCLS_REMS" );
  }
  std::map<std::string,std::string>::iterator isubcls_rem = SUBCLS_REMS.find( remCode );
  if ( isubcls_rem != SUBCLS_REMS.end() && isubcls_rem->second != className ) {
        throw UserException( "MSG.SALONS.REMARK_NOT_SET_IN_CLASS",
                             LParams()<<LParam("remark", remCode )<<LParam("class", ElemIdToCodeNative(etClass,className) ));
  }
}

int TPlaceList::GetXsCount()
{
  return xs.size();
}

int TPlaceList::GetYsCount()
{
  return ys.size();
}

TPlace *TPlaceList::place( int idx )
{
  if ( idx < 0 || idx >= (int)places.size() )
    throw EXCEPTIONS::Exception( "place index out of range" );
  return &places[ idx ];
}

TPlace *TPlaceList::place( TPoint &p )
{
  return place( GetPlaceIndex( p ) );
}

int TPlaceList::GetPlaceIndex( TPoint &p )
{
  return GetPlaceIndex( p.x, p.y );
}

int TPlaceList::GetPlaceIndex( int x, int y )
{
  return GetXsCount()*y + x;
}

bool TPlaceList::ValidPlace( TPoint &p )
{
 return ( p.x < GetXsCount() && p.x >= 0 && p.y < GetYsCount() && p.y >= 0 );
}

string TPlaceList::GetPlaceName( TPoint &p )
{
  if ( !ValidPlace( p ) )
    throw EXCEPTIONS::Exception( "���ࠢ���� ���न���� ����" );
  return ys[ p.y ] + xs[ p.x ];
}

string TPlaceList::GetXsName( int x )
{
  if ( x < 0 || x >= GetXsCount() ) {
    throw EXCEPTIONS::Exception( "���ࠢ���� x ���न��� ����" );
  }
  return xs[ x ];
}

string TPlaceList::GetYsName( int y )
{
  if ( y < 0 || y >= GetYsCount() )
    throw EXCEPTIONS::Exception( "���ࠢ���� y ���न��� ����" );
  return ys[ y ];
}

bool TPlaceList::GetisPlaceXY( string placeName, TPoint &p )
{
    TrimString(placeName);
    if ( placeName.empty() )
        return false;
  /* ��������� ����஢ ���� � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
  size_t i = 0;
  for (; i < placeName.size(); i++)
    if ( placeName[ i ] != '0' )
      break;
  if ( i )
    placeName.erase( 0, i );
  string seat_no = placeName, salon_seat_no;
  if ( placeName == CharReplace( seat_no, rus_seat, lat_seat ) )
    CharReplace( seat_no, lat_seat, rus_seat );
  if ( placeName == seat_no )
    seat_no.clear();
  for( vector<string>::iterator ix=xs.begin(); ix!=xs.end(); ix++ )
    for ( vector<string>::iterator iy=ys.begin(); iy!=ys.end(); iy++ ) {
        salon_seat_no = denorm_iata_row(*iy) + denorm_iata_line(*ix,false);
      if ( placeName == salon_seat_no ||
           ( !seat_no.empty() && seat_no == salon_seat_no ) ) {
        p.x = distance( xs.begin(), ix );
        p.y = distance( ys.begin(), iy );
        return place( p )->isplace;
      }
    }
  return false;
}

void TPlaceList::Add( TPlace &pl )
{
  int prior_max_x = (int)xs.size();
  int prior_max_y = (int)ys.size();
  if ( pl.x >= prior_max_x )
    xs.resize( pl.x + 1, "" );
  if ( !pl.xname.empty() )
    xs[ pl.x ] = pl.xname;
  if ( pl.y >= prior_max_y )
    ys.resize( pl.y + 1, "" );
  if ( !pl.yname.empty() )
    ys[ pl.y ] = pl.yname;
  if ( (int)xs.size()*(int)ys.size() > (int)places.size() ) {
    for ( int iy=0; iy<prior_max_y-1; iy++ ) {
        IPlace ip = places.begin() + GetPlaceIndex( prior_max_x - 1, iy );
        TPlace p;
        if ( (int)xs.size() > prior_max_x ) {
          places.insert( ip + 1, (int)xs.size() - prior_max_x, p );
        }
    }
  }
  if ( (int)xs.size()*(int)ys.size() > (int)places.size() ) {
    places.resize( (int)xs.size()*(int)ys.size() );
  }

  int idx = GetPlaceIndex( pl.x, pl.y );
  if ( pl.xprior >= 0 && pl.yprior >= 0 ) {
    TPoint p( pl.xprior, pl.yprior );
    place( p )->xnext = pl.x;
    place( p )->ynext = pl.y;
  }
  places[ idx ] = pl;
}

void LoadCompRemarksPriority( std::map<std::string, int> &rems )
{
  rems.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT code, pr_comp FROM comp_rem_types WHERE pr_comp IS NOT NULL";
  Qry.Execute();
  while ( !Qry.Eof ) {
    rems[ Qry.FieldAsString( "code" ) ] = Qry.FieldAsInteger( "pr_comp" );
    Qry.Next();
  }
}

bool Checkin( int pax_id )
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT pax_id FROM pax where pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    return Qry.RowCount();
}

void GetTripParams( int trip_id, xmlNodePtr dataNode )
{
//  ProgTrace( TRACE5, "GetTripParams trip_id=%d", trip_id );

  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT " + TTripInfo::selectedFields() + ", bort "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TTripInfo info( Qry );

  NewTextChild( dataNode, "trip", GetTripName( info, ecCkin ) );
  NewTextChild( dataNode, "craft", ElemIdToElemCtxt( ecDisp, etCraft, Qry.FieldAsString( "craft" ), (TElemFmt)Qry.FieldAsInteger( "craft_fmt" ) ) );
  NewTextChild( dataNode, "bort", Qry.FieldAsString( "bort" ) );

  string craft = Qry.FieldAsString( "craft" ), airp = Qry.FieldAsString("airp");

  string travel_time_str = "00-00";
  TTripRoute route;
  route.GetRouteAfter(NoExists, trip_id, trtNotCurrent, trtNotCancelled);
  if (!route.empty())
  {
    TDateTime travel_time = getTimeTravel(craft, airp, route.back().airp);
    if(travel_time != NoExists)
      travel_time_str = DateTimeToStr(travel_time, "hh:nn", true);
  };
  NewTextChild( dataNode, "travel_time", travel_time_str);

  Qry.Clear();
  Qry.SQLText = "SELECT "\
                "       DECODE( trip_sets.comp_id, NULL, DECODE( e.pr_comp_id, 0, -2, -1 ), "\
                "               trip_sets.comp_id ) comp_id, "\
                "       comp.descr "\
                " FROM trip_sets, "\
                "  ( SELECT COUNT(*) pr_comp_id FROM trip_comp_elems "\
                "    WHERE point_id=:point_id AND rownum<2 ) e, "\
                "  ( SELECT comp_id, craft, bort, descr FROM comps ) comp "\
                " WHERE trip_sets.point_id = :point_id AND trip_sets.comp_id = comp.comp_id(+) ";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  /* comp_id>0 - ������; comp_id=-1 - ���������; comp_id=-2 - �� ����� */
  NewTextChild( dataNode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
  NewTextChild( dataNode, "descr", Qry.FieldAsString( "descr" ) );
}

void GetCompParams( int comp_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT comp_id,craft,bort,descr from comps "
                " WHERE comp_id=:comp_id";
  Qry.DeclareVariable( "comp_id", otInteger );
  Qry.SetVariable( "comp_id", comp_id );
  Qry.Execute();
  NewTextChild( dataNode, "trip" );
  NewTextChild( dataNode, "craft", ElemIdToCodeNative( etCraft, Qry.FieldAsString( "craft" ) ) );
  NewTextChild( dataNode, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( dataNode, "comp_id", comp_id );
  NewTextChild( dataNode, "descr", Qry.FieldAsString( "descr" ) );
}


bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull )
{
  TQuery Qry( &OraSession );
  string sql = "SELECT pax.pax_id FROM pax_grp, pax "
               " WHERE pax_grp.grp_id=pax.grp_id AND "
               "       point_dep=:point_id AND "
               "       pax_grp.status NOT IN ('E') AND "
               "       pax.pr_brd IS NOT NULL AND "
               "       seats > 0 AND rownum <= 1 ";
 if ( SeatNoIsNull ) {
  sql += " AND salons.is_waitlist(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,rownum)<>0 ";
 }
 Qry.SQLText = sql;
 Qry.CreateVariable( "point_id", otInteger, trip_id );
 Qry.Execute( );
 return Qry.RowCount();
}

int SIGN( int a ) {
    return (a > 0) - (a < 0);
};

struct TComp {
  int sum;
  int comp_id;
  TComp() {
    sum = 99999;
    comp_id = -1;
  };
};

int GetCompId( const std::string craft, const std::string bort, const std::string airline,
               vector<std::string> airps,  int f, int c, int y, bool pr_ignore_fcy )
{
    //!logProgTrace( TRACE5, "craft=%s, bort=%s, airline=%s, f=%d, c=%d, y=%d, airps.size=%zu",
    //!log           craft.c_str(), bort.c_str(), airline.c_str(), f, c, y, airps.size() );
    if ( f + c + y == 0 )
        return -1;
  if ( pr_ignore_fcy ) {
    f = 0;
    c = 0;
    y = 0;
  }
    map<int,TComp,std::less<int> > CompMap;
    int idx;
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT * FROM "
      "( SELECT COMPS.COMP_ID, BORT, AIRLINE, AIRP, "
    "         NVL( SUM( DECODE( CLASS, '�', CFG, 0 )), 0 ) AS F, "
    "         NVL( SUM( DECODE( CLASS, '�', CFG, 0 )), 0 ) AS C, "
    "         NVL( SUM( DECODE( CLASS, '�', cfg, 0 )), 0 ) AS Y "
    "   FROM COMPS, COMP_CLASSES "
    "  WHERE COMP_CLASSES.COMP_ID = COMPS.COMP_ID AND COMPS.CRAFT = :craft "
    "  GROUP BY COMPS.COMP_ID, BORT, AIRLINE, AIRP ) "
    "WHERE  f - :vf >= 0 AND "
    "       c - :vc >= 0 AND "
    "       y - :vy >= 0 AND "
    "       f < 1000 AND c < 1000 AND y < 1000 "
    " ORDER BY comp_id ";
    Qry.CreateVariable( "craft", otString, craft );
    Qry.CreateVariable( "vf", otString, f );
    Qry.CreateVariable( "vc", otString, c );
    Qry.CreateVariable( "vy", otString, y );
    Qry.Execute();
  while ( !Qry.Eof ) {
    string comp_airline = Qry.FieldAsString( "airline" );
    string comp_airp = Qry.FieldAsString( "airp" );
    bool airline_OR_airp = ( !comp_airline.empty() && airline == comp_airline ) ||
                             ( comp_airline.empty() &&
                             !comp_airp.empty() &&
                             find( airps.begin(), airps.end(), comp_airp ) != airps.end() );
    if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) && airline_OR_airp ) {
        idx = 0; // ����� ᮢ������ ����+������������ OR ��ய���
    }
    else
        if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) ) {
            idx = 1; // ����� ᮢ������ ����
      }
        else {
            if ( airline_OR_airp && !pr_ignore_fcy ) {
                idx = 2; // ����� ᮢ������ ������������ ��� ��ய���
        }
            else {
                Qry.Next();
                continue;
            }
     }
    // ᮢ������� �� ���-�� ���� ��� ������� �����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         Qry.FieldAsInteger( "f" ) >= f &&
         Qry.FieldAsInteger( "c" ) >= c &&
         Qry.FieldAsInteger( "y" ) >= y &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
      //!logProgTrace( TRACE5, "GetCompId:  CompMap add (idx=%d,comp_id=%d) sum=%d", idx, CompMap[ idx ].comp_id, CompMap[ idx ].sum );
    }

    idx += 3;
    // ᮢ������� �� ����ᠬ � �������� ���� >= ��饥 ���-�� ����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
      //!logProgTrace( TRACE5, "GetCompId:  CompMap add (idx=%d,comp_id=%d) sum=%d", idx, CompMap[ idx ].comp_id, CompMap[ idx ].sum );
    }
    // ᮢ������� �� �������� ���� >= ��饥 ���-�� ����
    idx += 3;
    if ( CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
      //!logProgTrace( TRACE5, "GetCompId:  CompMap add (idx=%d,comp_id=%d) sum=%d", idx, CompMap[ idx ].comp_id, CompMap[ idx ].sum );
    }
    Qry.Next();
  }
// ProgTrace( TRACE5, "CompMap.size()=%zu", CompMap.size() );
 if ( !CompMap.size() ) {
    return -1;
 }
 else {
  ProgTrace( TRACE5, "GetCompId:  CompMap begin (idx=%d,comp_id=%d) sum=%d", CompMap.begin()->first, CompMap.begin()->second.comp_id, CompMap.begin()->second.sum );
    return CompMap.begin()->second.comp_id; // ��������� ����� - ���஢�� ���� �� �����⠭��
 }
}

struct TTripClasses {
  int block;
  int protect;
  int cfg;
  TTripClasses() {
    block = 0;
    protect = 0;
    cfg = 0;
  }
};

void setTRIP_CLASSES( int point_id )
{
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "DELETE trip_classes WHERE point_id = :point_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText =
    "SELECT t.num, t.x, t.y, t.class, t.elem_type, r.layer_type"
    " FROM trip_comp_ranges r, trip_comp_elems t "
    "WHERE r.point_id=:point_id AND "
    "      t.point_id=r.point_id AND "
    "      t.num=r.num AND "
    "      t.x=r.x AND "
    "      t.y=r.y AND "
    "      r.layer_type in (:blockcent_layer,:disable_layer,:protect_layer)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "blockcent_layer", otString, EncodeCompLayerType(ASTRA::cltBlockCent) );
  Qry.CreateVariable( "disable_layer", otString, EncodeCompLayerType(ASTRA::cltDisable) );
  Qry.CreateVariable( "protect_layer", otString, EncodeCompLayerType(ASTRA::cltProtect) );
  Qry.Execute();
  int idx_num = Qry.FieldIndex( "num" );
  int idx_x = Qry.FieldIndex( "x" );
  int idx_y = Qry.FieldIndex( "y" );
  int idx_elem_type = Qry.FieldIndex( "elem_type" );
  int idx_class = Qry.FieldIndex( "class" );
  int idx_layer_type = Qry.FieldIndex( "layer_type" );
  map<string,vector<TPlace> > seats;
  for ( ; !Qry.Eof; Qry.Next() ) {
    if ( !TCompElemTypes::Instance()->isSeat( Qry.FieldAsString( idx_elem_type ) ) ) {
      continue;
    }
    TPlace seat;
    seat.num = Qry.FieldAsInteger( idx_num );
    seat.x = Qry.FieldAsInteger( idx_x );
    seat.y = Qry.FieldAsInteger( idx_y );
    seat.clname = Qry.FieldAsString( idx_class );
    TSeatLayer seatLayer;
    seatLayer.point_id = point_id;
    seatLayer.layer_type = DecodeCompLayerType( Qry.FieldAsString( idx_layer_type ) );
    vector<TPlace>::iterator iseat;
    for ( iseat=seats[ seat.clname ].begin(); iseat!=seats[ seat.clname ].end(); iseat++ ) {
      if ( iseat->num == seat.num &&
           iseat->x == seat.x &&
           iseat->y == seat.y ) {
        break;
      }
    }
    if ( iseat != seats[ seat.clname ].end() ) {
      iseat->AddLayer( point_id, seatLayer );
    }
    else {
      seat.AddLayer( point_id, seatLayer );
      seats[ seat.clname ].push_back( seat );
    }
  }
  map<string,TTripClasses> trip_classes;
  for ( map<string,vector<TPlace> >::iterator iclass=seats.begin(); iclass!=seats.end(); iclass++ ) {
    for ( vector<TPlace>::iterator iseat=iclass->second.begin(); iseat!=iclass->second.end(); iseat++ ) {
      if ( iseat->getCurrLayer( point_id ).layer_type == cltBlockCent ||
           iseat->getCurrLayer( point_id ).layer_type == cltDisable ) {
        trip_classes[ iclass->first ].block++;
      }
      if ( iseat->getCurrLayer( point_id ).layer_type == ASTRA::cltProtect ) {
        trip_classes[ iclass->first ].protect++;
      }
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT class, elem_type, COUNT( elem_type ) cfg "
    " FROM trip_comp_elems "
    "WHERE trip_comp_elems.point_id=:point_id AND "
    "      class IS NOT NULL "
    "GROUP BY class, elem_type";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    if ( !TCompElemTypes::Instance()->isSeat( Qry.FieldAsString( "elem_type" ) ) ) {
      continue;
    }
    trip_classes[ Qry.FieldAsString( "class" ) ].cfg += Qry.FieldAsInteger( "cfg" );
  }
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO trip_classes(point_id,class,cfg,block,prot) "
    " VALUES(:point_id,:class,:cfg,:block,:prot)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "cfg", otInteger );
  Qry.DeclareVariable( "block", otInteger );
  Qry.DeclareVariable( "prot", otInteger );
  for( map<string,TTripClasses>::iterator iclass=trip_classes.begin(); iclass!=trip_classes.end(); iclass++ ) {
    Qry.SetVariable( "class", iclass->first );
    Qry.SetVariable( "cfg", iclass->second.cfg );
    Qry.SetVariable( "block", iclass->second.block );
    Qry.SetVariable( "prot", iclass->second.protect );
    Qry.Execute();
  }
  CheckIn::TCounters().recount(point_id, CheckIn::TCounters::Total, __FUNCTION__);
}

void CreateComps( const TCompsRoutes &routes, int comp_id )
{
  TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  int pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

  TQuery QryTripSets(&OraSession);
  QryTripSets.SQLText =
    "UPDATE trip_sets SET comp_id=:comp_id, pr_lat_seat=:pr_lat_seat,crc_comp=:crc_comp WHERE point_id=:point_id";
  QryTripSets.CreateVariable( "comp_id", otInteger, comp_id );
  QryTripSets.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
  QryTripSets.DeclareVariable( "point_id", otInteger );
  QryTripSets.DeclareVariable( "crc_comp", otInteger );
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "DELETE trip_comp_rates WHERE point_id = :point_id;"
    "DELETE trip_comp_rfisc WHERE point_id = :point_id;"
    "DELETE trip_comp_rem WHERE point_id = :point_id; "
    "DELETE trip_comp_baselayers WHERE point_id = :point_id; "
    "DELETE trip_comp_elems WHERE point_id = :point_id; "
    "DELETE trip_comp_layers "
    " WHERE point_id=:point_id AND layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
    " SELECT :point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname "
    "  FROM comp_elems "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,rem,pr_denial "
    "  FROM comp_rem "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rfisc(point_id,num,x,y,color) "
    " SELECT :point_id,num,x,y,color FROM comp_rfisc "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
    " SELECT :point_id,num,x,y,color,rate,rate_cur FROM comp_rates "
    " WHERE comp_id = :comp_id; "
    "END;";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.DeclareVariable( "point_id", otInteger );
  TQuery QryBaseLayers(&OraSession);
  QryBaseLayers.SQLText =
    "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
    " SELECT :point_id,num,x,y,layer_type "
    "  FROM comp_baselayers "
    " WHERE comp_id=:comp_id AND layer_type=:layer_type ";
  QryBaseLayers.CreateVariable( "comp_id", otInteger, comp_id );
  QryBaseLayers.DeclareVariable( "point_id", otInteger );
  QryBaseLayers.DeclareVariable( "layer_type", otString );
  TQuery QryLayers(&OraSession);
  QryLayers.SQLText =
    "INSERT INTO trip_comp_layers("
    "       range_id,point_id,point_dep,point_arv,layer_type, "
    "       first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create)"
    "SELECT comp_layers__seq.nextval,:point_id,:point_id,NULL,:layer_type, "
    "       xname,xname,yname,yname,NULL,NULL,system.UTCSYSDATE "
    " FROM comp_baselayers, comp_elems "
    " WHERE comp_elems.comp_id=:comp_id AND "
    "       comp_elems.comp_id=comp_baselayers.comp_id AND "
    "       comp_elems.num=comp_baselayers.num AND "
    "       comp_elems.x=comp_baselayers.x AND "
    "       comp_elems.y=comp_baselayers.y AND "
    "       comp_baselayers.layer_type=:layer_type ";
  QryLayers.CreateVariable( "comp_id", otInteger, comp_id );
  QryLayers.DeclareVariable( "point_id", otInteger );
  QryLayers.DeclareVariable( "layer_type", otString );
  int crc_comp = 0;
  std::vector<int> points_check_wait_alarm;
  std::vector<int> points_tranzit_check_wait_alarm;
  for (TCompsRoutes::const_iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( i->inRoutes && i->auto_comp_chg && i->pr_reg ) {
      Qry.SetVariable( "point_id", i->point_id );
      Qry.Execute();
      for ( int ilayer=0; ilayer<ASTRA::cltTypeNum; ilayer++ ) {
        if ( isBaseLayer( (ASTRA::TCompLayerType)ilayer, true ) ) { // �롨ࠥ� �� ������ ᫮� ��� ������� ����������
          if ( isBaseLayer( (ASTRA::TCompLayerType)ilayer, false ) ) { // ������ ᫮� ��� ���������� ३�
            QryBaseLayers.SetVariable( "point_id", i->point_id );
            QryBaseLayers.SetVariable( "layer_type", EncodeCompLayerType( (ASTRA::TCompLayerType)ilayer ) );
            QryBaseLayers.Execute();
          }
          else { //�� ������
            QryLayers.SetVariable( "point_id", i->point_id );
            QryLayers.SetVariable( "layer_type", EncodeCompLayerType( (ASTRA::TCompLayerType)ilayer ) );
            QryLayers.Execute();
          }
        }
      }
      if ( crc_comp == 0 ) {
        crc_comp = CRC32_Comp( i->point_id );
      }
      InitVIP( i->point_id );
      setTRIP_CLASSES( i->point_id );
      QryTripSets.SetVariable( "point_id", i->point_id );
      QryTripSets.SetVariable( "crc_comp", crc_comp );
      QryTripSets.Execute();
      LEvntPrms prms;
      TCFG(i->point_id).param(prms);
      prms << PrmSmpl<int>("id", comp_id);
      TReqInfo::Instance()->LocaleToLog("EVT.ASSIGNE_BASE_LAYOUT", prms, evtFlt, i->point_id);
      if ( SALONS2::isTranzitSalons( i->point_id ) ) {
        if ( find( points_tranzit_check_wait_alarm.begin(),
                   points_tranzit_check_wait_alarm.end(),
                   i->point_id) == points_tranzit_check_wait_alarm.end() ) {
          points_tranzit_check_wait_alarm.push_back( i->point_id );
        }
      }
      else {
        if ( find( points_check_wait_alarm.begin(),
                   points_check_wait_alarm.end(),
                   i->point_id ) == points_check_wait_alarm.end() ) {
          points_check_wait_alarm.push_back( i->point_id );
        }
      }
    }
  }
  TCompsRoutes r(routes);
  check_diffcomp_alarm( r );
  for ( std::vector<int>::iterator i=points_check_wait_alarm.begin();
        i!=points_check_wait_alarm.end(); i++ ) {
    check_waitlist_alarm(*i);
  }
  check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm );
}

bool CompRouteinRoutes( const CompRoute &item1, const CompRoute &item2 )
{
  return ( (item1.craft == item2.craft || item2.craft.empty()) &&
           (item1.bort == item2.bort || item2.bort.empty()) &&
           item1.airline == item2.airline );
}

void push_routes( const CompRoute &currroute,
                  const TTripRoute &routes,
                  bool pr_before,
                  TCompsRoutes &comps_routes )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline,airp,bort,craft,pr_reg FROM points WHERE point_id=:point_id AND pr_del!=-1";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( vector<TTripRouteItem>::const_iterator i=routes.begin(); i!=routes.end(); i++ ) {
    //�뤥�塞 ������� 㤮���⢮���騩 ᫥�. �᫮���:
    //1. ����� �ਧ��� ��� �����祭�� ����������
    //2. ����� �ਧ��� ॣ����樨
    //3. ���� � ⨯ ��, ������������  ᮢ������ � ��室�� �㭪⮬
    //4. �� ��᫥���� �㭪� � �࠭��⭮� �������
    CompRoute route;
    bool inRoutes = true;
    route.point_id = i->point_id;
    route.auto_comp_chg = isAutoCompChg( i->point_id );
    Qry.SetVariable( "point_id", i->point_id );
    Qry.Execute();
    route.airline = Qry.FieldAsString( "airline" );
    route.airp = Qry.FieldAsString( "airp" );
    route.craft = Qry.FieldAsString( "craft" );
    route.bort = Qry.FieldAsString( "bort" );
    route.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
    route.inRoutes = ( CompRouteinRoutes( currroute, route ) && inRoutes);
    if ( !pr_before && (!route.inRoutes || i == routes.end() - 1) )
      inRoutes = false;
    comps_routes.push_back( route );
  }
}

void get_comp_routes( bool pr_tranzit_routes, int point_id, TCompsRoutes &routes )
{
  routes.clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT point_num,first_point,pr_tranzit,"
    "       airline,airp,bort,craft,pr_reg "
    " FROM points "
    " WHERE point_id=:point_id AND pr_del!=-1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return;
  int point_num = Qry.FieldAsInteger( "point_num" );
  int first_point = Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  bool pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" ) != 0;
  CompRoute currroute;
  currroute.point_id = point_id;
  currroute.airline = Qry.FieldAsString( "airline" );
  currroute.airp = Qry.FieldAsString( "airp" );
  currroute.bort = Qry.FieldAsString( "bort" );
  currroute.craft = Qry.FieldAsString( "craft" );
  currroute.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
  currroute.auto_comp_chg = isAutoCompChg( point_id );
  currroute.inRoutes = true;
    //!logProgTrace( TRACE5, "get_comp_routes: point_id=%d,pr_tranzit_routes=%d,point_num=%d,first_point=%d,pr_tranzit=%d,bort=%s,craft=%s",
  //!log           point_id,pr_tranzit_routes,point_num,first_point,pr_tranzit,currroute.bort.c_str(),currroute.craft.c_str() );
  TTripRoute routesB, routesA;
  if ( pr_tranzit ) {
    routesB.GetRouteBefore( NoExists,
                            point_id,
                            point_num,
                            first_point,
                            pr_tranzit,
                            trtNotCurrent,
                            trtNotCancelled );
  }
  routesA.GetRouteAfter( NoExists,
                         point_id,
                         point_num,
                         first_point,
                         pr_tranzit,
                         trtNotCurrent,
                         trtNotCancelled );
  if ( routesA.empty() ) { // ३� �� �ਫ��
    routes.push_back( currroute );
    ProgTrace( TRACE5, "get_comp_routes: routesA.empty()" );
    return;
  }
  tst();
  if ( pr_tranzit_routes ) { //������� ���������� ��� �ᥣ� �������
    push_routes( currroute, routesB, true, routes );
  }
  routes.push_back( currroute );
  if ( pr_tranzit_routes )  //������� ���������� ��� �ᥣ� �������
    push_routes( currroute, routesA, false, routes );
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    ProgTrace( TRACE5, "get_comp_routes: i->point_id=%d, i->inRoutes=%d, i->pr_reg=%d", i->point_id, i->inRoutes, i->pr_reg );
  }
}

struct TCounters {
  int point_id;
  int f, c, y;
  TCounters() {
    f = 0;
    c = 0;
    y = 0;
    point_id = -1;
  };
};

void getCrsData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) f, "
    "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) c, "
    "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) y "
    " FROM crs_data,tlg_binding,points "
    " WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
    "       points.point_id=:point_id AND "
    "       point_id_spp=:point_id AND system='CRS' AND airp_arv=points.airp ";
  Qry.DeclareVariable( "point_id", otInteger );

  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    //!logProgTrace( TRACE5, "getCrsData: routes->point_id=%d", *i );
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    crs_data[ *i ].f = Qry.FieldAsInteger( "f" );
    crs_data[ *i ].c = Qry.FieldAsInteger( "c" );
    crs_data[ *i ].y = Qry.FieldAsInteger( "y" );
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
         crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
  }
}

void getCountersData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT airp_arv,class, "
    "       0 AS priority, "
    "       crs_ok + crs_tranzit AS c "
    " FROM crs_counters "
    "WHERE point_dep=:point_id "
    "UNION "
    "SELECT airp_arv,class,1,resa + tranzit "
    " FROM trip_data "
    "WHERE point_id=:point_id "
    "ORDER BY priority DESC ";
  Qry.DeclareVariable( "point_id", otInteger );

  string vclass;
  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    //!logProgTrace( TRACE5, "getCountersData: routes->point_id=%d", *i );
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    int priority = -1;
    while ( !Qry.Eof ) {
        if ( Qry.FieldAsInteger( "c" ) > 0 ) {
          priority = Qry.FieldAsInteger( "priority" );
        if ( priority != Qry.FieldAsInteger( "priority" ) )
          break;
        vclass = Qry.FieldAsString( "class" );
        //!logProgTrace( TRACE5, "point_id=%d, class=%s, count=%d", *i, vclass.c_str(), Qry.FieldAsInteger( "c" ) );
        if ( vclass == "�" ) crs_data[ *i ].f += Qry.FieldAsInteger( "c" );
            if ( vclass == "�" ) crs_data[ *i ].c += Qry.FieldAsInteger( "c" );
        if ( vclass == "�" ) crs_data[ *i ].y += Qry.FieldAsInteger( "c" );
      }
        Qry.Next();
    }
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
      crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
    //!logProgTrace( TRACE5, "crs_data[ %d ].f=%d, crs_data[ %d ].c=%d, crs_data[ %d ].y=%d",
    //!log           *i, crs_data[ *i ].f, *i, crs_data[ *i ].c, *i, crs_data[ *i ].y );
  }
  //!logProgTrace( TRACE5, "point_id=%d, crs_data[ -1 ].f=%d, crs_data[ -1 ].c=%d, crs_data[ -1 ].y=%d",
  //!log           crs_data[ -1 ].point_id, crs_data[ -1 ].f, crs_data[ -1 ].c, crs_data[ -1 ].y );
}

void getSeasonData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT ABS(f) f, ABS(c) c, ABS(y) y FROM trip_sets WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );

  int priorf = NoExists, priorc = NoExists, priory = NoExists;
  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    if ( !Qry.Eof ) {
      //!logProgTrace( TRACE5, "point_id=%d", *i );
      if ( priorf == Qry.FieldAsInteger( "f" ) &&
           priorc == Qry.FieldAsInteger( "c" ) &&
           priory == Qry.FieldAsInteger( "y" ) )
        continue;
      crs_data[ *i ].f = Qry.FieldAsInteger( "f" );
      crs_data[ *i ].c = Qry.FieldAsInteger( "c" );
      crs_data[ *i ].y = Qry.FieldAsInteger( "y" );
      priorf = Qry.FieldAsInteger( "f" );
      priorc = Qry.FieldAsInteger( "c" );
      priory = Qry.FieldAsInteger( "y" );
    }
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
      crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
    //!logProgTrace( TRACE5, "crs_data[ %d ].f=%d, crs_data[ %d ].c=%d, crs_data[ %d ].y=%d",
    //!log           *i, crs_data[ *i ].f, *i, crs_data[ *i ].c, *i, crs_data[ *i ].y );
  }
  //!logProgTrace( TRACE5, "point_id=%d, crs_data[ -1 ].f=%d, crs_data[ -1 ].c=%d, crs_data[ -1 ].y=%d",
  //!log           crs_data[ -1 ].point_id, crs_data[ -1 ].f, crs_data[ -1 ].c, crs_data[ -1 ].y );
}

int CRC32_Comp( const SALONS2::TSalonList &comp )
{
  std::vector<std::string> elem_types;
  constructiveElemTypes( elem_types );
  ostringstream buf;
  for ( std::vector<TPlaceList*>::const_iterator isalon=comp.begin(); isalon!=comp.end(); isalon++ ) {
    for ( TPlaces::const_iterator iseat=(*isalon)->places.begin(); iseat!=(*isalon)->places.end(); iseat++ ) {
      if ( !iseat->visible || isConstructivePlace( iseat->elem_type, elem_types ) ) {
        continue;
      }
      buf << (*isalon)->num << iseat->x << iseat->y << iseat->clname << iseat->xname << iseat->yname << TCompElemTypes::Instance()->isSeat( iseat->elem_type );
    }
  }
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  crc32.process_bytes( buf.str().c_str(), buf.str().size() );
  int comp_crc = crc32.checksum();
  //!logProgTrace( TRACE5, "CRC32_Comp: point_id=%d, crc_comp=%d", point_id, comp_id );
  return comp_crc;

}

int CRC32_Comp( int point_id )
{
  vector<TSalonPoint> cltDisables;
  TQuery QryDisableLayer(&OraSession);
  //⮫쪮 ��� ࠧ��祭��� ᫮�� � ����������
  QryDisableLayer.SQLText =
    "SELECT num,x,y FROM trip_comp_ranges "
    " WHERE point_id=:point_id AND layer_type=:disable_layer "
    "ORDER BY num,x,y";
  QryDisableLayer.CreateVariable( "point_id", otInteger, point_id );
  QryDisableLayer.CreateVariable( "disable_layer", otString, EncodeCompLayerType( cltDisable ) );
  QryDisableLayer.Execute();
  int idx_num_dis = QryDisableLayer.FieldIndex( "num" );
  int idx_x_dis = QryDisableLayer.FieldIndex( "x" );
  int idx_y_dis = QryDisableLayer.FieldIndex( "y" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT num,x,y,elem_type,class,xname,yname FROM trip_comp_elems "
    " WHERE point_id=:point_id "
    "ORDER BY num,x,y";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return 0;
  string buf;
  int idx_num = Qry.FieldIndex( "num" );
  int idx_x = Qry.FieldIndex( "x" );
  int idx_y = Qry.FieldIndex( "y" );
  int idx_elem_type = Qry.FieldIndex( "elem_type" );
  int idx_class = Qry.FieldIndex( "class" );
  int idx_xname = Qry.FieldIndex( "xname" );
  int idx_yname = Qry.FieldIndex( "yname" );
  for ( ; !Qry.Eof; Qry.Next() ) {
    TSalonPoint p( Qry.FieldAsInteger( idx_x ),
                   Qry.FieldAsInteger( idx_y ),
                   Qry.FieldAsInteger( idx_num ) );
    buf += IntToString( p.num );
    buf += IntToString( p.x );
    buf += IntToString( p.y );
    buf += IntToString( TCompElemTypes::Instance()->isSeat( Qry.FieldAsString( idx_elem_type ) ) );
    bool pr_disable = ( !QryDisableLayer.Eof &&
                         QryDisableLayer.FieldAsInteger( idx_num_dis ) == p.num &&
                         QryDisableLayer.FieldAsInteger( idx_x_dis ) == p.x &&
                         QryDisableLayer.FieldAsInteger( idx_y_dis ) == p.y );
    if ( pr_disable ) {
      QryDisableLayer.Next();
      buf += "1";
    }
    else {
      buf += "0";
    }
    buf += Qry.FieldAsString( idx_class );
    buf += Qry.FieldAsString( idx_xname );
    buf += Qry.FieldAsString( idx_yname );
  }
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  crc32.process_bytes( buf.c_str(), buf.size() );
  int comp_id = crc32.checksum();
  //!logProgTrace( TRACE5, "CRC32_Comp: point_id=%d, crc_comp=%d", point_id, comp_id );
  return comp_id;
}

int getCRC_Comp( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT crc_comp,comp_id FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgError( STDLOG, "getCRC_Comp: point_id=%d, trip_sets not exists record", point_id );
    return 0;
  }
  return Qry.FieldAsInteger( "crc_comp" );
}

void calc_diffcomp_alarm( TCompsRoutes &routes )
{
  TCompsRoutes::iterator iprior = routes.end();
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    //!logProgTrace( TRACE5, "i->point_id=%d, i->pr_reg=%d", i->point_id, i->pr_reg );
    i->pr_alarm = false;
    if ( !i->pr_reg )
      continue;
    if ( iprior != routes.end() && i != routes.end()-1 ) {
      int crc_comp1 = getCRC_Comp( iprior->point_id );
      int crc_comp2 = getCRC_Comp( i->point_id );
      //!logProgTrace( TRACE5, "iprior->point_id=%d, prior_crc_comp1=%d, i->point_id=%d, crc_comp2=%d",
      //!log           iprior->point_id, crc_comp1, i->point_id, crc_comp2 );
      if ( !CompRouteinRoutes( *iprior, *i ) ||
           ( crc_comp1 != 0 &&
             crc_comp2 != 0 &&
             crc_comp1 != crc_comp2 ) ) {
         i->pr_alarm = true;
      }
    }
    iprior = i;
  }
}

void check_diffcomp_alarm( TCompsRoutes &routes )
{
 calc_diffcomp_alarm( routes );
 for (  TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
   //!logProgTrace( TRACE5, "check_diffcomp_alarm: i->point_id=%d, pr_alarm=%d",
   //!log           i->point_id, i->pr_alarm );
   set_alarm( i->point_id, Alarm::DiffComps, i->pr_alarm );
 }
}

void check_diffcomp_alarm( int point_id )
{
  TCompsRoutes routes;
  get_comp_routes( true, point_id, routes );
  check_diffcomp_alarm( routes );
}

std::string getDiffCompsAlarmRoutes( int point_id )
{
/*  TCompsRoutes routes;
  get_comp_routes( true, point_id, routes );
  calc_diffcomp_alarm( routes );
  string res;
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( !i->pr_alarm )
      continue;
    if ( res.empty() )
      res += ":";
    else
      res = "-";
    res += i->airp;
  }
  ProgTrace( TRACE5, "getDiffCompsAlarmRoutes: point_id=%d, res=%s", point_id, res.c_str() );
  return res; */
  string res;
  return res;
}

TFindSetCraft SetCraft( bool pr_tranzit_routes, int point_id, TSetsCraftPoints &points )
{
  TFlights flights;
    flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

  points.Clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT bort,airline,airp,craft, NVL(comp_id,-1) comp_id "
    " FROM points, trip_sets "
    " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";// FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string bort = Qry.FieldAsString( "bort" );
  string airline = Qry.FieldAsString( "airline" );
  string airp = Qry.FieldAsString( "airp" );
  int old_comp_id = Qry.FieldAsInteger( "comp_id" );
    string craft = Qry.FieldAsString( "craft" );

    //!logProgTrace( TRACE5, "SetCraft: point_id=%d,pr_tranzit_routes=%d,bort=%s,craft=%s,old_comp_id=%d",
  //!log           point_id,pr_tranzit_routes,bort.c_str(),craft.c_str(),old_comp_id );
    if ( craft.empty() ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoCraftComps, craft.empty()" );
    return rsComp_NoCraftComps;
  }
  // �஢�ઠ �� ����⢮����� �������� ���������� �� ⨯� ��
    Qry.Clear();
    Qry.SQLText =
   "SELECT comp_id FROM comps "
   " WHERE craft=:craft AND rownum < 2";
    Qry.CreateVariable( "craft", otString, craft );
    Qry.Execute();
    if ( Qry.Eof ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoCraftComps" );
    return rsComp_NoCraftComps;
  }
  TCompsRoutes routes;
  get_comp_routes( pr_tranzit_routes, point_id, routes );
  vector<string> airps;
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( i->inRoutes && i->auto_comp_chg && i->pr_reg ) {
      points.push_back( i->point_id );
      airps.push_back( i->airp );
    }
    if ( i->point_id == point_id && !i->auto_comp_chg )
      return rsComp_NoChanges;
  }
  if ( points.empty() )
    return rsComp_NoChanges;
  map<int,TCounters> crs_data;
  
  for ( int step=0; step<=5; step++ ) {
    // �롨ࠥ� ����. ���������� �� CFG �� PNL/ADL ��� ��� 業�஢ �஭�஢����
    //!logProgTrace( TRACE5, "step=%d", step );
    if ( step == 0 ) {
      getCrsData( points, crs_data );
    }
    if ( step == 2 ) {
      getCountersData( points, crs_data );
    }
    if ( step == 4 ) {
      getSeasonData( points, crs_data );
    }
    for ( map<int,TCounters>::iterator i=crs_data.begin(); i!=crs_data.end(); i++ ) {
      if ( step == 0 || step == 2 || step == 4 ) {
        if ( i->first != -1 ||
             i->second.f + i->second.c + i->second.y <= 0 )
          continue;
      }
      if ( step == 1 || step == 3 || step == 5 ) {
        if ( !pr_tranzit_routes ||
             i->first == -1 ||
             i->first == crs_data[ -1 ].point_id ||
             i->second.f + i->second.c + i->second.y <= 0 )
          continue;
      }
      bool pr_ignore_fcy = ( step == 4 && i->second.f == 0 && i->second.c == 0 && i->second.y == 1 ); // ᥧ���� 㬮�砭�� - ������㥬
      points.comp_id = GetCompId( craft, bort, airline, airps,
                                  i->second.f, i->second.c, i->second.y, pr_ignore_fcy );
      if ( points.comp_id >= 0 )
        break;
    }
    if ( points.comp_id >= 0 )
        break;
  }
  if ( points.comp_id < 0 ) {
    if ( !bort.empty() && !craft.empty() ) {
      points.comp_id = GetCompId( craft, bort, airline, airps,
                                  0, 0, 1, true );
    }
  }
  if ( points.comp_id < 0 ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoFound" );
    return rsComp_NoFound;
  }
    // ������ ��ਠ�� ����������
    if ( old_comp_id == points.comp_id ) {
      ProgTrace( TRACE5, "SetCraft: return rsComp_NoChanges" );
        return rsComp_NoChanges; // �� �㦭� �������� ����������
  }
    CreateComps( routes, points.comp_id );

/* ???	check_diffcomp_alarm( routes );
    if ( isTranzitSalons( point_id ) ) {
    check_waitlist_alarm_on_tranzit_routes( point_id );
  }*/
    ProgTrace( TRACE5, "SetCraft: return rsComp_Found" );
  return rsComp_Found;
}

TFindSetCraft AutoSetCraft( int point_id )
{
  TSetsCraftPoints points;
  return AutoSetCraft( true, point_id, points );
}

TFindSetCraft AutoSetCraft( int point_id, TSetsCraftPoints &points )
{
  return AutoSetCraft( true, point_id, points );
}

bool isAutoCompChg( int point_id )
{
    TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT auto_comp_chg FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "auto_comp_chg" ) ); // ��⮬���᪮� �����祭�� ����������
}

void setManualCompChg( int point_id )
{
  //set flag auto change in false state
  TQuery Qry(&OraSession);
    Qry.SQLText = "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

TFindSetCraft AutoSetCraft( bool pr_tranzit_routes, int point_id, TSetsCraftPoints &points )
{
    ProgTrace( TRACE5, "AutoSetCraft, pr_tranzit_routes=%d, point_id=%d", pr_tranzit_routes, point_id );
    try {
      points.Clear();
    if ( isAutoCompChg( point_id ) && !isFreeSeating( point_id ) ) {
        ProgTrace( TRACE5, "Auto set comp, point_id=%d", point_id );
      return SetCraft( pr_tranzit_routes, point_id, points );
    }
    return rsComp_NoChanges; // �� �ॡ���� �����祭�� ����������
  }
  catch( EXCEPTIONS::Exception &e ) {
    ProgError( STDLOG, "AutoSetCraft: Exception %s, point_id=%d", e.what(), point_id );
  }
  catch( ... ) {
    ProgError( STDLOG, "AutoSetCraft: unknown error, point_id=%d", point_id );
  }
  return rsComp_NoChanges;
}

void InitVIP( int point_id )
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
    TTripInfo info( Qry );
    if ( !GetTripSets( tsCraftInitVIP, info ) )
    return;
    // ���樠������ - ࠧ��⪠ ᠫ��� �� 㬮�砭�
    TQuery QryVIP(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "SELECT num, class, MIN( y ) miny, MAX( y ) maxy "
    " FROM trip_comp_elems, comp_elem_types "
    "WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "      comp_elem_types.pr_seat <> 0 AND "
    "      trip_comp_elems.point_id = :point_id "
    "GROUP BY trip_comp_elems.num, trip_comp_elems.class";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  QryVIP.SQLText =
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,'VIP', 0 FROM "
    " ( SELECT :point_id,num,x,y FROM trip_comp_elems "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        elem_type IN ( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) AND "
    "        trip_comp_elems.y = :y "
    "   MINUS "
    "  SELECT :point_id,num,x,y FROM trip_comp_ranges "
    "   WHERE point_id=:point_id AND layer_type IN (:block_cent_layer, :checkin_layer) "
    " ) "
    " MINUS "
    " SELECT :point_id,num,x,y,rem, 0 FROM trip_comp_rem "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        trip_comp_rem.y = :y "; //??? �᫨ ���� ६�ઠ �����祭��� ���짮��⥫��, � �� �ண��� ����
  QryVIP.CreateVariable( "point_id", otInteger, point_id );
  QryVIP.DeclareVariable( "num", otInteger );
  QryVIP.DeclareVariable( "y", otInteger );
  QryVIP.CreateVariable( "block_cent_layer", otString, EncodeCompLayerType(cltBlockCent) );
  QryVIP.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(cltCheckin) );
  while ( !Qry.Eof ) {
    int ycount;
    switch( DecodeClass( Qry.FieldAsString( "class" ) ) ) {
        case F: ycount = REM_VIP_F;
            break;
        case C: ycount = REM_VIP_C;
            break;
        case Y: ycount = REM_VIP_Y;
            break;
        default: ycount = 0;
    };
    if ( ycount ) {
        int vy = 0;
        while ( Qry.FieldAsInteger( "miny" ) + vy <= Qry.FieldAsInteger( "maxy" ) && vy < ycount ) {
            QryVIP.SetVariable( "num", Qry.FieldAsInteger( "num" ) );
            QryVIP.SetVariable( "y", Qry.FieldAsInteger( "miny" ) + vy );
            QryVIP.Execute();
            if ( !QryVIP.RowsProcessed( ) )
                break;
            vy++;
        }
    }
    Qry.Next();
  }
}

bool EqualSalon( TPlaceList* oldsalon, TPlaceList* newsalon,
                 TCompareCompsFlags compareFlags )
{
    //�������� ����� ⮭�� �業����� ᠫ��: 㤠����� ���� �� �鸞/����� - �� ���� �⠭������ ��������, �� �� �⮬ ������ ᠬ� �������� �鸞/�����
    bool res = ( oldsalon->places.size() == newsalon->places.size() &&
                 oldsalon->GetXsCount() == newsalon->GetXsCount() &&
                 oldsalon->GetYsCount() == newsalon->GetYsCount() );
  if ( !res ) {
    return false;
  }

  for ( TPlaces::iterator po = oldsalon->places.begin(),
                          pn = newsalon->places.begin();
        po != oldsalon->places.end(),
        pn != newsalon->places.end();
        po++, pn++ ) {
    if ( po->isChange( *pn, compareFlags  ) ) {
      return false;
    }
  }
  return true;
}

bool ChangeCfg( const vector<TPlaceList*> &list1,
                const vector<TPlaceList*> &list2 )
{
  TCompareCompsFlags compareFlags;
  compareFlags.setFlag( SALONS2::ccXY );
  compareFlags.setFlag( SALONS2::ccElemType );
  return ChangeCfg( list1, list2, compareFlags );
}

bool ChangeCfg( const vector<TPlaceList*> &list1,
                const vector<TPlaceList*> &list2,
                const TCompareCompsFlags &compareFlags )
{
  if ( list1.size() != list2.size() ) {
    return true;
  }
  for ( vector<TPlaceList*>::const_iterator s1=list1.begin(),
        s2=list2.begin();
        s1!=list1.end(),
        s2!=list2.end();
        s1++, s2++ ) {
    if ( !EqualSalon( *s1, *s2, compareFlags ) ) {
      return true;
    }
  }
  return false;
}


//use only in salonChangesToText - new version TSalonList
bool getSalonChanges( const vector<TPlaceList*> &list1, bool pr_craft_lat1,
                      const vector<TPlaceList*> &list2, bool pr_craft_lat2,
                      TRFISCMode RFISCMode,
                      TSalonChanges &seats )
{
  //!logProgTrace( TRACE5, "getSalonChanges" );
    seats.clear();
    seats.RFISCMode = RFISCMode;
    if ( pr_craft_lat1 != pr_craft_lat2 ||
           list1.size() != list2.size() )
        return false;
  TCompareCompsFlags compareNotChangeFlags, comparePropChangeFlag;
  compareNotChangeFlags.setFlag( ccXY );
  compareNotChangeFlags.setFlag( ccElemType );
  compareNotChangeFlags.setFlag( ccXYPrior );
  compareNotChangeFlags.setFlag( ccXYNext );
  compareNotChangeFlags.setFlag( ccAgle );
  compareNotChangeFlags.setFlag( ccClass );
  compareNotChangeFlags.setFlag( ccName );
  comparePropChangeFlag.setFlag( ccRemarks );
  comparePropChangeFlag.setFlag( ccLayers );
  comparePropChangeFlag.setFlag( ccTariffs );
  comparePropChangeFlag.setFlag( ccRFISC );
  comparePropChangeFlag.setFlag( ccDrawProps );

    for ( vector<TPlaceList*>::const_iterator s1=list1.begin(),
            s2=list2.begin();
            s1!=list1.end(),
            s2!=list2.end();
            s1++, s2++ ) {
        if ( (*s1)->places.size() != (*s2)->places.size() )
            return false;
    for ( TPlaces::const_iterator p1 = (*s1)->places.begin(),
            p2 = (*s2)->places.begin();
          p1 != (*s1)->places.end(),
          p2 != (*s2)->places.end();
          p1++, p2++ ) {
      if ( p1->isChange( *p2, compareNotChangeFlags ) ) {
        return false;
      }
      if ( !p1->visible )
        continue;
      if ( p1->isChange( *p2, comparePropChangeFlag ) ) {
        seats.push_back( make_pair((*s1)->num,*p2) );
      }
    }
    }
    return true;
}

//use only in salonChangesToText - new version TSalonList
void getSalonChanges( const TSalonList &salonList,
                      int tariff_pax_id,
                      TSalonChanges &seats )
{
  //!logProgTrace( TRACE5, "getSalonChanges: salonList.empty()=%d",
  //!log           salonList.empty() );
  seats.clear();
  TSalonList NewSalonList;
  NewSalonList.ReadFlight( salonList.getFilterRoutes(), rfTranzitVersion, salonList.getFilterClass(), tariff_pax_id );
  if ( !getSalonChanges( salonList, salonList.isCraftLat(), NewSalonList, NewSalonList.isCraftLat(), NewSalonList.getRFISCMode(), seats ) )
    throw UserException( "MSG.SALONS.COMPON_CHANGED.REFRESH_DATA" );
}

//////////////////////////////////////////////////////
bool CompareRems( const vector<TRem> &rems1, const vector<TRem> &rems2 )
{
    if ( rems1.size() != rems2.size() )
        return false;
    for ( vector<TRem>::const_iterator p1=rems1.begin(),
            p2=rems2.begin();
            p1!=rems1.end(),
            p2!=rems2.end();
            p1++, p2++ ) {
        if ( p1->rem != p2->rem ||
               p1->pr_denial != p2->pr_denial )
            return false;
  }
  return true;
}

bool CompareLayers( const vector<TPlaceLayer> &layer1, const vector<TPlaceLayer> &layer2 )
{
    if ( layer1.size() != layer2.size() )
        return false;
    for ( vector<TPlaceLayer>::const_iterator p1=layer1.begin(),
            p2=layer2.begin();
            p1!=layer1.end(),
            p2!=layer2.end();
            p1++, p2++ ) {
        if ( p1->layer_type != p2->layer_type )
            return false;
  }
  return true;
}

bool getSalonChanges( TSalons &OldSalons, TSalons &NewSalons,
                      TRFISCMode RFISCMode,
                      TSalonChanges &seats )
{
    seats.clear();
    seats.RFISCMode = RFISCMode;
    if ( NewSalons.getLatSeat() != OldSalons.getLatSeat() ||
           NewSalons.placelists.size() != OldSalons.placelists.size() )
        return false;
    for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
                                          sn=NewSalons.placelists.begin();
            so!=OldSalons.placelists.end(),
            sn!=NewSalons.placelists.end();
            so++, sn++ ) {
        if ( (*so)->places.size() != (*sn)->places.size() )
            return false;
    for ( TPlaces::iterator po = (*so)->places.begin(),
            /*TPlaces::iterator*/ pn = (*sn)->places.begin();
          po != (*so)->places.end(),
          pn != (*sn)->places.end();
          po++, pn++ ) {
      if ( po->visible != pn->visible ||
           (po->visible == pn->visible &&
            ( po->x != pn->x ||
              po->y != pn->y ||
              po->elem_type != pn->elem_type ||
              po->isplace != pn->isplace ||
              po->xprior != pn->xprior ||
              po->yprior != pn->yprior ||
              po->xnext != pn->xnext ||
              po->ynext != pn->ynext ||
              po->agle != pn->agle ||
              po->clname != pn->clname ||
              po->xname != pn->xname ||
              po->yname != pn->yname ) ) )
        return false;
      if ( !po->visible )
        continue;
      if ( !CompareRems( po->rems, pn->rems ) ||
           !CompareLayers( po->layers, pn->layers ) ||
           !(po->drawProps == pn->drawProps) ) {
        seats.push_back( make_pair((*so)->num,*pn) );
      }
    }
    }
    return true;
}

void getSalonChanges( TSalons &OldSalons, TRFISCMode RFISCMode, TSalonChanges &seats )
{
    seats.clear();
    seats.RFISCMode = RFISCMode;
    TSalons Salons( OldSalons.trip_id, rTripSalons );
    Salons.Read();
    if ( !getSalonChanges( OldSalons, Salons, RFISCMode, seats ) )
        throw UserException( "MSG.SALONS.COMPON_CHANGED.REFRESH_DATA" );
}



void BuildSalonChanges( xmlNodePtr dataNode, const TSalonChanges &seats )
{
  if ( seats.empty() )
    return;
  xmlNodePtr node = NewTextChild( dataNode, "update_salons" );
  SetProp( node, "RFISCMode", seats.RFISCMode );
  node = NewTextChild( node, "seats" );
  int num = -1;
  xmlNodePtr salonNode = NULL;
  BitSet<TDrawPropsType> props;
  for ( vector<TSalonSeat>::const_iterator p=seats.begin(); p!=seats.end(); p++ ) {
      if ( num != p->first ) {
        salonNode = NewTextChild( node, "salon" );
        SetProp( salonNode, "num", p->first );
        num = p->first;
      }
      p->second.Build( NewTextChild( salonNode, "place" ), true, true ); //!!props - ����� ���������� - ��� �� ��।��� �� ������
      props += p->second.drawProps;
  }
}

void BuildSalonChanges( xmlNodePtr dataNode,
                        int point_dep,
                        const TSalonChanges &seats,
                        bool with_pax, const std::map<int,SALONS2::TPaxList> &pax_lists )
{
  if ( seats.empty() )
    return;
  xmlNodePtr node = NewTextChild( dataNode, "update_salons" );
  SetProp( node, "RFISCMode", (int)seats.RFISCMode );
  node = NewTextChild( node, "seats" );
  int num = -1;
  xmlNodePtr salonNode = NULL;
  BitSet<TDrawPropsType> props;
  for ( vector<TSalonSeat>::const_iterator p=seats.begin(); p!=seats.end(); p++ ) {
      if ( num != p->first ) {
          salonNode = NewTextChild( node, "salon" );
          SetProp( salonNode, "num", p->first );
          num = p->first;
      }
      p->second.Build( NewTextChild( salonNode, "place" ), point_dep, true,
                       seats.RFISCMode, true,
                       with_pax, pax_lists ); //!!props - ����� ����������!!! - ��� �� ��।��� �� ������
      props += p->second.drawProps;
  }
}


struct TRowRef {
    string yname;
    string xnames;
};

typedef map<int,TRowRef,std::less<int> > RowsRef;

struct TStringRef {
    string value;
    bool pr_header;
    TStringRef( string vvalue, bool vpr_header ) {
        value = vvalue;
        pr_header = vpr_header;
    }
};

void SeparateEvents( vector<TStringRef> referStrs, vector<string> &eventsStrs, unsigned int line_len )
{
    for ( vector<TStringRef>::iterator i=referStrs.begin(); i!=referStrs.end(); i++ ) {
        //!logProgTrace( TRACE5, "i->value=%s, i->pr_header=%d",i->value.c_str(), i->pr_header);
  }
    eventsStrs.clear();
    if ( referStrs.empty() )
        return;
    vector<TStringRef>::iterator iheader=referStrs.end();
    string str_line;
    vector<string> strs, strs_header;
    bool pr_add_header;
    for ( vector<TStringRef>::iterator istr=referStrs.begin(); istr!=referStrs.end(); istr++ ) {
    if ( !istr->pr_header ) {
        if ( !str_line.empty() ) {
        str_line += " ";
      }
      str_line += istr->value;
    }
    if ( istr->pr_header ||
         istr == referStrs.end() - 1 ) {
      if ( !str_line.empty() ) { //putline
        int len = line_len;
        pr_add_header = ( iheader != referStrs.end() &&
                          line_len - 1 > iheader->value.size() );
        if ( pr_add_header ) {
          len -= iheader->value.size() + 1;
        }
        else {
          if ( iheader != referStrs.end() ) {
            SeparateString( iheader->value.c_str(), line_len, strs_header );
          }
        }
        SeparateString( str_line.c_str(), len, strs );
        if ( !pr_add_header &&
             iheader != referStrs.end() ) {
          eventsStrs.insert( eventsStrs.end(), strs_header.begin(), strs_header.end() );
        }
        for ( vector<string>::iterator iline=strs.begin(); iline!=strs.end(); iline++ ) {
          if ( pr_add_header ) {
            eventsStrs.push_back( iheader->value + " " + *iline );
            //!logProgTrace( TRACE5, "eventsStrs.push_back(%s)", string(iheader->value + *iline).c_str() );
          }
          else {
            eventsStrs.push_back( *iline );
            //!logProgTrace( TRACE5, "eventsStrs.push_back(%s)", iline->c_str() );
          }
        }
        str_line.clear();
      }
      iheader = istr;
    }
    if ( istr->pr_header &&
         istr == referStrs.end() - 1 ) {
      SeparateString( iheader->value.c_str(), line_len, strs_header );
      eventsStrs.insert( eventsStrs.end(), strs_header.begin(), strs_header.end() );
    }
    }
}


bool RightRows( const string &row1, const string &row2 )
{
    int r1, r2;
    if ( StrToInt( row1.c_str(), r1 ) == EOF || StrToInt( row2.c_str(), r2 ) == EOF )
        return false;
    //!logProgTrace( TRACE5, "r1=%d, r2=%d, EOF=%d, row1=%s, row2=%s", r1, r2, EOF, row1.c_str(), row2.c_str() );
    return ( r1 == r2 - 1 );
}



void getStrSeats( const RowsRef &rows, PrmEnum &params, bool pr_lat )
{
    for ( RowsRef::const_iterator i=rows.begin(); i!=rows.end(); i++ ) {
         //!logProgTrace( TRACE5, "i->first=%d, i->second.yname=%s, i->second.xnames=%s", i->first, i->second.yname.c_str(), i->second.xnames.c_str() );
    }
    vector<TStringRef> strs1, strs2;
  string str, max_lines, denorm_max_lines;
  int var1_size=0, var2_size=0;
  bool pr_rr = false, pr_right_rows=true;
  for ( int i=0; i<=1; i++ ) {
    RowsRef::const_iterator first_isr=rows.begin();
    RowsRef::const_iterator prior_isr=rows.begin();
    RowsRef::const_iterator isr=rows.begin();
    while ( !rows.empty() ) {
        if ( isr == rows.end() ||
               ( prior_isr != isr && !(pr_rr=RightRows( prior_isr->second.yname, isr->second.yname )) ) ||
               ( i == 0 && first_isr->second.xnames != isr->second.xnames ) || //���ᠭ�� ����� ���� � ��������묨 ����ﬨ
               ( i != 0 && first_isr->second.xnames.find_first_of( isr->second.xnames ) != string::npos ) ) { //���ᠭ�� ����� ���� � ���ᥪ��騬��� ����ﬨ
          if ( !pr_rr ) {
            pr_right_rows = false;
          }
          for ( string::const_iterator sp=prior_isr->second.xnames.begin(); sp!=prior_isr->second.xnames.end(); sp++ ) {
            if ( max_lines.find( *sp ) == string::npos ) {
              max_lines += *sp;
              denorm_max_lines += denorm_iata_line( string(1,*sp), pr_lat );
            }
          }
          if ( i == 0 ) {
            str += denorm_iata_row( first_isr->second.yname );
              if ( prior_isr->first != first_isr->first )
                str += "-" + denorm_iata_row( prior_isr->second.yname );
              for ( string::const_iterator sp=first_isr->second.xnames.begin(); sp!=first_isr->second.xnames.end(); sp++ ) {
              str += denorm_iata_line( string(1,*sp), pr_lat );
            }
            var1_size += str.size();
            strs1.push_back( TStringRef(str,false) );
            first_isr = isr;
            str.clear();
          }
          else
            if ( isr == rows.end() ) {
                if ( first_isr->first != prior_isr->first )
                  str = denorm_iata_row( first_isr->second.yname ) + "-" + denorm_iata_row( prior_isr->second.yname );
                else
                    str = denorm_iata_row( first_isr->second.yname );
                str += denorm_max_lines;
                // ��襬 � ����, ��� ���
                string minus_lines;
                for ( RowsRef::const_iterator isr=rows.begin(); isr!=rows.end(); isr++ ) {
                    minus_lines.clear();
                    for ( string::iterator sp=max_lines.begin(); sp!=max_lines.end(); sp++ ) {
                        if ( isr->second.xnames.find( *sp ) == string::npos )
                            minus_lines += denorm_iata_line( string(1,*sp), pr_lat );
                    }
                    if ( !minus_lines.empty() ) {
                        str += " -" + denorm_iata_row( isr->second.yname ) + minus_lines;
                    }
                }
                var2_size += str.size();
                strs2.push_back( TStringRef(str,false) );
            }
      }
      if ( isr == rows.end() )
          break;
      prior_isr = isr;
      isr++;
    }
  }
  if ( var1_size < var2_size || !pr_right_rows ) {
    for ( vector<TStringRef>::iterator i=strs1.begin(); i!=strs1.end(); i++ ) {
      //!logProgTrace( TRACE5, "getStrSeats: str1=%s", i->value.c_str() );
      params.prms << PrmSmpl<string>("", i->value);
    }
  }
  else {
    for ( vector<TStringRef>::iterator i=strs2.begin(); i!=strs2.end(); i++ ) {
      //!logProgTrace( TRACE5, "getStrSeats: str2=%s", i->value.c_str() );
      params.prms << PrmSmpl<string>("", i->value);
    }
  }
}

void ReferPlaces( int point_id, string name, TPlaces places, PrmEnum &params, bool pr_lat )
{
    //!logProgTrace( TRACE5, "ReferPlacesRow: name=%s", name.c_str() );
    string tmp;
    if ( places.empty() )
        return;
  TCompElemType elem_type;
  TCompElemTypes::Instance()->getElem( places.begin()->elem_type, elem_type );
    tmp = "ADD_COMMON_SALON_REF";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+") << PrmLexema("", "EVT.SALON")
                << PrmSmpl<string>("", " "+name.substr(name.find( tmp )+tmp.size() ) + " ")
                << PrmElem<string>("", etClass, places.begin()->clname) << PrmSmpl<string>("", " ")
                << PrmElem<string>("", etCompElemType, elem_type.getCode(), efmtNameLong) << PrmSmpl<string>("", ":");
    name.clear();
  }
    tmp = "ADD_SALON";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+") << PrmLexema("", "EVT.SALON")
                << PrmSmpl<string>("", " "+name.substr(name.find( tmp )+tmp.size() ) + ":");
  }
  tmp = string("DEL_SALON" );
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "-") << PrmLexema("", "EVT.SALON")
                << PrmSmpl<string>("", " "+name.substr(name.find( tmp )+tmp.size() ) + ":");
  }
  tmp = "ADD_SEATS";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+") << PrmElem<string>("", etCompElemType, elem_type.getCode(), efmtNameLong)
                << PrmSmpl<string>("", " ") << PrmElem<string>("", etClass, places.begin()->clname)
                << PrmSmpl<string>("", ":");
  }
  tmp = "DEL_SEATS";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "-") << PrmElem<string>("", etCompElemType, elem_type.getCode(), efmtNameLong)
                << PrmSmpl<string>("", " ") << PrmElem<string>("", etClass, places.begin()->clname)
                << PrmSmpl<string>("", ":");
  }
  tmp = "ADD_REMS";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+"+name.substr(name.find( tmp )+tmp.size() ) + ":");
  }
  tmp = "DEL_REMS";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "-"+name.substr(name.find( tmp )+tmp.size() ) + ":");
  }
  tmp = "ADD_LAYERS";
  if ( name.find( tmp ) != string::npos ) {
    ASTRA::TCompLayerType layer_type = DecodeCompLayerType( name.substr( name.find( tmp )+tmp.size() ).c_str() );
    BASIC_SALONS::TCompLayerTypes *compLayerTypes = BASIC_SALONS::TCompLayerTypes::Instance();
    params.prms << PrmSmpl<string>("", "+")
                << PrmElem<string>("", etCompLayerType, compLayerTypes->getCode((layer_type)), efmtNameLong)
                << PrmSmpl<string>("", ":");
  }
  tmp = "DEL_LAYERS";
  if ( name.find( tmp ) != string::npos ) {
    ASTRA::TCompLayerType layer_type = DecodeCompLayerType( name.substr( name.find( tmp )+tmp.size() ).c_str() );
    BASIC_SALONS::TCompLayerTypes *compLayerTypes = BASIC_SALONS::TCompLayerTypes::Instance();
    params.prms << PrmSmpl<string>("", "-")
                << PrmElem<string>("", etCompLayerType, compLayerTypes->getCode((layer_type)), efmtNameLong)
                << PrmSmpl<string>("", ":");
  }
  tmp = "ADD_WEB_TARIFF";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+") << PrmLexema("", "EVT.WEB_TARIFF") << PrmSmpl<string>("", " ");
    if ( /*TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION )*/ true ) {
      std::map<int, TSeatTariff,classcomp> tariffs;
      places.begin()->GetTariffs( tariffs );
      if ( tariffs.find( point_id ) != tariffs.end() ) {
        params.prms << PrmSmpl<string>("", tariffs[ point_id ].rateView())
                    << PrmElem<string>("", etCurrency, tariffs[ point_id ].currency_id);
      }
    }
    else {
      params.prms << PrmSmpl<string>("", places.begin()->SeatTariff.rateView())
                  << PrmElem<string>("", etCurrency, places.begin()->SeatTariff.currency_id);
    }
    params.prms << PrmSmpl<string>("", ":");
  }
  tmp = "DEL_WEB_TARIFF";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "-") << PrmLexema("", "EVT.WEB_TARIFF") << PrmSmpl<string>("", " ");
    if ( /*TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION )*/ true ) {
      std::map<int, TSeatTariff,classcomp> tariffs;
      places.begin()->GetTariffs( tariffs );
      if ( tariffs.find( point_id ) != tariffs.end() ) {
        params.prms << PrmSmpl<string>("", tariffs[ point_id ].rateView())
                    << PrmElem<string>("", etCurrency, tariffs[ point_id ].currency_id);
      }
    }
    else {
      params.prms << PrmSmpl<string>("", places.begin()->SeatTariff.rateView())
                  << PrmElem<string>("", etCurrency, places.begin()->SeatTariff.currency_id);
    }
    params.prms << PrmSmpl<string>("", ":");
  }

  tmp = "ADD_RFISC";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "+") << PrmLexema("", "EVT.RFISC") << PrmSmpl<string>("", " ");
    if ( /*TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION )*/ true ) {
      std::map<int, TRFISC,classcomp> rfiscs;
      places.begin()->GetRFISCs( rfiscs );
      if ( rfiscs.find( point_id ) != rfiscs.end() ) {
        tst();
        params.prms << PrmSmpl<string>("", ElemIdToNameLong( etRateColor, rfiscs[ point_id ].color ));
      }
    }
/*???    else {
      params.prms << PrmSmpl<string>("", places.begin()->SeatTariff.rateView())
                  << PrmElem<string>("", etCurrency, places.begin()->SeatTariff.currency_id);
    }*/
    params.prms << PrmSmpl<string>("", ":");
  }
  tmp = "DEL_RFISC";
  if ( name.find( tmp ) != string::npos ) {
    params.prms << PrmSmpl<string>("", "-") << PrmLexema("", "EVT.RFISC") << PrmSmpl<string>("", " ");
    if ( /*TReqInfo::Instance()->desk.compatible( TRANSIT_CRAFT_VERSION )*/ true ) {
      std::map<int, TRFISC,classcomp> rfiscs;
      places.begin()->GetRFISCs( rfiscs );
      if ( rfiscs.find( point_id ) != rfiscs.end() ) {
        tst();
        params.prms << PrmSmpl<string>("", ElemIdToNameLong( etRateColor, rfiscs[ point_id ].color ));
      }
    }
/*    else {
      params.prms << PrmSmpl<string>("", places.begin()->SeatTariff.rateView())
                  << PrmElem<string>("", etCurrency, places.begin()->SeatTariff.currency_id);
    }*/
    params.prms << PrmSmpl<string>("", ":");
  }


    RowsRef rows;
    SALONS2::TPlace first_in_row;
  // ����� ����� �������� ���� - ���஡㥬 ᤥ���� �� ��� ��ꥤ������ �� �����
    //ᮡ�ࠥ� ���� ��㯯� ����
  for ( TPlaces::iterator ip=places.begin(), priorip=places.begin(); ip!=places.end(); ip++ ) {
    //ProgTrace( TRACE5, "ReferPlacesRow: name=%s, place(%d,%d)=%s", name.c_str(), ip->x, ip->y, string(ip->xname+ip->yname).c_str() );
    if ( ip == priorip ) {
        first_in_row = *ip;
        rows[ ip->y ].xnames += ip->xname;
        rows[ ip->y ].yname = ip->yname;
        continue;
    }
    if ( priorip->y == ip->y/*priorip->x == ip->x - 1*/ ) {
        rows[ ip->y ].xnames += ip->xname;
        rows[ ip->y ].yname = ip->yname;
    }
    else { // � ��� ����� ���騥 ���� �����稫���
        if ( first_in_row.y == ip->y - 1 ) {
            //!logProgTrace( TRACE5, "new row, ip, first_in_row (%d,%d)=%s", ip->x, ip->y, string(ip->xname+ip->yname).c_str() );
            priorip = ip;
            first_in_row = *ip;
            rows[ ip->y ].xnames += ip->xname;
            rows[ ip->y ].yname = ip->yname;
      }
      else { // �६� ᮡ���� ��ப�
        getStrSeats( rows, params, pr_lat );
        //ProgTrace( TRACE5, "str=%s, single=%d", str.c_str(), pr_single_place );
        rows.clear();
            first_in_row = *ip;
            rows[ ip->y ].xnames += ip->xname;
            rows[ ip->y ].yname = ip->yname;
        priorip = ip;
      }
    }
  }
    getStrSeats( rows, params, pr_lat );
}

struct TRP {
    TPlaces places;
    vector<TStringRef> refs;
};

struct TRefPlaces {
    vector<TPlaceList*>::const_iterator salonIter;
    map<string,TRP> mapRef;
};

//use only in salonChangesToText - new version TSalonList
void fillMapChangesLayersSeats( int point_id,
                                const TPlaces::const_iterator &seat1,
                                const TPlaces::const_iterator &seat2,
                                const BitSet<ASTRA::TCompLayerType> &editabeLayers,
                                map<string,TRP> &mapChanges,
                                const string &key_value )
{
  // ࠧ�� ᫮�
  bool pr_find_layer;
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > p1_layers, p2_layers;
  seat1->GetLayers( p1_layers, glAll );
  seat2->GetLayers( p2_layers, glAll );
  for ( std::set<TSeatLayer,SeatLayerCompare>::const_iterator ip1_layer=p1_layers[ point_id ].begin();
        ip1_layer!=p1_layers[ point_id ].end(); ip1_layer++ ) {
    if ( !editabeLayers.isFlag( ip1_layer->layer_type ) ) {
          //!logProgTrace( TRACE5, "ip1_layer->layer_type=%s, not editable", EncodeCompLayerType( ip1_layer->layer_type ) );
            continue;
    }
    pr_find_layer = false;
    for ( std::set<TSeatLayer,SeatLayerCompare>::const_iterator ip2_layer=p2_layers[ point_id ].begin();
          ip2_layer!=p2_layers[ point_id ].end(); ip2_layer++ ) {
      if ( !editabeLayers.isFlag( ip2_layer->layer_type ) ) {
        //!logProgTrace( TRACE5, "ip2_layer->layer_type=%s, not editable", EncodeCompLayerType( ip2_layer->layer_type ) );
        continue;
      }
        if ( ip1_layer->layer_type == ip2_layer->layer_type ) { //�������� �㦭� �ࠢ����� � ��㣨� ��ਡ�⮢ ᫮�??? - � ���饬 ��� ࠧ��⪨ �� ��������
        pr_find_layer = true;
        break;
      }
    }
    if ( !pr_find_layer ) {
      mapChanges[ key_value + string(EncodeCompLayerType(ip1_layer->layer_type)) ].places.push_back( *seat1 );
    }
  }
}

//use only in salonChangesToText - new version TSalonList
void fillMapChangesRemarksSeats( int point_id,
                                 const TPlaces::const_iterator &seat1,
                                 const TPlaces::const_iterator &seat2,
                                 map<string,TRP> &mapChanges,
                                 const string &key_value )
{
  //bool pr_find_rem;
  std::map<int, std::vector<TSeatRemark>,classcomp > remarks1, remarks2;
  seat1->GetRemarks( remarks1 );
  seat2->GetRemarks( remarks2 );
  for ( vector<TSeatRemark>::const_iterator iremark1=remarks1[ point_id ].begin();
        iremark1!=remarks1[ point_id ].end(); iremark1++ ) { // �஡�� �� ���� ६�ઠ�
//    pr_find_rem = false;
    if ( find( remarks2[ point_id ].begin(), remarks2[ point_id ].end(), *iremark1 ) != remarks2[ point_id ].end() ) {  //���� �����
      continue;
    }
    if ( iremark1->pr_denial ) {
      mapChanges[ key_value + "!" + iremark1->value ].places.push_back( *seat1 );
    }
    else {
        mapChanges[ key_value + iremark1->value ].places.push_back( *seat1 );
    }
  }
}

//use only in salonChangesToText - new version TSalonList
void fillMapChangesTariffsSeats( int point_id,
                                 const TPlaces::const_iterator &seat1,
                                 const TPlaces::const_iterator &seat2,
                                 map<string,TRP> &mapChanges,
                                 const string &key_value )
{
  std::map<int, TSeatTariff,classcomp> tariffs1, tariffs2;
  seat1->GetTariffs( tariffs1 );
  seat2->GetTariffs( tariffs2 );
  if ( tariffs1.find( point_id ) != tariffs1.end() &&
       ( tariffs2.find( point_id ) == tariffs1.end() ||
         tariffs1[ point_id ] != tariffs2[ point_id ] ) ) {
    mapChanges[ key_value + tariffs1[ point_id ].str() ].places.push_back( *seat1 );
  }
}

//use only in salonChangesToText - new version TSalonList
void fillMapChangesRFISCsSeats( int point_id,
                                const TPlaces::const_iterator &seat1,
                                const TPlaces::const_iterator &seat2,
                                map<string,TRP> &mapChanges,
                                const string &key_value )
{
  std::map<int, TRFISC,classcomp> rfisc1, rfisc2;
  seat1->GetRFISCs( rfisc1 );
  seat2->GetRFISCs( rfisc2 );
  if ( rfisc1.find( point_id ) != rfisc1.end() &&
       ( rfisc2.find( point_id ) == rfisc2.end() ||
         rfisc1[ point_id ] != rfisc2[ point_id ] ) ) {
    tst();
    mapChanges[ key_value + rfisc1[ point_id ].str() ].places.push_back( *seat1 );
  }
}

//only new version TSalonList
void salonChangesToText( int point_id,
                         const std::vector<TPlaceList*> &oldlist, bool oldpr_craft_lat,
                         const std::vector<TPlaceList*> &newlist, bool newpr_craft_lat,
                         const BitSet<ASTRA::TCompLayerType> &editabeLayers,
                         LEvntPrms &params, bool pr_set_base )
{
    ProgTrace( TRACE5, "salonChangesToText: point_id=%d, placelists.size()=%zu,placelists.size()=%zu",
             point_id, oldlist.size(), newlist.size() );
    typedef vector<string> TVecStrs;
    vector<TRefPlaces> vecChanges;
    map<string,TRP> mapChanges;
    vector<int> salonNums;
  // ���� � ����� ���������� �㦭��� ᠫ���
  ProgTrace( TRACE5, "pr_set_base=%d", pr_set_base );
  TCompareCompsFlags compareFlags;
  compareFlags.setFlag( ccXYVisible );
  compareFlags.setFlag( ccName );
  if ( !pr_set_base ) { //��������� ��������
    for ( vector<TPlaceList*>::const_iterator so=oldlist.begin(); so!=oldlist.end(); so++ ) {
        bool pr_find_salon=false;
        for ( vector<TPlaceList*>::const_iterator sn=newlist.begin(); sn!=newlist.end(); sn++ ) {
        pr_find_salon = EqualSalon( *so, *sn, compareFlags );
          //!logProgTrace( TRACE5, "so->num=%d, pr_find_salon=%d", (*so)->num, pr_find_salon );
          if ( !pr_find_salon )
              continue;
          salonNums.push_back( (*sn)->num );
          // �� �㦭� ᠫ��
        for ( TPlaces::iterator po = (*so)->places.begin(), // ����� �� ���⠬
                                pn = (*sn)->places.begin();
                                po != (*so)->places.end(),
                                pn != (*sn)->places.end();
                                po++, pn++ ) {
          if ( ( pn->visible && !pn->visible ) || ( pn->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) ) )
            mapChanges[ "ADD_SEATS" + pn->clname + pn->elem_type ].places.push_back( *pn );
          if ( ( po->visible && !pn->visible ) || ( po->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) ) ) {
              mapChanges[ "DEL_SEATS" + po->clname + po->elem_type ].places.push_back( *po );
              // �� ���� ����� ������� ���ଠ樨 � ����
              continue;
            }
            TCompareCompsFlags compareFlags;
            compareFlags.setFlag( ccRemarks );
            if ( po->isChange( *pn, compareFlags ) ) { // ࠧ�� ६�ન
              fillMapChangesRemarksSeats( point_id, po, pn, mapChanges, "DEL_REMS" );
              fillMapChangesRemarksSeats( point_id, pn, po, mapChanges, "ADD_REMS" );
          }
          compareFlags.clearFlags();
          compareFlags.setFlag( ccLayers );
          if ( po->isChange( *pn, compareFlags ) ) {
            fillMapChangesLayersSeats( point_id, po, pn, editabeLayers, mapChanges, "DEL_LAYERS" );
            fillMapChangesLayersSeats( point_id, pn, po, editabeLayers, mapChanges, "ADD_LAYERS" );
          }
          compareFlags.clearFlags();
          compareFlags.setFlag( ccTariffs );
          if ( po->isChange( *pn, compareFlags ) ) {
            fillMapChangesTariffsSeats( point_id, po, pn, mapChanges, "DEL_WEB_TARIFF" );
            fillMapChangesTariffsSeats( point_id, pn, po, mapChanges, "ADD_WEB_TARIFF" );
          }
          compareFlags.clearFlags();
          compareFlags.setFlag( ccRFISC );
          if ( po->isChange( *pn, compareFlags ) ) {
            tst();
            fillMapChangesRFISCsSeats( point_id, po, pn, mapChanges, "DEL_RFISC" );
            fillMapChangesRFISCsSeats( point_id, pn, po, mapChanges, "ADD_RFISC" );
          }
        } // end for places
        break;
      } // end for NewSalons
      if ( !pr_find_salon ) {
        // �� ��諨 ᠫ�� - ��⠥� �� 㤠��� ��� � �������� �������� ����
        for ( TPlaces::iterator po = (*so)->places.begin(); po != (*so)->places.end(); po++ ) {
          if ( po->visible )
            mapChanges[ "DEL_SALON"+IntToString((*so)->num+1) ].places.push_back( *po ); //+���ᠭ�� ᠫ���
            }
      }

      TRefPlaces refp;
      refp.salonIter = so;
      refp.mapRef = mapChanges;
      vecChanges.push_back( refp );
      mapChanges.clear();
    } // end for OldSalon
  } // �� ���� �뤠���� ��������� �᫨ �����稫� ������� ����������
  mapChanges.clear();
  // ���� �������� �� ᠫ��� �� NewSalons, ����� �� ��諨�� � OldSalons
  TPlaces oldseats;
  oldseats.push_back( TPlace() );
  TPlaces::const_iterator old_seat = oldseats.begin();
  //!logProgTrace( TRACE5, "NewSalons->placelists.size()=%zu", newlist.size() );
  for ( vector<TPlaceList*>::const_iterator sn=newlist.begin(); sn!=newlist.end(); sn++ ) {
    //!logProgTrace( TRACE5, "(*sn)->num=%d", (*sn)->num );
    if ( find( salonNums.begin(), salonNums.end(), (*sn)->num ) != salonNums.end() ) {
      continue; // ��� ᠫ�� 㦥 ���ᠭ
    }
    bool pr_equal_salon_and_seats=true;
    string clname, elem_type;
      for ( TPlaces::iterator pn = (*sn)->places.begin(); pn != (*sn)->places.end(); pn++ ) {
        if ( pn->visible ) {
            if ( clname.empty() ) {
                clname = pn->clname;
                elem_type = pn->elem_type;
                //!logProgTrace( TRACE5, "clname=%s, elem_type=%s", clname.c_str(), elem_type.c_str() );
            }
            mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.push_back( *pn ); //+���ᠭ�� ᠫ���
            mapChanges[ "ADD_SEATS" + pn->clname + pn->elem_type ].places.push_back( *pn );
            if ( clname != pn->clname ||
                   elem_type != pn->elem_type ) {
            pr_equal_salon_and_seats = false;
          }
          fillMapChangesRemarksSeats( point_id, pn, old_seat, mapChanges, "ADD_REMS" );
          fillMapChangesLayersSeats( point_id, pn, old_seat, editabeLayers, mapChanges, "ADD_LAYERS" );
          fillMapChangesTariffsSeats( point_id, pn, old_seat, mapChanges, "ADD_WEB_TARIFF" );
          fillMapChangesRFISCsSeats( point_id, pn, old_seat, mapChanges, "ADD_RFISC" );
          }
        }
        if ( pr_equal_salon_and_seats && !mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.empty() ) {
            mapChanges[ "ADD_COMMON_SALON_REF"+IntToString((*sn)->num+1) ].places.assign( mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.begin(),
                                                                                          mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.end() );
            mapChanges.erase( "ADD_SALON"+IntToString((*sn)->num+1) );
            mapChanges.erase( "ADD_SEATS" + clname + elem_type );
        }
      TRefPlaces refp;
      refp.salonIter = sn;
      refp.mapRef = mapChanges;
      vecChanges.push_back( refp );
      mapChanges.clear();
  }
  // ����� ���ᨢ �������� � ���⠬� � ᠫ�����
  //����室��� ���஢��� �� ᠫ���� � ����⢨�
  bool pr_lat;
    for ( int i=0; i<=1; i++ ) {
      for ( int j=0; j<6; j++ ) {
        // �஡�� �� ᠫ����
        for ( vector<TRefPlaces>::iterator iref=vecChanges.begin(); iref!=vecChanges.end(); iref++ ) {
          // �஡�� �� ���������
          for ( map<string,TRP>::iterator im=iref->mapRef.begin(); im!=iref->mapRef.end(); im++ ) {
            // ���砫� �� 㤠����� ᢮��⢠
            if ( im->second.places.empty() )
                continue;
            if ( ( i == 0 && im->first.find( "DEL" ) == string::npos ) ||
                 ( i == 1 && im->first.find( "DEL" ) != string::npos ) )
                continue;
            ProgTrace(TRACE5, "i=%d, j=%d, %s", i, j, im->first.c_str() );
            if ( j == 0 && im->first.find( "SALON" ) == string::npos ) continue;
            if ( j == 1 && im->first.find( "SEATS" ) == string::npos ) continue;
            if ( j == 2 && im->first.find( "LAYERS" ) == string::npos ) continue;
            if ( j == 3 && im->first.find( "REMS" ) == string::npos ) continue;
            if ( j == 4 && im->first.find( "WEB_TARIFF" ) == string::npos ) continue;
            if ( j == 5 && im->first.find( "RFISC" ) == string::npos ) continue;
            if ( i == 0 )
                pr_lat = oldpr_craft_lat;
            else
                pr_lat = newpr_craft_lat;
            PrmEnum salon("salon", "");
            ReferPlaces( point_id, im->first, im->second.places, salon, pr_lat );
            params << salon;
          }
        }
    }
  }
  // ��� �뫮 �� �஠������஢��� �� ᮢ������� ���� �� ��᪮�쪨� ����������/㤠����� �-���
}

void getXYName( int point_id, std::string seat_no, std::string &xname, std::string &yname )
{
    xname.clear();
    yname.clear();
    //!!! ࠡ�� �� �� �����ᠬ!!!
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT xname, yname FROM trip_comp_elems "
      " WHERE point_id=:point_id AND "
      "       (salons.denormalize_yname(yname,NULL)||salons.denormalize_xname(xname,0)=:seat_no OR "
      "        salons.denormalize_yname(yname,NULL)||salons.denormalize_xname(xname,1)=:seat_no)";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "seat_no", otString, seat_no );
    Qry.Execute();
    if ( !Qry.Eof ) {
        xname = Qry.FieldAsString( "xname" );
        yname = Qry.FieldAsString( "yname" );
    }
}

void getLayerPlacesCompSection( const TSalonList &salonList,
                                TCompSection &compSection,
                                map<ASTRA::TCompLayerType, TPlaces> &uselayers_places,
                                int &seats_count )
{
  seats_count = 0;
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    uselayers_places[ il->first ].clear();
  }
  int Idx = 0;
  for ( vector<TPlaceList*>::const_iterator si=salonList.begin(); si!=salonList.end(); si++ ) {
    for ( int y=0; y<(*si)->GetYsCount(); y++ ) {
      if ( compSection.inSection( Idx ) ) { // ����� ᥪ樨   !!!����
        for ( int x=0; x<(*si)->GetXsCount(); x++ ) {
          TPlace *seat = (*si)->place( (*si)->GetPlaceIndex( x, y ) );
          if ( !seat->isplace || !seat->visible ) {
             continue;
          }
          seats_count++;
          for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
            if ( seat->getCurrLayer( salonList.getDepartureId() ).layer_type == il->first ) {
              uselayers_places[ il->first ].push_back( *seat );
              break;
            }
          }
        }
      }
      Idx++;
    }
  }
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    //!logProgTrace( TRACE5, "getPaxPlacesCompSection: layer_type=%s, count=%zu", EncodeCompLayerType(il->first), il->second.size() );
  }
}

void getLayerPlacesCompSection( SALONS2::TSalons &NSalons, TCompSection &compSection,
                                bool only_high_layer,
                                map<ASTRA::TCompLayerType, TPlaces> &uselayers_places,
                                int &seats_count )
{
  seats_count = 0;
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    uselayers_places[ il->first ].clear();
  }
  int Idx = 0;
  for ( vector<TPlaceList*>::iterator si=NSalons.placelists.begin(); si!=NSalons.placelists.end(); si++ ) {
    for ( int y=0; y<(*si)->GetYsCount(); y++ ) {
      if ( compSection.inSection( Idx ) ) { // ����� ᥪ樨 !!!����
        for ( int x=0; x<(*si)->GetXsCount(); x++ ) {
         TPlace *p = (*si)->place( (*si)->GetPlaceIndex( x, y ) );
         if ( !p->isplace || !p->visible )
           continue;
         seats_count++;
         for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
           if ( ( only_high_layer && !p->layers.empty() && p->layers.begin()->layer_type == il->first ) || // !!!������ ᠬ� �ਮ���� ᫮�
                ( !only_high_layer && p->isLayer( il->first ) ) ) {
             uselayers_places[ il->first ].push_back( *p );
             break;
           }
         }
        }
      }
      Idx++;
    }
  }
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    //!logProgTrace( TRACE5, "getPaxPlacesCompSection: layer_type=%s, count=%zu", EncodeCompLayerType(il->first), il->second.size() );
  }
}

bool filterComponsForView( const string &airline, const string &airp )
{
  if ((airline.empty() && airp.empty()) ||
      (!airline.empty() && !airp.empty())) return false;  //���������� ����� �ਭ�������� ���� �����, ���� ��������

  TAccess access=TReqInfo::Instance()->user.access;
  if (!airline.empty())
    access.merge_airlines(TAccessElems<string>(airline, true));
  if (!airp.empty())
    access.merge_airps(TAccessElems<string>(airp, true));
  if (access.totally_not_permitted()) return false;
  if (TReqInfo::Instance()->user.user_type==utAirline && !airp.empty()) return false;
  return true;
}

bool filterComponsForEdit( const string &airline, const string &airp )
{
  if (!filterComponsForView(airline, airp)) return false;
  if (TReqInfo::Instance()->user.user_type==utAirport && !airline.empty()) return false;
  return true;
}

void TComponSets::CheckAirlAirp( xmlNodePtr reqNode, string &airline, string &airp )
{
  TReqInfo *r = TReqInfo::Instance();
  TElemFmt fmt;

  airline.clear();
  airp.clear();

  xmlNodePtr a = GetNode( "airline", reqNode );
  if ( a ) {
    airline = ElemToElemId( etAirline, NodeAsString( a ), fmt );
    if ( fmt == efmtUnknown )
      throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_INPUT" );
  }

  a = GetNode( "airp", reqNode );
  if ( a ) {
    airp = ElemToElemId( etAirp, NodeAsString( a ), fmt );
    if ( fmt == efmtUnknown )
      throw AstraLocale::UserException( "MSG.AIRP.INVALID_SET_CODE" );
  }

  if (airline.empty() && airp.empty())
  {
    switch (r->user.user_type)
    {
      case utAirline:
        if (r->user.access.airlines().only_single_permit())
          airline=*(r->user.access.airlines().elems().begin());
        break;
      case utAirport:
        if (r->user.access.airps().only_single_permit())
          airp=*(r->user.access.airps().elems().begin());
        break;
      case utSupport:
        if (r->user.access.airlines().only_single_permit() && !r->user.access.airps().only_single_permit())
          airline=*(r->user.access.airlines().elems().begin());
        if (r->user.access.airps().only_single_permit() && !r->user.access.airlines().only_single_permit())
          airp=*(r->user.access.airps().elems().begin());
        break;
    };
  }

  if (airline.empty() && airp.empty())
  {
    switch (r->user.user_type)
    {
      case utAirline:
        throw AstraLocale::UserException( "MSG.AIRLINE.UNDEFINED" );
      case utAirport:
        throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.NOT_SET_AIRP" );
      case utSupport:
        throw AstraLocale::UserException( "MSG.AIRLINE_OR_AIRP_MUST_BE_SET" );
    };
  }
  if (!airline.empty() && !airp.empty())
    throw AstraLocale::UserException( "MSG.NOT_SET_ONE_TIME_AIRLINE_AND_AIRP" ); // ��⮬� �� ���������� �ਭ������� ���� ������������, ���� �����

  if (!filterComponsForEdit(airline, airp))
  {
    if (!airline.empty())
      throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRLINE" );
    if (!airp.empty())
      throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRP" );
  }
}

void TComponSets::Parse( xmlNodePtr reqNode )
{
  Clear();
  string smodify = NodeAsString( "modify", reqNode );
  if ( smodify == "delete" )
    modify = SALONS2::mDelete;
  else
    if ( smodify == "add" )
      modify = SALONS2::mAdd;
    else
      if ( smodify == "change" )
        modify = SALONS2::mChange;

  if (modify != SALONS2::mDelete)
    TComponSets::CheckAirlAirp(reqNode, airline, airp);

  TElemFmt fmt;
  craft = NodeAsString( "craft", reqNode );
  if ( craft.empty() )
    throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
  craft = ElemToElemId( etCraft, craft, fmt );
  if ( fmt == efmtUnknown )
    throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
  bort = NodeAsString( "bort", reqNode );
  descr = NodeAsString( "descr", reqNode );
  string vclasses = NodeAsString( "classes", reqNode );
  classes = RTrimString( vclasses );
}


bool compareSeatLayer( const TSeatLayer &layer1, const TSeatLayer &layer2 )
{
  BASIC_SALONS::TCompLayerTypes *compTypes = BASIC_SALONS::TCompLayerTypes::Instance();
  if ( layer1.point_dep_num != layer2.point_dep_num ) {
    bool ret;
    if ( layer1.point_dep_num == pdPrior ) {
      ret = compTypes->priority_on_routes( layer1.layer_type, layer2.layer_type, SIGND( layer1.time_create - layer2.time_create ) );
    }
    else {
      ret = !compTypes->priority_on_routes( layer2.layer_type, layer1.layer_type, SIGND( layer2.time_create - layer1.time_create ) );
    }
    //!logProgTrace( TRACE5, "compareSeatLayer: layer1.point_dep_num=%d, layer1.point_dep_num=%d, ret=%d",
    //!log           layer1.point_dep_num, layer2.point_dep_num, ret );
    //!logProgTrace( TRACE5, "compareSeatLayer: return layer1(point_id=%d, point_dep=%d, point_arv=%d, layer_type=%s, pax_id=%d, crs_pax_id=%d, time_create=%s",
    //!log           layer1.point_id, layer1.point_dep, layer1.point_arv, EncodeCompLayerType( layer1.layer_type ),
    //!log           layer1.pax_id, layer1.crs_pax_id, DateTimeToStr( layer1.time_create ).c_str() );
    //!logProgTrace( TRACE5, "compareSeatLayer: return layer2(point_id=%d, point_dep=%d, point_arv=%d, layer_type=%s, pax_id=%d, crs_pax_id=%d, time_create=%s",
    //!log           layer2.point_id, layer2.point_dep, layer2.point_arv, EncodeCompLayerType( layer2.layer_type ),
    //!log           layer2.pax_id, layer2.crs_pax_id, DateTimeToStr( layer2.time_create ).c_str() );
    return ret;
  }
  if ( compTypes->priority( layer1.layer_type ) < compTypes->priority( layer2.layer_type ) ) {
    return true;
  };
  if ( compTypes->priority( layer1.layer_type ) > compTypes->priority( layer2.layer_type ) ) {
    return false;
  }
  if ( compTypes->priority( layer1.layer_type ) == compTypes->priority( layer2.layer_type ) ) {
    if ( layer1.time_create < layer2.time_create ) {
      return true;
    }
    if ( layer1.time_create > layer2.time_create ) {
      return false;
    }
  }
  if ( layer1.getPaxId() != layer2.getPaxId() ) {
    return ( layer1.getPaxId() < layer2.getPaxId() );
  }
  return false;
};
//�����

int getCrsPaxPointArv( int crs_pax_id, int point_id_spp )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airp_arv, point_id "
    " FROM crs_pax, crs_pnr "
    " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
    "       crs_pax.pnr_id=crs_pnr.pnr_id";
  Qry.CreateVariable( "pax_id", otInteger, crs_pax_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    return ASTRA::NoExists; //???
  }
  int point_id_tlg = Qry.FieldAsInteger( "point_id" );
  string airp_arv = Qry.FieldAsString( "airp_arv" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT points.point_id, point_num, first_point, pr_tranzit "
    " FROM tlg_binding, points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      tlg_binding.point_id_tlg=:point_id_tlg AND "
    "      point_id_spp=:point_id_spp";
  Qry.CreateVariable( "point_id_tlg", otInteger, point_id_tlg );
  Qry.CreateVariable( "point_id_spp", otInteger, point_id_spp );
  Qry.Execute();
  if ( Qry.Eof ) {
    return ASTRA::NoExists; //???
  }
  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       Qry.FieldAsInteger( "point_id" ),
                       Qry.FieldAsInteger("point_num" ),
                       Qry.FieldIsNULL( "first_point" )?NoExists:Qry.FieldAsInteger( "first_point" ),
                       Qry.FieldAsInteger( "pr_tranzit" ) != 0,
                       trtNotCurrent, trtWithCancelled );
  for( TTripRoute::const_iterator iroute=route.begin(); iroute!=route.end(); iroute++ ) { //横� �� ��������
    if ( iroute->airp == airp_arv ) {
      //!logProgTrace( TRACE5, "getCrsPaxPointArv: crs_pax_id=%d, point_id_spp=%d, return %d",
      //!log           crs_pax_id, point_id_spp, iroute->point_id );
      return iroute->point_id;
    }
  }
  return ASTRA::NoExists; //???
}

void DeleteSalons( int point_id )
{
  TFlights flights;
    flights.Get( point_id, ftTranzit );
    flights.Lock(__FUNCTION__);
    TQuery Qry( &OraSession );
  Qry.SQLText =
    "BEGIN "
    " UPDATE trip_sets SET comp_id=NULL WHERE point_id=:point_id; "
    " DELETE trip_comp_rem WHERE point_id=:point_id; "
    " DELETE trip_comp_baselayers WHERE point_id=:point_id; "
    " DELETE trip_comp_rates WHERE point_id=:point_id; "
    " DELETE trip_comp_rfisc WHERE point_id=:point_id; "
    " DELETE trip_comp_elems WHERE point_id=:point_id; "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  setTRIP_CLASSES( point_id );
}

bool isEmptySalons( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return Qry.Eof;
}

bool isFreeSeating( int point_id )
{
  return TTripSetList().fromDB(point_id).value(tsFreeSeating, false);
}

bool isTranzitSalons( int point_id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_new, "
    "       DECODE( tranzit_algo_seats.airp, NULL, 0, 4 ) + "
    "       DECODE( tranzit_algo_seats.airline, NULL, 0, 2 ) + "
    "       DECODE( tranzit_algo_seats.flt_no, NULL, 0, 1 ) AS priority "
    " FROM tranzit_algo_seats, points "
    " WHERE point_id=:point_id AND "
    "       ( tranzit_algo_seats.airp IS NULL OR tranzit_algo_seats.airp=points.airp ) AND "
    "       ( tranzit_algo_seats.airline IS NULL OR tranzit_algo_seats.airline=points.airline ) AND "
    "       ( tranzit_algo_seats.flt_no IS NULL OR tranzit_algo_seats.flt_no=points.flt_no ) "
    " ORDER BY priority DESC";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "pr_new" ) != 0 );
}

void TSalonPax::int_get_seats( TWaitListReason &waitListReason,
                               vector<TPlace*> &seats ) const {
  waitListReason = TWaitListReason();
  seats.clear();
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  //!logProgTrace( TRACE5, "grp_status=%s, pax_id=%d", grp_status.c_str(), pax_id );
  if ( DecodePaxStatus(grp_status.c_str()) == psCrew )
    throw EXCEPTIONS::Exception("TSalonPax::get_seats: DecodePaxStatus(grp_status) == psCrew");
  const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status );
  ASTRA::TCompLayerType grp_layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
  TLayersPax::const_iterator ilayer=layers.begin();
  for ( ; ilayer!=layers.end(); ilayer++ ) {
    if ( ilayer->first.layer_type == grp_layer_type ) {
      //!logProgTrace( TRACE5, "pax_id=%d, %s, grp_layer_type=%s, ilayer->second.layerType is valid %d",
      //!log           pax_id, ilayer->first.toString().c_str(),
      //!log           EncodeCompLayerType( grp_layer_type ),
      //!log           ilayer->second.waitListReason.layerStatus == layerValid );
      break;
    }
  }
  if ( ilayer != layers.end() ) {
    waitListReason = ilayer->second.waitListReason;
    if ( ilayer->second.waitListReason.layerStatus == layerValid ) { //��諨 ᫮�
      waitListReason.layer = ilayer->first; //�����頥� ������� ᫮�
      for ( std::set<TPlace*,CompareSeats>::const_iterator iseat=ilayer->second.seats.begin();
            iseat!=ilayer->second.seats.end(); iseat++ ) {
        seats.push_back( *iseat );
      }
    }
  }
}


void TSalonPax::get_seats( TWaitListReason &waitListReason,
                           TPassSeats &ranges ) const {
  ranges.clear();
  vector<TPlace*> seats;
  int_get_seats( waitListReason, seats );
  for ( std::vector<TPlace*>::const_iterator iseat=seats.begin();
        iseat!=seats.end(); iseat++ ) {
    ranges.insert( TSeat( (*iseat)->yname, (*iseat)->xname ) );
  }
}

std::string TSalonPax::seat_no( const std::string &format, bool pr_lat_seat, TWaitListReason &waitListReason ) const
{
  if ( is_jmp ) {
    tst();
    return "JMP";
  }
  TPassSeats seats;
  TSeatRanges ranges;
  get_seats( waitListReason, seats );
  for ( TPassSeats::const_iterator ipass_seat=seats.begin(); ipass_seat!=seats.end(); ipass_seat++ ) {
    ranges.push_back( TSeatRange( *ipass_seat, *ipass_seat ) );
  }
  return GetSeatRangeView(ranges, format, pr_lat_seat);
}

std::string TSalonPax::event_seat_no( bool pr_lat_seat, int point_dep, TWaitListReason &waitListReason, LEvntPrms &evntPrms) const
{
  evntPrms.clear();
  TSeatRanges ranges;
  vector<TPlace*> seats;
  int_get_seats( waitListReason, seats );
  ostringstream res;
  for ( std::vector<TPlace*>::const_iterator iseat=seats.begin();
        iseat!=seats.end(); iseat++ ) {
    ranges.clear();
    ranges.push_back( TSeatRange(  TSeat( (*iseat)->yname, (*iseat)->xname ),  TSeat( (*iseat)->yname, (*iseat)->xname ) ) );
    string seat_view=GetSeatRangeView(ranges, "list", pr_lat_seat);

    if (iseat!=seats.begin())
    {
      res << " ";
      evntPrms << PrmSmpl<string>("", " ");
    }
    res << seat_view;
    evntPrms << PrmSmpl<string>("", seat_view);

    TSeatTariffMap passTariffs;
    TRFISC rfisc;
    passTariffs.get( pax_id );
    if ( passTariffs.status() == TSeatTariffMap::stUseRFISC ) {
      SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
      SelfCkinSalonTariff.setTariffMap( point_dep, passTariffs );
      (*iseat)->SetRFISC( point_dep, passTariffs );
      std::map<int, TRFISC,classcomp> vrfiscs;
      (*iseat)->GetRFISCs( vrfiscs );
      if ( vrfiscs.find( point_dep ) != vrfiscs.end() ) {
        rfisc = vrfiscs[ point_dep ];
      }
    }
    else { //���� ०�� ࠡ��� ???
      (*iseat)->convertSeatTariffs( point_dep );
      rfisc.color = (*iseat)->SeatTariff.color;
      rfisc.rate = (*iseat)->SeatTariff.rate;
      rfisc.currency_id = (*iseat)->SeatTariff.currency_id;
    }
    //(*iseat)->SetRFICSRemarkByColor( point_dep, passTariffs );

    if ( !rfisc.empty() )
    {
      evntPrms << PrmSmpl<string>("", "(");
      if ( !rfisc.code.empty() )
        evntPrms << PrmSmpl<string>("", rfisc.code);
      if ( !rfisc.code.empty() )
        evntPrms << PrmSmpl<string>("", "/");

      if (passTariffs.status()==TSeatTariffMap::stUseRFISC ||
          passTariffs.status()==TSeatTariffMap::stNotRFISC)
        evntPrms << PrmSmpl<string>("", rfisc.rateView())
                 << PrmElem<string>("", etCurrency, rfisc.currency_id);
      else
        evntPrms << PrmLexema("", "EVT.UNKNOWN_RATE");
      evntPrms << PrmSmpl<string>("", ")");
    }
  }
  return res.str();
}

std::string TSalonPax::prior_seat_no( const std::string &format, bool pr_lat_seat ) const
{
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  //!logProgTrace( TRACE5, "grp_status=%s, pax_id=%d", grp_status.c_str(), pax_id );
  if ( DecodePaxStatus(grp_status.c_str()) == psCrew )
    throw EXCEPTIONS::Exception("TSalonPax::prior_seat_no: DecodePaxStatus(grp_status) == psCrew");
  const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status );
  ASTRA::TCompLayerType grp_layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
  string res;
  if ( /*res.empty() &&*/
       !invalid_ranges.empty() ) { //᫮�� ���, ᬮ�ਬ invalid_ranges
//!log     tst();
    TSeatRanges ranges;
    for ( std::map<TSeatLayer,TInvalidRange,SeatLayerCompare>::const_iterator iranges=invalid_ranges.begin();
          iranges!=invalid_ranges.end(); iranges++ ) {
      if ( iranges->first.layer_type != grp_layer_type ) {
        continue;
      }
      for ( TInvalidRange::const_iterator irange=iranges->second.begin();
            irange!=iranges->second.end(); irange++ ) {
        ranges.push_back( *irange );
      }
    }
    res = GetSeatRangeView(ranges, format, pr_lat_seat);
  }
  return res;
}

bool _TSalonPassengers::isWaitList( )
{
  if ( status_wait_list == wlNotInit ) {
    status_wait_list = wlNo;
    TPassSeats seats;
    //route
    for ( _TSalonPassengers::iterator iroute=begin(); iroute!=end(); iroute++ ) {
    //class
      for ( TIntClassSalonPassengers::iterator iclass=iroute->second.begin();
            iclass!=iroute->second.end(); iclass++ ) {
        //grp_status
        for ( TIntStatusSalonPassengers::iterator igrp_layer=iclass->second.begin();
              igrp_layer!=iclass->second.end(); igrp_layer++ ) {
          for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=igrp_layer->second.begin();
                ipass!=igrp_layer->second.end(); ipass++ ) {
            TWaitListReason waitListReason;
            ipass->get_seats( waitListReason, seats );
            if ( !ipass->is_jmp && waitListReason.layerStatus != layerValid ) {
              status_wait_list = wlYes;
              break;
            }
          }
        }
      }
    }
  }
  return ( status_wait_list == wlYes );
}

bool _TSalonPassengers::BuildWaitList( xmlNodePtr dataNode )
{
  ProgTrace( TRACE5, "TSalonPassengers::BuildWaitList: point_dep=%d, pr_craft_lat=%d",
             point_dep, pr_craft_lat );
  status_wait_list = wlNo;
  bool createDefaults = false;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline "
    " FROM points "
    " WHERE points.point_id=:point_id AND points.pr_del!=-1 AND points.pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.Execute();
  if ( Qry.Eof )
    throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  SEATS2::TSublsRems subcls_rems( string(Qry.FieldAsString( "airline" )) );
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  TClsGrp &cls_grp = (TClsGrp &)base_tables.get("CLS_GRP");    //cls_grp.code subclass,
  SEATS2::TDefaults def;

  TQuery RemsQry( &OraSession );
  RemsQry.SQLText =
    "SELECT rem, rem_code, comp_rem_types.pr_comp "
    " FROM pax_rem, comp_rem_types "
    "WHERE pax_rem.pax_id=:pax_id AND "
    "      rem_code=comp_rem_types.code(+) "
    " ORDER BY pr_comp, code ";
  RemsQry.DeclareVariable( "pax_id", otInteger );

  Qry.Clear();
  Qry.SQLText =
    "SELECT ticket_no, wl_type, tid, "
    "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:rnum) AS bag_weight, "
    "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:rnum) AS bag_amount, "
    "       ckin.get_excess(pax.grp_id,pax.pax_id) AS excess, "
    "       tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no "
    "FROM pax, tckin_pax_grp "
    "WHERE pax_id=:pax_id AND "
    "      pax.grp_id=tckin_pax_grp.grp_id(+)";
  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.DeclareVariable( "rnum", otInteger );
  int rownum = 0;
  TCkinRoute tckin_route;
  xmlNodePtr passengersNode = NULL;
  xmlNodePtr layerNode;
  //std::map<int,std::map<std::string,map<string,std::set<TSalonPax,ComparePassenger>,CompareGrpStatus >,CompareClass >,CompareArv > {

  std::map<std::string,set<TSalonPax,ComparePassenger>,CompareGrpStatus> salonGrpStatusPaxs;
  //point_arv
  for ( _TSalonPassengers::iterator iroute=begin(); iroute!=end(); iroute++ ) {
    //class
    for ( TIntClassSalonPassengers::iterator iclass=iroute->second.begin();
          iclass!=iroute->second.end(); iclass++ ) {
      //grp_status
      for ( TIntStatusSalonPassengers::iterator igrp_layer=iclass->second.begin();
            igrp_layer!=iclass->second.end(); igrp_layer++ ) {
        //pass.grp+reg_no
        salonGrpStatusPaxs[ igrp_layer->first ].insert( igrp_layer->second.begin(), igrp_layer->second.end() );
      }
    }
  }
  //grp_status
  for ( map<string,std::set<TSalonPax,ComparePassenger>,CompareGrpStatus >::iterator igrp_layer=salonGrpStatusPaxs.begin();
        igrp_layer!=salonGrpStatusPaxs.end(); igrp_layer++ ) {
    layerNode = NULL;
    const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", igrp_layer->first );
    //!logProgTrace( TRACE5, "igrp_layer=%s, igrp_layer->second.size()=%zu", igrp_layer->first.c_str(), igrp_layer->second.size() );
    if ( DecodePaxStatus(igrp_layer->first.c_str()) == psCrew )
      throw EXCEPTIONS::Exception("TSalonPassengers::BuildWaitList: DecodePaxStatus(igrp_layer->first) == psCrew");
    for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=igrp_layer->second.begin();
          ipass!=igrp_layer->second.end(); ipass++ ) {
      if ( passengersNode == NULL ) {
        passengersNode = NewTextChild( dataNode, "passengers" );
      }
      if ( layerNode == NULL ) {
        layerNode = NewTextChild( passengersNode, "layer_type", grp_status_row.layer_type );
        SetProp( layerNode, "name", grp_status_row.AsString( "name" ) );
      }
      //!logProgTrace( TRACE5, "ipax->pax_id=%d, rownum=%d", ipass->pax_id, rownum );
      xmlNodePtr passNode = NewTextChild( layerNode, "pass" );
      rownum++;
      createDefaults = true;
      Qry.SetVariable( "pax_id", ipass->pax_id );
      Qry.SetVariable( "rnum", rownum );
      Qry.Execute();
      NewTextChild( passNode, "grp_id", ipass->grp_id );
      NewTextChild( passNode, "pax_id", ipass->pax_id );
      NewTextChild( passNode, "clname", ipass->cl, def.clname );
      NewTextChild( passNode, "grp_layer_type",
                    grp_status_row.layer_type,
                    EncodeCompLayerType( def.grp_status ) );
      NewTextChild( passNode, "pers_type",
                    ElemIdToCodeNative(etPersType, ipass->pers_type),
                    ElemIdToCodeNative(etPersType, def.pers_type) );
      NewTextChild( passNode, "reg_no", ipass->reg_no );
      string name = ipass->surname;
      NewTextChild( passNode, "name", TrimString( name ) + string(" ") + ipass->name );
      TWaitListReason waitListReason;
      string seat_no = ipass->seat_no( "list", pr_craft_lat, waitListReason );
      if ( seat_no.empty() ) {
        if ( Qry.FieldIsNULL( "wl_type" ) ) {
          seat_no = string("(") + ipass->prior_seat_no( "seats", pr_craft_lat ) + string(")");
        }
        else {
          seat_no = AstraLocale::getLocaleText("��");
        }
      }
      NewTextChild( passNode, "seat_no", seat_no, def.placeName );
      if ( !ipass->is_jmp && waitListReason.layerStatus != layerValid && status_wait_list == wlNo ) {
        status_wait_list = wlYes; //���� ��
      }
      NewTextChild( passNode, "wl_type", Qry.FieldAsString( "wl_type" ), def.wl_type );
      NewTextChild( passNode, "seats", ipass->seats, def.countPlace );
      NewTextChild( passNode, "tid", Qry.FieldAsInteger( "tid" ) );
      NewTextChild( passNode, "isseat", (int)waitListReason.layerStatus == layerValid || ipass->is_jmp, (int)def.isSeat );
      NewTextChild( passNode, "ticket_no", Qry.FieldAsString( "ticket_no" ), def.ticket_no );
      NewTextChild( passNode, "document",
                    CheckIn::GetPaxDocStr(NoExists, ipass->pax_id, true),
                    def.document );
      NewTextChild( passNode, "bag_weight", Qry.FieldAsInteger( "bag_weight" ), def.bag_weight );
      NewTextChild( passNode, "bag_amount", Qry.FieldAsInteger( "bag_amount" ), def.bag_amount );
      NewTextChild( passNode, "excess", Qry.FieldAsInteger( "excess" ), def.excess );
      ostringstream trip;
      if ( !Qry.FieldIsNULL("tckin_id") ) {
        TCkinRouteItem priorSeg;
        tckin_route.GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                                Qry.FieldAsInteger("seg_no"),
                                crtIgnoreDependent,
                                priorSeg);
        if (priorSeg.grp_id!=NoExists)
        {
          TDateTime local_scd_out = UTCToClient(priorSeg.operFlt.scd_out,AirpTZRegion(priorSeg.operFlt.airp));
            trip << ElemIdToElemCtxt( ecDisp, etAirline, priorSeg.operFlt.airline, priorSeg.operFlt.airline_fmt )
                 << setw(3) << setfill('0') << priorSeg.operFlt.flt_no
                 << ElemIdToElemCtxt( ecDisp, etSuffix, priorSeg.operFlt.suffix, priorSeg.operFlt.suffix_fmt )
                 << "/" << DateTimeToStr( local_scd_out, "dd" );
        }
      }
      NewTextChild( passNode, "trip_from", trip.str(), def.trip_from );
      string comp_rem, pass_rem;
      bool pr_down = false;
      RemsQry.SetVariable( "pax_id", ipass->pax_id );
      RemsQry.Execute();
      for( ; !RemsQry.Eof; RemsQry.Next() ) {
        if ( !RemsQry.FieldIsNULL( "pr_comp" ) ) {
          comp_rem += string(RemsQry.FieldAsString( "rem_code" )) + " ";
        }
        pass_rem += string( ".R/" ) + RemsQry.FieldAsString( "rem" ) + "   ";
        if ( string(RemsQry.FieldAsString( "rem_code" )) == "STCR" ) {
          pr_down = true;
        }
      }
      string rem;
      const TBaseTableRow &row=cls_grp.get_row( "id", ipass->class_grp );
      if ( subcls_rems.IsSubClsRem( row.AsString( "code" ), rem ) ) {
        comp_rem += rem;
      }
      //!logProgTrace( TRACE5, "pax_id=%d, comp_rem=%s, pass_rem=%s",
      //!log           ipass->pax_id, comp_rem.c_str(), pass_rem.c_str() );
      NewTextChild( passNode, "comp_rem", TrimString( comp_rem ), def.comp_rem );
      NewTextChild( passNode, "pr_down", (int)pr_down, (int)def.pr_down );
      NewTextChild( passNode, "pass_rem", TrimString( pass_rem ), def.pass_rem );
    } //end pass
  } //end grp_status
  if (createDefaults)
  {
    xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
    NewTextChild( defNode, "clname", def.clname );
    NewTextChild( defNode, "grp_layer_type", EncodeCompLayerType(def.grp_status) );
    NewTextChild( defNode, "pers_type", ElemIdToCodeNative(etPersType, def.pers_type) );
    NewTextChild( defNode, "seat_no", def.placeName );
    NewTextChild( defNode, "wl_type", def.wl_type );
    NewTextChild( defNode, "seats", def.countPlace );
    NewTextChild( defNode, "isseat", (int)def.isSeat );
    NewTextChild( defNode, "ticket_no", def.ticket_no );
    NewTextChild( defNode, "document", def.document );
    NewTextChild( defNode, "bag_weight", def.bag_weight );
    NewTextChild( defNode, "bag_amount", def.bag_amount );
    NewTextChild( defNode, "excess", def.excess );
    NewTextChild( defNode, "trip_from", def.trip_from );
    NewTextChild( defNode, "comp_rem", def.comp_rem );
    NewTextChild( defNode, "pr_down", (int)def.pr_down );
    NewTextChild( defNode, "pass_rem", def.pass_rem );
  };
  return isWaitList();
}

void TAutoSeats::WritePaxSeats( int point_dep, int pax_id )
{
  TAutoSeats::iterator ipax=begin();
  for ( ; ipax!=end(); ipax++ ) {
    //!logProgTrace( TRACE5, "ipax->pax_id=%d, pax_id=%d", ipax->pax_id, pax_id );
    if ( ipax->pax_id == pax_id ) {
      break;
    }
  }
  if ( ipax == end() ) {
    throw EXCEPTIONS::Exception( "TAutoSeats::WritePaxSeats: pax not found %d", pax_id );
  }
  if ( ipax->seats != (int)ipax->ranges.size() ) {
    throw EXCEPTIONS::Exception( "TAutoSeats::WritePaxSeats: seats not equal pax_id=%d, ipax->seats=%d, ipax->values.size()=%zu",
                                 pax_id, ipax->seats, ipax->ranges.size() );
  }
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "DELETE pax_seats WHERE pax_id=:pax_id AND NVL(pr_wl,0)=0";
  Qry.CreateVariable( "pax_id", otInteger, ipax->pax_id );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO pax_seats(point_id,pax_id,xname,yname,pr_wl) "
    "VALUES(:point_id,:pax_id,:xname,:yname,0)";
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.CreateVariable( "pax_id", otInteger, ipax->pax_id );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );
  for ( std::vector<TSeat>::iterator irange=ipax->ranges.begin();
        irange!=ipax->ranges.end(); irange++ ) {
    Qry.SetVariable( "xname", irange->line );
    Qry.SetVariable( "yname", irange->row );
    Qry.Execute();
  }
}

bool isUserProtectLayer( ASTRA::TCompLayerType layer_type )
{
  return ( layer_type == ASTRA::cltProtCkin ||
           layer_type == ASTRA::cltProtBeforePay ||
           layer_type == ASTRA::cltPNLBeforePay ||
           layer_type == ASTRA::cltProtAfterPay ||
           layer_type == ASTRA::cltPNLAfterPay ||
           layer_type == ASTRA::cltProtSelfCkin );
};

void resetLayers( int point_id, ASTRA::TCompLayerType layer_type,
                  const TSeatRanges &seatRanges,
                  const std::string &lexema_id )
{
  TDateTime time_create = NowUTC();
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);
  SALONS2::TSalonList priorsalonList, salonList;
  // ���� ������� ������
  /*�ᥣ�� ࠡ�⠥� � ����� �����������, �.�. �. !salonChangesToText*/
  priorsalonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id ), SALONS2::rfTranzitVersion, "", NoExists );
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id ), SALONS2::rfTranzitVersion, "", NoExists );
  std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;

  for ( std::vector<TPlaceList*>::iterator isalonList=salonList.begin();
        isalonList!=salonList.end(); isalonList++ ) {
    for ( TPlaces::iterator iseat=(*isalonList)->places.begin(); iseat!=(*isalonList)->places.end(); iseat++ ) {
      TSeatRanges::const_iterator iseatRange=seatRanges.begin();
      for ( ; iseatRange!=seatRanges.end(); iseatRange++ ) {
        if ( iseatRange->first != iseatRange->second ) {
          ProgTrace( TRACE5, "setLayers: layer_type=%s ranges=%s-%s!!!", EncodeCompLayerType( layer_type ),
                     string( string(iseatRange->first.row) + iseatRange->first.line).c_str(),
                     string( string(iseatRange->second.row) + iseatRange->second.line).c_str() );
        }
        TSeat seat( iseat->yname, iseat->xname );
        if ( seat == iseatRange->first ) {
          break;
        }
      }
      iseat->GetLayers( layers, glBase );
      std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.find( point_id );
      bool pr_find = false;
      if ( ilayers != layers.end() ) {
        for ( std::set<TSeatLayer,SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
              ilayer!=ilayers->second.end(); ilayer++ ) {
          if ( ilayer->layer_type == layer_type ) {
            pr_find = true;
            if ( iseatRange == seatRanges.end() ) {
              iseat->ClearLayer( ilayer->point_id, *ilayer );
            }
            break;
         }
        }
      }
      if ( !pr_find && iseatRange != seatRanges.end() ) {
        TSeatLayer seatLayer;
        seatLayer.point_id = point_id;
        seatLayer.point_dep = point_id;
        seatLayer.layer_type = layer_type;
        seatLayer.time_create = time_create;
        iseat->AddLayer( point_id, seatLayer );
      }
    }
  }
  salonList.WriteFlight( point_id, false );
  SALONS2::setTRIP_CLASSES( point_id );

  BitSet<ASTRA::TCompLayerType> editabeLayers;
  LEvntPrms salon_changes;
  salonList.getEditableFlightLayers( editabeLayers );
  salonChangesToText( point_id, priorsalonList, priorsalonList.isCraftLat(), salonList,
                      salonList.isCraftLat(), editabeLayers, salon_changes, false );
  TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms(), evtFlt, point_id);
  for (std::deque<LEvntPrm*>::const_iterator iter=salon_changes.begin(); iter != salon_changes.end(); iter++) {
      TReqInfo::Instance()->LocaleToLog("EVT.SALON_CHANGES", LEvntPrms() << *(dynamic_cast<PrmEnum*>(*iter)), evtFlt, point_id);
  }
  // ����� ����⪨
  SALONS2::check_diffcomp_alarm( point_id );
  if ( SALONS2::isTranzitSalons( point_id ) ) {
    SALONS2::check_waitlist_alarm_on_tranzit_routes( point_id );
  }
  else {
    check_waitlist_alarm( point_id );
  }
}

void getSalonDesrcs( int point_id, TSalonDesrcs &descrs )
{
  descrs.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline,craft,bort FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw UserException( "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" );
  }
  string airline = Qry.FieldAsString( "airline" );
  string craft = Qry.FieldAsString( "craft" );
  string bort = Qry.FieldAsString( "bort" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT desc_code,salon_num,bort FROM compart_desc_sets "
    "WHERE airline=:airline AND craft=:craft AND (bort IS NULL OR bort=:bort) "
    "ORDER BY salon_num, bort, desc_code";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "craft", otString, craft );
  Qry.CreateVariable( "bort", otString, bort );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    descrs[ Qry.FieldAsInteger( "salon_num" ) ].insert( Qry.FieldAsString( "desc_code" ) );
  }
}

void getPaxSeatsWL( int point_id, std::map< bool,std::map < int,TSeatRanges > > &seats )
{
  seats.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pax_id, xname, yname, NVL(pr_wl,0) pr_wl FROM pax_seats "
    "WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  for ( ;!Qry.Eof;Qry.Next() ) {
    seats[ Qry.FieldAsInteger( "pr_wl" ) ][ Qry.FieldAsInteger( "pax_id" ) ].push_back( TSeatRange( TSeat( Qry.FieldAsString( "yname" ), Qry.FieldAsString( "xname" ) ),
                                                                                                    TSeat( Qry.FieldAsString( "yname" ), Qry.FieldAsString( "xname" ) ) ) );
  }
}

void processSalonsCfg_TestMode(int point_id, int comp_id)
{
    //TTripClasses tripClasses(point_id);
    //tripClasses.processBaseCompCfg(comp_id);
    setTRIP_CLASSES(point_id);
}

} // end namespace SALONS2



void AddPass( int pax_id, const std::string &surname,  ASTRA::TCompLayerType layer_type,
              TDateTime time_create,
              const std::vector<std::string> &seatnames, SALONS2::TPaxList &paxList )
{
  SALONS2::TSeatLayer seatLayer;
  //���ᠦ��
  if ( pax_id != ASTRA::NoExists ) {
    paxList[ pax_id ].seats = seatnames.size();
    paxList[ pax_id ].cl = "�";
    paxList[ pax_id ].reg_no = 1;
    paxList[ pax_id ].pers_type = ASTRA::adult;
    paxList[ pax_id ].surname = surname;
    paxList[ pax_id ].pr_infant = ASTRA::NoExists;
  }
  //᫮�
  seatLayer.point_id = 1;
  seatLayer.point_dep = 0;
  seatLayer.point_arv = 2;
  seatLayer.layer_type = layer_type;
  seatLayer.pax_id = pax_id;
  seatLayer.crs_pax_id = ASTRA::NoExists;
  seatLayer.time_create = time_create;

  for ( vector<string>::const_iterator i=seatnames.begin(); i!=seatnames.end(); i++ ) {
    SALONS2::TPlace* place;
    bool pr_find = false;
    for ( std::map<int,SALONS2::TSalonPax>::iterator ipax=paxList.begin(); ipax!=paxList.end(); ipax++ ) {
      for ( std::map<SALONS2::TSeatLayer,SALONS2::TPaxLayerSeats>::iterator ilayer=ipax->second.layers.begin(); ilayer!=ipax->second.layers.end(); ilayer++ ) {
        for ( std::set<SALONS2::TPlace*,SALONS2::CompareSeats>::iterator iseat = ilayer->second.seats.begin(); iseat != ilayer->second.seats.end(); iseat++ ) {
          SALONS2::TPlace* pl = *iseat;
          if ( pl->yname + pl->xname == *i ) {
            place = pl;
            pr_find = true;
            break;
          }
        }
        if ( pr_find )
          break;
      }
      if ( pr_find )
        break;
    }
    if ( !pr_find ) {
      place = new SALONS2::TPlace;
      //����
      place->visible = true;
      place->x = 1;
      place->y = 1;
      place->num = 1;
      place->elem_type = "�";
      place->isplace = true;
      place->xprior = -1;
      place->yprior = -1;
      place->xnext = -1;
      place->ynext = -1;
      place->agle = 0;
      place->clname = "�";
      place->xname = *i;
    }
    place->AddLayer( seatLayer.point_id, seatLayer );
    if ( pax_id != ASTRA::NoExists ) {
      paxList[ pax_id ].layers[ seatLayer ].seats.insert( place );
    }
  }
  if ( pax_id != ASTRA::NoExists ) {
    paxList[ pax_id ].layers[ seatLayer ].waitListReason.layerStatus = SALONS2::layerMultiVerify;
  }
}

void viewPass( SALONS2::TPaxList &paxList )
{
  for ( std::map<int,SALONS2::TSalonPax>::iterator ipax=paxList.begin(); ipax!=paxList.end(); ipax++ ) {
    string str;
    str += string("Pass: pax_id=") + IntToString( ipax->first ) + " ";
    str += ipax->second.surname;
    for ( std::map<SALONS2::TSeatLayer,SALONS2::TPaxLayerSeats>::iterator ilayer=ipax->second.layers.begin(); ilayer!=ipax->second.layers.end(); ilayer++ ) {
      if ( ilayer->second.waitListReason.layerStatus != SALONS2::layerValid ) {
        continue;
      }
      str += string(" valid layer=") + EncodeCompLayerType( ilayer->first.layer_type ) + "," + DateTimeToStr( ilayer->first.time_create );
      str += " seats (";
      for ( std::set<SALONS2::TPlace*,SALONS2::CompareSeats>::iterator iseat = ilayer->second.seats.begin(); iseat != ilayer->second.seats.end(); iseat++ ) {
        SALONS2::TPlace* place;
        place = *iseat;
        str += place->yname + place->xname;
      }
      str += ")";
    }
    //!logProgTrace( TRACE5, "%s", str.c_str() );
  }
}

/*TESTS:
1. �஢�ઠ ��।������ ��������� ᫮�� � ���⪠ ���������� ᫮��
2. �஢�ઠ �ࠢ��쭮�� ��।������ �࠭��⭮�� �������
3. �஢�ઠ 䨫��� �� ᢮��⢠� ᠫ��� � 㪠������ ��砫� � ���� ����⢨� ᢮��⢠
4.
*/

int testsalons(int argc,char **argv)
{
  //!log tst();
  //������塞 ���ᠦ�஢
  TDateTime time_create = NowUTC();
  SALONS2::TPaxList paxList;
  std::vector<std::string> seatnames;
  seatnames.push_back( "1A" );
  seatnames.push_back( "2A" );
  AddPass( 1, "TEST", ASTRA::cltProtCkin, time_create-1, seatnames, paxList );
  seatnames.clear();
  seatnames.push_back( "10A" );
  seatnames.push_back( "20A" );
  AddPass( 1, "TEST", ASTRA::cltCheckin, time_create-1.0/2.0, seatnames, paxList );


  seatnames.clear();
  seatnames.push_back( "11A" );
  seatnames.push_back( "20A" );
  AddPass( 10, "FIRST", ASTRA::cltCheckin, time_create-1, seatnames, paxList );
  seatnames.clear();
  seatnames.push_back( "10A" );
  seatnames.push_back( "22A" );
  AddPass( 10, "FIRST", ASTRA::cltProtCkin, time_create-1, seatnames, paxList );
  seatnames.clear();
  seatnames.push_back( "1A" );
  AddPass( ASTRA::NoExists, "BLOCK", ASTRA::cltBlockCent, time_create-1, seatnames, paxList );

  seatnames.clear();
  seatnames.push_back( "20A" );
  AddPass( ASTRA::NoExists, "BLOCK", ASTRA::cltBlockCent, time_create-1, seatnames, paxList );


  //paxList.validatePaxLayers();
  paxList.dumpValidLayers();
  viewPass( paxList );
  std::map<string,ASTRA::TCompLayerType> seats;
  seats.insert( make_pair( "20A", cltBlockCent ) );
  seats.insert( make_pair( "1A", cltBlockCent ) );
  seats.insert( make_pair( "10A", cltProtCkin ) );
  seats.insert( make_pair( "22A", cltProtCkin ) );

  for ( std::map<int,SALONS2::TSalonPax>::iterator ipax=paxList.begin(); ipax!=paxList.end(); ipax++ ) {
    for ( std::map<SALONS2::TSeatLayer,SALONS2::TPaxLayerSeats>::iterator ilayer=ipax->second.layers.begin(); ilayer!=ipax->second.layers.end(); ilayer++ ) {
      for ( std::set<SALONS2::TPlace*,SALONS2::CompareSeats>::iterator iseat = ilayer->second.seats.begin(); iseat != ilayer->second.seats.end(); iseat++ ) {
        SALONS2::TPlace *seat = *iseat;
        if ( !( ( seats.find( string(seat->yname + seat->xname) ) == seats.end() && seat->getCurrLayer( 1 ).layer_type == cltUnknown ) ||
                ( seats.find( string(seat->yname + seat->xname) ) != seats.end() &&
                ( ( seat->yname + seat->xname == "20A" && seat->getCurrLayer( 1 ).layer_type == cltBlockCent ) ||
                  ( seat->yname + seat->xname == "1A" && seat->getCurrLayer( 1 ).layer_type == cltBlockCent ) ||
                  ( seat->yname + seat->xname == "10A" && seat->getCurrLayer( 1 ).layer_type == cltProtCkin ) ||
                  ( seat->yname + seat->xname == "22A" && seat->getCurrLayer( 1 ).layer_type == cltProtCkin ) ) ) ) ) {
          ProgError( STDLOG, "test1: seat=%s, layer=%s", string( seat->yname + seat->xname ).c_str(), EncodeCompLayerType( seat->getCurrLayer( 1 ).layer_type ) );
          printf( "test1 - not ok\n" );
          return 1;
        }
      }
    }
  }
  printf( "test1 - ok\n" );
  //test2
  std::map<int,int> vpointNum;
  vpointNum[ 0 ] = 0;
  vpointNum[ 1 ] = 1;
  vpointNum[ 2 ] = 2;
  vpointNum[ 3 ] = 3;
  vpointNum[ 4 ] = 4;
  //!log tst();
  /*SALONS2::FilterRoutesProperty filterProp( 1, 3, vpointNum );
  TTripRouteItem item;
  item.point_id = 1;
  item.point_num = 1;
  filterProp.push_back( item );
  item.point_id = 2;
  item.point_num = 2;
  filterProp.push_back( item );
  tst();
  for ( int point_dep = 0; point_dep<=4; point_dep++ ) {
    for ( int point_arv = 0; point_arv<=4; point_arv++ ) {
      if ( filterProp.useRouteProperty( point_dep, point_arv ) &&
           ( point_dep > point_arv ||
             point_dep >= 3 ||
             (point_arv <= 1 && point_arv != point_dep) ||
             ( point_dep < 1 && point_arv <= 1 ) ) ) {
        printf( "test2 - not ok\n" );
        return 1;
      }
    }
  }
  printf( "test2 - ok\n" );
  //test3: point_arv = NoExists
  //test4: vpointNum.size()=1
    */
  return 0;
}



