#include <stdlib.h>
#include "salonform.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_api.h"
#include "oralib.h"
#include "stl_utils.h"
#include "images.h"
#include "points.h"
#include "salons.h"
#include "seats.h"
#include "seats_utils.h"
#include "iatci.h"
#include "iatci_help.h"
#include "astra_misc.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "term_version.h"
#include "alarms.h"
#include "comp_props.h"
#include "serverlib/str_utils.h"
#include "seat_number.h"
#include "flt_settings.h"
#include "crafts/ComponCreator.h"

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace SEATS2;

void ZoneLoads(int point_id, map<string, int> &zones, bool occupied)
{
    std::vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
    vector<TZoneOccupiedSeats> zoneSeats;
    ZoneLoads(point_id, false, false, false, zoneSeats,CompSectionsLayers);
    for ( vector<TZoneOccupiedSeats>::iterator i=zoneSeats.begin(); i!=zoneSeats.end(); i++ ) {
        zones[ i->name ] = (occupied ? i->seats.size() : i->total_seats - i->seats.size());
        if(zones[i->name] < 0)
            throw EXCEPTIONS::Exception("ZoneLoads: negative seats");
    }
}

void ZoneLoads(int point_id,
               bool only_checkin_layers, bool only_high_layer, bool drop_not_used_pax_layers,
               std::vector<TZoneOccupiedSeats> &zones, std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers )
{
  vector<SALONS2::TCompSection> compSections;
  ZoneLoads( point_id, only_checkin_layers, only_high_layer, drop_not_used_pax_layers,
             zones, CompSectionsLayers, compSections );
}

void bagSectionTocompSection( const simpleProps &sections, vector<SALONS2::TCompSection> &compSections ) {
  compSections.clear();
  for ( simpleProps::const_iterator i=sections.begin(); i!=sections.end(); i++ ) {
    TCompSection section;
    section = *i;
    compSections.push_back( section );
  }
}

void ZoneLoadsTranzitSalons(int point_id,
                            bool only_checkin_layers,
                            vector<TZoneOccupiedSeats> &zones,
                            std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers,
                            vector<SALONS2::TCompSection> &compSections )
{
  compSections.clear();
  CompSectionsLayers.clear();
  zones.clear();
  TQuery Qry(&OraSession);
  try {
    TTripInfo info;
    info.getByPointId( point_id );
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), "", NoExists );
    if ( !(salonList.getCompId() > 0 && SALONS2::isComponSeatsNoChanges( info )) ) {
      return;
    }
    simpleProps sections( SECTION );
    sections.read( salonList.getCompId() );
    bagSectionTocompSection( sections, compSections );
    Qry.Clear();
    Qry.SQLText =
      "select layer_type from grp_status_types WHERE layer_type IS NOT NULL";
    Qry.Execute();
    std::map<ASTRA::TCompLayerType,SALONS2::TPlaces> layersSeats, checkinLayersSeats;
    for( ; !Qry.Eof; Qry.Next() ) {
      checkinLayersSeats[DecodeCompLayerType(Qry.FieldAsString("layer_type"))].clear();
    }
    for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
      ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
      BASIC_SALONS::TCompLayerElem layer_elem;
      if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
           layer_elem.getOccupy() ) {
        layersSeats[ layer_type ].clear();
      }
    }
    for ( vector<SALONS2::TCompSection>::iterator i=compSections.begin(); i!=compSections.end(); i++ ) {
      SALONS2::TCompSectionLayers compSectionLayers;
      compSectionLayers.layersSeats = layersSeats;
      salonList.getLayerPlacesCompSection( *i, compSectionLayers.layersSeats, i->seats );
      compSectionLayers.compSection = *i;
      TZoneOccupiedSeats zs;
      zs.name = i->getName();
      zs.total_seats = i->seats;
      for(std::map<ASTRA::TCompLayerType, SALONS2::TPlaces>::iterator im = compSectionLayers.layersSeats.begin(); im != compSectionLayers.layersSeats.end(); im++) {
        ProgTrace( TRACE5, "im->first=%s, im->second=%zu", EncodeCompLayerType( im->first ), im->second.size() );
        if ( !only_checkin_layers || checkinLayersSeats.find( im->first ) != checkinLayersSeats.end() ) { // работаем только со слоями регистрации??? ДЕН!!!
          zs.seats.insert( zs.seats.end(), im->second.begin(), im->second.end() );
        }
      }
      zones.push_back( zs );
      CompSectionsLayers.push_back( compSectionLayers );
    }
  }
  catch(exception &E) {
      ProgTrace(TRACE5, "ZoneLoads failed, so result would be empty: %s", E.what());
  }
  catch(...) {
      ProgTrace(TRACE5, "ZoneLoads failed, so result would be empty");
  }
}

void ZoneLoads(int point_id,
               bool only_checkin_layers, bool only_high_layer, bool drop_not_used_pax_layers,
               vector<TZoneOccupiedSeats> &zones,
               std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers,
               vector<SALONS2::TCompSection> &compSections )
{
  compSections.clear();
  CompSectionsLayers.clear();
  zones.clear();
  ZoneLoadsTranzitSalons( point_id,
                          only_checkin_layers,
                          zones,
                          CompSectionsLayers,
                          compSections );
}

enum TCompType { ctBase = 0, ctBaseBort = 1, ctPrior = 2, ctCurrent = 3 };

struct TShowComps {
    int comp_id;
    int crc_comp;
    string craft;
    string bort;
    string classes;
    string descr;
    TCompType comp_type;
    string airline;
    string airp;
    string name;
    TShowComps() {
    crc_comp = 0;
    };
};

void setCompName( TShowComps &comp )
{
  if ( !comp.airline.empty() )
    comp.name = ElemIdToCodeNative(etAirline,comp.airline);
  else
    comp.name = ElemIdToCodeNative(etAirp,comp.airp);
  if ( comp.name.length() == 2 )
    comp.name += "  ";
  else
    comp.name += " ";
  if ( !comp.bort.empty() && comp.comp_type != ctCurrent )
    comp.name += comp.bort;
  else
    comp.name += "  ";
  comp.name += "  " + comp.classes;
  if ( !comp.descr.empty() && comp.comp_type != ctCurrent )
    comp.name += "  " + comp.descr;
  if ( comp.comp_type == ctCurrent ) {
    comp.name += " (";
    comp.name += AstraLocale::getLocaleText( "ТЕК." );
    comp.name += ")";
  }
}

void getFlightTuneCompRef( int point_id, bool use_filter, const string &trip_airline, vector<TShowComps> &comps )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT craft, bort, crc_comp,"
    "       NVL( SUM( DECODE( class, 'П', 1, 0 ) ), 0 ) as f, "
    "       NVL( SUM( DECODE( class, 'Б', 1, 0 ) ), 0 ) as c, "
    "       NVL( SUM( DECODE( class, 'Э', 1, 0 ) ), 0 ) as y "
    "  FROM trip_comp_elems, comp_elem_types, points, trip_sets "
    " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "       comp_elem_types.pr_seat <> 0 AND "
    "       trip_comp_elems.point_id = points.point_id AND "
    "       points.point_id = :point_id AND "
    "       trip_sets.point_id(+) = points.point_id AND "
    "       trip_sets.comp_id IS NULL "
    " GROUP BY craft, bort, crc_comp ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TShowComps comp;
    comp.comp_id = -1;
    comp.craft = Qry.FieldAsString("craft");
    comp.bort = Qry.FieldAsString("bort");
    comp.crc_comp = Qry.FieldAsInteger("crc_comp");
    if (Qry.FieldAsInteger("f")) {
      comp.classes += ElemIdToCodeNative(etClass,"П");
      comp.classes += IntToString(Qry.FieldAsInteger("f"));
    };
    if (Qry.FieldAsInteger("c")) {
        if ( !comp.classes.empty() )
            comp.classes += " ";
      comp.classes += ElemIdToCodeNative(etClass,"Б");
      comp.classes += IntToString(Qry.FieldAsInteger("c"));
    }
    if (Qry.FieldAsInteger("y")) {
        if ( !comp.classes.empty() )
            comp.classes += " ";
      comp.classes += ElemIdToCodeNative(etClass,"Э");
      comp.classes += IntToString(Qry.FieldAsInteger("y"));
    }
    comp.comp_type = ctCurrent;
    if ( !use_filter ||
         ( comp.comp_type != ctBase && comp.comp_type != ctBaseBort ) || /* поиск компоновки только по компоновкам нужной А/К или портовым компоновкам */
             ( ( comp.airline.empty() || trip_airline == comp.airline ) &&
               filterComponsForView( comp.airline, comp.airp ) ) ) {
      setCompName( comp );
      comps.push_back( comp );
    }
    Qry.Next();
  }
}

struct CompParams {
   enum TCond  { fcOnleSetCompon, fcFiltered, fcLibra };
   BitSet<TCond> conds;
   std::string filter_airline;
   std::string filter_craft;
   std::string filter_bort;
   CompParams( const BitSet<TCond>& vconds,
               std::string vfilter_airline,
               std::string vfilter_craft,
               std::string vfilter_bort ) {
     conds = vconds;
     filter_airline = vfilter_airline;
     filter_craft = vfilter_craft;
     filter_bort = vfilter_bort;
  }
};

