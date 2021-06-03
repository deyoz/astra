#include <stdlib.h>
#include "stages.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "telegram.h"
#include "astra_service.h"
#include "astra_date_time.h"
#include "apis_creator.h"
#include "crafts/ComponCreator.h"
#include "term_version.h"
#include "comp_layers.h"
#include "alarms.h"
#include "rozysk.h"
#include "points.h"
#include "trip_tasks.h"
#include "qrys.h"
#include "stat/stat_main.h"
#include "apps_interaction.h"
#include "etick.h"
#include "counters.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;
using namespace ASTRA::date_time;

const struct {
    const char *name;
    TStage stage;
} TStagesS[]={
    {"NoActive",             sNoActive},
    {"PrepCheckIn",          sPrepCheckIn},
    {"OpenCheckIn",          sOpenCheckIn},
    {"OpenWEBCheckIn",       sOpenWEBCheckIn},
    {"OpenKIOSKCheckIn",     sOpenKIOSKCheckIn},
    {"CloseCheckIn",         sCloseCheckIn},
    {"CloseWEBCancel",       sCloseWEBCancel},
    {"CloseWEBCheckIn",      sCloseWEBCheckIn},
    {"CloseKIOSKCheckIn",    sCloseKIOSKCheckIn},
    {"OpenBoarding",         sOpenBoarding},
    {"CloseBoarding",        sCloseBoarding},
    {"RemovalGangWay",       sRemovalGangWay},
    {"Takeoff",              sTakeoff}
};


TStage DecodeStage(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TStagesS)/sizeof(TStagesS[0]);i+=1) if (strcmp(s,TStagesS[i].name)==0) break;
  if (i<sizeof(TStagesS)/sizeof(TStagesS[0]))
    return TStagesS[i].stage;
  else
    return sNoActive;
};

const char* EncodeStage(TStage s)
{
  unsigned int i;
  for(i=0;i<sizeof(TStagesS)/sizeof(TStagesS[0]);i+=1) if (TStagesS[i].stage == s) break;
  if (i<sizeof(TStagesS)/sizeof(TStagesS[0]))
    return TStagesS[i].name;
  else
    return EncodeStage(sNoActive);
};


bool CompatibleStage( TStage stage )
{
  if ( !TStagesRules::Instance()->isClientStage( stage ) )
    return true;

  if ( stage == sOpenWEBCheckIn ||
       stage == sOpenKIOSKCheckIn ||
       stage == sCloseWEBCheckIn ||
       stage == sCloseKIOSKCheckIn ||
       stage == sCloseWEBCancel )
    return true;

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
    return true;

  if ( stage_type == stWEBCancel )
    return true;

  return false;
}

void FillTakeoffStage(const int poind_id, const TStage stage, TTripStage &tripStage)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT scd_out, est_out, act_out "
       "FROM points WHERE point_id = :point_id",
        PgOra::getROSession("POINTS")
    );

    Dates::DateTime_t scd;
    Dates::DateTime_t est;
    Dates::DateTime_t act;

    cur.stb()
       .defNull(scd, Dates::not_a_date_time)
       .defNull(est, Dates::not_a_date_time)
       .defNull(act, Dates::not_a_date_time)
       .bind(":point_id", poind_id)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound != cur.err()) {
        tripStage.scd = dbo::isNull(scd) ? NoExists : BoostToDateTime(scd);
        tripStage.est = dbo::isNull(est) ? NoExists : BoostToDateTime(est);
        tripStage.act = dbo::isNull(act) ? NoExists : BoostToDateTime(act);
        tripStage.old_est = tripStage.est;
        tripStage.old_act = tripStage.act;
        tripStage.pr_auto = 0;
        tripStage.stage = stage;
    }
}

