#include <stdlib.h>
#include "basic.h"
#include "stages.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "telegram.h"
#include "astra_service.h"
#include "timer.h"
#include "salons2.h"
#include "tripinfo.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

TTripStages::TTripStages( int vpoint_id )
{
  point_id = vpoint_id;
  TTripStages::LoadStages( vpoint_id, tripstages );
}

void TTripStages::LoadStages( int vpoint_id, TMapTripStages &ts )
{
  ts.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT stage_id, scd, est, act, pr_auto, pr_manual "\
                " FROM trip_stages WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", vpoint_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TTripStage  tripStage;
    if ( Qry.FieldIsNULL( "scd" ) )
      tripStage.scd = NoExists;
    else
      tripStage.scd = Qry.FieldAsDateTime( "scd" );
    if ( Qry.FieldIsNULL( "est" ) )
      tripStage.est = NoExists;
    else
      tripStage.est = Qry.FieldAsDateTime( "est" );
    if ( Qry.FieldIsNULL( "act" ) )
      tripStage.act = NoExists;
    else
      tripStage.act = Qry.FieldAsDateTime( "act" );
    tripStage.pr_auto = Qry.FieldAsInteger( "pr_auto" );
    TStage stage = (TStage)Qry.FieldAsInteger( "stage_id" );
    ts.insert( make_pair( stage, tripStage ) );
    Qry.Next();
  }
}

void TTripStages::ParseStages( xmlNodePtr node, TMapTripStages &ts )
{
	ts.clear();
	node = node->children;
	xmlNodePtr n,x;
	while ( node ) {
		TTripStage  tripStage;
		n = node->children;
		x = GetNodeFast( "scd", n );
		if ( x )
		  tripStage.scd = NodeAsDateTime( x );
		else
		  tripStage.scd = NoExists;
		x = GetNodeFast( "est", n );
		if ( x )
		  tripStage.est = NodeAsDateTime( x );
		else
		  tripStage.est = NoExists;
		x = GetNodeFast( "act", n );
		if ( x )
		  tripStage.act = NodeAsDateTime( x );
		else
		  tripStage.act = NoExists;
		x = GetNodeFast( "old_est", n );
		if ( x )
		  tripStage.old_est = NodeAsDateTime( x );
		else
		  tripStage.old_est = NoExists;
		x = GetNodeFast( "old_act", n );
		if ( x )
		  tripStage.old_act = NodeAsDateTime( x );
		else
		  tripStage.old_act = NoExists;
		ts.insert( make_pair( (TStage)NodeAsIntegerFast( "stage_id", n ), tripStage ) );
		node = node->next;
	}
}