void getFlightBaseCompRef( int point_id,
                           const CompParams& compParams,
                           vector<TShowComps> &comps )
{
  TQuery Qry( &OraSession );
  string sql;
  if ( !compParams.conds.isFlag( CompParams::TCond::fcOnleSetCompon ) ) {
    sql =
      "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "
      "       comps.descr,0 as pr_comp, comps.airline, comps.airp "
      " FROM comps, points ";
    if ( compParams.conds.isFlag( CompParams::TCond::fcLibra ) ) {
      sql += ",libra_comps ";
    }
    sql +=
      "WHERE points.craft = comps.craft AND points.point_id = :point_id ";
    if ( compParams.conds.isFlag( CompParams::TCond::fcLibra ) ) {
      sql += " AND libra_comps.comp_id=comps.comp_id ";
    }
    sql +=  " UNION ";
  }
  sql +=
    "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "
    "       comps.descr,1 as pr_comp, null airline, null airp "
    " FROM comps, points, trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND ";
//  if ( !compParams.isLibra ) {
    sql += "      points.craft = comps.craft AND ";
//  }
  sql +=
    " points.point_id = :point_id AND "
    "      trip_sets.comp_id = comps.comp_id "
    "ORDER BY craft, bort, classes, descr";
  LogTrace(TRACE5) << sql;
  Qry.SQLText = sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TShowComps comp;
    comp.comp_id = Qry.FieldAsInteger( "comp_id" );
    comp.craft = Qry.FieldAsString( "craft" );
    comp.bort = Qry.FieldAsString( "bort" );
    comp.classes = Qry.FieldAsString( "classes" );
    comp.descr = Qry.FieldAsString( "descr" );
    switch ( Qry.FieldAsInteger( "pr_comp" ) ) {
      case 0:
        if ( !comp.craft.empty() && comp.craft == compParams.filter_craft &&
             !comp.bort.empty() && comp.bort == compParams.filter_bort )
          comp.comp_type = ctBaseBort;
        else
          comp.comp_type = ctBase;
        break;
      case 1:
        comp.comp_type = ctCurrent;
        break;
    };
    comp.airline = Qry.FieldAsString( "airline" );
    comp.airp = Qry.FieldAsString( "airp" );
    if ( !compParams.conds.isFlag( CompParams::TCond::fcFiltered ) ||
         ( comp.comp_type != ctBase && comp.comp_type != ctBaseBort ) || /* поиск компоновки только по компоновкам нужной А/К или портовым компоновкам */
             ( ( comp.airline.empty() || compParams.filter_airline == comp.airline ) &&
               filterComponsForView( comp.airline, comp.airp ) ) ) {
      setCompName( comp );
      comps.push_back( comp );
    }
    Qry.Next();
  }
}

bool CompareShowComps( const TShowComps &item1, const TShowComps &item2 )
{
  if ( (int)item1.comp_type > (int)item2.comp_type )
    return true;
  else
    if ( (int)item1.comp_type < (int)item2.comp_type )
      return false;
    else
      if ( item1.name < item2.name )
        return true;
      else
        if ( item1.name > item2.name )
          return false;
        else
          return ( item1.comp_id < item2.comp_id );
}

int getPointArvFromPaxId( int pax_id, int point_dep )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_arv FROM pax, pax_grp "
    " WHERE pax.grp_id=pax_grp.grp_id AND pax_id=:pax_id";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    return Qry.FieldAsInteger( "point_arv" );
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT airp_arv FROM crs_pnr, crs_pax "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:pax_id";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    TTripRoute routes;
    if ( routes.GetRouteAfter( ASTRA::NoExists,
                               point_dep,
                               trtNotCurrent,
                               trtNotCancelled ) ) {
      for ( std::vector<TTripRouteItem>::iterator iroute=routes.begin();
            iroute!=routes.end(); iroute++ ) {
        if ( iroute->airp == Qry.FieldAsString( "airp_arv" ) ) {
          return iroute->point_id;
        }
      }
    }
  }
  return NoExists;
}

void SalonFormInterface::RefreshPaxSalons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  tst();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int point_id = NodeAsInteger( "point_id", reqNode );
  int point_arv = NodeAsInteger( "point_arv", reqNode, NoExists );
  int tariff_pax_id = NodeAsInteger( "tariff_pax_id", reqNode );
  int prior_tariff_pax_id = NodeAsInteger( "prior_tariff_pax_id", reqNode, NoExists );
  int comp_crc = NodeAsInteger( "comp_crc", reqNode );
  TSalonChanges seats;
  string filterClass;
  if ( GetNode( "ClName", reqNode ) ) {
    filterClass = NodeAsString( "ClName", reqNode );
  }

  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  int ncomp_crc = CompCheckSum::calcFromDB( point_id ).total_crc32;
  SALONS2::TSalonList salonList;
  try {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), filterClass, prior_tariff_pax_id );
    if ( prior_tariff_pax_id == NoExists ) { //delete all color + tariff + rfics

    }
  }
  catch( AstraLocale::UserException& ue ) {
    AstraLocale::showErrorMessage( ue.getLexemaData() );
    ncomp_crc = 0;
  }
  if ( comp_crc != 0 && ncomp_crc != 0 && comp_crc == ncomp_crc ) { //update
    SALONS2::getSalonChanges( salonList, tariff_pax_id, seats );
    std::map<int,SALONS2::TPaxList> pax_lists;
    SALONS2::BuildSalonChanges( dataNode, point_id, seats, false, pax_lists );
  }
  else { //load all
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), filterClass, tariff_pax_id );
    SALONS2::GetTripParams( point_id, dataNode );
    salonList.Build( NewTextChild( dataNode, "salons" ) );
    int comp_id = salonList.getCompId();
    if ( comp_id > 0 ) { //строго завязать базовые компоновки с назначенными на рейс
      TTripInfo info;
      info.getByPointId( point_id );
      if ( isComponSeatsNoChanges( info ) ) {
        componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
      }
    }
  }
}

void SalonFormInterface::Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int point_id = NodeAsInteger( "trip_id", reqNode );
  if(point_id < 0)
  {
      ProgTrace(TRACE3, "Query iatci seat map");
      IatciInterface::SeatmapRequest(reqNode);
      return AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR");
  }
  ProgTrace(TRACE5, "SalonFormInterface::Show point_id=%d", point_id );

  int point_arv = NoExists;
  int pax_id = NoExists;
  int tariff_pax_id = NoExists;
  xmlNodePtr tmpNode;
  tmpNode = GetNode( "point_arv", reqNode );
  if ( tmpNode ) {
    point_arv = NodeAsInteger( tmpNode );
  }
  tmpNode = GetNode( "pax_id", reqNode );
  if ( tmpNode ) {
    pax_id = NodeAsInteger( tmpNode );
  }
  tmpNode = GetNode( "tariff_pax_id", reqNode );
  if ( tmpNode ) {
    tariff_pax_id = NodeAsInteger( tmpNode );
  }
  TQuery Qry( &OraSession );
  SALONS2::GetTripParams( point_id, dataNode );
  if ( point_arv == NoExists && pax_id != NoExists ) {
    point_arv = getPointArvFromPaxId( pax_id, point_id );
  }
  bool pr_comps = GetNode( "pr_comps", reqNode );
  bool pr_images = GetNode( "pr_images", reqNode ); // не используется в новом терминале!!!
  ProgTrace( TRACE5, "trip_id=%d, point_arv=%d, pax_id=%d, pr_comps=%d, pr_images=%d",
             point_id, point_arv, pax_id, pr_comps, pr_images );
  if ( pr_comps ) {
    Qry.SQLText =
      "SELECT " + TAdvTripInfo::selectedFields("points") + ", "
      "       bort, NVL(comp_id,-1) comp_id "
      " FROM points, trip_sets "
      " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+) AND points.pr_del>=0";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( !Qry.RowCount() )
        throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    string trip_airline = Qry.FieldAsString( "airline" );
    string craft = Qry.FieldAsString( "craft" );
    string bort = Qry.FieldAsString( "bort" );
    vector<TShowComps> comps, comps_tmp;
    TTripInfo info;
    info.Init( Qry );
    LogTrace(TRACE5) << info.airline << info.airp;
    if ( ComponCreator::LibraComps::isLibraMode( info ) ) {
      tst();
      ComponCreator::SychAHMCompsFromBort( trip_airline, craft, bort ); //синхронизация компоновок
      BitSet<CompParams::TCond> conds;
      conds.setFlag( CompParams::TCond::fcLibra );
      getFlightBaseCompRef( point_id, CompParams( conds, trip_airline, craft, bort ), comps );
    }
    else {
      getFlightTuneCompRef( point_id, true, trip_airline, comps );
      if ( Qry.FieldAsInteger( "pr_tranzit" ) ) {
        TTripRoute route;
        TTripRouteItem item;
        route.GetPriorAirp( NoExists,
                            point_id,
                            Qry.FieldAsInteger( "point_num" ),
                            Qry.FieldAsInteger( "first_point" ),
                            Qry.FieldAsInteger( "pr_tranzit" ),
                            trtNotCancelled,
                            item );
        if ( item.point_id != NoExists ) { // нашли пред. пункт
          // если компоновка базовая в пред. пункте, то надо передать базовую иначе текущую
          Qry.SetVariable( "point_id", item.point_id );
          Qry.Execute();
          if ( !Qry.Eof && craft == Qry.FieldAsString( "craft" ) ) {
            ProgTrace( TRACE5, "GetPriorAirp: point_id=%d, comp_id=%d", item.point_id, Qry.FieldAsInteger( "comp_id" ) );
            if ( Qry.FieldAsInteger( "comp_id" ) >= 0 ) { // базовая
              BitSet<CompParams::TCond> conds;
              conds.setFlag( CompParams::TCond::fcOnleSetCompon );
              getFlightBaseCompRef( item.point_id, CompParams( conds, trip_airline, craft, bort ), comps_tmp );
            }
            else {
              getFlightTuneCompRef( item.point_id, false, trip_airline, comps_tmp ); // редактированная
            }
            if ( !comps_tmp.empty() ) {
              info.Init( Qry );
              string StrVal = GetTripName( info, ecDisp, true, false ) + " ";
              if ( craft != Qry.FieldAsString( "craft" ) )
                StrVal += string(" ") + Qry.FieldAsString( "craft" );
              if ( bort != Qry.FieldAsString( "bort" ) )
                StrVal += string(" ") + Qry.FieldAsString( "bort" );
              comps_tmp.begin()->comp_type = ctPrior;
              setCompName( *comps_tmp.begin() );
              comps_tmp.begin()->name = StrVal + " " + comps_tmp.begin()->name;
              comps.insert( comps.end(), *comps_tmp.begin() );
            }
          }
        }
      }
      BitSet<CompParams::TCond> conds;
      conds.setFlag( CompParams::TCond::fcFiltered );
      getFlightBaseCompRef( point_id, CompParams( conds, trip_airline, craft, bort ), comps );
    }
    //sort comps for client
    sort( comps.begin(), comps.end(), CompareShowComps );
    xmlNodePtr compsNode = NULL;
    for (vector<TShowComps>::iterator i=comps.begin(); i!=comps.end(); i++ ) {
      if ( !compsNode )
        compsNode = NewTextChild( dataNode, "comps"  );
      xmlNodePtr compNode = NewTextChild( compsNode, "comp" );
      NewTextChild( compNode, "name", i->name );
      xmlNodePtr tmpNode = NewTextChild( compNode, "comp_id", i->comp_id );
      if ( i->crc_comp != 0 )
        SetProp( tmpNode, "crc_comp", i->crc_comp );
      NewTextChild( compNode, "pr_comp", (int)(i->comp_type == ctCurrent) );
      NewTextChild( compNode, "craft", i->craft );
      NewTextChild( compNode, "bort", i->bort );
      NewTextChild( compNode, "classes", i->classes );
      NewTextChild( compNode, "descr", i->descr );
    }
    if ( !compsNode ) {
        AstraLocale::showErrorMessage( "MSG.SALONS.NOT_FOUND_FOR_THIS_CRAFT" );
        return;
    }
  } //END PR_comps

  string filterClass;
  int comp_id;
  if ( GetNode( "ClName", reqNode ) ) {
    filterClass = NodeAsString( "ClName", reqNode );
  }

  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }

  SALONS2::TSalonList salonList(true);
  try {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), filterClass, tariff_pax_id );
  }
  catch( AstraLocale::UserException& ue ) {
    AstraLocale::showErrorMessage( ue.getLexemaData() );
  }
  salonList.Build( NewTextChild( dataNode, "salons" ) );
  comp_id = salonList.getCompId();
  if ( pr_comps ) {
    if ( get_alarm( point_id, Alarm::Waitlist ) ) {
        AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
        SetProp(NewTextChild( dataNode, "passengers" ), "pr_waitlist", 1);
    }
  }
  if ( pr_images ) { // не используется в новом терминале
    GetDataForDrawSalon( reqNode, resNode );
  }
  if ( comp_id > 0 ) { //строго завязать базовые компоновки с назначенными на рейс
    TTripInfo info;
    info.getByPointId( point_id );
      if ( isComponSeatsNoChanges( info ) ) {
        componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
    }
  }
}

void SalonFormInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  if ( SALONS2::isFreeSeating( trip_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace(TRACE5, "SalonFormInterface::Write point_id=%d", trip_id );
  xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
  bool cBase = false;
  bool cChange = false;
  bool cSet = false;
  xmlNodePtr ctypeNode = NodeAsNode( "ctype", refcompNode );
  if ( ctypeNode ) {
    ctypeNode = ctypeNode->children; /* value */
    while ( ctypeNode ) {
        string stype = NodeAsString( ctypeNode );
      cBase = ( stype == string( "cBase" ) || cBase ); // базовая
      cChange = ( stype == string( "cChange" ) || cChange ); // измененнная
      cSet = ( stype == string( "cSet" ) || cSet ); // установленная или новая=false
      ctypeNode = ctypeNode->next;
    }
  }
  ProgTrace( TRACE5, "cBase=%d, cChange=%d, cSet=%d", cBase, cChange, cSet );
  TFlights flights;
  flights.Get( trip_id, ftTranzit );
  flights.Lock(__FUNCTION__);
  TTripInfo info;
  info.getByPointId( trip_id );
  SALONS2::TSalonList salonList(true), priorsalonList(true);
  salonList.Parse( info, info.airline, NodeAsNode( "salons", reqNode ) );
  //была ли до этого момента компоновка
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();

  bool pr_base_change = false;
  bool saveContructivePlaces = false;
  if ( !Qry.Eof ) { // была старая компоновка
    priorsalonList.ReadFlight( SALONS2::TFilterRoutesSets( trip_id ), "", NoExists );
    pr_base_change = salonList._seats.basechecksum() != priorsalonList._seats.basechecksum();
    if ( (!TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION )) &&
         SALONS2::haveConstructiveCompon( trip_id, rTripSalons ) ) {
      saveContructivePlaces = true;
      if ( pr_base_change ) {
        throw UserException( "MSG.CHANGE_COMPON_NOT_AVAILABLE_UPDATE_TERM" );
      }
    }
  }
  Qry.Clear();
  Qry.SQLText = "UPDATE trip_sets SET comp_id=:comp_id WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.DeclareVariable( "comp_id", otInteger );
  // пришла новая компоновка, но не пришел comp_id - значит были изменения компоновки - "сохраните базовую компоновку."
  bool pr_notchangecraft = isComponSeatsNoChanges( info );
  if ( pr_notchangecraft ) {
    if ( comp_id == -2 && !cSet )
      throw UserException( "MSG.SALONS.SAVE_BASE_COMPON" );
    // может вызвать ошибку, если салон не был назначен на рейс
    if ( comp_id == -2 && pr_base_change ) // была старая компоновка
      throw UserException( "MSG.SALONS.NOT_CHANGE_CFG_ON_FLIGHT" );
    if ( comp_id != -2 && !cSet ) { //новая компоновку
      Qry.SetVariable( "comp_id", comp_id );
      Qry.Execute();
    }
  }
  else {
    if ( pr_base_change && cSet ) {
      comp_id = -2;
    }
    if ( comp_id == -2 ) {
      Qry.SetVariable( "comp_id", FNull );
    }
    else {
      Qry.SetVariable( "comp_id", comp_id );
    }
    Qry.Execute();
  }

  salonList.WriteFlight( trip_id, saveContructivePlaces );
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
  /* инициализация VIP */
  ComponCreator::InitVIP( info, Qry );
  string lexema_id;
  LEvntPrms params;
  string comp_lang;
  if ( NodeAsInteger( "pr_lat", refcompNode ) != 0 )
    comp_lang = "лат.";
  else
    comp_lang = "рус.";

  if ( pr_initcomp ) { /* изменение компоновки */
    if ( cBase && cChange ) {
      lexema_id = "EVT.FLIGHT_CRAFT_LAYOUT_ASSIGNED";
      params << PrmSmpl<string>("cls", NodeAsString("classes", refcompNode));
    }
    else if ( cBase ) {
      lexema_id = "EVT.LAYOUT_ASSIGNED_SALON_CHANGES";
      params << PrmSmpl<int>("id", comp_id) << PrmSmpl<string>("cls", NodeAsString("classes", refcompNode)); // !!!DJEK
    }
    params << PrmLexema("lang", (comp_lang == "лат.")?"EVT.LANGUAGE_LAT":"EVT.LANGUAGE_RUS");
  }
  else {
    lexema_id = "EVT.LAYOUT_MODIFIED_SALON_CHANGES";
    params << PrmSmpl<string>("cls", NodeAsString("classes", refcompNode))
              << PrmLexema("lang", (comp_lang == "лат.")?"EVT.LANGUAGE_LAT":"EVT.LANGUAGE_RUS");
  }
  ComponCreator::setFlightClasses( trip_id );
  //set flag auto change in false state
  ComponCreator::setManualCompChg( trip_id );
  SALONS2::CompCheckSum checksum = SALONS2::CompCheckSum::calcFromDB( trip_id );
  Qry.Clear();
  Qry.SQLText = "UPDATE trip_sets SET crc_comp=:crc_comp,crc_base_comp=:crc_base_comp WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.CreateVariable( "crc_comp", otInteger, checksum.total_crc32 );
  Qry.CreateVariable( "crc_base_comp", otString, checksum.base_crc32 );
  Qry.Execute();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  SALONS2::GetTripParams( trip_id, dataNode );
  // надо перечитать заново
  /*всегда работаем с новой компоновкой, т.к. см. !salonChangesToText*/
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( trip_id ), "", NoExists );
  BitSet<ASTRA::TCompLayerType> editabeLayers;
  salonList.getEditableFlightLayers( editabeLayers );

  LEvntPrms salon_changes;
  salonChangesToText( trip_id, priorsalonList._seats, priorsalonList.isCraftLat(),
                        salonList._seats, salonList.isCraftLat(),
                        editabeLayers,
                        salon_changes, cBase && comp_id != -2);
  TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtFlt, trip_id);
  for (std::deque<LEvntPrm*>::const_iterator iter=salon_changes.begin(); iter != salon_changes.end(); iter++) {
      TReqInfo::Instance()->LocaleToLog("EVT.SALON_CHANGES", LEvntPrms() << *(dynamic_cast<PrmEnum*>(*iter)), evtFlt, (*iter)->get_sub_type(), trip_id);
  }
  // конец перечитки
  ComponCreator::check_diffcomp_alarm( trip_id );
  SALONS2::check_waitlist_alarm_on_tranzit_routes( trip_id, "SalonFormInterface::Write" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
//  #warning 8. new parse + salonChangesToText
  salonList.Build( salonsNode );
  comp_id = salonList.getCompId();
  if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
    componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
  }
  if ( get_alarm( trip_id, Alarm::Waitlist ) ) {
    AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
    SetProp(NewTextChild( dataNode, "passengers" ), "pr_waitlist", 1);
  }
  else
    AstraLocale::showMessage( "MSG.DATA_SAVED" );
}

