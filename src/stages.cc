#include <stdlib.h>
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
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

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
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
   "SELECT airp FROM points WHERE points.point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string region, airp;
  airp = Qry.FieldAsString( "airp" );
	if ( reqInfo->user.sets.time == ustTimeLocalAirp )
 	  region = AirpTZRegion( airp );
  Qry.Clear();
  Qry.SQLText =
   "BEGIN "\
   " UPDATE points SET point_id = :point_id WHERE point_id = :point_id; "\
   "  SELECT point_id INTO :point_id FROM trip_stages WHERE point_id = :point_id AND stage_id = :stage_id; "\
   " EXCEPTION WHEN NO_DATA_FOUND THEN "\
   "  INSERT INTO trip_stages(point_id,stage_id,scd,est,act,pr_auto,pr_manual) "\
   "   VALUES(:point_id,:stage_id,NVL(:act,:est),NULL,:act,:pr_auto,DECODE(:pr_manual,-1,0,:pr_manual)); "\
   "END; ";
   Qry.CreateVariable( "point_id", otInteger, point_id );
   Qry.DeclareVariable( "stage_id", otInteger );
   Qry.DeclareVariable( "est", otDate );
   Qry.DeclareVariable( "act", otDate );
   Qry.CreateVariable( "pr_auto", otInteger, 0 );
   Qry.DeclareVariable( "pr_manual", otInteger );

   TQuery UpdQry( &OraSession );
   UpdQry.SQLText =
    "UPDATE trip_stages SET est=:est,act=:act,pr_auto=DECODE(:pr_auto,-1,pr_auto,:pr_auto), "
    "                       pr_manual=DECODE(:pr_manual,-1,pr_manual,:pr_manual) "\
    "  WHERE point_id=:point_id AND stage_id=:stage_id";
   UpdQry.CreateVariable( "point_id", otInteger, point_id );
   UpdQry.DeclareVariable( "stage_id", otInteger );
   UpdQry.DeclareVariable( "est", otDate );
   UpdQry.DeclareVariable( "act", otDate );
   UpdQry.DeclareVariable( "pr_auto", otInteger );
   UpdQry.DeclareVariable( "pr_manual", otInteger );

   for ( TMapTripStages::iterator i=ts.begin(); i!=ts.end(); i++ ) {
   	 Qry.SetVariable( "stage_id", (int)i->first );
     if ( i->second.est == NoExists )
       Qry.SetVariable( "est", FNull );
     else
   		try {
    	  Qry.SetVariable( "est", ClientToUTC( i->second.est, region ) );
    	}
      catch( boost::local_time::ambiguous_result ) {
        throw UserException( "����⭮� �६� �믮������ 蠣� '%s' � �㭪� %s �� ��।����� �������筮",
                             TStagesRules::Instance()->Graph_Stages[ i->first ].c_str(),
                             airp.c_str() );
      }
     if ( i->second.act == NoExists )
       Qry.SetVariable( "act", FNull );
     else
     	 try {
         Qry.SetVariable( "act", ClientToUTC( i->second.act, region ) );
       }
      catch( boost::local_time::ambiguous_result ) {
        throw UserException( "�����᪮� �६� �믮������ 蠣� '%s' � �㭪� %s �� ��।����� �������筮",
                             TStagesRules::Instance()->Graph_Stages[ i->first ].c_str(),
                             airp.c_str() );
      }
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
     tst();
     if ( i->second.old_act == NoExists && i->second.act > NoExists ) { // �맮� �㭪樨 ��ࠡ�⪨ 蠣�
       try
       {
         exec_stage( point_id, (int)i->first );
       }
       catch( std::exception &E ) {
         ProgError( STDLOG, "Exception: %s", E.what() );
       }
       catch( ... ) {
         ProgError( STDLOG, "Unknown error" );
       };
     }
   	 UpdQry.SetVariable( "stage_id", (int)i->first );
     if ( i->second.est == NoExists )
       UpdQry.SetVariable( "est", FNull );
     else
   	   UpdQry.SetVariable( "est", ClientToUTC( i->second.est, region ) );
     if ( i->second.act == NoExists )
       UpdQry.SetVariable( "act", FNull );
     else
       UpdQry.SetVariable( "act", ClientToUTC( i->second.act, region ) );
     if ( i->second.old_act > NoExists && i->second.act == NoExists )
     	UpdQry.SetVariable( "pr_auto", 0 );
     else
     	UpdQry.SetVariable( "pr_auto", -1 );
     UpdQry.SetVariable( "pr_manual", pr_manual );
     UpdQry.Execute();
     tst();
     TStagesRules *r = TStagesRules::Instance();
     string tolog = string( "�⠯ '" ) + r->Graph_Stages[ i->first ] + "'";
     if ( i->second.old_act == NoExists && i->second.act > NoExists )
       tolog += " �믮����";
     if ( i->second.old_act > NoExists && i->second.act == NoExists )
       tolog += " �⬥���";
     tolog += ": ���. �६�";
     if ( i->second.est > NoExists )
       tolog += DateTimeToStr( ClientToUTC( i->second.est, region ), "=hh:nn dd.mm.yy (UTC)" );
     else
       tolog += " �� ������";
     tolog += ", 䠪�. �६�";
     if ( i->second.act > NoExists )
       tolog += DateTimeToStr( ClientToUTC( i->second.act, region ), "=hh:nn dd.mm.yy (UTC)" );
     else
        tolog += " �� ������";
     reqInfo->MsgToLog( tolog, evtGraph, point_id, (int)i->first );
   }
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

