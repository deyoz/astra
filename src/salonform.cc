#include <stdlib.h>
#include "salonform.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "images.h"
#include "points.h"
#include "salons.h"
#include "seats.h"
#include "seats_utils.h"
#include "convert.h"
#include "astra_misc.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "term_version.h"
#include "alarms.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace AstraLocale;
using namespace ASTRA;

//new terminal

void BuildCompSections( xmlNodePtr dataNode, const vector<SALONS2::TCompSection> &CompSections )
{
  if ( !CompSections.empty() ) {
    xmlNodePtr n = NewTextChild( dataNode, "CompSections" );
    for ( vector<SALONS2::TCompSection>::const_iterator i=CompSections.begin(); i!=CompSections.end(); i++ ) {
      xmlNodePtr cnode = NewTextChild( n, "section", i->name );
      SetProp( cnode, "FirstRowIdx", i->getFirstRow() );
      SetProp( cnode, "LastRowIdx", i->getLastRow() );
    }
  }
}

void ReadCompSections( int comp_id, vector<SALONS2::TCompSection> &CompSections )
{
  CompSections.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT name, first_rownum, last_rownum FROM comp_sections WHERE comp_id=:comp_id "
    "ORDER BY first_rownum";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    SALONS2::TCompSection cs;
    cs.name = Qry.FieldAsString( "name" );
    cs.setSectionRows( Qry.FieldAsInteger( "first_rownum" ),
                       Qry.FieldAsInteger( "last_rownum" ) );
    CompSections.push_back( cs );
    Qry.Next();
  }
}

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
    Qry.SQLText =
      "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    TTripInfo info( Qry );
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), SALONS2::rfTranzitVersion, "" );
    if ( !(salonList.getCompId() > 0 && GetTripSets( tsCraftNoChangeSections, info )) ) {
      return;
    }
    ReadCompSections( salonList.getCompId(), compSections );
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
      BASIC_SALONS::TCompLayerType layer_elem;
      if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
           layer_elem.getOccupy() ) {
        layersSeats[ layer_type ].clear();
      }
    }
    for ( vector<SALONS2::TCompSection>::iterator i=compSections.begin(); i!=compSections.end(); i++ ) {
      SALONS2::TCompSectionLayers compSectionLayers;
      compSectionLayers.layersSeats = layersSeats;
      getLayerPlacesCompSection( salonList, *i, compSectionLayers.layersSeats, i->seats );
      compSectionLayers.compSection = *i;
      TZoneOccupiedSeats zs;
      zs.name = i->name;
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
  if ( SALONS2::isTranzitSalons( point_id ) ) {
    ZoneLoadsTranzitSalons( point_id,
                            only_checkin_layers,
                            zones,
                            CompSectionsLayers,
                            compSections );
    return;
  }
    SALONS2::TSalons SalonsTmp( point_id, SALONS2::rTripSalons );
    TQuery Qry(&OraSession);
    try {
	      Qry.SQLText =
	        "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
	      TTripInfo info( Qry );
	      tst();
        SalonsTmp.Read( drop_not_used_pax_layers ); //!!!
        if ( SalonsTmp.comp_id > 0 && GetTripSets( tsCraftNoChangeSections, info ) ) { //!!!строго завязать базовые компоновки с назначенными на рейс
            tst();
            ReadCompSections( SalonsTmp.comp_id, compSections );
            TQuery Qry(&OraSession);
            Qry.SQLText = "select layer_type from grp_status_types WHERE layer_type IS NOT NULL";
            Qry.Execute();
            std::map<ASTRA::TCompLayerType,SALONS2::TPlaces> layersSeats, checkinLayersSeats;
            for(; not Qry.Eof; Qry.Next())
                checkinLayersSeats[DecodeCompLayerType(Qry.FieldAsString("layer_type"))].clear();
            Qry.Clear();
            Qry.SQLText =
              "SELECT code from comp_layer_types WHERE pr_occupy = 1";
            Qry.Execute();
            for(; not Qry.Eof; Qry.Next())
              layersSeats[DecodeCompLayerType(Qry.FieldAsString("code"))].clear();
            for ( vector<SALONS2::TCompSection>::iterator i=compSections.begin(); i!=compSections.end(); i++ ) {
                SALONS2::TCompSectionLayers compSectionLayers;
                compSectionLayers.layersSeats = layersSeats;
                getLayerPlacesCompSection( SalonsTmp, *i, only_high_layer, compSectionLayers.layersSeats, i->seats );
                compSectionLayers.compSection = *i;
                TZoneOccupiedSeats zs;
                zs.name = i->name;
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
    } catch(exception &E) {
        ProgTrace(TRACE5, "ZoneLoads failed, so result would be empty: %s", E.what());
    } catch(...) {
        ProgTrace(TRACE5, "ZoneLoads failed, so result would be empty");
    }
}

void WriteCompSections( int id, const vector<SALONS2::TCompSection> &CompSections )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "DELETE comp_sections WHERE comp_id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  ProgTrace( TRACE5, "RowCount=%d", Qry.RowCount() );
  bool pr_exists = Qry.RowCount();
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO comp_sections(comp_id,name,first_rownum,last_rownum) VALUES(:id,:name,:first_rownum,:last_rownum)";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "first_rownum", otInteger );
  Qry.DeclareVariable( "last_rownum", otInteger );
  PrmLexema lexema("is_sect_del", "");
  if ( CompSections.empty() && pr_exists )
    lexema.ChangeLexemaId("EVT.DELETE_LUGGAGE_SECTIONS");
  PrmEnum prmenum("sect", " ");
  bool empty = true;
  for ( vector<SALONS2::TCompSection>::const_iterator i=CompSections.begin(); i!=CompSections.end(); i++ ) {
    PrmLexema new_lexema("", "EVT.SECTIONS");
    new_lexema.prms << PrmSmpl<std::string>("name", i->name) << PrmSmpl<int>("FirstRow", i->getFirstRow())
                    << PrmSmpl<int>("LastRow", i->getLastRow());//!!!ANNA ряды выводятся неправильно
    prmenum.prms << new_lexema;
    Qry.SetVariable( "name", i->name );
    Qry.SetVariable( "first_rownum", i->getFirstRow() + 1 );
    Qry.SetVariable( "last_rownum", i->getLastRow() + 1 );
    Qry.Execute();
    empty = false;
  }
  if (CompSections.empty() && pr_exists && empty)
    TReqInfo::Instance()->LocaleToLog("EVT.DELETE_LUGGAGE_SECTIONS", evtComp, id);
  else
    TReqInfo::Instance()->LocaleToLog("EVT.ASSIGNE_LUGGAGE_SECTIONS", LEvntPrms() << lexema << prmenum, evtComp, id);
}

bool filterCompons( const string &airline, const string &airp )
{
	TReqInfo *r = TReqInfo::Instance();
  return
       ( (int)airline.empty() + (int)airp.empty() == 1 &&
 		   ((
 		     r->user.user_type == utAirline &&
 		     find( r->user.access.airlines.begin(),
 		           r->user.access.airlines.end(), airline ) != r->user.access.airlines.end()
  		  )
  		  ||
  		  (
  		    r->user.user_type == utAirport &&
  		    ( ( airp.empty() && ( r->user.access.airlines.empty() ||
  		                          find( r->user.access.airlines.begin(),
  		                                r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() ) ) ||
  		        find( r->user.access.airps.begin(),
  		              r->user.access.airps.end(), airp ) != r->user.access.airps.end() )
  		   )
  		   ||
  		   (
  		     r->user.user_type == utSupport &&
  		     ( airp.empty() ||
  		       r->user.access.airps.empty() ||
   		       find( r->user.access.airps.begin(),
    	             r->user.access.airps.end(), airp ) != r->user.access.airps.end() ) &&
  		     ( airline.empty() ||
  		       r->user.access.airlines.empty() ||
  		       find( r->user.access.airlines.begin(),
  		             r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() )
  		   ))
  		 );
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
    		   filterCompons( comp.airline, comp.airp ) ) ) {
      setCompName( comp );
      comps.push_back( comp );
    }
  	Qry.Next();
  }
}