void FillStage(const int poind_id, const TStage stage, TTripStage &tripStage)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT scd, est, act, pr_auto "
       "FROM trip_stages "
       "WHERE point_id = :point_id "
         "AND stage_id = :stage",
        PgOra::getROSession("TRIP_STAGES")
    );

    Dates::DateTime_t scd;
    Dates::DateTime_t est;
    Dates::DateTime_t act;

    cur.stb()
       .def(scd)
       .defNull(est, Dates::not_a_date_time)
       .defNull(act, Dates::not_a_date_time)
       .def(tripStage.pr_auto)
       .bind(":point_id", poind_id)
       .bind(":stage", (int)stage)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound != cur.err()) {
        tripStage.scd = BoostToDateTime(scd);
        tripStage.est = dbo::isNull(est) ? NoExists : BoostToDateTime(est);
        tripStage.act = dbo::isNull(act) ? NoExists : BoostToDateTime(act);
        tripStage.old_est = tripStage.est;
        tripStage.old_act = tripStage.act;
        tripStage.stage = stage;
    }
}

void TTripStages::LoadStage(const int point_id, const TStage stage, TTripStage &ts )
{
    if (sTakeoff == stage) {
        FillTakeoffStage(point_id, stage, ts);
    } else {
        FillStage(point_id, stage, ts);
    }
}

TMapTripStages loadStages(const int point_id)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_id, scd, est, act, pr_auto "
       "FROM trip_stages "
       "WHERE point_id = :point_id",
        PgOra::getROSession("TRIP_STAGES")
    );

    TTripStage tripStage;

    int stage;
    Dates::DateTime_t scd;
    Dates::DateTime_t est;
    Dates::DateTime_t act;

    cur.stb()
       .def(stage)
       .def(scd)
       .defNull(est, Dates::not_a_date_time)
       .defNull(act, Dates::not_a_date_time)
       .def(tripStage.pr_auto)
       .bind(":point_id", point_id)
       .exec();

    TMapTripStages result;

    while (!cur.fen()) {
        tripStage.scd = BoostToDateTime(scd);
        tripStage.est = dbo::isNull(est) ? NoExists : BoostToDateTime(est);
        tripStage.act = dbo::isNull(act) ? NoExists : BoostToDateTime(act);
        tripStage.old_est = tripStage.est;
        tripStage.old_act = tripStage.act;
        tripStage.stage = (TStage)stage;
        result.insert({tripStage.stage, tripStage});
    }

    return result;
}

TTripStages::TTripStages( int vpoint_id )
{
    point_id = vpoint_id;
    tripstages = loadStages(vpoint_id);
}

void TTripStages::LoadStages( int vpoint_id, TMapTripStages &ts )
{
    ts = loadStages(vpoint_id);
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

void fillAirpPrDelPrActOut(const int point_id, std::string& airp, bool& pr_del, bool& pr_act_out)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT act_out, pr_del, airp "
       "FROM points "
       "WHERE point_id = :point_id",
        PgOra::getROSession("POINTS")
    );

    Dates::DateTime_t act_out;

    cur.stb()
       .defNull(act_out, Dates::not_a_date_time)
       .def(pr_del)
       .def(airp)
       .bind(":point_id", point_id)
       .exfet();

    if (cur.rowcount() != 1) {
        throw EXCEPTIONS::Exception( "fillAirpPrDelPrActOut: point_id not defined");
    }

    pr_act_out = dbo::isNotNull(act_out);
}

