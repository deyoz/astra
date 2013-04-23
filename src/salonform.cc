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
      SetProp( cnode, "FirstRowIdx", i->firstRowIdx );
      SetProp( cnode, "LastRowIdx", i->lastRowIdx );
    }
  }
}

void ReadCompSections( int comp_id, vector<SALONS2::TCompSection> &CompSections )
{
  CompSections.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT name, first_rownum, last_rownum FROM comp_sections WHERE comp_id=:comp_id "
    "ORDER By first_rownum";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    SALONS2::TCompSection cs;
    cs.name = Qry.FieldAsString( "name" );
    cs.firstRowIdx = Qry.FieldAsInteger( "first_rownum" );
    cs.lastRowIdx = Qry.FieldAsInteger( "last_rownum" );
    CompSections.push_back( cs );
    Qry.Next();
  }
}

void ZoneLoads(int point_id, map<string, int> &zones)
{
  std::vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
  vector<TZoneOccupiedSeats> zoneSeats;
  ZoneLoads(point_id, false, false, false, zoneSeats,CompSectionsLayers);
  for ( vector<TZoneOccupiedSeats>::iterator i=zoneSeats.begin(); i!=zoneSeats.end(); i++ ) {
    zones[ i->name ] = i->seats.size();
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

void ZoneLoads(int point_id,
               bool only_checkin_layers, bool only_high_layer, bool drop_not_used_pax_layers,
               vector<TZoneOccupiedSeats> &zones,
               std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers,
               vector<SALONS2::TCompSection> &compSections )
{
    compSections.clear();
    CompSectionsLayers.clear();
    zones.clear();
    SALONS2::TSalons SalonsTmp( point_id, SALONS2::rTripSalons, drop_not_used_pax_layers );
    TQuery Qry(&OraSession);
    try {
	      Qry.SQLText =
	        "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
	      TTripInfo info( Qry );
	      tst();
        SalonsTmp.Read();
        if ( SalonsTmp.comp_id > 0 && GetTripSets( tsCraftNoChangeSections, info ) ) { //!!!строго завязать базовые компоновки с назначенными на рейс
            tst();
            ReadCompSections( SalonsTmp.comp_id, compSections );
            TQuery Qry(&OraSession);
            Qry.SQLText = "select layer_type from grp_status_types";
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
  string msg, ms;
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
  if ( CompSections.empty() && pr_exists )
    msg = "Удалены все багажные секции";
  for ( vector<SALONS2::TCompSection>::const_iterator i=CompSections.begin(); i!=CompSections.end(); i++ ) {
    if ( i == CompSections.begin() )
      msg = "Назначены багажные секции: ";
    ms = "название:" + i->name + ",первый ряд:" + IntToString( i->firstRowIdx ) + ",последний ряд:" + IntToString( i->lastRowIdx );
    if ( msg.size() + ms.size() >= 250 ) {
      TReqInfo::Instance()->MsgToLog( msg, evtComp, id );
      msg.clear();
    }
    msg += ms;
    Qry.SetVariable( "name", i->name );
    Qry.SetVariable( "first_rownum", i->firstRowIdx );
    Qry.SetVariable( "last_rownum", i->lastRowIdx );
    Qry.Execute();
  }
  if ( !msg.empty() ) {
    TReqInfo::Instance()->MsgToLog( msg, evtComp, id );
  }
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

void SalonFormInterface::Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonFormInterface::Show" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int point_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", point_id );

  TQuery Qry( &OraSession );
  SALONS2::GetTripParams( point_id, dataNode );
  bool pr_comps = GetNode( "pr_comps", reqNode );
  bool pr_images = GetNode( "pr_images", reqNode ); // не используется в новом терминале!!!
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
        //!!! если компоновка базовая в пред. пункте, то надо передать базовую иначе текущую
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
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  if ( GetNode( "ClName", reqNode ) )
  	Salons.FilterClass = NodeAsString( "ClName", reqNode );
  else
    Salons.FilterClass.clear();
  try {
    Salons.Read();
  }
  catch( AstraLocale::UserException ue ) {
    AstraLocale::showErrorMessage( ue.getLexemaData() );
  }
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  Salons.Build( salonsNode );
  if ( pr_comps ) {
    if ( get_alarm( point_id, atWaitlist ) ) {
    	AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
    	SetProp(NewTextChild( dataNode, "passengers" ), "pr_waitlist", 1);
    }
 	}
 	if ( pr_images ) { // не используется в новом терминале!!!
    GetDataForDrawSalon( reqNode, resNode );
 	}
  if ( Salons.comp_id > 0 ) { //!!!строго завязать базовые компоновки с назначенными на рейс
    Qry.Clear();
    Qry.SQLText =
	    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
	  TTripInfo info( Qry );
	  if ( GetTripSets( tsCraftNoChangeSections, info ) ) {
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( Salons.comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
    }
  }
}

void SalonFormInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int comp_id = NodeAsInteger( "comp_id", reqNode );
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
  Qry.SQLText = "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  TTripInfo info( Qry );
  SALONS2::TSalons Salons( trip_id, SALONS2::rTripSalons );
  Salons.Parse( NodeAsNode( "salons", reqNode ) );
  Salons.verifyValidRem( "MCLS", "Э"); //???
  Salons.verifyValidRem( "SCLS", "Э"); //???
  Salons.verifyValidRem( "YCLS", "Э"); //???
  Salons.verifyValidRem( "LCLS", "Э"); //???
  Salons.trip_id = trip_id;
  Salons.FilterClass = "";
  bool pr_base_change = false;
  SALONS2::TSalons OldSalons( trip_id, SALONS2::rTripSalons );
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if ( !Qry.Eof ) { // была старая компоновка
    OldSalons.Read();
    SALONS2::TCompareCompsFlags compareFlags;
    compareFlags.setFlag( SALONS2::ccCoord );
    compareFlags.setFlag( SALONS2::ccPlaceType );
    pr_base_change = ChangeCfg( OldSalons, Salons, compareFlags );
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
    if ( pr_base_change && cSet )
      comp_id = -2;
    if ( comp_id == -2 )
      Qry.SetVariable( "comp_id", FNull );
    else
      Qry.SetVariable( "comp_id", comp_id );
    Qry.Execute();
  }
  Salons.Write();

  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
  /* инициализация VIP */
  SALONS2::InitVIP( trip_id );

  string msg;
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
    if ( cBase ) {
      msg = string( "Назначена базовая компоновка (ид=" ) +
            IntToString( comp_id ) +
            "). Классы: " + NodeAsString( "classes", refcompNode );
      if ( cChange )
        msg = string( "Назначена компоновка рейса. Классы: " ) +
              NodeAsString( "classes", refcompNode );
    }
    msg += string( ", кодировка: " ) + comp_lang;
    //TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );
  }
  else {
  	msg = string( "Изменена компоновка рейса. Классы: " ) + NodeAsString( "classes", refcompNode );
  	msg += string( ", кодировка: " ) + comp_lang;
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
  vector<SEATS2::TSalonSeat> seats;
  // надо перечитать заново
  Salons.Clear();
  Salons.Read();
  vector<string> referStrs;
  salonChangesToText( OldSalons, Salons, referStrs, cBase && comp_id != -2, 100 );
  referStrs.insert( referStrs.begin(), msg );
  for ( vector<string>::iterator i=referStrs.begin(); i!=referStrs.end(); i++ ) {
  	TReqInfo::Instance()->MsgToLog( *i, evtFlt, trip_id );
  }
  // конец перечитки
  SALONS2::check_diffcomp_alarm( trip_id );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  Salons.Build( salonsNode );
  if ( Salons.comp_id > 0 && pr_notchangecraft ) { //!!!строго завязать базовые компоновки с назначенными на рейс
 	  vector<SALONS2::TCompSection> CompSections;
    ReadCompSections( Salons.comp_id, CompSections );
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
  int prior_point_id = NoExists;
  TQuery Qry( &OraSession );
  xmlNodePtr pNode;
  if ( comp_id < 0 && ( pNode = GetNode( "point_id", reqNode ) ) ) { // выбрана компоновка пред. пункта в транзитном маршруте
    int point_id = NodeAsInteger( pNode );
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
  SALONS2::TSalons Salons( id, readStyle );
  Salons.Read( );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  if ( comp_id < 0 )
    SALONS2::GetTripParams( prior_point_id, dataNode );
  else
    SALONS2::GetCompParams( comp_id, dataNode );
  Salons.Build( salonsNode );
  bool pr_notchangecraft = true;
  std::vector<SALONS2::TDrawPropsType> props;
  pNode = GetNode( "point_id", reqNode );
  if ( pNode ) {
    int point_id = NodeAsInteger( pNode );
    Qry.Clear();
    Qry.SQLText = "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    TTripInfo info( Qry );
    pr_notchangecraft = GetTripSets( tsCraftNoChangeSections, info );
    SALONS2::TSalons SalonsL( point_id, SALONS2::rTripSalons );
    SalonsL.BuildLayersInfo( salonsNode, props );
    ProgTrace( TRACE5, "BuildLayersInfo for point_id=%d", point_id );
  }
  else {
    Salons.BuildLayersInfo( salonsNode, props );
    ProgTrace( TRACE5, "BuildLayersInfo for base comp" );
  }
  if ( pr_notchangecraft && comp_id >= 0 ) {
    vector<SALONS2::TCompSection> CompSections;
    ReadCompSections( comp_id, CompSections );
    BuildCompSections( dataNode, CompSections );
  }
}

void SalonFormInterface::ComponWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::ComponWrite, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amWrite );
  SALONS2::TSalons Salons( comp_id, SALONS2::rComponSalons );
  Salons.Parse( GetNode( "salons", reqNode ) );
  string smodify = NodeAsString( "modify", reqNode );
  if ( smodify == "delete" )
    Salons.modify = SALONS2::mDelete;
  else
    if ( smodify == "add" )
      Salons.modify = SALONS2::mAdd;
    else
      if ( smodify == "change" )
        Salons.modify = SALONS2::mChange;
  TReqInfo *r = TReqInfo::Instance();
  TElemFmt fmt;
  xmlNodePtr a = GetNode( "airline", reqNode );
  if ( a ) {
     Salons.airline = ElemToElemId( etAirline, NodeAsString( a ), fmt );
     if ( fmt == efmtUnknown )
     	 throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_INPUT" );
  }
  else
  	if ( r->user.access.airlines.size() == 1 )
  		Salons.airline = *r->user.access.airlines.begin();
 	a = GetNode( "airp", reqNode );
 	if ( a ) {
 		Salons.airp = ElemToElemId( etAirp, NodeAsString( a ), fmt );
 		if ( fmt == efmtUnknown )
 			throw AstraLocale::UserException( "MSG.AIRP.INVALID_SET_CODE" );
 		Salons.airline.clear();
 	}
 	else
  	if ( r->user.user_type != utAirline && r->user.access.airps.size() == 1 && !GetNode( "airline", reqNode ) ) {
  		Salons.airp = *r->user.access.airps.begin();
  		Salons.airline.clear();
    }
  if ( Salons.modify != SALONS2::mDelete ) {
    if ( (int)Salons.airline.empty() + (int)Salons.airp.empty() != 1 ) {
    	if ( Salons.airline.empty() )
    	  throw AstraLocale::UserException( "MSG.AIRLINE_OR_AIRP_MUST_BE_SET" );
    	else
    		throw AstraLocale::UserException( "MSG.NOT_SET_ONE_TIME_AIRLINE_AND_AIRP" ); // птому что компоновка принадлежит или авиакомпании или порту
    }

    if ( ( r->user.user_type == utAirline ||
           ( r->user.user_type == utSupport && Salons.airp.empty() && !r->user.access.airlines.empty() ) ) &&
    	   find( r->user.access.airlines.begin(),
    	         r->user.access.airlines.end(), Salons.airline ) == r->user.access.airlines.end() ) {
 	  	if ( Salons.airline.empty() )
 		  	throw AstraLocale::UserException( "MSG.AIRLINE.UNDEFINED" );
  	  else
    		throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRLINE" );
    }
    if ( ( r->user.user_type == utAirport ||
    	     ( r->user.user_type == utSupport && Salons.airline.empty() && !r->user.access.airps.empty() ) ) &&
    	   find( r->user.access.airps.begin(),
    	         r->user.access.airps.end(), Salons.airp ) == r->user.access.airps.end() ) {
 	  	if ( Salons.airp.empty() )
 	  		throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.NOT_SET_AIRP" );
 	  	else
 	  	  throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRP" );
    }
  }
  Salons.craft = NodeAsString( "craft", reqNode );
  if ( Salons.craft.empty() )
    throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
  Salons.craft = ElemToElemId( etCraft, Salons.craft, fmt );
  if ( fmt == efmtUnknown )
  	throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  
  //так стало
  Salons.verifyValidRem( "MCLS", "Э"); //???
  Salons.verifyValidRem( "SCLS", "Э"); //???
  Salons.verifyValidRem( "YCLS", "Э"); //???
  Salons.verifyValidRem( "LCLS", "Э"); //???

//так было!!!  Salons.verifyValidRem( "MCLS", "Э" );
  Salons.Write();
  if ( Salons.modify != SALONS2::mNone ) {
    string msg;
    switch ( Salons.modify ) {
      case SALONS2::mDelete:
        msg = string( "Удалена базовая компоновка (ид=" ) + IntToString( comp_id ) + ").";
        Salons.comp_id = -1;
        break;
      default:
        comp_id = Salons.comp_id;
        if ( Salons.modify == SALONS2::mAdd ) {
          msg = "Создана базовая компоновка (ид=";
        }
        else
          msg = "Изменена базовая компоновка (ид=";
        msg += IntToString( Salons.comp_id );
        msg += "). Код а/к: ";
        if ( Salons.airline.empty() )
        	msg += "не указан";
        else
      	  msg += Salons.airline;
        msg += ", код а/п: ";
        if ( Salons.airp.empty() )
      	  msg += "не указан";
        else
      	  msg += Salons.airp;
        msg += ", тип ВС: " + Salons.craft + ", борт: ";
        if ( Salons.bort.empty() )
          msg += "не указан";
        else
          msg += Salons.bort;
        msg += ", классы: " + Salons.classes + ", описание: ";
        if ( Salons.descr.empty() )
          msg += "не указано";
        else
          msg += Salons.descr;
        break;
    }
    r->MsgToLog( msg, evtComp, comp_id );
  }
  //bagsections
  vector<SALONS2::TCompSection> CompSections;
  xmlNodePtr sectionsNode = GetNode( "CompSections", reqNode );
  if ( sectionsNode && Salons.modify != SALONS2::mDelete ) {
    ParseCompSections( sectionsNode, CompSections );
    WriteCompSections( comp_id, CompSections );
  }
  if ( Salons.modify == SALONS2::mDelete )
    comp_id = -1;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", comp_id );
  if ( !Salons.airline.empty() )
    NewTextChild( dataNode, "airline", ElemIdToCodeNative( etAirline, Salons.airline ) );
  if ( !Salons.airp.empty() )
    NewTextChild( dataNode, "airp", ElemIdToCodeNative( etAirp, Salons.airp ) );
  if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
    NewTextChild( dataNode, "craft", ElemIdToCodeNative( etCraft, Salons.craft ) );
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
};

void IntChangeSeats( int point_id, int pax_id, int &tid, string xname, string yname,
                     SEATS2::TSeatsType seat_type,
                     TCompLayerType layer_type,
                     bool pr_waitlist, bool pr_question_reseat,
                     xmlNodePtr resNode )
{
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
   "SELECT layer_type FROM grp_status_types, pax, pax_grp "
   " WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND pax_grp.status=grp_status_types.code ";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
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
         TReqInfo::Instance()->client_type == ctTerm ) { //!!!строго завязать базовые компоновки с назначенными на рейс
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
  string xname;
  string yname;
  if ( seat_type != SEATS2::stDropseat ) {
    xname = NodeAsString( "xname", reqNode );
    yname = NodeAsString( "yname", reqNode );
  }
  IntChangeSeats( point_id, pax_id, tid, xname, yname,
                  seat_type,
                  DecodeCompLayerType( NodeAsString( "layer", reqNode, "" ) ),
                  GetNode( "waitlist", reqNode ),
                  GetNode( "question_reseat", reqNode ), resNode );

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
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool pr_update_salons = GetNode( "update_salons", reqNode );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

	ProgTrace(TRACE5, "SalonsInterface::DeleteProtCkinSeat, point_id=%d, pax_id=%d, tid=%d, pr_update_salons=%d",
	          point_id, pax_id, tid, pr_update_salons );

  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  vector<SALONS2::TSalonSeat> seats;

  try {
  	SEATS2::ChangeLayer( cltProtCkin, point_id, pax_id, tid, "", "", SEATS2::stDropseat, pr_lat_seat );
  	if ( pr_update_salons )
  	  SALONS2::getSalonChanges( Salons, seats );
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
      Salons.Build( salonsNode );
      if ( Salons.comp_id > 0 ) { //!!!строго завязать базовые компоновки с назначенными на рейс
        Qry.Clear();
 	      Qry.SQLText =
	        "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
	      TTripInfo info( Qry );
        if ( GetTripSets( tsCraftNoChangeSections, info ) ) {
   	      vector<SALONS2::TCompSection> CompSections;
          ReadCompSections( Salons.comp_id, CompSections );
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
	bool pr_filter = GetNode( "filter", reqNode );
	bool pr_salons = GetNode( "salons", reqNode );
	TQuery Qry( &OraSession );
  SEATS2::TPassengers p;
  if ( SEATS2::GetPassengersForWaitList( point_id, p ) ) {
  	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    p.Build( dataNode );
    if ( pr_filter ) {
      Qry.SQLText =
        "SELECT code, layer_type FROM grp_status_types";
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
      SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
      Salons.Read();
      Salons.Build( NewTextChild( dataNode, "salons" ) );
      SALONS2::GetTripParams( point_id, dataNode );
      if ( Salons.comp_id > 0 ) {
        Qry.Clear();
 	      Qry.SQLText =
	        "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
	      TTripInfo info( Qry );
        if ( GetTripSets( tsCraftNoChangeSections, info ) ) { //!!!строго завязать базовые компоновки с назначенными на рейс
   	      vector<SALONS2::TCompSection> CompSections;
          ReadCompSections( Salons.comp_id, CompSections );
          BuildCompSections( dataNode, CompSections );
        }
      }
    }
  }
}

void SalonFormInterface::AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "trip_id", reqNode );
	ProgTrace( TRACE5, "AutoSeats: point_id=%d", point_id );
	bool pr_waitlist = GetNode( "waitlist", reqNode );
	TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo info( Qry );
  SEATS2::TPassengers p;
  if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
  	throw UserException( "MSG.SEATS.ALL_PASSENGERS_PLANED" );
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  vector<SALONS2::TSalonSeat> seats;
  Salons.Read();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  bool pr_notchangecraft = GetTripSets( tsCraftNoChangeSections, info );
  try {
    SEATS2::AutoReSeatsPassengers( Salons, p, SEATS2::GetSeatAlgo( Qry, info.airline, info.flt_no, info.airp ) );
    tst();
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
    	p.Clear();
    	if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
            AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
      else
          AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      p.Build( dataNode );
    }
    tst();
    if ( Salons.comp_id > 0 && pr_notchangecraft ) { //!!!строго завязать базовые компоновки с назначенными на рейс
      tst();
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( Salons.comp_id, CompSections );
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
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
      p.Clear();
    	if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
            AstraLocale::showErrorMessage( "MSG.SEATS.PAX_SEATS_NOT_FULL" );
      else
          AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
      p.Build( dataNode );
    }
    if ( Salons.comp_id > 0 && pr_notchangecraft ) { //!!!строго завязать базовые компоновки с назначенными на рейс
 	    vector<SALONS2::TCompSection> CompSections;
      ReadCompSections( Salons.comp_id, CompSections );
      BuildCompSections( dataNode, CompSections );
    }
  	showErrorMessageAndRollback( ue.getLexemaData( ) );
  }
}

void trace( int pax_id, int grp_id, int parent_pax_id, int crs_pax_id, const std::string &pers_type, int seats )
{
  ProgTrace( TRACE5, "pax_id=%d, grp_id=%d, parent_pax_id=%d, crs_pax_id=%d, pers_type=%s, seats=%d",
             pax_id, grp_id, parent_pax_id, crs_pax_id, pers_type.c_str(), seats );
}