void getFlightBaseCompRef( int point_id, bool onlySetComp, bool use_filter,
                           const string &trip_airline,
                           const string &trip_craft,
                           const string &trip_bort,
                           vector<TShowComps> &comps )
{
  TQuery Qry( &OraSession );
  string sql;
  if ( !onlySetComp )
    sql =
      "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "
      "       comps.descr,0 as pr_comp, comps.airline, comps.airp "
      " FROM comps, points "
      "WHERE points.craft = comps.craft AND points.point_id = :point_id "
      " UNION ";
  sql +=
    "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "
    "       comps.descr,1 as pr_comp, null airline, null airp "
    " FROM comps, points, trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.craft = comps.craft AND points.point_id = :point_id AND "
    "      trip_sets.comp_id = comps.comp_id "
    "ORDER BY craft, bort, classes, descr";
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
        if ( !comp.craft.empty() && comp.craft == trip_craft &&
             !comp.bort.empty() && comp.bort == trip_bort )
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
   	if ( !use_filter ||
         ( comp.comp_type != ctBase && comp.comp_type != ctBaseBort ) || /* поиск компоновки только по компоновкам нужной А/К или портовым компоновкам */
    		 ( ( comp.airline.empty() || trip_airline == comp.airline ) &&
    		   filterCompons( comp.airline, comp.airp ) ) ) {
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

void SalonFormInterface::Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int point_id = NodeAsInteger( "trip_id", reqNode );
  bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( point_id );
  ProgTrace(TRACE5, "SalonFormInterface::Show point_id=%d, isTranzitSalonsVersion=%d", point_id, isTranzitSalonsVersion );
  
  int point_arv = NoExists;
  int pax_id = NoExists;
  if ( isTranzitSalonsVersion ) {
    xmlNodePtr tmpNode;
    tmpNode = GetNode( "point_arv", reqNode );
    if ( tmpNode ) {
      point_arv = NodeAsInteger( tmpNode );
    }
    tmpNode = GetNode( "pax_id", reqNode );
    if ( tmpNode ) {
      pax_id = NodeAsInteger( tmpNode );
    }
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
  	  "SELECT point_num,first_point,pr_tranzit,pr_del,scd_out, "
  	  "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
  	  "       airline_fmt,suffix_fmt,airp_fmt,"
      "       bort,airline,flt_no,suffix,airp,craft,NVL(comp_id,-1) comp_id "
      " FROM points, trip_sets "
      " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( !Qry.RowCount() )
    	throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    string trip_airline = Qry.FieldAsString( "airline" );
    string craft = Qry.FieldAsString( "craft" );
    string bort = Qry.FieldAsString( "bort" );
    vector<TShowComps> comps, comps_tmp;
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
            getFlightBaseCompRef( item.point_id, true, false, trip_airline, craft, bort, comps_tmp );
          }
          else {
            getFlightTuneCompRef( item.point_id, false, trip_airline, comps_tmp ); // редактированная
          }
          if ( !comps_tmp.empty() ) {
            TTripInfo info;
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
    getFlightBaseCompRef( point_id, false, true, trip_airline, craft, bort, comps );
    //sort comps for client
    sort( comps.begin(), comps.end(), CompareShowComps );
    string StrVal;
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

  if ( isTranzitSalonsVersion ) {
    SALONS2::TSalonList salonList;
    try {
      salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), SALONS2::rfTranzitVersion, filterClass );
    }
    catch( AstraLocale::UserException ue ) {
      AstraLocale::showErrorMessage( ue.getLexemaData() );
    }
    salonList.Build( true, NewTextChild( dataNode, "salons" ) );
    comp_id = salonList.getCompId();
  }
  else {
    SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
   	Salons.FilterClass = filterClass;
    try {
      Salons.Read();
    }
    catch( AstraLocale::UserException ue ) {
      AstraLocale::showErrorMessage( ue.getLexemaData() );
    }
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    Salons.Build( salonsNode );
    comp_id = Salons.comp_id;
  }
  
  
  if ( pr_comps ) {
    if ( get_alarm( point_id, atWaitlist ) ) {
    	AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
    	SetProp(NewTextChild( dataNode, "passengers" ), "pr_waitlist", 1);
    }
 	}
 	if ( pr_images ) { // не используется в новом терминале
    GetDataForDrawSalon( reqNode, resNode );
 	}
  if ( comp_id > 0 ) { //строго завязать базовые компоновки с назначенными на рейс
    Qry.Clear();
    Qry.SQLText =
	    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
	  TTripInfo info( Qry );
	  if ( GetTripSets( tsCraftNoChangeSections, info ) ) {
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
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
  bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( trip_id );
  ProgTrace(TRACE5, "SalonFormInterface::Write point_id=%d, isTranzitSalonsVersion=%d", trip_id, isTranzitSalonsVersion );
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
  flights.Lock();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  TTripInfo info( Qry );
  SALONS2::TSalons Salons( trip_id, SALONS2::rTripSalons ), OldSalons( trip_id, SALONS2::rTripSalons );;
  SALONS2::TSalonList salonList, priorsalonList;
  salonList.Parse( trip_id, NodeAsNode( "salons", reqNode ) );
  if ( !isTranzitSalonsVersion ) {
    Salons.Parse( NodeAsNode( "salons", reqNode ) );
  }
  //была ли до этого момента компоновка
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();

  bool pr_base_change = false;
  if ( !Qry.Eof ) { // была старая компоновка
    priorsalonList.ReadFlight( SALONS2::TFilterRoutesSets( trip_id ), SALONS2::rfTranzitVersion, "" );
    pr_base_change = ChangeCfg( priorsalonList, salonList );
  }
  Qry.Clear();
  Qry.SQLText = "UPDATE trip_sets SET comp_id=:comp_id WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.DeclareVariable( "comp_id", otInteger );
  // пришла новая компоновка, но не пришел comp_id - значит были изменения компоновки - "сохраните базовую компоновку."
  bool pr_notchangecraft = GetTripSets( tsCraftNoChangeSections, info );
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
  if ( isTranzitSalonsVersion ) {
    salonList.WriteFlight( trip_id );
  }
  else {
    SALONS2::TComponSets compSets;
    Salons.Write( compSets );
  }
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
  /* инициализация VIP */
  SALONS2::InitVIP( trip_id );
  string lexema_id;
  LEvntPrms params;
  string comp_lang;
  if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION)) {
    if ( NodeAsInteger( "pr_lat", refcompNode ) != 0 )
  	  comp_lang = "лат.";
  	else
  		comp_lang = "рус.";
  }
  else
    comp_lang = NodeAsString( "lang", refcompNode );

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
  SALONS2::setTRIP_CLASSES( trip_id );
  //set flag auto change in false state
  SALONS2::setManualCompChg( trip_id );
  Qry.Clear();
	Qry.SQLText = "UPDATE trip_sets SET crc_comp=:crc_comp WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, trip_id );
	Qry.CreateVariable( "crc_comp", otInteger, SALONS2::CRC32_Comp( trip_id ) );
  Qry.Execute();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  SALONS2::GetTripParams( trip_id, dataNode );
  // надо перечитать заново
  /*всегда работаем с новой компоновкой, т.к. см. !salonChangesToText*/
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( trip_id ), SALONS2::rfTranzitVersion, "" );
  BitSet<ASTRA::TCompLayerType> editabeLayers;
  salonList.getEditableFlightLayers( editabeLayers );
  LEvntPrms salon_changes;
  salonChangesToText( trip_id, priorsalonList, priorsalonList.isCraftLat(),
                        salonList, salonList.isCraftLat(),
                        editabeLayers,
                        salon_changes, cBase && comp_id != -2);
  TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtFlt, trip_id);
  for (std::deque<LEvntPrm*>::const_iterator iter=salon_changes.begin(); iter != salon_changes.end(); iter++) {
      TReqInfo::Instance()->LocaleToLog("EVT.SALON_CHANGES", LEvntPrms() << *(dynamic_cast<PrmEnum*>(*iter)), evtFlt, trip_id);
  }
  // конец перечитки
  SALONS2::check_diffcomp_alarm( trip_id );
  if ( SALONS2::isTranzitSalons( trip_id ) ) {
    SALONS2::check_waitlist_alarm_on_tranzit_routes( trip_id );
  }
  else {
    check_waitlist_alarm( trip_id );
  }
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
//  #warning 8. new parse + salonChangesToText
  if ( isTranzitSalonsVersion ) {
    salonList.Build( true, salonsNode );
    comp_id = salonList.getCompId();
  }
  else {
    Salons.Clear();
    Salons.Read();
    Salons.Build( salonsNode );
    comp_id = Salons.comp_id;
  }

  if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
 	  vector<SALONS2::TCompSection> CompSections;
    ReadCompSections( comp_id, CompSections );
    BuildCompSections( dataNode, CompSections );
  }
  if ( get_alarm( trip_id, atWaitlist ) ) {
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
  xmlNodePtr pNode;
  if ( comp_id < 0 && ( pNode = GetNode( "point_id", reqNode ) ) ) { // выбрана компоновка пред. пункта в транзитном маршруте
    point_id = NodeAsInteger( pNode );
    int crc_comp = 0;
    if ( GetNode( "@crc_comp", tmpNode ) )
      crc_comp = NodeAsInteger( "@crc_comp", tmpNode );
    ProgTrace( TRACE5, "point_id=%d, crc_comp=%d", point_id, crc_comp );
    Qry.SQLText =
  	  "SELECT point_num,first_point,pr_tranzit,pr_del,scd_out, "
  	  "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
  	  "       airline_fmt,suffix_fmt,airp_fmt,"
      "       bort,airline,flt_no,suffix,airp,craft,crc_comp "
      " FROM points, trip_sets "
      " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";
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
    if ( !Qry.Eof )
      ProgTrace( TRACE5, "prior_crc_comp=%d", Qry.FieldAsInteger( "crc_comp" ) );
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
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( id ), SALONS2::rfTranzitVersion, "", point_id );
  }
  else {
    salonList.ReadCompon( id );
  }
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  if ( comp_id < 0 )
    SALONS2::GetTripParams( prior_point_id, dataNode );
  else
    SALONS2::GetCompParams( comp_id, dataNode );
  salonList.Build( true, salonsNode );