void TTripStages::WriteStagesUTC( int point_id, TMapTripStages &ts )
{
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

  TReqInfo *reqInfo = TReqInfo::Instance();

  std::string airp;
  bool pr_del;
  bool pr_act_out;

  fillAirpPrDelPrActOut(point_id, airp, pr_del, pr_act_out);

  DB::TQuery Qry(PgOra::getRWSession("TRIP_STAGES"), STDLOG);
  Qry.SQLText = PgOra::supportsPg("TRIP_STAGES")
   ? "INSERT INTO trip_stages( point_id, stage_id, scd, est, act, pr_auto, "     "pr_manual, ignore_auto) "
                     "VALUES (:point_id,:stage_id,:scd,:est,:act, "    "0,:insert_pr_manual,:ignore_auto) "
     "ON CONFLICT(point_id, stage_id) "
     "DO UPDATE trip_stages "
     "SET est = :est, act = :act, "
         "pr_auto = CASE :pr_auto WHEN -1 THEN pr_auto ELSE :pr_auto END, "
         "pr_manual = CASE :pr_manual WHEN -1 THEN pr_manual ELSE :pr_manual END "
     "WHERE point_id = :point_id "
       "AND stage_id = :stage_id; "
   : "BEGIN "
         "UPDATE trip_stages "
         "SET est = :est, act = :act, "
             "pr_auto = CASE :pr_auto WHEN -1 THEN pr_auto ELSE :pr_auto END, "
             "pr_manual = CASE :pr_manual WHEN -1 THEN pr_manual ELSE :pr_manual END "
         "WHERE point_id = :point_id "
           "AND stage_id = :stage_id; "
         "IF SQL%NOTFOUND THEN "
             "INSERT INTO trip_stages( point_id, stage_id, scd, est, act, pr_auto, "     "pr_manual, ignore_auto) "
                             "VALUES (:point_id,:stage_id,:scd,:est,:act, "    "0,:insert_pr_manual,:ignore_auto); "
         "END IF; "
     "END;";

  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "stage_id", otInteger );
  Qry.DeclareVariable( "scd", otDate );
  Qry.DeclareVariable( "est", otDate );
  Qry.DeclareVariable( "act", otDate );
  Qry.DeclareVariable( "pr_auto", otInteger );
  Qry.DeclareVariable( "pr_manual", otInteger );
  Qry.DeclareVariable( "insert_pr_manual", otInteger );
  Qry.CreateVariable( "ignore_auto", otInteger, pr_act_out || pr_del != 0 );

  TStagesRules *sr = TStagesRules::Instance();
  TCkinClients CkinClients;
  TTripStages::ReadCkinClients( point_id, CkinClients );

  for ( TMapTripStages::iterator i=ts.begin(); i!=ts.end(); i++ ) {

    TStage stage = i->first;
    TTripStage& trip_stage = i->second;

    if ( sr->isClientStage( (int)stage ) && !sr->canClientStage( CkinClients, (int)stage ) )
      continue;

    Qry.SetVariable( "stage_id", (int)stage );

    if ( trip_stage.est == NoExists ) {
        Qry.SetVariable( "est", FNull );
    } else {
        Qry.SetVariable( "est", trip_stage.est );
    }

    if ( trip_stage.act == NoExists ) {
        Qry.SetVariable( "act", FNull );
    } else {
        Qry.SetVariable( "act", trip_stage.act );
    }

    if ( trip_stage.old_act > NoExists && trip_stage.act == NoExists ) {
        Qry.SetVariable( "pr_auto", 0 );
    } else {
        Qry.SetVariable( "pr_auto", -1 );
    }

    if ( trip_stage.act != NoExists ) {
        Qry.SetVariable("scd", trip_stage.act);
    } else if ( trip_stage.est != NoExists ) {
        Qry.SetVariable("scd", trip_stage.est);
    } else {
        Qry.SetVariable("scd", FNull);
    }

    int pr_manual = 1;
    int insert_pr_manual = 1;
    if ( trip_stage.est == trip_stage.old_est ) {
        pr_manual = -1;
        insert_pr_manual = 0;
    } else if ( trip_stage.est == NoExists ) {
        pr_manual = 0;
        insert_pr_manual = 0;
    }
    Qry.SetVariable( "pr_manual", pr_manual );
    Qry.SetVariable( "insert_pr_manual", insert_pr_manual );
    Qry.Execute( );

    if ( trip_stage.old_act == NoExists && trip_stage.act > NoExists ) { // вызов функции обработки шага
      try {
         exec_stage( point_id, (int)stage );
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "std::exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
    }

    TTripAlarmHook::set(Alarm::Brd, point_id);
    check_unattached_trfer_alarm( point_id );
    check_crew_alarms( point_id );
    TTripAlarmHook::set(Alarm::UnboundEMD, point_id);
    string lexema_id;
    LEvntPrms params;
    params << PrmStage("stage", stage, airp);
    if ( trip_stage.old_act == NoExists && trip_stage.act > NoExists )
      lexema_id = "EVT.STAGE.COMPLETED_ACT_EST_TIME";
    if ( trip_stage.old_act > NoExists && trip_stage.act == NoExists )
      lexema_id = "EVT.STAGE.CANCELED";
    if ( trip_stage.est > NoExists )
      params << PrmDate("est_time", trip_stage.est, "=hh:nn dd.mm.yy (UTC)");
    else
      params << PrmLexema("est_time", "EVT.UNKNOWN");
    if ( trip_stage.act > NoExists )
      params << PrmDate("act_time", trip_stage.act, "=hh:nn dd.mm.yy (UTC)");
    else
      params << PrmLexema("act_time", "EVT.UNKNOWN");
    reqInfo->LocaleToLog(lexema_id, params, evtGraph, point_id, (int)stage );
  }
  TQuery syncTripFinalStagesQry( &OraSession );
  syncTripFinalStagesQry.SQLText =
    "BEGIN "
    "  gtimer.sync_trip_final_stages(:point_id); "
    "END;";
  syncTripFinalStagesQry.CreateVariable( "point_id", otInteger, point_id );
  syncTripFinalStagesQry.Execute();
}