void TTripStages::WriteStages( int point_id, TMapTripStages &ts )
{
	TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT airp FROM points WHERE points.point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string region, airp;
  airp = Qry.FieldAsString( "airp" );
	if ( reqInfo->user.sets.time == ustTimeLocalAirp )
 	  region = AirpTZRegion( airp );
  TQuery UpdQry( &OraSession );
  Qry.SQLText =
    "BEGIN "
    " UPDATE trip_stages SET est=:est,act=:act,pr_auto=DECODE(:pr_auto,-1,pr_auto,:pr_auto), "
    "                        pr_manual=DECODE(:pr_manual,-1,pr_manual,:pr_manual) "
    "  WHERE point_id=:point_id AND stage_id=:stage_id; "
    " IF SQL%NOTFOUND THEN "
    "  INSERT INTO trip_stages(point_id,stage_id,scd,est,act,pr_auto,pr_manual) "
    "   SELECT :point_id,:stage_id,NVL(:act,:est),:est,:act,0,DECODE(:pr_manual,-1,0,:pr_manual) FROM dual; "
    " END IF; "
    "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "stage_id", otInteger );
  Qry.DeclareVariable( "est", otDate );
  Qry.DeclareVariable( "act", otDate );
  Qry.DeclareVariable( "pr_auto", otInteger );
  Qry.DeclareVariable( "pr_manual", otInteger );

  TStagesRules *sr = TStagesRules::Instance();
  TCkinClients CkinClients;
  TTripStages::ReadCkinClients( point_id, CkinClients );

  for ( TMapTripStages::iterator i=ts.begin(); i!=ts.end(); i++ ) {

    if ( sr->isClientStage( (int)i->first ) && !sr->canClientStage( CkinClients, (int)i->first ) )
      continue;

   	Qry.SetVariable( "stage_id", (int)i->first );
    if ( i->second.est == NoExists )
       Qry.SetVariable( "est", FNull );
    else
   	 try {
    	  Qry.SetVariable( "est", ClientToUTC( i->second.est, region ) );
     }
     catch( boost::local_time::ambiguous_result ) {
       throw AstraLocale::UserException( "MSG.STAGE.EST_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
               LParams() << LParam("stage", TStagesRules::Instance()->stage_name( i->first, airp, true ))
               << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
     }
    if ( i->second.act == NoExists )
      Qry.SetVariable( "act", FNull );
    else
    	try {
        Qry.SetVariable( "act", ClientToUTC( i->second.act, region ) );
      }
      catch( boost::local_time::ambiguous_result ) {
       throw AstraLocale::UserException( "MSG.STAGE.ACT_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
               LParams() << LParam("stage", TStagesRules::Instance()->stage_name( i->first, airp, true ))
               << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
      }
    if ( i->second.old_act > NoExists && i->second.act == NoExists )
       Qry.SetVariable( "pr_auto", 0 );
     else
       Qry.SetVariable( "pr_auto", -1 );
    int pr_manual;
    if ( i->second.est == i->second.old_est )
      pr_manual = -1;
    else
      if ( i->second.est == NoExists )
        pr_manual = 0;
      else
     	  pr_manual = 1;
    Qry.SetVariable( "pr_manual", pr_manual );
    Qry.Execute( );


    if ( i->second.old_act == NoExists && i->second.act > NoExists ) { // вызов функции обработки шага
      try {
         exec_stage( point_id, (int)i->first );
      }
      catch( Exception &E ) {
        ProgError( STDLOG, "Exception: %s", E.what() );
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "std::exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
    }

 	  check_brd_alarm( point_id );

    string tolog = string( "Этап '" ) + sr->stage_name( i->first, airp, false ) + "'";
    if ( i->second.old_act == NoExists && i->second.act > NoExists )
      tolog += " выполнен";
    if ( i->second.old_act > NoExists && i->second.act == NoExists )
      tolog += " отменен";
    tolog += ": расч. время";
    if ( i->second.est > NoExists )
      tolog += DateTimeToStr( ClientToUTC( i->second.est, region ), "=hh:nn dd.mm.yy (UTC)" );
    else
      tolog += " не задано";
    tolog += ", факт. время";
    if ( i->second.act > NoExists )
      tolog += DateTimeToStr( ClientToUTC( i->second.act, region ), "=hh:nn dd.mm.yy (UTC)" );
    else
       tolog += " не задано";
    reqInfo->MsgToLog( tolog, evtGraph, point_id, (int)i->first );
  }
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "  gtimer.sync_trip_final_stages(:point_id); "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}


void TTripStages::LoadStages( int vpoint_id )
{
  point_id = vpoint_id;
  TTripStages::LoadStages( vpoint_id, tripstages );
}

TDateTime TTripStages::time( TStage stage )
{
  if ( tripstages.empty() )
    throw Exception( "tripstages is empty" );
  TTripStage tripStage = tripstages[ stage ];
  if ( tripStage.act > NoExists )
    return tripStage.act;
  else
    if ( tripStage.est > NoExists )
      return tripStage.est;
    else
      return tripStage.scd;
}

void TTripStages::ReadCkinClients( int point_id, TCkinClients &ckin_clients )
{
	ckin_clients.clear();
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT client_type FROM trip_ckin_client "
  	" WHERE point_id=:point_id AND pr_permit!=0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
  	ProgTrace( TRACE5, "client_type=%s", Qry.FieldAsString( "client_type" ) );
  	ckin_clients.push_back( Qry.FieldAsString( "client_type" ) );
  	Qry.Next();
  }
}

TStage TTripStages::getStage( TStage_Type stage_type )
{
	if ( CkinClients.empty() )
		TTripStages::ReadCkinClients( point_id, CkinClients );
  TStagesRules *sr = TStagesRules::Instance();
  int level = 0;
  int p_level = 0;
  TStage res = sNoActive;
  for ( TGraph_Level::iterator l=sr->GrphLvl.begin(); l!=sr->GrphLvl.end(); l++ ) {
    if ( p_level > 0 && p_level < l->level ) /* пока не перешли на след. ветку */
      continue;

    if ( sr->isClientStage( l->stage ) && !sr->canClientStage( CkinClients, l->stage ) )
    	continue;

    if ( tripstages[ l->stage ].act == NoExists ) { /* надо отсечь все низшие вершины */
      p_level = l->level;
      continue;
    }
    p_level = 0; /* этот узел удовлетворяет нас */
    /* предыдущий узел был ниже и нас удовлетворял - это первый попавшийся */
    if ( level >= l->level )
      break;
    if ( sr->CanStatus( stage_type, l->stage ) ) { /* наша служба и опасна и трудна */
      res = l->stage;
      level = l->level;
    }
  }
  return res;
}
/********************************************************************************/
TStagesRules *TStagesRules::Instance()
{
  static TStagesRules *instance_ = 0;
  if ( !instance_ )
    instance_ = new TStagesRules();
  return instance_;
}

TStagesRules::TStagesRules()
{
	UpdateGraph_Stages( );

  Update();
}

void TStagesRules::UpdateGraph_Stages( )
{
	Graph_Stages.clear();
	ClientStages.clear();
	TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT stage_id, name, name_lat, NULL airp FROM graph_stages "
    "UNION "
    "SELECT stage_id, name, name_lat, airp FROM stage_names "
    " ORDER BY stage_id, airp";
  Qry.Execute();

  while ( !Qry.Eof ) {
  	TStage_name n;
  	n.stage = (TStage)Qry.FieldAsInteger( "stage_id" );
  	n.airp = Qry.FieldAsString( "airp" );
  	n.name = Qry.FieldAsString( "name" );
  	n.name_lat = Qry.FieldAsString( "name_lat" );
  	Graph_Stages.push_back( n );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT client_type, stage_id FROM ckin_client_stages ORDER BY stage_id, client_type";
  Qry.Execute();
  while ( !Qry.Eof ) {
  	ClientStages[ Qry.FieldAsInteger( "stage_id" ) ].push_back( Qry.FieldAsString( "client_type" ) );
  	Qry.Next();
  }

}

void TStagesRules::Update()
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText = "SELECT target_stage,cond_stage,num,next "\
                "FROM graph_rules "\
                "ORDER BY target_stage,num";
 Qry.Execute();
 TStageStep Step;
 TRule rule;
 while ( !Qry.Eof ) {
   TStage Stage = (TStage)Qry.FieldAsInteger( "target_stage" );
   if ( Qry.FieldAsInteger( "next" ) )
     Step = stNext;
   else
     Step = stPrior;
   rule.num = Qry.FieldAsInteger( "num" );
   rule.cond_stage = (TStage)Qry.FieldAsInteger( "cond_stage" );
   GrphRls[ Step ][ Stage ].push_back( rule );
   Qry.Next();
 }
 /* загрузка статусов */
 Qry.Clear();
 Qry.SQLText =
   "SELECT stage_id, stage_type, status, status_lat "
   " FROM stage_statuses ORDER BY stage_type";
 Qry.Execute();
 TStage_Type stage_type;
 TStage_Status status;
 while ( !Qry.Eof ) {
   stage_type = (TStage_Type)Qry.FieldAsInteger( "stage_type" );
   status.stage = (TStage)Qry.FieldAsInteger( "stage_id" );
   status.status = Qry.FieldAsString( "status" );
   status.status_lat = Qry.FieldAsString( "status_lat" );
   StageStatuses[ stage_type ].push_back( status );
   Qry.Next();
 }

 Qry.Clear();
 Qry.SQLText = "SELECT target_stage, level "\
               " FROM "\
               "( SELECT target_stage, cond_stage "\
               "    FROM graph_rules "\
               " WHERE next = 1 ) a "\
               " START WITH cond_stage = 0 "\
               " CONNECT BY PRIOR target_stage = cond_stage";
 Qry.Execute();
 while ( !Qry.Eof ) {
   TStage_Level gl;
   gl.stage = (TStage)Qry.FieldAsInteger( "target_stage" );
   gl.level = Qry.FieldAsInteger( "level" );
   GrphLvl.push_back( gl );
   Qry.Next();
 }
}

string getLocaleName( const string &name, const string &name_lat, bool pr_locale )
{
	string res;
	if ( !pr_locale || TReqInfo::Instance()->desk.lang == AstraLocale::LANG_RU || name_lat.empty() )
		res = name;
	else
		res = name_lat;
	return res;
}

void TStagesRules::BuildGraph_Stages( const string airp, xmlNodePtr dataNode )
{
  xmlNodePtr snode, node = NewTextChild( dataNode, "Graph_Stages" );
  if ( !airp.empty() )
  	SetProp( node, "airp", airp );
  for ( vector<TStage_name>::iterator i=Graph_Stages.begin(); i!=Graph_Stages.end(); i++ ) {
  	if ( airp.empty() || airp == i->airp ) {
      if ( !isClientStage( i->stage ) || TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )	{
        snode = NewTextChild( node, "stage" );
        NewTextChild( snode, "stage_id", (int)i->stage );
        NewTextChild( snode, "name", getLocaleName( i->name, i->name_lat, true ) );
        NewTextChild( snode, "airp", i->airp );
      }
    }
  }
}

void TStagesRules::Build( xmlNodePtr dataNode )
{
  xmlNodePtr node = NewTextChild( dataNode, "GrphRls" );
  xmlNodePtr snode;
  for ( map<TStageStep,TMapRules>::iterator st=GrphRls.begin(); st!=GrphRls.end(); st++ ) {
  	snode = NewTextChild( node, "stagestep" );
    if ( st->first == stPrior )
      SetProp( snode, "step", "stPrior" );
    else
      SetProp( snode, "step", "stNext" );
    for ( map<TStage,vecRules>::iterator r=st->second.begin(); r!=st->second.end(); r++ ) {
    	if ( !isClientStage( r->first ) || TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )	{
        xmlNodePtr stagerulesNode = NewTextChild( snode, "stagerules" );
        SetProp( stagerulesNode, "stage", r->first );
        for ( vecRules::iterator v=r->second.begin(); v!=r->second.end(); v++ ) {
        	if ( !isClientStage( v->cond_stage ) || TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )	{
            xmlNodePtr ruleNode = NewTextChild( stagerulesNode, "rule" );
            NewTextChild( ruleNode, "num", v->num );
            NewTextChild( ruleNode, "cond_stage", v->cond_stage );
          }
        }
      }
    }
  }
  node = NewTextChild( dataNode, "StageStatuses" );
  for ( TMapStatuses::iterator m=StageStatuses.begin(); m!=StageStatuses.end(); m++ ) {
  	if ( m->first != stCheckIn &&
  		   m->first != stBoarding &&
  		   m->first != stCraft &&
  		   !TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )
  		continue;
    snode = NewTextChild( node, "stage_type" );
    SetProp( snode, "type", m->first );
    for ( TStage_Statuses::iterator s=m->second.begin(); s!=m->second.end(); s++ ) {
      if ( !isClientStage( s->stage ) || TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )	{
        xmlNodePtr stagestatusNode = NewTextChild( snode, "stagestatus" );
        NewTextChild( stagestatusNode, "stage", (int)s->stage );
        NewTextChild( stagestatusNode, "status", getLocaleName( s->status, s->status_lat, true ) );
      }
    }
  }

  BuildGraph_Stages( "", dataNode );

  node = NewTextChild( dataNode, "GrphLvl" );
  for ( TGraph_Level::iterator l=GrphLvl.begin(); l!=GrphLvl.end(); l++ ) {
    if ( !isClientStage( l->stage ) || TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION ) )	{
 	    snode = NewTextChild( node, "stage_level" );
      NewTextChild( snode, "stage", (int)l->stage );
      NewTextChild( snode, "level", (int)l->level );
    }
  }
}

bool TStagesRules::CanStatus( TStage_Type stage_type, TStage stage )
{
  TStage_Statuses &v = StageStatuses[ stage_type ];
  for( TStage_Statuses::iterator s=v.begin(); s!=v.end(); s++ ) {
    if ( s->stage == stage )
      return true;
  }
  return false;
}

string TStagesRules::status( TStage_Type stage_type, TStage stage, bool pr_locale )
{
  TStage_Statuses &v = StageStatuses[ stage_type ];
  for( TStage_Statuses::iterator s=v.begin(); s!=v.end(); s++ ) {
    if ( s->stage == stage )
      return getLocaleName( s->status, s->status_lat, pr_locale );
  }
  return "";
}

string TStagesRules::stage_name( TStage stage, std::string airp, bool pr_locale )
{
	string res, res1;
	for ( vector<TStage_name>::iterator n=Graph_Stages.begin(); n!=Graph_Stages.end(); n++ ) {
		if ( n->stage == stage )
  		if ( n->airp.empty() )
	  		res1 = getLocaleName( n->name, n->name_lat, pr_locale );
			else
			  if ( n->airp == airp )
			  	res = getLocaleName( n->name, n->name_lat, pr_locale );
	}
	if ( res.empty() )
		return res1;
	else
		return res;
}

bool TStagesRules::canClientStage( const TCkinClients &ckin_clients, int stage_id )
{
	for ( TCkinClients::const_iterator i=ckin_clients.begin(); i!=ckin_clients.end(); i++ ) {
	 ProgTrace( TRACE5, "stage_id=%d, TCkinClients::const_iterator i=%s, clientstages.size()=%d", stage_id, (*i).c_str(), ClientStages[ stage_id ].size() );
	 if ( find( ClientStages[ stage_id ].begin(), ClientStages[ stage_id ].end(), *i ) !=  ClientStages[ stage_id ].end() ) {
	 	 tst();
	 	 return true;
	 }
	}
	return false;
}

bool TStagesRules::isClientStage( int stage_id )
{
	return !ClientStages[ stage_id ].empty();
}



TStageTimes::TStageTimes( TStage istage )
{
	stage = istage;
	GetStageTimes( );
}

void TStageTimes::GetStageTimes( )
{
  times.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText  = "SELECT airp, craft, trip_type, time, "
                 "       DECODE( airp, NULL, 0, 4 )+ "
                 "       DECODE( craft, NULL, 0, 2 )+ "
                 "       DECODE( trip_type, NULL, 0, 1 ) AS priority "
                 " FROM graph_times "
                 " WHERE stage_id=:stage "
                 "UNION "
                 "SELECT NULL, NULL, NULL, time, -1 FROM graph_stages "
                 "WHERE stage_id=:stage "
                 " ORDER BY priority, airp, craft, trip_type ";
  Qry.CreateVariable( "stage", otInteger, stage );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TStageTime st;
    st.airp = Qry.FieldAsString( "airp" );
    st.craft = Qry.FieldAsString( "craft" );
    st.trip_type = Qry.FieldAsString( "trip_type" );
    st.time = Qry.FieldAsInteger( "time" );
    st.priority = Qry.FieldAsInteger( "priority" );
    times.push_back( st );
    Qry.Next();
  }
}