//  #warning 7. зачем еще раз делать BuildLayersInfo???
  bool pr_notchangecraft = true;
  BitSet<SALONS2::TDrawPropsType> props;
  pNode = GetNode( "point_id", reqNode );
  if ( pNode ) {
    point_id = NodeAsInteger( pNode );
    Qry.Clear();
    Qry.SQLText = "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    TTripInfo info( Qry );
    pr_notchangecraft = GetTripSets( tsCraftNoChangeSections, info );
    SALONS2::CreateSalonMenu( point_id, salonsNode );
  }
  if ( pr_notchangecraft && comp_id >= 0 ) {
    vector<SALONS2::TCompSection> CompSections;
    ReadCompSections( comp_id, CompSections );
    BuildCompSections( dataNode, CompSections );
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
  salonList.Parse( ASTRA::NoExists, GetNode( "salons", reqNode ) );
  if ( componSets.modify != SALONS2::mNone &&
       componSets.modify != SALONS2::mAdd ) {
    priorsalonList.ReadCompon( comp_id );
  }
  salonList.WriteCompon( comp_id, componSets );
  LEvntPrms salon_changes;
  salonChangesToText( NoExists, priorsalonList, priorsalonList.isCraftLat(),
                      salonList, salonList.isCraftLat(),
                      editabeLayers,
                      salon_changes, componSets.modify != SALONS2::mDelete);
  if ( componSets.modify !=  SALONS2::mDelete ) {
    salonList.ReadCompon( comp_id );
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
  //bagsections
  vector<SALONS2::TCompSection> CompSections;
  xmlNodePtr sectionsNode = GetNode( "CompSections", reqNode );
  if ( sectionsNode && componSets.modify != SALONS2::mDelete ) {
    ParseCompSections( sectionsNode, CompSections );
    WriteCompSections( comp_id, CompSections );
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
  if ( r->desk.compatible(LATIN_VERSION) )
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
  if ( SQry.Eof )
  	throw UserException( "MSG.PASSENGER.NOT_FOUND" );
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
    "  :seat_no:=salons.get_seat_no(:pax_id,:seats,:grp_status,:point_id,:format,:pax_row); "
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
			  slayer_type = ((TGrpStatusTypesRow&)grp_status_types.get_row("code",grp_status)).layer_type;
			}
			catch(EBaseTableError){};
		}
		else
			slayer_type = SQry.GetVariableAsString( "layer_type" );
	}