void SalonFormInterface::ComponShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr tmpNode = GetNode( "comp_id", reqNode );
  int comp_id = NodeAsInteger( tmpNode );
  int prior_point_id = NoExists, point_id = NoExists;
  TQuery Qry( &OraSession );
  xmlNodePtr pNode = GetNode( "point_id", reqNode );
  if ( comp_id < 0 && ( pNode ) ) { // выбрана компоновка пред. пункта в транзитном маршруте
    point_id = NodeAsInteger( pNode );
    int crc_comp = 0;
    if ( GetNode( "@crc_comp", tmpNode ) )
      crc_comp = NodeAsInteger( "@crc_comp", tmpNode );
    ProgTrace( TRACE5, "point_id=%d, crc_comp=%d", point_id, crc_comp );
    Qry.SQLText =
      "SELECT " + TAdvTripInfo::selectedFields("points") + ", "
      "       bort, crc_comp "
      " FROM points, trip_sets "
      " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+) AND points.pr_del>=0";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( !Qry.RowCount() )
        throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    TTripRoute route;
    TTripRouteItem item;
    route.GetPriorAirp( NoExists,
                        point_id,
                        Qry.FieldAsInteger( "point_num" ),
                        Qry.FieldAsInteger( "first_point" ),
                        Qry.FieldAsInteger( "pr_tranzit" ),
                        trtNotCancelled,
                        item );
    ProgTrace( TRACE5, "prior_point_id=%d", item.point_id );
    if ( item.point_id == NoExists )
      throw UserException( "MSG.SALONS.NOT_FOUND" );
    Qry.SetVariable( "point_id", item.point_id );
    Qry.Execute();
    if ( !Qry.Eof ) {
      ProgTrace( TRACE5, "prior_crc_comp=%d", Qry.FieldAsInteger( "crc_comp" ) );
    }
    if ( Qry.Eof ||
         ( crc_comp != 0 &&
           Qry.FieldAsInteger( "crc_comp" ) != 0 &&
           crc_comp != Qry.FieldAsInteger( "crc_comp" ) ) )
      throw UserException( "MSG.SALONS.NOT_FOUND" );
    prior_point_id = item.point_id;
  }
  int id;
  SALONS2::TReadStyle readStyle;
  if ( comp_id < 0 ) {
    id = prior_point_id;
    readStyle =  SALONS2::rTripSalons;
  }
  else {
    id = comp_id;
    readStyle = SALONS2::rComponSalons;
  }

  SALONS2::TSalonList salonList;  //всегда новый алгоритм
  if ( readStyle == SALONS2::rTripSalons ) {
    if ( SALONS2::isFreeSeating( point_id ) ) {
      throw UserException( "MSG.SALONS.FREE_SEATING" );
    }
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( id ), "", ASTRA::NoExists, false, point_id );
  }
  else {
    int point_id = ASTRA::NoExists;
    if ( pNode ) {
      point_id = NodeAsInteger( pNode );
    }
    salonList.ReadCompon( id, point_id );
  }
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  if ( comp_id < 0 )
    SALONS2::GetTripParams( prior_point_id, dataNode );
  else
    SALONS2::GetCompParams( comp_id, dataNode );
  salonList.Build( salonsNode );
//  #warning 7. зачем еще раз делать BuildLayersInfo???
  bool pr_notchangecraft = true;
  //BitSet<SALONS2::TDrawPropsType> props;
  if ( pNode ) {
    TTripInfo info;
    info.getByPointId( NodeAsInteger( pNode ) );
    pr_notchangecraft = isComponSeatsNoChanges( info );
    SALONS2::CreateSalonMenu( info, salonsNode );
  }
  if ( pr_notchangecraft && comp_id >= 0 ) {
    componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
  }
}

void getEditabeLayers( BitSet<ASTRA::TCompLayerType> &editabeLayers, bool isComponCraft )
{
  editabeLayers.clearFlags();
    for ( int l=0; l!=cltTypeNum; l++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)l;
    if ( !SALONS2::isBaseLayer( layer_type, isComponCraft ) ) {
      continue;
    }
    editabeLayers.setFlag( layer_type );
  }
}

void SalonFormInterface::ComponWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::ComponWrite, comp_id=%d", comp_id );
  TReqInfo *r = TReqInfo::Instance();
  SALONS2::TSalonList salonList, priorsalonList;
  SALONS2::TComponSets componSets;
  BitSet<ASTRA::TCompLayerType> editabeLayers;
  getEditabeLayers( editabeLayers, true );
  componSets.Parse( reqNode );
  salonList.Parse( boost::none, componSets.airline, GetNode( "salons", reqNode ) );
  bool saveContructivePlaces = false;
  if ( componSets.modify != SALONS2::mNone &&
       componSets.modify != SALONS2::mAdd ) {
    priorsalonList.ReadCompon( comp_id, ASTRA::NoExists );
    bool isLibraComps = ComponCreator::LibraComps::isLibraComps(comp_id, ComponCreator::LibraComps::ctComp);
    if ( isLibraComps &&
         salonList._seats.basechecksum() != priorsalonList._seats.basechecksum() ) {
      throw UserException( "MSG.CHANGE_NOT_AVAILABLE_LIBRA_COMPS" );
    }
    if (  componSets.modify == SALONS2::mChange &&
         (!TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION )||isLibraComps) &&
         SALONS2::haveConstructiveCompon( comp_id, rComponSalons ) ) {
      if ( salonList._seats.basechecksum() != priorsalonList._seats.basechecksum() ) {
        throw UserException( "MSG.CHANGE_COMPON_NOT_AVAILABLE_UPDATE_TERM" );
      }
      saveContructivePlaces = true;
    }
  }
  salonList.WriteCompon( comp_id, componSets, saveContructivePlaces );
  LEvntPrms salon_changes;
  salonChangesToText( NoExists, priorsalonList._seats, priorsalonList.isCraftLat(),
                      salonList._seats, salonList.isCraftLat(),
                      editabeLayers,
                      salon_changes, componSets.modify != SALONS2::mDelete);
  if ( componSets.modify !=  SALONS2::mDelete ) {
    salonList.ReadCompon( comp_id, ASTRA::NoExists );
  }
  if ( componSets.modify != SALONS2::mNone ) {
    string lexema_id;
    LEvntPrms params;
    switch ( componSets.modify ) {
      case SALONS2::mDelete:
        lexema_id = "EVT.LAYOUT_DELETED_SALON_CHANGES";
        params << PrmSmpl<int>("id", comp_id);
        break;
      default:
        if ( componSets.modify == SALONS2::mAdd ) {
          lexema_id = "EVT.LAYOUT_CREATED_SALON_CHANGES";
        }
        else
          lexema_id = "EVT.LAYOUT_UPDATED_SALON_CHANGES";
        params << PrmSmpl<int>("id", comp_id);
        break;
    }
    if ( componSets.airline.empty() )
      params << PrmLexema("airl", "EVT.UNKNOWN");
    else
      params << PrmElem<string>("airl", etAirline, componSets.airline);
    if ( componSets.airp.empty() )
      params << PrmLexema("airp", "EVT.UNKNOWN");
    else
      params << PrmElem<string>("airp", etAirp, componSets.airp);
    params << PrmElem<string>("craft", etCraft, componSets.craft);
    if ( componSets.bort.empty() )
      params << PrmLexema("bort", "EVT.UNKNOWN");
    else
      params << PrmSmpl<string>("bort", componSets.bort);
    params << PrmSmpl<string>("cls", componSets.classes);
    if ( componSets.descr.empty() )
      params << PrmLexema("descr", "EVT.UNKNOWN");
    else
      params << PrmSmpl<string>("descr", componSets.descr);
    r->LocaleToLog(lexema_id, params, evtComp, comp_id);
    for (std::deque<LEvntPrm*>::const_iterator iter=salon_changes.begin(); iter != salon_changes.end(); iter++) {
        r->LocaleToLog("EVT.SALON_CHANGES", LEvntPrms() << *(dynamic_cast<PrmEnum*>(*iter)), evtComp, comp_id);
    }
  }
  if ( componSets.modify != SALONS2::mDelete ) {
    //bagsections new terminal not exists node CompSections
    simpleProps(SECTION).parse_check_write( GetNode( "CompSections", reqNode ), comp_id );
    xmlNodePtr n = GetNode( "sections", reqNode );
    if ( n ) {
      n = n->children;
    }
    while ( n && string("sections") == (const char*)n->name ) {
      simpleProps( NodeAsString( "@code", n ) ).parse_write( n, comp_id );
      n = n->next;
    }
  }
  if ( componSets.modify == SALONS2::mDelete ) {
    comp_id = -1;
  }
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", comp_id );
  if ( !componSets.airline.empty() )
    NewTextChild( dataNode, "airline", ElemIdToCodeNative( etAirline, componSets.airline ) );
  if ( !componSets.airp.empty() )
    NewTextChild( dataNode, "airp", ElemIdToCodeNative( etAirp, componSets.airp ) );
  NewTextChild( dataNode, "craft", ElemIdToCodeNative( etCraft, componSets.craft ) );
  AstraLocale::showMessage( "MSG.CHANGED_DATA_COMMIT" );
}