TDateTime TStageTimes::GetTime( const string &airp, const string &craft, const string &triptype,
                                TDateTime vtime )
{
	TDateTime res = NoExists;
	if ( vtime == NoExists )
		return res;
	if ( times.empty() )
	  GetStageTimes( );
	for ( vector<TStageTime>::iterator st=times.begin(); st!=times.end(); st++ ) {
   	if ( ( st->airp == airp || st->airp.empty() ) &&
   		   ( st->craft == craft || st->craft.empty() ) &&
         ( st->trip_type == triptype || st->trip_type.empty() ) ) {
       res = vtime - (double)st->time/1440.0;
       break;
    }
	}
	return res;
}

void exec_stage( int point_id, int stage_id )
{
//	ProgTrace( TRACE5, "exec_stage: point_id=%d, stage_id=%d", point_id, stage_id );
  switch( (TStage)stage_id ) {
  	case sNoActive:
           /*не активен*/
  	   break;
    case sPrepCheckIn:
           /*Подготовка к регистрации*/
           PrepCheckIn( point_id );
           break;
    case sOpenCheckIn:
           /*Открытие регистрации*/
           OpenCheckIn( point_id );
           break;
  	case sOpenWEBCheckIn:
           /*открытие WEB-регистрации*/
  	     break;
  	case sOpenKIOSKCheckIn:
           /*открытие KIOSK-регистрации*/
  	     break;
    case sCloseCheckIn:
           /*Закрытие регистрации*/
           CloseCheckIn( point_id );
           break;
  	case sCloseWEBCheckIn:
           /*закрытие WEB-регистрации*/
  	     break;
  	case sCloseKIOSKCheckIn:
           /*закрытие KIOSK-регистрации*/
  	     break;
    case sOpenBoarding:
           /*Начало посадки*/
           break;
    case sCloseBoarding:
           /*Окончание посадки (оформление документации)*/
           CloseBoarding( point_id );
           break;
    case sRemovalGangWay:
           /*Уборка трапа*/
           break;
    case sTakeoff:
           /*Вылетел*/
           Takeoff( point_id );
           break;
  }
}