/*!!!djek  if ( slayer_type.empty() )
  	throw EXCEPTIONS::Exception( "getSeat_no: slayer_type.empty()" );*/
};

void IntChangeSeatsN( int point_id, int pax_id, int &tid, string xname, string yname,
                      SEATS2::TSeatsType seat_type,
                      TCompLayerType layer_type,
                      bool pr_waitlist, bool pr_question_reseat,
                      xmlNodePtr resNode )
{
	TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock();
  int point_arv = NoExists;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, airp, scd_out "
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
    xname = norm_iata_line( xname );
    yname = norm_iata_row( yname );
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

  }
  else {
    point_arv = SALONS2::getCrsPaxPointArv( pax_id, point_id );
  }

  ProgTrace( TRACE5, "IntChangeSeats: point_id=%d, point_arv=%d, pax_id=%d, tid=%d, layer=%s",
            point_id, point_arv, pax_id, tid, EncodeCompLayerType( layer_type ) );

  SALONS2::TSalonList salonList, salonListPriorVersion;

  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), SALONS2::rfTranzitVersion, "" );
  //!!!  для слоев предварительной рассадки point_arv = NoExists!!!
    
  // если место у пассажира имеет предварительную рассадку для этого пассажира, и мы еще не спрашивали, то спросить!
  if ( seat_type != SEATS2::stDropseat && !pr_waitlist && pr_question_reseat ) {
    // возможны следующие варианты:
    // 1. пересадка зарегистрированного пассажира
    // 2. предварительная пересадка/рассадка
    // старое место может иметь след. слои:
    // cltProtCkin, cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay
    
    // вычисляем занятое место
    SALONS2::TSeatLayer seatLayer;
    std::set<SALONS2::TPlace*,SALONS2::CompareSeats> seats;
    salonList.getPaxLayer( point_id, pax_id, seatLayer, seats );
    string used_seat_no;
    if ( seatLayer.layer_type == layer_type && !seats.empty() ) {
      used_seat_no = (*seats.begin())->yname + (*seats.begin())->xname;
    }
    ProgTrace( TRACE5, "IntChangeSeats: occupy point_dep=%d, pax_id=%d, %s, used_seat_no=%s",
               point_id, pax_id, seatLayer.toString().c_str(), used_seat_no.c_str() );
    // вычисляем предв. места по слоям, для того чтобы предупредить о том, что есть предв. рассадка на тек. месте
    Qry.Clear();
    Qry.SQLText =
      "SELECT first_yname||first_xname pre_seat_no, layer_type, priority "
      " FROM trip_comp_layers, comp_layer_types "
      " WHERE point_id=:point_id AND "
      "       trip_comp_layers.layer_type IN (:protckin_layer,:prot_pay1,:prot_pay2) AND "
      "       crs_pax_id=:pax_id AND "
      "       comp_layer_types.code=trip_comp_layers.layer_type "
      "ORDER BY priority";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType( cltProtCkin ) );
    Qry.CreateVariable( "prot_pay1", otString, EncodeCompLayerType( cltPNLAfterPay ) );
    Qry.CreateVariable( "prot_pay2", otString, EncodeCompLayerType( cltProtAfterPay ) );
    Qry.Execute();
    if ( !Qry.Eof && !used_seat_no.empty() && used_seat_no == Qry.FieldAsString( "pre_seat_no" ) ) {
      ProgTrace( TRACE5, "pax_id=%d,point_id=%d,used_seat_no=%s,pre_seat_no=%s",
                  pax_id, point_id, used_seat_no.c_str(), Qry.FieldAsString( "pre_seat_no" ) );
      if ( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) == cltProtCkin )
      	NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PRESEAT_SEATS.RESEAT") );
      else
        NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PAID_SEATS.RESEAT"));
    	return;
    }
  }

  vector<SALONS2::TSalonSeat> seats;
  try {
  	SEATS2::ChangeLayer( salonList, layer_type, point_id, pax_id, tid, xname, yname, seat_type );
  	if ( TReqInfo::Instance()->client_type != ctTerm || resNode == NULL )
  		return; // web-регистрация
    salonList.JumpToLeg( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ) );
    SALONS2::TSalonList NewSalonList;
    NewSalonList.ReadFlight( salonList.getFilterRoutes(), SALONS2::rfTranzitVersion, salonList.getFilterClass() );
    SALONS2::getSalonChanges( salonList, salonList.isCraftLat(), NewSalonList, NewSalonList.isCraftLat(), seats );
  	ProgTrace( TRACE5, "salon changes seats.size()=%zu", seats.size() );
  	string seat_no, slayer_type;
  	if ( layer_type == cltProtCkin )
  	  getSeat_no( pax_id, true, string("_seats"), seat_no, slayer_type, tid );
    else
    	getSeat_no( pax_id, false, string("one"), seat_no, slayer_type, tid );

    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
      	seat_no = LTrimString( seat_no );
    	NewTextChild( dataNode, "seat_no", seat_no );
    	NewTextChild( dataNode, "layer_type", slayer_type );
    }
    SALONS2::BuildSalonChanges( dataNode, point_id, seats, true, NewSalonList.pax_lists );
    if ( pr_waitlist ) {
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      SALONS2::TSalonPassengers passengers;
      NewSalonList.getPassengers( passengers, flags );
      passengers.BuildWaitList( point_id, dataNode );
    	if ( !passengers.isWaitList( point_id ) ) {
      	AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      }
    }
  }
  catch( UserException ue ) {
    tst();
  	if ( TReqInfo::Instance()->client_type != ctTerm )
  		throw;
    xmlNodePtr dataNode = GetNode( "data", resNode );
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
  	dataNode = NewTextChild( resNode, "data" );
    SALONS2::GetTripParams( point_id, dataNode );
    salonList.JumpToLeg( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ) );
    int comp_id;
    salonList.Build( true, NewTextChild( dataNode, "salons" ) );
    comp_id = salonList.getCompId();
    if ( pr_waitlist ) {
      SALONS2::TSalonPassengers passengers;
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      salonList.getPassengers( passengers, flags );
      passengers.BuildWaitList( point_id, dataNode );
    }
    if ( comp_id > 0 && GetTripSets( tsCraftNoChangeSections, fltInfo ) &&
         TReqInfo::Instance()->client_type == ctTerm ) { //строго завязать базовые компоновки с назначенными на рейс
   	  vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
    }
  	showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
}

