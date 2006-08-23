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
    tripStage.scd = Qry.FieldAsDateTime( "scd" );
    tripStage.est = Qry.FieldAsDateTime( "est" );
    tripStage.act = Qry.FieldAsDateTime( "act" );
    TStage stage = (TStage)Qry.FieldAsInteger( "stage_id" );
    ts.insert( make_pair( stage, tripStage ) );
    Qry.Next();
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
  if ( tripStage.act > 0 )
    return tripStage.act;
  else
    if ( tripStage.est > 0 )
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
    if ( p_level > 0 && p_level < l->level ) /* пока не перешли на след. ветку */
      continue;
    if ( !tripstages[ l->stage ].act ) { /* надо отсечь все низшие вершины */
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
 /* загрузка статусов */
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