void TTripStages::WriteStages( int point_id, TMapTripStages &ts )
{
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

    TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT airp FROM points WHERE points.point_id=:point_id";// FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string region, airp;
  airp = Qry.FieldAsString( "airp" );
    if ( reqInfo->user.sets.time == ustTimeLocalAirp )
      region = AirpTZRegion( airp );
  for ( TMapTripStages::iterator i=ts.begin(); i!=ts.end(); i++ ) {
    if ( i->second.est != NoExists ) {
      try {
        i->second.est = ClientToUTC( i->second.est, region );
      }
      catch( const boost::local_time::ambiguous_result& ) {
         throw AstraLocale::UserException( "MSG.STAGE.EST_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
                 LParams() << LParam("stage", TStagesRules::Instance()->stage_name_view( i->first, airp ))
                           << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
      }
    }
    if ( i->second.act != NoExists ) {
      try {
        i->second.act = ClientToUTC( i->second.act, region );
      }
      catch( const boost::local_time::ambiguous_result& ) {
         throw AstraLocale::UserException( "MSG.STAGE.ACT_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP",
                 LParams() << LParam("stage", TStagesRules::Instance()->stage_name_view( i->first, airp ))
                           << LParam("airp", ElemIdToCodeNative(etAirp,airp)));
      }
    }
  }
  WriteStagesUTC( point_id, ts );
}

void TTripStages::LoadStages( int vpoint_id )
{
  point_id = vpoint_id;
  tripstages = loadStages(vpoint_id);
}

TDateTime TTripStages::time( TStage stage ) const
{
  if ( tripstages.empty() )
    throw Exception( "tripstages is empty" );
  TMapTripStages::const_iterator tripStage = tripstages.find( stage );
  if (tripStage==tripstages.end())
    return NoExists;
  else
    return tripStage->second.time();
}

TTripStageTimes TTripStages::getStageTimes( TStage stage ) const
{
  if ( tripstages.empty() )
    throw Exception( "tripstages is empty" );
  TMapTripStages::const_iterator tripStage = tripstages.find( stage );
  if (tripStage==tripstages.end())
    return TTripStageTimes();
  else
    return tripStage->second;
}

void TTripStages::ReadCkinClients( int point_id, TCkinClients &ckin_clients )
{
    ckin_clients.clear();
    DB::TQuery Qry(PgOra::getROSession("TRIP_CKIN_CLIENT"), STDLOG);
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

std::vector<TStage_name> loadGraphStages()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_id, name, name_lat, NULL airp FROM graph_stages "
       "UNION "
       "SELECT stage_id, name, name_lat, airp FROM stage_names "
       "ORDER BY stage_id, airp",
        PgOra::getROSession("GRAPH_STAGES")
    );

    int stage_id;
    std::string name;
    std::string name_lat;
    std::string airp;

    cur.def(stage_id)
       .def(name)
       .defNull(name_lat, "")
       .defNull(airp, "")
       .exec();

    std::vector<TStage_name> result;

    while (!cur.fen()) {
        TStage_name stage_name;
        stage_name.stage = TStage(stage_id);
        stage_name.name = name;
        stage_name.name_lat = name_lat;
        stage_name.airp = airp;
        result.push_back(stage_name);
    }

    return result;
}