void IntChangeSeats( int point_id, int pax_id, int &tid, string xname, string yname,
                     SEATS2::TSeatsType seat_type,
                     TCompLayerType layer_type,
                     bool pr_waitlist, bool pr_question_reseat,
                     xmlNodePtr resNode )
{
  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
	TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, airp, scd_out, pr_lat_seat "
    "FROM points, trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND points.point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo fltInfo( Qry );
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

  if ( seat_type != SEATS2::stDropseat ) {
    xname = norm_iata_line( xname );
    yname = norm_iata_row( yname );
  }
  Qry.Clear();
  Qry.SQLText =
   "SELECT layer_type, pax_grp.status FROM grp_status_types, pax, pax_grp "
   " WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND pax_grp.status=grp_status_types.code ";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    if (DecodePaxStatus(Qry.FieldAsString("status"))==psCrew)
      throw UserException("MSG.CREW.IMPOSSIBLE_CHANGE_SEAT");
  	layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
  }

  ProgTrace(TRACE5, "SalonsInterface::Reseat, point_id=%d, pax_id=%d, tid=%d, layer=%s", point_id, pax_id, tid, EncodeCompLayerType( layer_type ) );

  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();

  // если место у пассажира имеет предварительную рассадку для этого пассажира, и мы еще не спрашивали, то спросить!
  if ( seat_type != SEATS2::stDropseat && !pr_waitlist && pr_question_reseat ) {
    // возможны следующие варианты:
    // 1. пересадка зарегистрированного пассажира
    // 2. предварительная пересадка/рассадка
    // старое место может иметь след. слои:
    // cltProtCkin, cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay

    // вычисляем занятое место
    Qry.Clear();
    Qry.SQLText =
      "SELECT first_yname||first_xname seat_no1 FROM trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type = :layer_type AND pax_id=:pax_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType(layer_type) );
    Qry.Execute();
    string used_seat_no;
    if ( !Qry.Eof ) {
      used_seat_no = Qry.FieldAsString( "seat_no1" );
      ProgTrace( TRACE5, "Qry.Eof=%d, pax_id=%d,point_id=%d,prot_layer=%s,seat_no1=%s",
                 Qry.Eof,pax_id,point_id,EncodeCompLayerType( cltProtCkin ), used_seat_no.c_str() );
    }
    // вычисляем предв. места по слоям
    Qry.Clear();
    Qry.SQLText =
      "SELECT first_yname||first_xname pre_seat_no, layer_type, priority "
      " FROM trip_comp_layers, comp_layer_types "
      " WHERE point_id=:point_id AND "
      "       trip_comp_layers.layer_type IN (:protckin_layer,:prot_pay1,:prot_pay2) AND "
      "       crs_pax_id=:pax_id AND "
      "       comp_layer_types.code=trip_comp_layers.layer_type "
      "ORDER BY priority";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType( cltProtCkin ) );
    Qry.CreateVariable( "prot_pay1", otString, EncodeCompLayerType( cltPNLAfterPay ) );
    Qry.CreateVariable( "prot_pay2", otString, EncodeCompLayerType( cltProtAfterPay ) );
    Qry.Execute();
    if ( !Qry.Eof && !used_seat_no.empty() && used_seat_no == Qry.FieldAsString( "pre_seat_no" ) ) {
      ProgTrace( TRACE5, "pax_id=%d,point_id=%d,used_seat_no=%s,pre_seat_no=%s",
                  pax_id, point_id, used_seat_no.c_str(), Qry.FieldAsString( "pre_seat_no" ) );
      if ( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) == cltProtCkin )
      	NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PRESEAT_SEATS.RESEAT") );
      else
        NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PAID_SEATS.RESEAT"));
    	return;
    }
  }

  vector<SALONS2::TSalonSeat> seats;

  try {
  	SEATS2::ChangeLayer( layer_type, point_id, pax_id, tid, xname, yname, seat_type, pr_lat_seat );
  	if ( TReqInfo::Instance()->client_type != ctTerm || resNode == NULL )
  		return; // web-регистрация
  	SALONS2::getSalonChanges( Salons, seats );
  	ProgTrace( TRACE5, "salon changes seats.size()=%zu", seats.size() );
  	string seat_no, slayer_type;
  	if ( layer_type == cltProtCkin )
  	  getSeat_no( pax_id, true, string("_seats"), seat_no, slayer_type, tid );
    else
    	getSeat_no( pax_id, false, string("one"), seat_no, slayer_type, tid );

    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
      	seat_no = LTrimString( seat_no );
    	NewTextChild( dataNode, "seat_no", seat_no );
    	NewTextChild( dataNode, "layer_type", slayer_type );
    }
    SALONS2::BuildSalonChanges( dataNode, seats );
    if ( pr_waitlist ) {
    	SEATS2::TPassengers p;
    	if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
      	AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      p.Build( dataNode );
    }
  }
  catch( UserException ue ) {
  	if ( TReqInfo::Instance()->client_type != ctTerm )
  		throw;
    xmlNodePtr dataNode = GetNode( "data", resNode );
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
  	dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
      SEATS2::TPassengers p;
    	SEATS2::GetPassengersForWaitList( point_id, p );
      p.Build( dataNode );
    }
    if ( Salons.comp_id > 0 && GetTripSets( tsCraftNoChangeSections, fltInfo ) &&
         TReqInfo::Instance()->client_type == ctTerm ) { //строго завязать базовые компоновки с назначенными на рейс
   	  vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( Salons.comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
    }
  	showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
}