void getSeat_no( int pax_id, bool pr_pnl, const string &format, string &seat_no, string &slayer_type, int &tid )
{
  seat_no.clear();
  TQuery SQry( &OraSession );
  if ( pr_pnl ) {
      SQry.SQLText =
        "SELECT "
        "      crs_pax.tid tid, "
        "      pax_grp.point_dep, "
        "      crs_pax.seat_xname, "
        "      crs_pax.seat_yname, "
        "      crs_pax.seats seats, "
        "      crs_pnr.point_id AS point_id_tlg, "
        "      pax.seats pax_seats, "
        "      pax_grp.status, "
        "      pax.grp_id, "
        "      pax.refuse "
        "FROM crs_pnr,crs_pax,pax,pax_grp "
        "WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pax_id=pax.pax_id(+) AND "
        "      pax.grp_id=pax_grp.grp_id(+)";
  }
  else {
      SQry.SQLText =
        "SELECT "
        "      pax.tid tid, "
        "      pax_grp.point_dep, "
        "      NULL seat_xname, "
        "      NULL seat_yname, "
        "      NULL seats, "
        "      NULL point_id_tlg, "
        "      pax.seats pax_seats, "
        "      pax_grp.status, "
        "      pax.grp_id, "
        "      pax.refuse "
        "FROM pax,pax_grp "
        "WHERE pax.pax_id=:pax_id AND "
        "      pax.grp_id=pax_grp.grp_id";
  };
  SQry.CreateVariable( "pax_id", otInteger, pax_id );
  SQry.Execute();
  if ( SQry.Eof ) {
    tst();
    throw UserException( "MSG.PASSENGER.NOT_FOUND" );
  }
  tid = SQry.FieldAsInteger( "tid" );
  int point_dep = SQry.FieldAsInteger( "point_dep" );
  string xname = SQry.FieldAsString( "seat_xname" );
  string yname = SQry.FieldAsString( "seat_yname" );
  int seats = SQry.FieldAsInteger( "seats" );
  string grp_status = SQry.FieldAsString( "status" );
  int point_id_tlg = SQry.FieldAsInteger( "point_id_tlg" );
  int pax_seats = SQry.FieldAsInteger( "pax_seats" );
  bool pr_grp_id = !SQry.FieldIsNULL( "grp_id" );
  bool pr_refuse = !SQry.FieldIsNULL( "refuse" );
  if ( pr_grp_id && pr_refuse )
    return;
  SQry.Clear();
  SQry.SQLText =
    "BEGIN "
    " IF :mode=0 THEN "
    "  :seat_no:=salons.get_seat_no(:pax_id,:seats,NULL,:grp_status,:point_id,:format,:pax_row); "
    " ELSE "
    "  :seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,:format,:crs_row); "
    " END IF; "
    "END;";
  SQry.CreateVariable( "format", otString, format.c_str() );
  SQry.CreateVariable( "mode", otInteger, (int)!pr_grp_id );
  SQry.CreateVariable( "pax_id", otInteger, pax_id );
  SQry.CreateVariable( "xname", otString, xname );
  SQry.CreateVariable( "yname", otString, yname );
  SQry.CreateVariable( "grp_status", otString, grp_status );
  SQry.CreateVariable( "layer_type", otString, FNull );
  if ( pr_grp_id ) {
    SQry.CreateVariable( "seats", otInteger, pax_seats );
    SQry.CreateVariable( "point_id", otInteger, point_dep );
  }
  else {
    SQry.CreateVariable( "seats", otInteger, seats );
    SQry.CreateVariable( "point_id", otInteger, point_id_tlg );
  }
  SQry.CreateVariable( "pax_row", otInteger, 1 );
  SQry.CreateVariable( "crs_row", otInteger, 1 );
  SQry.CreateVariable( "seat_no", otString, FNull );
    SQry.Execute();
    seat_no = SQry.GetVariableAsString( "seat_no" );
    if ( !seat_no.empty() ) {
        if ( pr_grp_id ) {
            TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
            try {
              slayer_type = ((const TGrpStatusTypesRow&)grp_status_types.get_row("code",grp_status)).layer_type;
            }
            catch(EBaseTableError&){};
        }
        else
            slayer_type = SQry.GetVariableAsString( "layer_type" );
    }
/*!!!djek  if ( slayer_type.empty() )
    throw EXCEPTIONS::Exception( "getSeat_no: slayer_type.empty()" );*/
}