TStage TTripStages::getStage( TStage_Type stage_type )
{
  TStagesRules *sr = TStagesRules::Instance();
  int level = 0;
  int p_level = 0;
  TStage res = sNoActive;
  for ( TGraph_Level::iterator l=sr->GrphLvl.begin(); l!=sr->GrphLvl.end(); l++ ) {
    if ( p_level > 0 && p_level < l->level ) /* ���� �� ���諨 �� ᫥�. ���� */
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
  Update();
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
 Qry.SQLText = "SELECT stage_id, stage_type, status FROM stage_statuses ORDER BY stage_type";
 Qry.Execute();
 TStage_Type stage_type;
 TStage_Status status;
 while ( !Qry.Eof ) {
   stage_type = (TStage_Type)Qry.FieldAsInteger( "stage_type" );
   status.stage = (TStage)Qry.FieldAsInteger( "stage_id" );
   status.status = Qry.FieldAsString( "status" );
   StageStatuses[ stage_type ].push_back( status );
   Qry.Next();
 }

 Qry.Clear();
 Qry.SQLText = "SELECT stage_id, name from graph_stages ORDER BY stage_id";
 Qry.Execute();

 while ( !Qry.Eof ) {
   Graph_Stages[ (TStage)Qry.FieldAsInteger( "stage_id" ) ] = Qry.FieldAsString( "name" );
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
      xmlNodePtr stagerulesNode = NewTextChild( snode, "stagerules" );
      SetProp( stagerulesNode, "stage", r->first );
      for ( vecRules::iterator v=r->second.begin(); v!=r->second.end(); v++ ) {
        xmlNodePtr ruleNode = NewTextChild( stagerulesNode, "rule" );
        NewTextChild( ruleNode, "num", v->num );
        NewTextChild( ruleNode, "cond_stage", v->cond_stage );
      }
    }
  }
  node = NewTextChild( dataNode, "StageStatuses" );
  for ( TMapStatuses::iterator m=StageStatuses.begin(); m!=StageStatuses.end(); m++ ) {
    snode = NewTextChild( node, "stage_type" );
    SetProp( snode, "type", m->first );
    for ( TStage_Statuses::iterator s=m->second.begin(); s!=m->second.end(); s++ ) {
      xmlNodePtr stagestatusNode = NewTextChild( snode, "stagestatus" );
      NewTextChild( stagestatusNode, "stage", (int)s->stage );
      NewTextChild( stagestatusNode, "status", s->status );
//      NewTextChild( stagestatusNode, "lvl", s->lvl );
    }
  }
  node = NewTextChild( dataNode, "Graph_Stages" );
  for ( map<TStage,std::string>::iterator i=Graph_Stages.begin(); i!=Graph_Stages.end(); i++ ) {
    snode = NewTextChild( node, "stage" );
    NewTextChild( snode, "stage_id", (int)i->first );
    NewTextChild( snode, "name", i->second );
  }
  node = NewTextChild( dataNode, "GrphLvl" );
  for ( TGraph_Level::iterator l=GrphLvl.begin(); l!=GrphLvl.end(); l++ ) {
    snode = NewTextChild( node, "stage_level" );
    NewTextChild( snode, "stage", (int)l->stage );
    NewTextChild( snode, "level", (int)l->level );
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

string TStagesRules::status( TStage_Type stage_type, TStage stage )
{
  TStage_Statuses &v = StageStatuses[ stage_type ];
  for( TStage_Statuses::iterator s=v.begin(); s!=v.end(); s++ ) {
    if ( s->stage == stage )
      return s->status;
  }
  return "";
}

void GetStageTimes( vector<TStageTimes> &stagetimes, TStage stage )
{
  stagetimes.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText  = "SELECT airp, craft, trip_type, time, "\
                 "       DECODE( airp, NULL, 0, 4 )+ "\
                 "       DECODE( craft, NULL, 0, 2 )+ "\
                 "       DECODE( trip_type, NULL, 0, 1 ) AS priority "\
                 " FROM graph_times "\
                 " WHERE stage_id=:stage "\
                 "UNION "\
                 "SELECT NULL, NULL, NULL, time, -1 FROM graph_stages "\
                 "WHERE stage_id=:stage "\
                 " ORDER BY priority, airp, craft, trip_type ";
  Qry.CreateVariable( "stage", otInteger, stage );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TStageTimes st;
    st.airp = Qry.FieldAsString( "airp" );
    st.craft = Qry.FieldAsString( "craft" );
    st.trip_type = Qry.FieldAsString( "trip_type" );
    st.time = Qry.FieldAsInteger( "time" );
    st.priority = Qry.FieldAsInteger( "priority" );
    stagetimes.push_back( st );
    Qry.Next();
  }
  tst();
}

void exec_stage( int point_id, int stage_id )
{
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
    case sCloseCheckIn:
           /*�����⨥ ॣ����樨*/
           CloseCheckIn( point_id );
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
}


void astra_timer( TDateTime utcdate )
{	
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT trip_stages.point_id point_id, trip_stages.stage_id stage_id"\
   " FROM points, trip_stages "\
   "WHERE points.point_id = trip_stages.point_id AND "\
   "      points.act_out IS NULL AND "\
   "      points.pr_del = 0 AND "\
   "      trip_stages.pr_auto = 1 AND "\
   "      trip_stages.act IS NULL AND "\
   "      NVL( trip_stages.est, trip_stages.scd ) <= :now "\
   " ORDER BY trip_stages.point_id, trip_stages.stage_id ";
  Qry.CreateVariable( "now", otDate, utcdate );
  TQuery QCanStage(&OraSession);
  QCanStage.SQLText =
   "DECLARE msg VARCHAR2(255); "\
   "BEGIN "\
   " :canstage := gtimer.CanStage(:point_id,:stage_id); "\
   " IF :canstage != 0 THEN "\
   "  UPDATE points SET point_id = point_id WHERE point_id = :point_id; "\
   "  BEGIN "\
   "   UPDATE trip_stages SET act = TRUNC( :now, 'MI' ) "\
   "    WHERE point_id = :point_id AND stage_id = :stage_id; "\
   "   EXCEPTION WHEN NO_DATA_FOUND THEN "\
   "    INSERT INTO trip_stages(point_id,stage_id,scd,est,act,pr_auto,pr_manual) "\
   "     VALUES(:point_id,:stage_id,:now,NULL,:now,0,1); "\
   "  END; "\
   " END IF; "\
   "END; ";
  QCanStage.DeclareVariable( "point_id", otInteger );
  QCanStage.DeclareVariable( "stage_id", otInteger );
  QCanStage.DeclareVariable( "canstage", otInteger );
  QCanStage.CreateVariable( "now", otDate, utcdate );
  bool pr_exit = false;
  int count=0;
  TDateTime execTime0 = NowUTC();
  while ( !pr_exit ) {
  	pr_exit = true;
  	TDateTime execTime1 = NowUTC();
  	Qry.Execute();
		if ( NowUTC() - execTime1 > 1.0/(1440.0*60) )
  		ProgTrace( TRACE5, "Attention execute Query1 time > 1 sec !!!, time=%s, count=%d", DateTimeToStr( NowUTC() - execTime1, "nn:ss" ).c_str(), count );

  	while ( !Qry.Eof ) {
  		count++;
  		int point_id = Qry.FieldAsInteger( "point_id" );
  		int stage_id = Qry.FieldAsInteger( "stage_id" );
  		QCanStage.SetVariable( "point_id", point_id );
  		QCanStage.SetVariable( "stage_id", stage_id );
  	  TDateTime execTime2 = NowUTC();  		
  		QCanStage.Execute();
		  if ( NowUTC() - execTime2 > 1.0/(1440.0*60) )
  		  ProgTrace( TRACE5, "Attention execute Query2 time > 1 sec !!!, time=%s, count=%d", DateTimeToStr( NowUTC() - execTime2, "nn:ss" ).c_str(), count );  		
  		if ( QCanStage.GetVariableAsInteger( "canstage" ) ) {
  			try {
  				try {
  					TDateTime execStep = NowUTC();
  				  exec_stage( point_id, stage_id );
		        if ( NowUTC() - execStep > 1.0/(1440.0*60) )
  		        ProgTrace( TRACE5, "Attention execute point_id=%d, stage_id=%d time > 1 sec !!!, time=%s, count=%d", point_id, stage_id, 
  		                   DateTimeToStr( NowUTC() - execStep, "nn:ss" ).c_str(), count );
  				}
  				catch( Exception &E ) {
            ProgError( STDLOG, "�訡�� astra_timer: %s. �६� %s, point_id=%d, stage_id=%d",
                       E.what(),
                       DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                       point_id, stage_id );
  				}
  				catch( ... ) {
  					ProgError( STDLOG, "unknown timer error" );
  				}
          TStagesRules *r = TStagesRules::Instance();
          string tolog = string( "�⠯ '" ) + r->Graph_Stages[ (TStage)stage_id ] + "'";
          tolog += " �믮����: 䠪�. �६�=";
          tolog += DateTimeToStr( utcdate, "hh:nn dd.mm.yy (UTC)" );
          TReqInfo::Instance()->MsgToLog( tolog, evtGraph, point_id, stage_id );
  				pr_exit = false;
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
  			}
  			OraSession.Commit();
  		}
  		Qry.Next();
  	}
  }
	if ( NowUTC() - execTime0 > 5.0/(1440.0*60) )
  	ProgTrace( TRACE5, "Attention execute astra_time > 5 sec !!!, time=%s, steps count=%d", DateTimeToStr( NowUTC() - execTime0, "nn:ss" ).c_str(), count );
}

void PrepCheckIn( int point_id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "DECLARE "\
	 "ve NUMBER; "\
	 "vcraft  points.craft%TYPE; "\
   "vbort   points.bort%TYPE; "\
   "vairp   points.airp%TYPE; "\
   "BEGIN "\
   "SELECT craft, b.bort, airp INTO vcraft, vbort, vairp FROM points, "\
   "( SELECT points.bort, points.point_id FROM comps, points "\
   "  WHERE points.point_id = :point_id AND "\
   "        points.craft = comps.craft AND "\
   "        points.bort IS NOT NULL AND "\
   "        points.bort = comps.bort AND "\
   "        rownum < 2 ) b "\
   "WHERE points.point_id = :point_id AND "
   "      points.point_id = b.point_id(+); "\
   "IF vbort IS NOT NULL OR vairp != '���' THEN "\
   " SELECT COUNT(*) INTO ve FROM trip_comp_elems "\
   "  WHERE point_id = :point_id AND rownum<2; "\
   " IF ve = 0 THEN "\
   "  ve := salons.setcraft( :point_id, vcraft ); "\
   "    IF ve < 0 THEN "\
   "      system.MsgToLog('���室��� ��� ३� ���������� '||vcraft||' �� �������',system.evtFlt,:point_id); "\
   "    END IF; "\
   " END IF; "\
   "END IF; "\
   "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

void OpenCheckIn( int point_id )
{
  TQuery Qry(&OraSession);
	Qry.SQLText =
	 "DECLARE "\
	 "ve NUMBER; "\
   "vc NUMBER; "\
   "vcraft  points.craft%TYPE; "\
   "vairp   points.airp%TYPE; "\
   " BEGIN "\
   "  SELECT craft, airp INTO vcraft, vairp FROM points WHERE point_id=:point_id; "\
   "  IF vairp = '���' THEN "\
   "   SELECT COUNT(*) INTO vc FROM trip_comp_elems "\
   "    WHERE point_id = :point_id AND rownum<2; "\
   "   IF vc = 0 THEN  "\
   "    ve := salons.setcraft( :point_id, vcraft ); "\
   "    IF ve < 0 THEN "\
   "      system.MsgToLog('���室��� ��� ३� ���������� '||vcraft||' �� �������',system.evtFlt,:point_id); "\
   "    END IF; "\
   "   END IF; "\
   "  END IF; "\
   " END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

void CloseCheckIn( int point_id )
{
  try
  {
    vector<string> tlg_types;
    tlg_types.push_back("COM");
    TelegramInterface::SendTlg(point_id,tlg_types);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
  };

  try
  {
    CreateCentringFileDATA( point_id );
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
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.SendTlg (point_id=%d): %s",point_id,E.what());
  };

  try
  {
    CreateCentringFileDATA( point_id );
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
  	  "  statist.get_full_stat(:point_id); "
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
    vector<string>  tlg_types;
    tlg_types.push_back("PTM");
    tlg_types.push_back("PTMN");
    tlg_types.push_back("BTM");
    tlg_types.push_back("PSM");
    tlg_types.push_back("PFS");
    tlg_types.push_back("PFSN");
    tlg_types.push_back("FTL");
    tlg_types.push_back("PRL");
    tlg_types.push_back("ETL");
    tlg_types.push_back("LDM");
    TelegramInterface::SendTlg(point_id,tlg_types);
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
    create_czech_police_file(point_id);
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