void ChangeSeats( xmlNodePtr reqNode, xmlNodePtr resNode, SEATS2::TSeatsType seat_type )
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( point_id );
	ProgTrace( TRACE5, "ChangeSeats: point_id=%d, isTranzitSalonsVersion=%d", point_id, isTranzitSalonsVersion );
  string xname;
  string yname;
  if ( seat_type != SEATS2::stDropseat ) {
    xname = NodeAsString( "xname", reqNode );
    yname = NodeAsString( "yname", reqNode );
  }
  if ( isTranzitSalonsVersion ) {
    IntChangeSeatsN( point_id, pax_id, tid, xname, yname,
                    seat_type,
                    DecodeCompLayerType( NodeAsString( "layer", reqNode, "" ) ),
                    GetNode( "waitlist", reqNode ),
                    GetNode( "question_reseat", reqNode ), resNode );
  }
  else {
    IntChangeSeats( point_id, pax_id, tid, xname, yname,
                    seat_type,
                    DecodeCompLayerType( NodeAsString( "layer", reqNode, "" ) ),
                    GetNode( "waitlist", reqNode ),
                    GetNode( "question_reseat", reqNode ), resNode );
  }
};

void SalonFormInterface::DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	 // удаление мест пассажира
  ChangeSeats( reqNode, resNode, SEATS2::stDropseat );
};

void SalonFormInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeSeats( reqNode, resNode, SEATS2::stReseat );
};

void SalonFormInterface::DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int point_arv = NodeAsInteger( "point_arv", reqNode, NoExists );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool pr_update_salons = GetNode( "update_salons", reqNode );
  bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( point_id );
  TQuery Qry( &OraSession );
 	ProgTrace( TRACE5, "SalonsInterface::DeleteProtCkinSeat, point_id=%d, pax_id=%d, tid=%d, pr_update_salons=%d, isTranzitSalonsVersion=%d",
	           point_id, pax_id, tid, pr_update_salons, isTranzitSalonsVersion );

  if ( SALONS2::isFreeSeating( point_id ) ) {
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );  //!!! плохо, т.к. обращения к БД
  SALONS2::TSalonList salonList;
  if ( isTranzitSalonsVersion ) {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), SALONS2::rfTranzitVersion, "" );
  }
  else {
    Salons.Read();
  }
  vector<SALONS2::TSalonSeat> seats;

  try {
    if ( isTranzitSalonsVersion ) {
  	  SEATS2::ChangeLayer( salonList, cltProtCkin, point_id, pax_id, tid, "", "", SEATS2::stDropseat );
    }
    else {
      SEATS2::ChangeLayer( cltProtCkin, point_id, pax_id, tid, "", "", SEATS2::stDropseat, Salons.getLatSeat() );
    }
  	if ( pr_update_salons ) {
      if ( isTranzitSalonsVersion ) {
        SALONS2::getSalonChanges( salonList, seats );
      }
      else {
  	    SALONS2::getSalonChanges( Salons, seats );
      }
    }
  	string seat_no, slayer_type;
  	getSeat_no( pax_id, true, string("_seats"), seat_no, slayer_type, tid );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
      	seat_no = LTrimString( seat_no );
    	NewTextChild( dataNode, "seat_no", seat_no );
    	NewTextChild( dataNode, "layer_type", slayer_type );
    }
    if ( pr_update_salons )
   	  SALONS2::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
  	if ( TReqInfo::Instance()->client_type != ctTerm )
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
      if ( isTranzitSalonsVersion ) {
        salonList.Build( true, salonsNode );
        comp_id = salonList.getCompId();
      }
      else {
        Salons.Build( salonsNode );
        comp_id = Salons.comp_id;
      }
      if ( comp_id > 0 ) { //строго завязать базовые компоновки с назначенными на рейс
        Qry.Clear();
 	      Qry.SQLText =
	        "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
	      TTripInfo info( Qry );
        if ( GetTripSets( tsCraftNoChangeSections, info ) ) {
   	      vector<SALONS2::TCompSection> CompSections;
          ReadCompSections( comp_id, CompSections );
          BuildCompSections( dataNode, CompSections );
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
	bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( point_id );
 	ProgTrace( TRACE5, "SalonsInterface::WaitList, point_id=%d, isTranzitSalonsVersion=%d", point_id, isTranzitSalonsVersion );
	TQuery Qry( &OraSession );
	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
	SALONS2::TSalonList salonList;
	if ( isTranzitSalonsVersion ) {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
    SALONS2::TSalonPassengers passengers;
    SALONS2::TGetPassFlags flags;
    flags.setFlag( SALONS2::gpPassenger );
    flags.setFlag( SALONS2::gpWaitList );
    salonList.getPassengers( passengers, flags );
    passengers.BuildWaitList( point_id, dataNode );
	}
	else {
    SEATS2::TPassengers p;
    SEATS2::GetPassengersForWaitList( point_id, p );
    p.Build( dataNode );
  }
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
    if ( isTranzitSalonsVersion ) {
      salonList.Build( true, NewTextChild( dataNode, "salons" ) );
      comp_id = salonList.getCompId();
    }
    else {
      SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
      Salons.Read();
      Salons.Build( NewTextChild( dataNode, "salons" ) );
      comp_id = Salons.comp_id;
    }
    SALONS2::GetTripParams( point_id, dataNode );
    if ( comp_id > 0 ) {
      Qry.Clear();
      Qry.SQLText =
       "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.Execute();
      TTripInfo info( Qry );
      if ( GetTripSets( tsCraftNoChangeSections, info ) ) { //строго завязать базовые компоновки с назначенными на рейс
 	      vector<SALONS2::TCompSection> CompSections;
        ReadCompSections( comp_id, CompSections );
        BuildCompSections( dataNode, CompSections );
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
  bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( point_id );
	ProgTrace( TRACE5, "AutoSeats: point_id=%d, isTranzitSalonsVersion=%d", point_id, isTranzitSalonsVersion );
	bool pr_waitlist = GetNode( "waitlist", reqNode );
	TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock();
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
  if ( isTranzitSalonsVersion ) {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
    SALONS2::TGetPassFlags flags;
    flags.setFlag( SALONS2::gpPassenger );
    flags.setFlag( SALONS2::gpWaitList );
    salonList.getPassengers( passengers, flags );
    if ( !passengers.isWaitList( point_id ) ) {
      throw UserException( "MSG.SEATS.ALL_PASSENGERS_PLANED" );
    }
  }
  else {
    if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
    	throw UserException( "MSG.SEATS.ALL_PASSENGERS_PLANED" );
  }
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  //vector<SALONS2::TSalonSeat> seats;
  if ( !isTranzitSalonsVersion ) {
    Salons.Read();
  }
  int comp_id;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  bool pr_notchangecraft = GetTripSets( tsCraftNoChangeSections, info );
  try {
    if ( isTranzitSalonsVersion ) {
      SALONS2::TSalonPassengers::const_iterator ipasses = passengers.find( point_id );
      if ( ipasses != passengers.end() ) {
        SEATS2::AutoReSeatsPassengers( salonList, ipasses->second, SEATS2::GetSeatAlgo( Qry, info.airline, info.flt_no, info.airp ) );
      }
    }
    else {
      SEATS2::AutoReSeatsPassengers( Salons, p, SEATS2::GetSeatAlgo( Qry, info.airline, info.flt_no, info.airp ) );
    }
    tst();
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    int comp_id;
    if ( isTranzitSalonsVersion ) {
      SALONS2::TSalonList salonList;
      salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), SALONS2::rfTranzitVersion, "" );
      salonList.Build( true, salonsNode );
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
        passengers.BuildWaitList( point_id, dataNode );
      }
    }
    else {
      Salons.Build( salonsNode );
      comp_id = Salons.comp_id;
      if ( pr_waitlist ) {
    	  p.Clear();
    	  if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
            AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
        else
            AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
        p.Build( dataNode );
      }
    }
    tst();
    if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
      tst();
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
    }
  }
  catch( UserException ue ) {
  	if ( TReqInfo::Instance()->client_type != ctTerm )
  		throw; // web-регистрация
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
  	dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    if ( isTranzitSalonsVersion ) {
      SALONS2::TSalonList salonList;
      salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, NoExists ), SALONS2::rfTranzitVersion, "" );
      salonList.Build( true, salonsNode );
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
        passengers.BuildWaitList( point_id, dataNode );
      }
    }
    else {
      Salons.Build( salonsNode );
      comp_id = Salons.comp_id;
      if ( pr_waitlist ) {
        p.Clear();
    	  if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
          AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
        else
          AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
        p.Build( dataNode );
      }
    }
    if ( comp_id > 0 && pr_notchangecraft ) { //строго завязать базовые компоновки с назначенными на рейс
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
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
  SALONS2::TPassSeats seats;
  SALONS2::TWaitListReason waitListReason;
	salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
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
              NewTextChild( node, "class", i->cl );
              NewTextChild( node, "pers_type", EncodePerson( i->pers_type ) );
              NewTextChild( node, "seats", (int)i->seats );
              NewTextChild( node, "reg_no", i->reg_no );
              NewTextChild( node, "pr_infant", i->pr_infant != ASTRA::NoExists );
              NewTextChild( node, "pr_web", i->pr_web );
              NewTextChild( node, "grp_status", i->grp_status );
              NewTextChild( node, "name", i->name + " " + i->surname );
              i->get_seats( waitListReason, seats );
              NewTextChild( node, "pr_waitlist", waitListReason.layerStatus != SALONS2::layerValid );
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
              NewTextChild( node, "class", i->cl );
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


void trace( int pax_id, int grp_id, int parent_pax_id, int crs_pax_id, const std::string &pers_type, int seats )
{
  ProgTrace( TRACE5, "pax_id=%d, grp_id=%d, parent_pax_id=%d, crs_pax_id=%d, pers_type=%s, seats=%d",
             pax_id, grp_id, parent_pax_id, crs_pax_id, pers_type.c_str(), seats );
}