BitSet<SEATS2::TChangeLayerSeatsProps>
     IntChangeSeatsN( int point_id, int pax_id, int &tid, string xname, string yname,
                      SEATS2::TSeatsType seat_type,
                      TCompLayerType layer_type,
                      int time_limit,
                      const BitSet<TChangeLayerFlags> &flags,
                      int comp_crc, int tariff_pax_id,
                      xmlNodePtr reqNode, xmlNodePtr resNode,
                      const std::string& whence  )
{
   BitSet<TChangeLayerSeatsProps> propsSeatsFlags;
   propsSeatsFlags.clearFlags();
   propsSeatsFlags.setFlag(changedOrNotPay);
  if ( flags.isFlag( flSetPayLayer ) &&
       ( seat_type != stSeat || ( layer_type != cltProtBeforePay && layer_type != cltProtAfterPay && layer_type != cltProtSelfCkin ) ) ) {
    throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
  }
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);
  int point_arv = NoExists;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out "
    "FROM points "
    "WHERE points.point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }

  TTripInfo fltInfo( Qry );

  if ( seat_type != SEATS2::stDropseat ) {
    xname = SeatNumber::tryNormalizeLine( xname );
    yname = SeatNumber::tryNormalizeRow( yname );
  }
  Qry.Clear();
  Qry.SQLText =
   "SELECT layer_type, pax_grp.point_arv, pax_grp.status FROM grp_status_types, pax, pax_grp "
   " WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND pax_grp.status=grp_status_types.code ";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    if (DecodePaxStatus(Qry.FieldAsString("status"))==psCrew)
      throw UserException("MSG.CREW.IMPOSSIBLE_CHANGE_SEAT");
    layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
    point_arv  = Qry.FieldAsInteger( "point_arv" );
    if ( flags.isFlag( flSetPayLayer ) ) {
      throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
    }
  }
  else {
    point_arv = SALONS2::getCrsPaxPointArv( pax_id, point_id );
  }

  ProgTrace( TRACE5, "IntChangeSeatsN: point_id=%d, point_arv=%d, pax_id=%d, tid=%d, layer=%s",
            point_id, point_arv, pax_id, tid, EncodeCompLayerType( layer_type ) );

  SALONS2::TSalonList salonList(true);

  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), "", pax_id );
  //!!!  для слоев предварительной рассадки point_arv = NoExists!!!
  // если место у пассажира имеет предварительную рассадку для этого пассажира, и мы еще не спрашивали, то спросить!
  if ( seat_type != SEATS2::stDropseat && !flags.isFlag( flWaitList ) && flags.isFlag( flQuestionReseat ) ) {
    // возможны следующие варианты:
    // 1. пересадка зарегистрированного пассажира
    // 2. предварительная пересадка/рассадка
    // старое место может иметь след. слои:
    // cltProtCkin, cltProtSelfCkin, cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay

    // вычисляем занятое место
    SALONS2::TLayerPrioritySeat layerPrioritySeat = SALONS2::TLayerPrioritySeat::emptyLayer();
    std::set<SALONS2::TPlace*,SALONS2::CompareSeats> seats;
    salonList.getPaxLayer( point_id, pax_id, layerPrioritySeat, seats );
    string used_seat_no;
    if ( layerPrioritySeat.layerType() == layer_type && !seats.empty() ) {
      used_seat_no = (*seats.begin())->yname + (*seats.begin())->xname;
    }
    ProgTrace( TRACE5, "IntChangeSeatsN: occupy point_dep=%d, pax_id=%d, %s, used_seat_no=%s",
               point_id, pax_id, layerPrioritySeat.toString().c_str(), used_seat_no.c_str() );
    // вычисляем предв. места по слоям, для того чтобы предупредить о том, что есть предв. рассадка на тек. месте
    if ( !used_seat_no.empty() && !flags.isFlag( flSetPayLayer ) ) {
      Qry.Clear();
      Qry.SQLText =
        "SELECT first_yname||first_xname pre_seat_no, t.layer_type, "
        "    CASE WHEN clp.airline IS NULL THEN clt.priority ELSE clp.priority END priority "
        "  FROM trip_comp_layers t "
        " JOIN comp_layer_types clt "
        "   on clt.code=t.layer_type "
        " LEFT JOIN comp_layer_priorities clp "
        "   on clp.layer_type=t.layer_type AND "
        "      clp.airline=:airline AND "
        "      :airline_priority=1 "
        " WHERE t.point_id=:point_id AND "
        "       t.layer_type IN (:protckin_layer,:prot_pay1,:prot_pay2,:prot_selfckin) AND "
        "       crs_pax_id=:pax_id "
        " ORDER BY priority ";
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.CreateVariable( "airline", otString, fltInfo.airline );
      Qry.CreateVariable( "pax_id", otInteger, pax_id );
      Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType( cltProtCkin ) );
      Qry.CreateVariable( "prot_pay1", otString, EncodeCompLayerType( cltPNLAfterPay ) );
      Qry.CreateVariable( "prot_pay2", otString, EncodeCompLayerType( cltProtAfterPay ) );
      Qry.CreateVariable( "prot_selfckin", otString, EncodeCompLayerType( cltProtSelfCkin ) );
      Qry.CreateVariable( "airline_priority", otInteger, GetTripSets( tsAirlineCompLayerPriority, fltInfo )?1:0 );
      Qry.Execute();
      int priority = -1;
      for ( ;!Qry.Eof; Qry.Next() ) {
        if ( priority == -1 ) {
          priority = Qry.FieldAsInteger( "priority" );
        }
        if ( priority != Qry.FieldAsInteger( "priority" ) ) {
          break;
        }
        if ( used_seat_no == Qry.FieldAsString( "pre_seat_no" ) ) {
          ProgTrace( TRACE5, "pax_id=%d,point_id=%d,used_seat_no=%s,pre_seat_no=%s",
                      pax_id, point_id, used_seat_no.c_str(), Qry.FieldAsString( "pre_seat_no" ) );
          if ( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) == cltProtCkin ) {
            NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PRESEAT_SEATS.RESEAT") );
          }
          else
            if ( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) != cltProtSelfCkin ) { //pay layers
              NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PAID_SEATS.RESEAT"));
            }
          return propsSeatsFlags;
        }
      }
    }
  }

  TSalonChanges seats;
  BitSet<TChangeLayerProcFlag> procFlags;
  procFlags.clearFlags();
  if ( flags.isFlag( flSetPayLayer ) ) {
    procFlags.setFlag( procPaySeatSet );
  }
  if ( flags.isFlag( flWaitList ) ) {
    procFlags.setFlag( procWaitList );
  }
  if ( flags.isFlag( flSyncCabinClass ) )  {
    procFlags.setFlag( procSyncCabinClass );
  }
  try {
    propsSeatsFlags = SEATS2::ChangeLayer( salonList, layer_type, time_limit, point_id, pax_id, tid, xname, yname, seat_type, procFlags, whence, reqNode, resNode );
    if ( TReqInfo::Instance()->client_type != ctTerm || resNode == NULL )
        return propsSeatsFlags; // web-регистрация
    if ( propsSeatsFlags.isFlag(propRFISCQuestion) ) {
      return propsSeatsFlags;
    }
    salonList.JumpToLeg( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ) );
    int ncomp_crc = CompCheckSum::keyFromDB( point_id ).total_crc32; //берем из БД, не надо заново расчитывать
    //int ncomp_crc = SALONS2::CompCheckSum::calcFromDB( point_id ).total_crc32;
    if ( (comp_crc != 0 && ncomp_crc != 0 && comp_crc != ncomp_crc) ) { //update
      throw UserException( "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH" );
    }

    SALONS2::TSalonList NewSalonList(true);
    NewSalonList.ReadFlight( salonList.getFilterRoutes(), salonList.getFilterClass(), tariff_pax_id );
    SALONS2::getSalonChanges( salonList._seats, salonList.isCraftLat(), NewSalonList._seats, NewSalonList.isCraftLat(), NewSalonList.getRFISCMode(), seats );
    ProgTrace( TRACE5, "salon changes seats.size()=%zu", seats.size() );
    string seat_no, slayer_type;
    if ( layer_type == cltProtCkin ||
         layer_type == cltProtAfterPay ||
         layer_type == cltPNLAfterPay ||
         layer_type == cltProtSelfCkin )
      getSeat_no( pax_id, true, string("_seats"), seat_no, slayer_type, tid );
    else
      getSeat_no( pax_id, false, string("one"), seat_no, slayer_type, tid );

    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
      NewTextChild( dataNode, "seat_no", seat_no );
      NewTextChild( dataNode, "layer_type", slayer_type );
    }
    SALONS2::BuildSalonChanges( dataNode, point_id, seats, true, NewSalonList.pax_lists );
    bool pr_show_error_message = false;
    if ( flags.isFlag( flWaitList ) ) {
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      SALONS2::TSalonPassengers passengers;
      NewSalonList.getPassengers( passengers, flags );
      passengers.BuildWaitList( point_id, NewSalonList.getSeatDescription(), dataNode );
      if ( !passengers.isWaitList( point_id ) ) {
        AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
        pr_show_error_message = true;
      }
    }
    std::set<TCompLayerType> checkinLayers { cltGoShow, cltTranzit, cltCheckin, cltTCheckin };
    if (!pr_show_error_message &&
        TReqInfo::Instance()->client_type == ctTerm &&
        seat_type == SEATS2::stReseat && //пересадка
        propsSeatsFlags.isFlag(propRFISC) && //!!!
        !procFlags.isFlag( procWaitList ) && //  не ругаемся, если пересадка идет с ЛО
        checkinLayers.find( layer_type ) != checkinLayers.end() ) { // уже зарегистрированного
      TTripInfo fltInfo;
      if (!fltInfo.getByPointId(point_id))
        throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
      if ( GetTripSets(tsReseatOnRFISC,fltInfo) ) {
        TRemGrp remGrp;
        remGrp.Load(retFORBIDDEN_FREE_SEAT, point_id);
        std::multiset<CheckIn::TPaxRemItem> rems;
        CheckIn::LoadPaxRem( pax_id, rems );
        std::vector<std::string> specRemarks;
        for ( const auto & r : rems ) {
          if ( remGrp.exists( r.code ) &&
               find( specRemarks.begin(), specRemarks.end(), r.code ) == specRemarks.end() ) {
            specRemarks.emplace_back( r.code );
            LogTrace(TRACE5)<<r.code;
          }
        }
        std::set<TPlace*,CompareSeats> _seats;
        SALONS2::TLayerPrioritySeat _layerPrioritySeat = SALONS2::TLayerPrioritySeat::emptyLayer();
        NewSalonList.getPaxLayer(point_id, pax_id, _layerPrioritySeat, _seats, false );
        bool show_msg = true;
        for ( const auto & s : _seats ) {
          std::vector<TSeatRemark> vremarks;
          s->GetRemarks( point_id, vremarks );
          for ( const auto r : vremarks ) {
            if (  !r.pr_denial &&
                  std::find( specRemarks.begin(), specRemarks.end(), r.value ) != specRemarks.end() ) {
              show_msg = false;
              break;
            }
          }
        }
        if ( show_msg )
          AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_WARNING_RFISC" );
      }
    }
  }
  catch( UserException& ue ) {
    tst();
    if ( (TReqInfo::Instance()->client_type != ctTerm &&
          ue.getLexemaData().lexema_id != "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH") || resNode == NULL )
        throw;
    xmlNodePtr dataNode = GetNode( "data", resNode );
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
    dataNode = NewTextChild( resNode, "data" );

    if ( !TReqInfo::Instance()->desk.compatible(RFISC_VERSION) ) {
      salonList.ReadFlight( salonList.getFilterRoutes(), salonList.getFilterClass(), tariff_pax_id );
    }
    SALONS2::GetTripParams( point_id, dataNode );
    salonList.JumpToLeg( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ) );
    int comp_id;
    salonList.Build( NewTextChild( dataNode, "salons" ) ); //crc32 из БД
    comp_id = salonList.getCompId();
    if ( flags.isFlag( flWaitList ) ) {
      SALONS2::TSalonPassengers passengers;
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      salonList.getPassengers( passengers, flags );
      passengers.BuildWaitList( point_id, salonList.getSeatDescription(), dataNode );
    }
    if ( comp_id > 0 && isComponSeatsNoChanges( fltInfo ) &&
         TReqInfo::Instance()->client_type == ctTerm ) { //строго завязать базовые компоновки с назначенными на рейс
      componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
    }
    showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
  return propsSeatsFlags;
}


void ConditionsForReseat(xmlNodePtr reqNode, xmlNodePtr resNode)
{

  class CompleteWithError {};
  bool reset=false;
  tst();
  try {
    xmlNodePtr dataNode = nullptr;
    for(int pass=1;pass<=1;pass++)
    {
      switch(pass)
      {
        case 1: if (GetNode("confirmations/msg1",reqNode)==NULL)
                {
                  tst();
                  if ( dataNode == nullptr ) {
                    dataNode =  NewTextChild(resNode,"data");
                  }
                  xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                  NewTextChild(confirmNode,"reset",(int)reset);
                  NewTextChild(confirmNode,"type","msg1");
                  NewTextChild(confirmNode,"dialogMode","");
                  ostringstream msg;
                  msg << getLocaleText("MSG.PASSENGER.RESEAT_TO_RFISC") << endl
                      << getLocaleText("QST.CONTINUE_RESEAT");
                  NewTextChild(confirmNode,"message",msg.str());
                  tst();
                  throw CompleteWithError();
                 }
                else
                  if ( NodeAsInteger("confirmations/msg1",reqNode) != 7 ) {
                    throw UserException("MSG.PASSENGER.RESEAT_BREAK");
                  }

                 break;
      }
    }
  }
  catch( CompleteWithError &er ) {
    return;
  }
}


static void ChangeIatciSeats(xmlNodePtr reqNode)
{
    ProgTrace(TRACE3, "Query iatci seat change");
    IatciInterface::UpdateSeatRequest(reqNode);
    return AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR");
}