void astra_timer( TDateTime utcdate )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT trip_stages.point_id point_id, trip_stages.stage_id stage_id, points.airp "
   " FROM points, trip_stages "\
   "WHERE points.point_id = trip_stages.point_id AND "\
   "      points.act_out IS NULL AND "\
   "      points.pr_del = 0 AND "\
   "      trip_stages.pr_auto = 1 AND "\
   "      trip_stages.act IS NULL AND "\
   "      NVL( trip_stages.est, trip_stages.scd ) <= :now "\
   " ORDER BY trip_stages.point_id, trip_stages.stage_id ";
  Qry.CreateVariable( "now", otDate, utcdate );
  TQuery QExecStage(&OraSession);
  QExecStage.SQLText =
   "BEGIN "
   " :exec_stage := gtimer.ExecStage(:point_id,:stage_id,:act);"
   "END;";
  QExecStage.DeclareVariable( "point_id", otInteger );
  QExecStage.DeclareVariable( "stage_id", otInteger );
  QExecStage.DeclareVariable( "exec_stage", otInteger );
  QExecStage.DeclareVariable( "act", otDate );
  bool pr_exit = false;
  int count=0;

  TDateTime execTime0 = NowUTC();
  while ( !pr_exit ) {
  	pr_exit = true;
  	TDateTime execTime1 = NowUTC();
  	Qry.Execute();
		if ( NowUTC() - execTime1 > 1.0/(1440.0*60) )
  		ProgTrace( TRACE5, "Attention execute Query1 time > 1 sec !!!, time=%s, count=%d", DateTimeToStr( NowUTC() - execTime1, "nn:ss" ).c_str(), count );

  	while ( !Qry.Eof ) { // пробег по шагам, которые пора выполнить
  		count++;
  		int point_id = Qry.FieldAsInteger( "point_id" );
  		int stage_id = Qry.FieldAsInteger( "stage_id" );
  		string airp = Qry.FieldAsString( "airp" );
  		QExecStage.SetVariable( "point_id", point_id );
  		QExecStage.SetVariable( "stage_id", stage_id );
  	  TDateTime execTime2 = NowUTC();
  	  bool pr_exec_stage = false;
  	  try {
  		  QExecStage.Execute(); // признак того должен ли выполниться шаг + отметка о выполнении шага тех. графика
  		  if ( NowUTC() - execTime2 > 1.0/(1440.0*60) )
    		  ProgTrace( TRACE5, "Attention execute QCanStage time > 1 sec !!!, time=%s, count=%d", DateTimeToStr( NowUTC() - execTime2, "nn:ss" ).c_str(), count );
  		  pr_exec_stage = QExecStage.GetVariableAsInteger( "exec_stage" );
  		  TDateTime act_stage = QExecStage.GetVariableAsDateTime( "act" );
  		  if ( pr_exec_stage ) {
    		  // запись в лог о выполнении шага
          TStagesRules *r = TStagesRules::Instance();
          string tolog = string( "Этап '" ) + r->stage_name( (TStage)stage_id, airp, false ) + "'";
          tolog += " выполнен: факт. время=";
          tolog += DateTimeToStr( act_stage, "hh:nn dd.mm.yy (UTC)" );
          TReqInfo::Instance()->MsgToLog( tolog, evtGraph, point_id, stage_id );
  		  }
  		}
      catch( Exception &E ) {
      	try { OraSession.Rollback( ); } catch(...) { };
        ProgError( STDLOG, "Ошибка astra_timer: %s. Время %s, point_id=%d, stage_id=%d",
                   E.what(),
                   DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                   point_id, stage_id );
      }
			catch( ... ) {
				try { OraSession.Rollback( ); } catch(...) { };
				ProgError( STDLOG, "unknown timer error" );
 			}
 			OraSession.Commit(); // запоминание факта выполнения шага + лога в БД
  		if ( pr_exec_stage ) { // выполняем действия связанные с этим шагом
				pr_exit = false; // признак того, что надо бы проверить следующие шаги графика на то, что их можно и пора выполнить
  			TDateTime execStep = NowUTC();
  			try {
			    exec_stage( point_id, stage_id );
          if ( NowUTC() - execStep > 1.0/(1440.0*60) )
 	          ProgTrace( TRACE5, "Attention execute point_id=%d, stage_id=%d time > 1 sec !!!, time=%s, count=%d", point_id, stage_id,
 	                     DateTimeToStr( NowUTC() - execStep, "nn:ss" ).c_str(), count );
			  }
        catch( Exception &E ) {
      	  try { OraSession.Rollback( ); } catch(...) { };
          ProgError( STDLOG, "Ошибка astra_timer: %s. Время %s, point_id=%d, stage_id=%d",
                     E.what(),
                     DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                     point_id, stage_id );
        }
			  catch( ... ) {
			  	try { OraSession.Rollback( ); } catch(...) { };
			  	ProgError( STDLOG, "unknown timer error" );
 			  }
  		}
      OraSession.Commit();
  		Qry.Next();
  	}
  }
	if ( NowUTC() - execTime0 > 5.0/(1440.0*60) )
  	ProgTrace( TRACE5, "Attention execute astra_time > 5 sec !!!, time=%s, steps count=%d", DateTimeToStr( NowUTC() - execTime0, "nn:ss" ).c_str(), count );
}

