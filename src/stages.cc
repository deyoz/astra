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
#include "apis.h"
#include "salons.h"
#include "term_version.h"
#include "comp_layers.h"
#include "alarms.h"
#include "rozysk.h"
#include "points.h"
#include "trip_tasks.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

bool CompatibleStage( TStage stage )
{
  if ( !TStagesRules::Instance()->isClientStage( stage ) )
    return true;
    
  if ( stage == sOpenWEBCheckIn ||
       stage == sOpenKIOSKCheckIn ||
       stage == sCloseWEBCheckIn ||
       stage == sCloseKIOSKCheckIn )
    return TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION );
    
  if ( stage == sCloseWEBCancel )
    return TReqInfo::Instance()->desk.compatible( WEB_CANCEL_VERSION );
    
  return false;
}

bool CompatibleStageType( TStage_Type stage_type )
{
  if ( stage_type == stCheckIn ||
  		 stage_type == stBoarding ||
  		 stage_type == stCraft )
  	return true;
  	
  if ( stage_type == stWEBCheckIn ||
  		 stage_type == stKIOSKCheckIn )
    return TReqInfo::Instance()->desk.compatible( WEB_CHECKIN_VERSION );
    
  if ( stage_type == stWEBCancel )
    return TReqInfo::Instance()->desk.compatible( WEB_CANCEL_VERSION );

  return false;
}

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
    tripStage.old_est = tripStage.est;
    if ( Qry.FieldIsNULL( "act" ) )
      tripStage.act = NoExists;
    else
      tripStage.act = Qry.FieldAsDateTime( "act" );
    tripStage.old_act = tripStage.act;
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