std::map<int,TCkinClients> loadClientStages()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT client_type, stage_id "
       "FROM ckin_client_stages "
       "ORDER BY stage_id, client_type",
        PgOra::getROSession("CKIN_CLIENT_STAGES")
    );

    std::string client_type;
    int stage_id;

    cur.def(client_type)
       .def(stage_id)
       .exec();

    std::map<int,TCkinClients> result;

    while (!cur.fen()) {
        result[stage_id].push_back(client_type);
    }

    return result;
}

void TStagesRules::UpdateGraph_Stages( )
{
    Graph_Stages = loadGraphStages();
    ClientStages = loadClientStages();
}

std::map<TStageStep,TMapRules> loadGraphRules()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT target_stage, cond_stage, num, next "
       "FROM graph_rules "
       "ORDER BY target_stage, num",
        PgOra::getROSession("GRAPH_RULES")
    );

    int stage;
    int cond_stage;
    TRule rule;
    int step;

    cur.def(stage)
       .def(cond_stage)
       .def(rule.num)
       .def(step)
       .exec();

    std::map<TStageStep,TMapRules> result;

    while (!cur.fen()) {
        rule.cond_stage = TStage(cond_stage);
        result[TStageStep(step)][TStage(stage)].push_back(rule);
    }

    return result;
}

TMapStatuses loadStageStatuses()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_type, stage_id, status, status_lat "
       "FROM stage_statuses "
       "ORDER BY stage_type",
        PgOra::getROSession("STAGE_STATUSES")
    );

    int stage_type;
    int stage;
    TStage_Status status;

    cur.def(stage_type)
       .def(stage)
       .def(status.status)
       .defNull(status.status_lat, "")
       .exec();

    TMapStatuses result;

    while (!cur.fen()) {
        status.stage = TStage(stage);
        result[TStage_Type(stage_type)].push_back(status);
    }

    return result;
}

TGraph_Level loadGraphLevel()
{
    TQuery Qry( &OraSession );
    Qry.SQLText = "SELECT target_stage, level "
                  "FROM (SELECT target_stage, cond_stage "
                        "FROM graph_rules "
                        "WHERE next = 1) a "
                  "START WITH cond_stage = 0 "
                  "CONNECT BY PRIOR target_stage = cond_stage";
    Qry.Execute();

    TGraph_Level result;

    while ( !Qry.Eof ) {
        TStage_Level gl;
        gl.stage = (TStage)Qry.FieldAsInteger( "target_stage" );
        gl.level = Qry.FieldAsInteger( "level" );
        result.push_back( gl );
        Qry.Next();
    }

    return result;
}

TStagesRules::TStagesRules()
{
    UpdateGraph_Stages();
    ClientStages = loadClientStages();
    GrphRls = loadGraphRules();
    StageStatuses = loadStageStatuses();
    GrphLvl = loadGraphLevel();
}