void SetCraft( int point_id, TStage stage )
{
	if ( stage != sPrepCheckIn && stage != sOpenCheckIn )
		return;
	TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT craft, b.bort, airp FROM points, "
    "( SELECT points.bort, points.point_id FROM comps, points "
    "  WHERE points.point_id = :point_id AND "
    "        points.craft = comps.craft AND "
    "        points.bort IS NOT NULL AND "
    "        points.bort = comps.bort AND "
    "        rownum < 2 ) b "
    "WHERE points.point_id = :point_id AND "
    "      points.point_id = b.point_id(+) ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string craft = Qry.FieldAsString( "craft" );
  if ( stage == sPrepCheckIn && (!Qry.FieldIsNULL( "bort" ) || string( "СОЧ" ) != Qry.FieldAsString( "airp" )) ||
  	   stage == sOpenCheckIn && string( "СОЧ" ) == Qry.FieldAsString( "airp" ) ) {
    if ( SALONS::AutoSetCraft( point_id, craft, -1 ) < 0 ) {
  	  TReqInfo::Instance()->MsgToLog( string( "Подходящая для рейса компоновка " ) + craft + " не найдена", evtFlt, point_id );
  	}
  }
}

void PrepCheckIn( int point_id )
{
	SetCraft( point_id, sPrepCheckIn );
}