void TTripStages::WriteStagesUTC( int point_id, TMapTripStages &ts )
{
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock();

	TReqInfo *reqInfo = TReqInfo::Instance();
	std::string airp;
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT act_out,airp,pr_del FROM points WHERE points.point_id=:point_id";// FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  int pr_del = Qry.FieldAsInteger( "pr_del" );
  airp = Qry.FieldAsString( "airp" );
  bool pr_act_out = !Qry.FieldIsNULL( "act_out" );
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    " UPDATE trip_stages SET est=:est,act=:act,pr_auto=DECODE(:pr_auto,-1,pr_auto,:pr_auto), "
    "                        pr_manual=DECODE(:pr_manual,-1,pr_manual,:pr_manual) "
    "  WHERE point_id=:point_id AND stage_id=:stage_id; "
    " IF SQL%NOTFOUND THEN "
    "  INSERT INTO trip_stages(point_id,stage_id,scd,est,act,pr_auto,pr_manual,ignore_auto) "
    "   SELECT :point_id,:stage_id,NVL(:act,:est),:est,:act,0,DECODE(:pr_manual,-1,0,:pr_manual),:ignore_auto FROM dual; "
    " END IF; "
    "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "stage_id", otInteger );
  Qry.DeclareVariable( "est", otDate );
  Qry.DeclareVariable( "act", otDate );
  Qry.DeclareVariable( "pr_auto", otInteger );
  Qry.DeclareVariable( "pr_manual", otInteger );
  Qry.CreateVariable( "ignore_auto", otInteger, pr_act_out || pr_del != 0 );

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
    	 Qry.SetVariable( "est", i->second.est );
    if ( i->second.act == NoExists )
      Qry.SetVariable( "act", FNull );
    else
      Qry.SetVariable( "act", i->second.act );
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

    if ( i->second.old_act == NoExists && i->second.act > NoExists ) { // �맮� �㭪樨 ��ࠡ�⪨ 蠣�
      try {
         exec_stage( point_id, (int)i->first );
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "std::exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
    }

 	  check_brd_alarm( point_id );
    check_unattached_trfer_alarm( point_id );
    check_crew_alarms( point_id );

    string tolog = string( "�⠯ '" ) + sr->stage_name( i->first, airp, false ) + "'";
    if ( i->second.old_act == NoExists && i->second.act > NoExists )
      tolog += " �믮����";
    if ( i->second.old_act > NoExists && i->second.act == NoExists )
      tolog += " �⬥���";
    tolog += ": ���. �६�";
    if ( i->second.est > NoExists )
      tolog += DateTimeToStr( i->second.est, "=hh:nn dd.mm.yy (UTC)" );
    else
      tolog += " �� ������";
    tolog += ", 䠪�. �६�";
    if ( i->second.act > NoExists )
      tolog += DateTimeToStr( i->second.act, "=hh:nn dd.mm.yy (UTC)" );
    else
       tolog += " �� ������";
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

void TTripStages::WriteStages( int point_id, TMapTripStages &ts )
{
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock();

	TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT airp FROM points WHERE points.point_id=:point_id";// FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string region, airp;
  airp = Qry.FieldAsString( "airp" );
  tst();
	if ( reqInfo->user.sets.time == ustTimeLocalAirp )
 	  region = AirpTZRegion( airp );
  for ( TMapTripStages::iterator i=ts.begin(); i!=ts.end(); i++ ) {
    if ( i->second.est != NoExists ) {
      try {
   	    i->second.est = ClientToUTC( i->second.est, region );
      }
      catch( boost::local_time::ambiguous_result ) {
         throw AstraLocale::UserException( "MSG.STAGE.EST_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
                 LParams() << LParam("stage", TStagesRules::Instance()->stage_name( i->first, airp, true ))
                 << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
      }
    }
    if ( i->second.act != NoExists ) {
   	  try {
        i->second.act = ClientToUTC( i->second.act, region );
      }
      catch( boost::local_time::ambiguous_result ) {
         throw AstraLocale::UserException( "MSG.STAGE.ACT_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
                 LParams() << LParam("stage", TStagesRules::Instance()->stage_name( i->first, airp, true ))
                 << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
      }
    }
  }
  WriteStagesUTC( point_id, ts );
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

BASIC::TDateTime TTripStages::time_scd( TStage stage )
{
  if ( tripstages.empty() )
    throw Exception( "tripstages is empty" );
  TTripStage tripStage = tripstages[ stage ];
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
    if ( p_level > 0 && p_level < l->level ) /* ���� �� ���諨 �� ᫥�. ���� */
      continue;

    if ( sr->isClientStage( l->stage ) && !sr->canClientStage( CkinClients, l->stage ) )
    	continue;

    if ( tripstages[ l->stage ].act == NoExists ) { /* ���� ����� �� ���訥 ���設� */
      p_level = l->level;
      continue;
    }
    p_level = 0; /* ��� 㧥� 㤮���⢮��� ��� */
    /* �।��騩 㧥� �� ���� � ��� 㤮���⢮�� - �� ���� �����訩�� */
    if ( level >= l->level )
      break;
    if ( sr->CanStatus( stage_type, l->stage ) ) { /* ��� �㦡� � ���᭠ � ��㤭� */
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
 /* ����㧪� ����ᮢ */
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
      if ( CompatibleStage( i->stage ) )	{
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
    	if ( CompatibleStage( r->first ) )	{
        xmlNodePtr stagerulesNode = NewTextChild( snode, "stagerules" );
        SetProp( stagerulesNode, "stage", r->first );
        for ( vecRules::iterator v=r->second.begin(); v!=r->second.end(); v++ ) {
        	if ( CompatibleStage( v->cond_stage ) )	{
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
  	if ( !CompatibleStageType( m->first ) )
  		continue;
    snode = NewTextChild( node, "stage_type" );
    SetProp( snode, "type", m->first );
    for ( TStage_Statuses::iterator s=m->second.begin(); s!=m->second.end(); s++ ) {
      if ( CompatibleStage( s->stage ) )	{
        xmlNodePtr stagestatusNode = NewTextChild( snode, "stagestatus" );
        NewTextChild( stagestatusNode, "stage", (int)s->stage );
        NewTextChild( stagestatusNode, "status", getLocaleName( s->status, s->status_lat, true ) );
      }
    }
  }

  BuildGraph_Stages( "", dataNode );

  node = NewTextChild( dataNode, "GrphLvl" );
  for ( TGraph_Level::iterator l=GrphLvl.begin(); l!=GrphLvl.end(); l++ ) {
    if ( CompatibleStage( l->stage ) )	{
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
		if ( n->stage == stage ) {
  		if ( n->airp.empty() ) {
	  		res1 = getLocaleName( n->name, n->name_lat, pr_locale );
      }
			else {
			  if ( n->airp == airp ) {
			  	res = getLocaleName( n->name, n->name_lat, pr_locale );
        }
      }
    }
	}
	if ( res.empty() )
		return res1;
	else
		return res;
}

bool TStagesRules::canClientStage( const TCkinClients &ckin_clients, int stage_id )
{
	for ( TCkinClients::const_iterator i=ckin_clients.begin(); i!=ckin_clients.end(); i++ ) {
	 if ( find( ClientStages[ stage_id ].begin(), ClientStages[ stage_id ].end(), *i ) != ClientStages[ stage_id ].end() ) {
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
  Qry.SQLText  = "SELECT airline, airp, craft, trip_type, time, "
                 "       DECODE( airline, NULL, 0, 8 )+ "
                 "       DECODE( airp, NULL, 0, 4 )+ "
                 "       DECODE( craft, NULL, 0, 2 )+ "
                 "       DECODE( trip_type, NULL, 0, 1 ) AS priority "
                 " FROM graph_times "
                 " WHERE stage_id=:stage "
                 "UNION "
                 "SELECT NULL, NULL, NULL, NULL, time, -1 FROM graph_stages "
                 "WHERE stage_id=:stage "
                 " ORDER BY priority, airline, airp, craft, trip_type ";
  Qry.CreateVariable( "stage", otInteger, stage );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TStageTime st;
    st.airline = Qry.FieldAsString( "airline" );
    st.airp = Qry.FieldAsString( "airp" );
    st.craft = Qry.FieldAsString( "craft" );
    st.trip_type = Qry.FieldAsString( "trip_type" );
    st.time = Qry.FieldAsInteger( "time" );
    st.priority = Qry.FieldAsInteger( "priority" );
    times.push_back( st );
    Qry.Next();
  }
}

TDateTime TStageTimes::GetTime( const string &airline, const string &airp,
                                const string &craft, const string &triptype,
                                TDateTime vtime )
{
	TDateTime res = NoExists;
	if ( vtime == NoExists )
		return res;
	if ( times.empty() )
	  GetStageTimes( );
	for ( vector<TStageTime>::iterator st=times.begin(); st!=times.end(); st++ ) {
   	if ( ( st->airline == airline || st->airline.empty() ) &&
         ( st->airp == airp || st->airp.empty() ) &&
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
           /*�� ��⨢��*/
  	   break;
    case sPrepCheckIn:
           /*�����⮢�� � ॣ����樨*/
           PrepCheckIn( point_id );
           break;
    case sOpenCheckIn:
           /*����⨥ ॣ����樨*/
           OpenCheckIn( point_id );
           break;
  	case sOpenWEBCheckIn:
           /*����⨥ WEB-ॣ����樨*/
  	     break;
  	case sOpenKIOSKCheckIn:
           /*����⨥ KIOSK-ॣ����樨*/
  	     break;
    case sCloseWEBCancel:
          /*����� ࠧॣ����樨 web-���ᠦ��*/
          break;
    case sCloseCheckIn:
           /*�����⨥ ॣ����樨*/
           CloseCheckIn( point_id );
           break;
  	case sCloseWEBCheckIn:
           /*�����⨥ WEB-ॣ����樨*/
  	     break;
  	case sCloseKIOSKCheckIn:
           /*�����⨥ KIOSK-ॣ����樨*/
  	     break;
    case sOpenBoarding:
           /*��砫� ��ᠤ��*/
           break;
    case sCloseBoarding:
           /*����砭�� ��ᠤ�� (��ଫ���� ���㬥��樨)*/
           CloseBoarding( point_id );
           break;
    case sRemovalGangWay:
           /*���ઠ �࠯�*/
           break;
    case sTakeoff:
           /*�뫥⥫*/
           Takeoff( point_id );
           break;
  }
  XXX( point_id );
}

void astra_timer( TDateTime utcdate )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT trip_stages.point_id point_id, trip_stages.stage_id stage_id, points.airp "
   " FROM points, trip_stages "
   "WHERE points.point_id = trip_stages.point_id AND "
   "      points.act_out IS NULL AND "
   "      points.pr_del = 0 AND "
   "      trip_stages.time_auto_not_act <= :now "
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

  	while ( !Qry.Eof ) { // �஡�� �� 蠣��, ����� ��� �믮�����
  		count++;
  		int point_id = Qry.FieldAsInteger( "point_id" );
  		int stage_id = Qry.FieldAsInteger( "stage_id" );
  		string airp = Qry.FieldAsString( "airp" );
  		QExecStage.SetVariable( "point_id", point_id );
  		QExecStage.SetVariable( "stage_id", stage_id );
  	  TDateTime execTime2 = NowUTC();
  	  bool pr_exec_stage = false;
  	  try {
  		  QExecStage.Execute(); // �ਧ��� ⮣� ������ �� �믮������� 蠣 + �⬥⪠ � �믮������ 蠣� ��. ��䨪�
  		  if ( NowUTC() - execTime2 > 1.0/(1440.0*60) )
    		  ProgTrace( TRACE5, "Attention execute QCanStage time > 1 sec !!!, time=%s, count=%d", DateTimeToStr( NowUTC() - execTime2, "nn:ss" ).c_str(), count );
  		  pr_exec_stage = QExecStage.GetVariableAsInteger( "exec_stage" );
  		  TDateTime act_stage = QExecStage.GetVariableAsDateTime( "act" );
  		  if ( pr_exec_stage ) {
    		  // ������ � ��� � �믮������ 蠣�
          TStagesRules *r = TStagesRules::Instance();
          string tolog = string( "�⠯ '" ) + r->stage_name( (TStage)stage_id, airp, false ) + "'";
          tolog += " �믮����: 䠪�. �६�=";
          tolog += DateTimeToStr( act_stage, "hh:nn dd.mm.yy (UTC)" );
          TReqInfo::Instance()->MsgToLog( tolog, evtGraph, point_id, stage_id );
  		  }
  		}
      catch( Exception &E ) {
      	try { OraSession.Rollback( ); } catch(...) { };
        ProgError( STDLOG, "�訡�� astra_timer: %s. �६� %s, point_id=%d, stage_id=%d",
                   E.what(),
                   DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                   point_id, stage_id );
      }
			catch( ... ) {
				try { OraSession.Rollback( ); } catch(...) { };
				ProgError( STDLOG, "unknown timer error" );
 			}
 			OraSession.Commit(); // ����������� 䠪� �믮������ 蠣� + ���� � ��
  		if ( pr_exec_stage ) { // �믮��塞 ����⢨� �易��� � �⨬ 蠣��
				pr_exit = false; // �ਧ��� ⮣�, �� ���� �� �஢���� ᫥���騥 蠣� ��䨪� �� �, �� �� ����� � ��� �믮�����
  			TDateTime execStep = NowUTC();
  			try {
			    exec_stage( point_id, stage_id );
          if ( NowUTC() - execStep > 1.0/(1440.0*60) )
 	          ProgTrace( TRACE5, "Attention execute point_id=%d, stage_id=%d time > 1 sec !!!, time=%s, count=%d", point_id, stage_id,
 	                     DateTimeToStr( NowUTC() - execStep, "nn:ss" ).c_str(), count );
			  }
        catch( Exception &E ) {
      	  try { OraSession.Rollback( ); } catch(...) { };
          ProgError( STDLOG, "�訡�� astra_timer: %s. �६� %s, point_id=%d, stage_id=%d",
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

  //��ࠡ�⠥� �६���� ᫮� �� tlg_comp_layers
  int curr_tid=NoExists;
  Qry.Clear();
  Qry.SQLText=
    "SELECT range_id, crs_pax_id "
    "FROM tlg_comp_layers "
    "WHERE time_remove<=SYSTEM.UTCSYSDATE "
    "ORDER BY crs_pax_id";
  Qry.Execute();
  vector<int> range_ids;
  for(;!Qry.Eof;)
  {
    int crs_pax_id=NoExists;
    if (!Qry.FieldIsNULL("crs_pax_id"))
      crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
    range_ids.push_back(Qry.FieldAsInteger("range_id"));

    Qry.Next();

    int next_crs_pax_id=NoExists;
    if (!Qry.Eof)
    {
      if (!Qry.FieldIsNULL("crs_pax_id"))
        next_crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
    };

    if (Qry.Eof ||
        crs_pax_id!=next_crs_pax_id ||
        range_ids.size()>=1000)
    {
      try
      {
        TPointIdsForCheck point_ids_spp;
        DeleteTlgSeatRanges(range_ids, crs_pax_id, curr_tid, point_ids_spp);
        check_layer_change(point_ids_spp);
      }
      catch(Exception &E)
      {
        try { OraSession.Rollback( ); } catch(...) { };
        ProgError( STDLOG, "DeleteTlgSeatRanges: %s", E.what() );
      }
      catch(...)
      {
        try { OraSession.Rollback( ); } catch(...) { };
        ProgError( STDLOG, "DeleteTlgSeatRanges: Unknown error" );
      };
      OraSession.Commit();
      range_ids.clear();
    };
  };
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
  if ( (stage == sPrepCheckIn && (!Qry.FieldIsNULL( "bort" ) || string( "���" ) != Qry.FieldAsString( "airp" ))) ||
  	   (stage == sOpenCheckIn && string( "���" ) == Qry.FieldAsString( "airp" )) ) {
    SALONS2::TFindSetCraft res = SALONS2::AutoSetCraft( point_id );
    if ( res != SALONS2::rsComp_Found && res != SALONS2::rsComp_NoChanges ) {
  	  TReqInfo::Instance()->MsgToLog( string( "���室��� ��� ३� ���������� " ) + craft + " �� �������", evtFlt, point_id );
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
  SALONS2::setManualCompChg( point_id );
  time_t time_start=time(NULL);
  try
  {
    vector<TypeB::TCreateInfo> createInfo;
    TypeB::TOpenCheckInCreator(point_id).getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"OpenCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
  };
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! TelegramInterface::SendTlg execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);
}

void CloseCheckIn( int point_id )
{
  try
  {
    vector<TypeB::TCreateInfo> createInfo;
    TypeB::TCloseCheckInCreator(point_id).getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
  };
  try
  {
    create_mintrans_file(point_id);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.create_mintrans_file (point_id=%d): %s",point_id,E.what());
  };
  try
  {
    create_apis_file(point_id, ON_CLOSE_CHECKIN);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.create_apis_file (point_id=%d): %s",point_id,E.what());
  };
};

void CloseBoarding( int point_id )
{
  try
  {
    vector<TypeB::TCreateInfo> createInfo;
    TypeB::TCloseBoardingCreator(point_id).getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.SendTlg (point_id=%d): %s",point_id,E.what());
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
    vector<TypeB::TCreateInfo> createInfo;
    TypeB::TTakeoffCreator(point_id).getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo);
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
    create_apis_file(point_id, ON_TAKEOFF);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.create_apis_file (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! create_apis_file execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);
  time_start=time(NULL);
  try
  {
    sync_checkin_data( point_id );
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.sync_checkin_data (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! sync_checkin_data execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);
  try
  {
    sync_aodb( point_id );
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.sync_aodb (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! sync_aodb execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);

  try
  {
    create_mintrans_file(point_id);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.create_mintrans_file (point_id=%d): %s",point_id,E.what());
  };
}


void SetTripStages_IgnoreAuto( int point_id, bool ignore_auto )
{
  ProgTrace( TRACE5, "SetTripStages_IgnoreAuto: point_id=%d, ignore_auto=%d", point_id, ignore_auto );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE trip_stages SET ignore_auto=:ignore_auto WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "ignore_auto", otInteger, ignore_auto );
  Qry.Execute();
  if ( Qry.RowsProcessed() > 0 ) {
    ProgTrace( TRACE5, "SetTripStages_IgnoreAuto: point_id=%d, set ignore_auto=%d",
               point_id, ignore_auto );
  }
}

bool CheckStageACT( int point_id, TStage stage_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT act FROM trip_stages WHERE point_id=:point_id AND stage_id=:stage_id AND act IS NOT NULL";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "stage_id", otInteger, stage_id );
  Qry.Execute();
  return !Qry.Eof;
};