void CheckResetLayer( const std::string& airline,
                      const BASIC_SALONS::TCompLayerTypes::Enum& flag,
                      TCompLayerType &layer_type, int crs_pax_id )
{
  if ( layer_type != cltProtCkin ) {
    return;
  }
  ProgTrace( TRACE5, "CheckResetLayer input layer_type=%s, airline=%s", EncodeCompLayerType(layer_type), airline.c_str() );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "DECLARE "
    "vseat_xname crs_pax.seat_xname%TYPE; "
    "vseat_yname crs_pax.seat_yname%TYPE; "
    "vseats      crs_pax.seats%TYPE; "
    "vpoint_id   crs_pnr.point_id%TYPE; "
    "BEGIN "
    "  SELECT crs_pax.seat_xname, crs_pax.seat_yname, crs_pax.seats, crs_pnr.point_id "
    "  INTO vseat_xname, vseat_yname, vseats, vpoint_id "
    "  FROM crs_pnr,crs_pax "
    "  WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:crs_pax_id AND crs_pax.pr_del=0; "
    "  :crs_seat_no:=salons.get_crs_seat_no(:crs_pax_id, vseat_xname, vseat_yname, vseats, vpoint_id, :layer_type, 'one', 1); "
    "EXCEPTION "
    "  WHEN NO_DATA_FOUND THEN NULL; "
    "END;";
  Qry.CreateVariable("crs_pax_id", otInteger, crs_pax_id);
  Qry.CreateVariable("layer_type", otString, FNull);
  Qry.CreateVariable("crs_seat_no", otString, FNull);
  Qry.Execute();
  TCompLayerType layer_type_out = DecodeCompLayerType( Qry.GetVariableAsString( "layer_type" ) );
  if ( layer_type_out == cltProtAfterPay || layer_type_out == cltPNLAfterPay ) {
    //проверка прав изменения платного слоя
    TReqInfo *reqInfo = TReqInfo::Instance();
    if ( !reqInfo->user.access.check_profile_by_crs_pax(crs_pax_id, 193) ) {
      throw UserException( "MSG.SEATS.CHANGE_PAY_SEATS_DENIED" );
    }
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( airline, layer_type_out ), flag ) <
         BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( airline, layer_type ), flag ) ) {
      layer_type = layer_type_out;
    }
  }
  ProgTrace( TRACE5, "CheckResetLayer return layer_type=%s", EncodeCompLayerType(layer_type) );
}

void ChangeSeats( xmlNodePtr reqNode, xmlNodePtr resNode, SEATS2::TSeatsType seat_type )
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  if(point_id < 0) {
      return ChangeIatciSeats(reqNode);
  }
  TTripInfo fltInfo;
  fltInfo.getByPointId( point_id );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  int comp_crc = NodeAsInteger( "comp_crc", reqNode, 0 );
  int tariff_pax_id = NodeAsInteger( "tariff_pax_id", reqNode, NoExists );
  ProgTrace( TRACE5, "ChangeSeats: point_id=%d, comp_crc=%d, pax_id=%d, tariff_pax_id=%d",
             point_id, comp_crc, pax_id, tariff_pax_id );
  string xname;
  string yname;
  if ( seat_type != SEATS2::stDropseat ) {
    xname = NodeAsString( "xname", reqNode );
    yname = NodeAsString( "yname", reqNode );
  }
  BitSet<SEATS2::TChangeLayerFlags> change_layer_flags;
  if ( GetNode( "waitlist", reqNode ) ) {
    change_layer_flags.setFlag( flWaitList );
  }
  if (  GetNode( "question_reseat", reqNode ) ) {
    change_layer_flags.setFlag( flQuestionReseat );
  }
  // если пришел слой cltProtCkin а есть более приоритетный clt..AfterPay то надо подменить слой
  TCompLayerType layer_type = DecodeCompLayerType( NodeAsString( "layer", reqNode, "" ) );
  BASIC_SALONS::TCompLayerTypes::Enum flag = (GetTripSets( tsAirlineCompLayerPriority, fltInfo )?
                                                  BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                                  BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
  CheckResetLayer( fltInfo.airline, flag, layer_type, pax_id );
  IntChangeSeatsN( point_id, pax_id, tid, xname, yname,
                   seat_type,
                   layer_type,
                   NoExists,
                   change_layer_flags,
                   comp_crc, tariff_pax_id,
                   reqNode, resNode,
                   __func__ );
}

void SalonFormInterface::DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
     // удаление мест пассажира
  ChangeSeats( reqNode, resNode, SEATS2::stDropseat );
}

void SalonFormInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeSeats( reqNode, resNode, SEATS2::stReseat);
}

void SalonFormInterface::DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int point_arv = NodeAsInteger( "point_arv", reqNode, NoExists );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool pr_update_salons = GetNode( "update_salons", reqNode );
  int comp_crc = NodeAsInteger( "comp_crc", reqNode, 0 );
  int tariff_pax_id = NodeAsInteger( "tariff_pax_id", reqNode, NoExists );
  TQuery Qry( &OraSession );
  ProgTrace( TRACE5, "SalonsInterface::DeleteProtCkinSeat, point_id=%d, pax_id=%d, tid=%d, pr_update_salons=%d, tariff_pax_id=%d, crc=%d",
             point_id, pax_id, tid, pr_update_salons, tariff_pax_id, comp_crc );

  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), "", pax_id );
  TSalonChanges seats;

  try {
    SEATS2::ChangeLayer( salonList, cltProtCkin, NoExists, point_id, pax_id, tid, "", "", SEATS2::stDropseat, BitSet<TChangeLayerProcFlag>(), __func__, NULL, NULL );
    if ( pr_update_salons ) {
      if ( tariff_pax_id != ASTRA::NoExists ) {
        salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), "", tariff_pax_id );
      }
      int ncomp_crc = SALONS2::CompCheckSum::keyFromDB( point_id ).total_crc32;
      if ( (comp_crc != 0 && ncomp_crc != 0 && comp_crc != ncomp_crc) ) { //update
        throw UserException( "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH" );
      }
      SALONS2::getSalonChanges( salonList, pax_id, seats );
    }
    string seat_no, slayer_type;
    getSeat_no( pax_id, true, string("_seats"), seat_no, slayer_type, tid );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
      NewTextChild( dataNode, "seat_no", seat_no );
      NewTextChild( dataNode, "layer_type", slayer_type );
    }
    if ( pr_update_salons ) {
      std::map<int,SALONS2::TPaxList> pax_lists;
      SALONS2::BuildSalonChanges( dataNode, point_id, seats, false, pax_lists );
    }
  }
  catch( UserException& ue ) {
    if ( TReqInfo::Instance()->client_type != ctTerm &&
         ue.getLexemaData().lexema_id != "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH"  )
        throw; // web-регистрация
    if ( pr_update_salons ) {
      xmlNodePtr dataNode = GetNode( "data", resNode );
      if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
        xmlUnlinkNode( dataNode );
        xmlFreeNode( dataNode );
      }
      dataNode = NewTextChild( resNode, "data" );
      xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
      SALONS2::GetTripParams( point_id, dataNode );
      int comp_id;
      if ( !TReqInfo::Instance()->desk.compatible(RFISC_VERSION) ) {
        salonList.ReadFlight( salonList.getFilterRoutes(), salonList.getFilterClass(), tariff_pax_id );
      }
      salonList.Build( salonsNode );
      comp_id = salonList.getCompId();
      if ( comp_id > 0 ) { //строго завязать базовые компоновки с назначенными на рейс
        TTripInfo info;
        info.getByPointId( point_id );
        if ( isComponSeatsNoChanges( info ) ) {
          componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
        }
      }
    }
    showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
}

void SalonFormInterface::WaitList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "trip_id", reqNode );
  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  bool pr_filter = GetNode( "filter", reqNode );
  bool pr_salons = GetNode( "salons", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::WaitList, point_id=%d", point_id );
  TQuery Qry( &OraSession );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), "", NoExists );
  SALONS2::TSalonPassengers passengers;
  SALONS2::TGetPassFlags flags;
  flags.setFlag( SALONS2::gpPassenger );
  flags.setFlag( SALONS2::gpWaitList );
  salonList.getPassengers( passengers, flags );
  passengers.BuildWaitList( point_id, salonList.getSeatDescription(), dataNode );
  if ( pr_filter ) {
    Qry.SQLText =
      "SELECT code, layer_type FROM grp_status_types WHERE layer_type IS NOT NULL";
    Qry.Execute();
    dataNode = NewTextChild( dataNode, "filter" );
    while ( !Qry.Eof ) {
        xmlNodePtr lNode = NewTextChild( dataNode, "status" );
        SetProp( lNode, "code", Qry.FieldAsString( "code" ) );
        SetProp( lNode, "name", ElemIdToNameLong(etGrpStatusType,Qry.FieldAsString( "code" )) );
        SetProp( lNode, "layer_type", Qry.FieldAsString( "layer_type" ) );
        Qry.Next();
    }
  }
  if ( pr_salons ) {
    xmlNodePtr dataNode = GetNode( "data", resNode );
    if ( !dataNode ) {
      dataNode = NewTextChild( resNode, "data" );
    }
    int comp_id;
    salonList.Build( NewTextChild( dataNode, "salons" ) );
    comp_id = salonList.getCompId();
    SALONS2::GetTripParams( point_id, dataNode );
    if ( comp_id > 0 ) {
      TTripInfo info;
      info.getByPointId( point_id );
      if ( isComponSeatsNoChanges( info ) ) { //строго завязать базовые компоновки с назначенными на рейс
        componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
      }
    }
  }
}