void OpenCheckIn( int point_id )
{
	tst();
	SetCraft( point_id, sOpenCheckIn );
	TQuery Qry(&OraSession);
	Qry.SQLText = "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
}

void CloseCheckIn( int point_id )
{
  try
  {
    vector<string> tlg_types;
    tlg_types.push_back("COM");
    tlg_types.push_back("PRLC");
    TelegramInterface::SendTlg(point_id,tlg_types);
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
  };

  try
  {
    CreateCentringFileDATA( point_id );
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.CreateCentringFileDATA (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.CreateCentringFileDATA (point_id=%d): %s",point_id,E.what());
  };

};

void CloseBoarding( int point_id )
{
  try
  {
    vector<string> tlg_types;
    tlg_types.push_back("COM");
    TelegramInterface::SendTlg(point_id,tlg_types);
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.SendTlg (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.SendTlg (point_id=%d): %s",point_id,E.what());
  };

  try
  {
    CreateCentringFileDATA( point_id );
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.CreateCentringFileDATA (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.CreateCentringFileDATA (point_id=%d): %s",point_id,E.what());
  };
};

void Takeoff( int point_id )
{
  time_t time_start,time_end;

  time_start=time(NULL);
  try
  {
  	TQuery Qry(&OraSession);
  	Qry.Clear();
  	Qry.SQLText=
  	  "BEGIN "
  	  "  statist.get_full_stat(:point_id, 0); "
  	  "END;";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.Execute();
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"Takeoff.get_full_stat (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.get_full_stat (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! statist.get_full_stat execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);

  time_start=time(NULL);
  try
  {
    vector<string>  tlg_types;
    tlg_types.push_back("PTM");
    tlg_types.push_back("PTMN");
    tlg_types.push_back("BTM");
    tlg_types.push_back("TPM");
    tlg_types.push_back("PSM");
    tlg_types.push_back("PFS");
    tlg_types.push_back("PFSN");
    tlg_types.push_back("FTL");
    tlg_types.push_back("PRL");
    tlg_types.push_back("SOM");
//    tlg_types.push_back("ETL"); формируем по прилету в конечные пункт если не было интерактива с СЭБ
    tlg_types.push_back("LDM");
    tlg_types.push_back("CPM");
    TelegramInterface::SendTlg(point_id,tlg_types);
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"Takeoff.SendTlg (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.SendTlg (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! TelegramInterface::SendTlg execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);

  time_start=time(NULL);
  try
  {
    create_czech_police_file(point_id,true);
    create_czech_police_file(point_id,false);
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"Takeoff.create_czech_police_file (point_id=%d): %s",point_id,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.create_czech_police_file (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! create_czech_police_file execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);
}