string getLocaleName( const string &name, const string &name_lat, bool is_lat )
{
    if ( !is_lat || name_lat.empty() )
        return name;
    else
        return name_lat;
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
        NewTextChild( snode, "name", getLocaleName( i->name, i->name_lat, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU ) );
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
        NewTextChild( stagestatusNode, "status", getLocaleName( s->status, s->status_lat, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU ) );
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

string TStagesRules::status_view( TStage_Type stage_type, TStage stage,
                                  boost::optional<AstraLocale::OutputLang> lang)
{
  return status(stage_type, stage, lang? (lang->get() != AstraLocale::LANG_RU):
                                        (TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
}

string TStagesRules::status( TStage_Type stage_type, TStage stage, bool is_lat )
{
  TStage_Statuses &v = StageStatuses[ stage_type ];
  for( TStage_Statuses::iterator s=v.begin(); s!=v.end(); s++ ) {
    if ( s->stage == stage )
      return getLocaleName( s->status, s->status_lat, is_lat );
  }
  return "";
}

string TStagesRules::stage_name_view( TStage stage, const std::string &airp )
{
  return stage_name(stage, airp, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
}

string TStagesRules::stage_name( TStage stage, const std::string &airp, bool is_lat )
{
    string res, res1;
    for ( vector<TStage_name>::iterator n=Graph_Stages.begin(); n!=Graph_Stages.end(); n++ ) {
        if ( n->stage == stage ) {
        if ( n->airp.empty() ) {
            res1 = getLocaleName( n->name, n->name_lat, is_lat );
      }
            else {
              if ( n->airp == airp ) {
                res = getLocaleName( n->name, n->name_lat, is_lat );
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

int calculatePriority(const TStageTime& stage_time)
{
    int result = 0;
    if (!stage_time.airline.empty())   { result |= 8; }
    if (!stage_time.airp.empty())      { result |= 4; }
    if (!stage_time.craft.empty())     { result |= 2; }
    if (!stage_time.trip_type.empty()) { result |= 1; }
    return result;
}

std::vector<TStageTime> loadStageTimes(const TStage stage)
{
    std::vector<TStageTime> result;
    {
        DbCpp::CursCtl cur = make_db_curs(
           "SELECT time "
           "FROM graph_stages "
           "WHERE stage_id = :stage",
            PgOra::getROSession("GRAPH_STAGES")
        );

        int stime;

        cur.stb()
           .def(stime)
           .bind(":stage", int(stage))
           .EXfet();

        if (DbCpp::ResultCode::NoDataFound != cur.err()) {
            result.emplace_back(TStageTime{ .time = stime, .priority = -1});
        }
    }

    {
        DbCpp::CursCtl cur = make_db_curs(
           "SELECT DISTINCT airline, airp, craft, trip_type, time "
           "FROM graph_times "
           "WHERE stage_id = :stage",
            PgOra::getROSession("GRAPH_TIMES")
        );

        TStageTime stage_time;

        cur.stb()
           .defNull(stage_time.airline, "")
           .defNull(stage_time.airp, "")
           .defNull(stage_time.craft, "")
           .defNull(stage_time.trip_type, "")
           .def(stage_time.time)
           .bind(":stage", int(stage))
           .exec();

        while (!cur.fen()) {
            stage_time.priority = calculatePriority(stage_time);
            result.push_back(stage_time);
        }
    }

    std::stable_sort(result.begin(), result.end(), [](const TStageTime& lhs, const TStageTime& rhs) {
        return std::tie(lhs.priority, lhs.airline, lhs.airp, lhs.craft, lhs.trip_type, lhs.time)
             < std::tie(rhs.priority, rhs.airline, rhs.airp, rhs.craft, rhs.trip_type, rhs.time);
    });

    return result;
}

TStageTimes::TStageTimes( TStage istage )
{
    stage = istage;
    GetStageTimes( );
}

void TStageTimes::GetStageTimes( )
{
    times = loadStageTimes(stage);
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
           OpenWEBCheckIn( point_id );
           break;
    case sOpenKIOSKCheckIn:
           /*открытие KIOSK-регистрации*/
           break;
    case sCloseWEBCancel:
           /*Запрет разрегистрации web-пассажира*/
           break;
    case sCloseCheckIn:
           /*Закрытие регистрации*/
           CloseCheckIn( point_id );
           break;
    case sCloseWEBCheckIn:
           /*закрытие WEB-регистрации*/
           CloseWEBCheckIn( point_id );
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
  on_change_trip( CALL_POINT, point_id, ChangeTrip::ExecStages );
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
   "      trip_stages.time_auto_not_act <= :now AND "
   "      trip_stages.time_auto_not_act >= :now - 3 "
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
      TFlights flightsForLock;
      flightsForLock.Get( point_id, ftTranzit );
      flightsForLock.Lock(__FUNCTION__);
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
          TReqInfo::Instance()->LocaleToLog( "EVT.STAGE.COMPLETED_ACT_TIME", LEvntPrms() << PrmStage("stage", (TStage)stage_id, airp)
                                             << PrmDate("act_time", act_stage, "hh:nn dd.mm.yy (UTC)"),
                                             evtGraph, point_id, stage_id );
          }
        }
      catch( Exception &E ) {
        try { ASTRA::rollback( ); } catch(...) { };
        ProgError( STDLOG, "Ошибка astra_timer: %s. Время %s, point_id=%d, stage_id=%d",
                   E.what(),
                   DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                   point_id, stage_id );
      }
            catch( ... ) {
                try { ASTRA::rollback( ); } catch(...) { };
                ProgError( STDLOG, "unknown timer error" );
            }
            ASTRA::commit(); // запоминание факта выполнения шага + лога в БД
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
          try { ASTRA::rollback( ); } catch(...) { };
          ProgError( STDLOG, "Ошибка astra_timer: %s. Время %s, point_id=%d, stage_id=%d",
                     E.what(),
                     DateTimeToStr(utcdate,"dd.mm.yyyy hh:nn:ss").c_str(),
                     point_id, stage_id );
        }
              catch( ... ) {
                try { ASTRA::rollback( ); } catch(...) { };
                ProgError( STDLOG, "unknown timer error" );
              }
        }
      ASTRA::commit();
        Qry.Next();
    }
  }
    if ( NowUTC() - execTime0 > 5.0/(1440.0*60) )
    ProgTrace( TRACE5, "Attention execute astra_time > 5 sec !!!, time=%s, steps count=%d", DateTimeToStr( NowUTC() - execTime0, "nn:ss" ).c_str(), count );

  //обработаем временные слои из tlg_comp_layers
  int curr_tid=NoExists;
  Qry.Clear();
  Qry.SQLText=
    "SELECT range_id, crs_pax_id "
    "FROM tlg_comp_layers "
    "WHERE time_remove<=SYSTEM.UTCSYSDATE "
    "ORDER BY crs_pax_id";
  Qry.Execute();
  SeatRangeIds range_ids;
  for(;!Qry.Eof;)
  {
    int crs_pax_id=NoExists;
    if (!Qry.FieldIsNULL("crs_pax_id"))
      crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
    range_ids.insert(Qry.FieldAsInteger("range_id"));

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
        check_layer_change(point_ids_spp, __FUNCTION__);
      }
      catch(Exception &E)
      {
        try { ASTRA::rollback( ); } catch(...) { };
        ProgError( STDLOG, "DeleteTlgSeatRanges: %s", E.what() );
      }
      catch(...)
      {
        try { ASTRA::rollback( ); } catch(...) { };
        ProgError( STDLOG, "DeleteTlgSeatRanges: Unknown error" );
      };
      ASTRA::commit();
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
  if ( (stage == sPrepCheckIn && (!Qry.FieldIsNULL( "bort" ) || string( "СОЧ" ) != Qry.FieldAsString( "airp" ))) ||
       (stage == sOpenCheckIn && string( "СОЧ" ) == Qry.FieldAsString( "airp" )) ) {
    ComponCreator::ComponSetter componSetter( point_id );
    ComponCreator::ComponSetter::TStatus status = componSetter.AutoSetCraft( true );
    if ( status != ComponCreator::ComponSetter::Created &&
         status != ComponCreator::ComponSetter::NoChanges ) {
        TReqInfo::Instance()->LocaleToLog("EVT.LAYOUT_NOT_FOUND", LEvntPrms()
                                          << PrmElem<std::string>("craft", etCraft, craft), evtFlt, point_id );
    }
  }
}

void PrepCheckIn( int point_id )
{
    SetCraft( point_id, sPrepCheckIn );
}

void OpenCheckIn( int point_id )
{
  SetCraft( point_id, sOpenCheckIn );
  ComponCreator::setManualCompChg( point_id );
}

void OpenWEBCheckIn( int point_id )
{
  try
  {
    TlgETDisplay(point_id);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"OpenWEBCheckIn.TlgETDisplay (point_id=%d): %s",point_id,E.what());
  };
}

void CloseWEBCheckIn( int point_id )
{
    try
    {
        vector<TypeB::TCreateInfo> createInfo;
        TypeB::TCloseWEBCheckInCreator(point_id).getInfo(createInfo);
        TelegramInterface::SendTlg(createInfo);
    }
    catch(std::exception &E)
    {
        ProgError(STDLOG,"CloseWEBCheckIn.SendTlg (point_id=%d): %s",point_id,E.what());
    };
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
  try
  {
      get_kuf_stat(point_id);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseCheckIn.get_kuf_stat (point_id=%d): %s",point_id,E.what());
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
  try
  {
    create_apis_file(point_id, ON_CLOSE_BOARDING);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"CloseBoarding.create_apis_file (point_id=%d): %s",point_id,E.what());
  };
};

void Takeoff( int point_id )
{
  add_trip_task(point_id, EMD_SYS_UPDATE, "");

  int paxCount=CheckIn::TCounters::totalRegisteredPassengers(point_id);
  deferOrExecuteFlightTask(TTripTaskKey(point_id, COLLECT_STAT, ""), paxCount);
  deferOrExecuteFlightTask(TTripTaskKey(point_id, SEND_TYPEB_ON_TAKEOFF, ""), paxCount);


  time_t time_start,time_end;

  time_start=time(NULL);
  try {
      TSyncTlgOutMng::Instance()->sync_all(point_id);
  } catch(std::exception &E) {
      ProgError(STDLOG,"%s.TSyncTlgOutMng::sync_all (point_id=%d): %s",__FUNCTION__, point_id,E.what());
  }
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! TSyncTlgOutMng::Instance()->sync_all execute time: %ld secs, point_id=%d",
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

  time_start=time(NULL);
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

  time_start=time(NULL);
  try
  {
    create_mintrans_file(point_id);
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.create_mintrans_file (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! create_mintrans_file execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);

  time_start=time(NULL);
  try
  {
    APPS::appsFlightCloseout(PointId_t(point_id));
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"Takeoff.APPSFlightCloseout (point_id=%d): %s",point_id,E.what());
  };
  time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! APPSFlightCloseout execute time: %ld secs, point_id=%d",
                     time_end-time_start,point_id);
}

void SetTripStages_IgnoreAuto( int point_id, bool ignore_auto )
{
    LogTrace(TRACE5) << __FUNCTION__ << ": point_id=" << point_id << ", ignore_auto=" << ignore_auto;

    DbCpp::CursCtl cur = make_db_curs(
       "UPDATE trip_stages "
       "SET ignore_auto = :ignore_auto "
       "WHERE point_id = :point_id",
        PgOra::getRWSession("TRIP_STAGES")
    );

    cur.stb()
       .bind(":ignore_auto", ignore_auto)
       .bind(":point_id", point_id)
       .exec();

    if (cur.rowcount() > 0) {
        LogTrace(TRACE5) << __FUNCTION__ << ": point_id=" << point_id << ", set ignore_auto=" << ignore_auto;
    }
}

bool CheckStageACT( int point_id, TStage stage_id )
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT 1 "
       "FROM trip_stages "
       "WHERE point_id = :point_id "
         "AND stage_id = :stage_id "
         "AND act IS NOT NULL",
        PgOra::getROSession("TRIP_STAGES")
    );

    cur.stb()
       .bind(":point_id", point_id)
       .bind(":stage_id", (int)stage_id)
       .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err();
}