void SalonFormInterface::AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "trip_id", reqNode );
  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  ProgTrace( TRACE5, "AutoSeats: point_id=%d", point_id );
  bool pr_waitlist = GetNode( "waitlist", reqNode );
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);
    TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo info( Qry );
  SEATS2::TPassengers p;
  SALONS2::TSalonList salonList;
  SALONS2::TSalonPassengers passengers;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), "", NoExists );
  TRemGrp remGrp;
  remGrp.Load(retFORBIDDEN_FREE_SEAT,point_id);
  SALONS2::TGetPassFlags flags;
  flags.setFlag( SALONS2::gpPassenger );
  flags.setFlag( SALONS2::gpWaitList );
  salonList.getPassengers( passengers, flags );
  if ( !passengers.isWaitList( point_id ) ) {
    throw UserException( "MSG.SEATS.ALL_PASSENGERS_PLANED" );
  }
  int comp_id;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  bool pr_notchangecraft = isComponSeatsNoChanges( info );
  try {
    SALONS2::TSalonPassengers::const_iterator ipasses = passengers.find( point_id );
    if ( ipasses != passengers.end() ) {
      SEATS2::AutoReSeatsPassengers( salonList, ipasses->second,
                                     SEATS2::GetSeatAlgo( Qry, info.airline, info.flt_no, info.airp ),
                                     remGrp );
    }
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    int comp_id;
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), "", NoExists );
    salonList.Build( salonsNode );
    comp_id = salonList.getCompId();
    if ( pr_waitlist ) {
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      salonList.getPassengers( passengers, flags );
      if ( passengers.isWaitList( point_id ) ) {
        AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
      }
      else {
        AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      }
      passengers.BuildWaitList( point_id, salonList.getSeatDescription(), dataNode );
    }
    if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
      componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
    }
  }
  catch( UserException& ue ) {
    if ( TReqInfo::Instance()->client_type != ctTerm )
        throw; // web-регистрация
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
    dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), "", NoExists );
    salonList.Build( salonsNode );
    comp_id = salonList.getCompId();
    if ( pr_waitlist ) {
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      salonList.getPassengers( passengers, flags );
      if ( passengers.isWaitList( point_id ) ) {
        AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
      }
      else {
        AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      }
      passengers.BuildWaitList( point_id, salonList.getSeatDescription(), dataNode );
    }
    if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
      componPropCodes::Instance()->buildSections( comp_id, TReqInfo::Instance()->desk.lang, dataNode, TAdjustmentRows().get( salonList ) );
    }
    showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
}

void SalonFormInterface::Tranzit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SALONS2::TSalonList salonList;
  SALONS2::TSalonPassengers passengers;
  SALONS2::TGetPassFlags flags;
  string flight = NodeAsString( "flight", reqNode );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id FROM points WHERE airline||flt_no||airp||'/'||to_char(scd_out,'DD') = :flight AND to_char(time_out,'MM.YYYY')=to_char(sysdate,'MM.YYYY')";
  Qry.CreateVariable( "flight", otString, flight );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw UserException( "flight not found" );
  }
    int point_dep = Qry.FieldAsInteger( "point_id" );
    ProgTrace( TRACE5, "point_dep=%d", point_dep );
    if ( NodeAsInteger( "waitlist_key", reqNode ) == 1 ) {
    flags.setFlag( SALONS2::gpWaitList );
  }
    if ( NodeAsInteger( "tranzit_key", reqNode ) == 1 ) {
    flags.setFlag( SALONS2::gpTranzits );
  }
    if ( NodeAsInteger( "infants_key", reqNode ) == 1 ) {
    flags.setFlag( SALONS2::gpInfants );
  }

  Qry.Clear();
  Qry.SQLText =
    "SELECT airp FROM points WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  TPassSeats seats;
  SALONS2::TWaitListReason waitListReason;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ), "", NoExists );
    salonList.getPassengers( passengers, flags );
    xmlNodePtr passNode = NewTextChild( resNode, "passses" );
    SALONS2::TSalonPassengers::iterator idep_pass = passengers.find( point_dep );
    if ( idep_pass != passengers.end() ) {
      for ( SALONS2::TIntArvSalonPassengers::iterator ipass_arv=idep_pass->second.begin();
          ipass_arv!=idep_pass->second.end(); ipass_arv++ ) {
      xmlNodePtr pointNode = NewTextChild( passNode, "point" );
      SetProp( pointNode, "point_id", ipass_arv->first );
        for ( SALONS2::TIntClassSalonPassengers::iterator ipass_class=ipass_arv->second.begin();
              ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
          for ( SALONS2::TIntStatusSalonPassengers::iterator ipass_status=ipass_class->second.begin();
                ipass_status!=ipass_class->second.end(); ipass_status++ ) {
            for ( std::set<SALONS2::TSalonPax,SALONS2::ComparePassenger>::iterator i=ipass_status->second.begin();
                  i!=ipass_status->second.end(); i++ ) {
              xmlNodePtr node = NewTextChild( pointNode, "pass" );
              NewTextChild( node, "pax_id", i->pax_id );
              NewTextChild( node, "point_dep", i->point_dep );
              Qry.SetVariable( "point_id", i->point_dep );
              Qry.Execute();
              NewTextChild( node, "airp_dep", Qry.FieldAsString( "airp" ) );
              NewTextChild( node, "point_arv", i->point_arv );
              Qry.SetVariable( "point_id", i->point_arv );
              Qry.Execute();
              NewTextChild( node, "airp_arv", Qry.FieldAsString( "airp" ) );
              NewTextChild( node, "class", i->cabin_cl );
              NewTextChild( node, "pers_type", EncodePerson( i->pers_type ) );
              NewTextChild( node, "seats", (int)i->seats );
              NewTextChild( node, "reg_no", i->reg_no );
              NewTextChild( node, "pr_infant", i->pr_infant != ASTRA::NoExists );
              NewTextChild( node, "pr_web", i->pr_web );
              NewTextChild( node, "grp_status", i->grp_status );
              NewTextChild( node, "name", i->name + " " + i->surname );
              i->get_seats( waitListReason, seats );
              NewTextChild( node, "pr_waitlist", waitListReason.status != SALONS2::layerValid );
              NewTextChild( node, "seat_no", i->seat_no( "list", false, waitListReason ) );
            }
          }
        }
      }
      for ( SALONS2::TIntArvSalonPassengers::iterator ipass_arv=idep_pass->second.infants.begin();
          ipass_arv!=idep_pass->second.infants.end(); ipass_arv++ ) {
      xmlNodePtr pointNode = NewTextChild( passNode, "point" );
      SetProp( pointNode, "point_id", ipass_arv->first );
        for ( SALONS2::TIntClassSalonPassengers::iterator ipass_class=ipass_arv->second.begin();
              ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
          for ( SALONS2::TIntStatusSalonPassengers::iterator ipass_status=ipass_class->second.begin();
                ipass_status!=ipass_class->second.end(); ipass_status++ ) {
            for ( std::set<SALONS2::TSalonPax,SALONS2::ComparePassenger>::iterator i=ipass_status->second.begin();
                  i!=ipass_status->second.end(); i++ ) {
              xmlNodePtr node = NewTextChild( pointNode, "pass" );
              NewTextChild( node, "pax_id", i->pax_id );
              NewTextChild( node, "point_dep", i->point_dep );
              Qry.SetVariable( "point_id", i->point_dep );
              Qry.Execute();
              NewTextChild( node, "airp_dep", Qry.FieldAsString( "airp" ) );
              NewTextChild( node, "point_arv", i->point_arv );
              Qry.SetVariable( "point_id", i->point_arv );
              Qry.Execute();
              NewTextChild( node, "airp_arv", Qry.FieldAsString( "airp" ) );
              NewTextChild( node, "class", i->cabin_cl );
              NewTextChild( node, "pers_type", EncodePerson( i->pers_type ) );
              NewTextChild( node, "seats", (int)i->seats );
              NewTextChild( node, "reg_no", i->reg_no );
              NewTextChild( node, "pr_infant", i->pr_infant != ASTRA::NoExists );
              NewTextChild( node, "pr_web", i->pr_web );
              NewTextChild( node, "grp_status", i->grp_status );
              NewTextChild( node, "name", i->name + " " + i->surname );
              i->get_seats( waitListReason, seats );
              NewTextChild( node, "pr_waitlist", 0 );
              NewTextChild( node, "seat_no", "" );
            }
          }
        }
      }


    }
}

void SalonFormInterface::ShowRemote(xmlNodePtr resNode, const iatci::dcrcka::Result& res)
{
    LogTrace(TRACE3) << __FUNCTION__;
    xmlNodePtr dataNode = newChild(resNode, "data");
    iatci::iatci2xmlSmp(dataNode, res);
}

void SalonFormInterface::ReseatRemote(xmlNodePtr resNode,
                                      const iatci::Seat& oldSeat,
                                      const iatci::Seat& newSeat,
                                      const iatci::dcrcka::Result& res)
{
    LogTrace(TRACE3) << __FUNCTION__;
    xmlNodePtr dataNode = newChild(resNode, "data");
    NewTextChild(dataNode, "tid",        0);
    NewTextChild(dataNode, "seat_no",    newSeat.toStr());
    NewTextChild(dataNode, "layer_type", "CHECKIN");
    iatci::iatci2xmlSmpUpd(dataNode, res, oldSeat, newSeat);
}

SalonFormInterface* SalonFormInterface::instance()
{
    static SalonFormInterface* inst = 0;
    if(!inst) {
        inst = new SalonFormInterface();
    }
    return inst;
}

void trace( int pax_id, int grp_id, int parent_pax_id, int crs_pax_id, const std::string &pers_type, int seats )
{
//  ProgTrace( TRACE5, "pax_id=%d, grp_id=%d, parent_pax_id=%d, crs_pax_id=%d, pers_type=%s, seats=%d",
//             pax_id, grp_id, parent_pax_id, crs_pax_id, pers_type.c_str(), seats );
}
