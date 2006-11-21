#ifndef _STAGES_H_
#define _STAGES_H_

#include <map>
#include <vector>
#include <string>
#include "basic.h"
#include "astra_consts.h"
#include <libxml/parser.h>

enum TStage { sNoActive = 0, /*�� ��⨢��*/
              sPrepCheckIn = 10, /*�����⮢�� � ॣ����樨*/
              sOpenCheckIn = 20, /*����⨥ ॣ����樨*/
              sCloseCheckIn = 30, /*�����⨥ ॣ����樨*/
              sOpenBoarding = 40, /*��砫� ��ᠤ��*/
              sCloseBoarding = 50, /*����砭�� ��ᠤ��*/
              sRegDoc = 60, /*��ଫ���� ���㬥��樨*/
              sRemovalGangWay = 70, /*���ઠ �࠯�*/
              sTakeoff = 99 /*�뫥⥫*/ };
              
enum TStage_Type { stCheckIn = 1, stBoarding = 2, stCraft = 3 };
enum TStageStep { stPrior, stNext };            

struct TTripStage {
  BASIC::TDateTime scd;
  BASIC::TDateTime est;
  BASIC::TDateTime act;
  BASIC::TDateTime old_est;
  BASIC::TDateTime old_act;
  TTripStage() {
    scd = ASTRA::NoExists;
    est = ASTRA::NoExists;
    act = ASTRA::NoExists;
    old_est = ASTRA::NoExists;	
    old_act = ASTRA::NoExists;
  }
};

typedef std::map<TStage, TTripStage> TMapTripStages;

class TTripStages {
  private:
    int point_id;
    TMapTripStages tripstages;
  public:
    TTripStages( int vpoint_id );  
    void LoadStages( int vpoint_id );    
    static void LoadStages( int vpoint_id, TMapTripStages &ts );
    static void ParseStages( xmlNodePtr tripNode, TMapTripStages &ts );
    static void WriteStages( int point_id, TMapTripStages &t );        
    BASIC::TDateTime time( TStage stage );    
    TStage getStage( TStage_Type stage_type );
};

struct TStage_Level {
  TStage stage;
  int level;
};

typedef std::vector<TStage_Level> TGraph_Level;

struct TRule {
  int num;
  TStage cond_stage;
};

struct TStage_Status {
  TStage stage;
  std::string status;
  int lvl;
};

typedef std::vector<TRule> vecRules; /* ���ᨢ �ࠢ�� ��� ������ stage*/
typedef std::map<TStage, vecRules> TMapRules; /* ���ᨢ stage � �ࠢ����� */
typedef std::vector<TStage_Status> TStage_Statuses;
typedef std::map<TStage_Type,TStage_Statuses> TMapStatuses;

class TStagesRules {
  private:
  public:
    std::map<TStageStep,TMapRules> GrphRls;
    TGraph_Level GrphLvl;
    TMapStatuses StageStatuses;
    std::map<TStage,std::string> Graph_Stages;
    TStagesRules();
    void Update();
    bool CanStatus( TStage_Type stage_type, TStage stage );
    std::string TStagesRules::status( TStage_Type stage_type, TStage stage );
    void Build( xmlNodePtr dataNode );
    static TStagesRules *Instance();

};

struct TStageTimes {
  std::string craft;
  std::string trip_type;
  int time;
};

void GetStageTimes( std::vector<TStageTimes> &stagetimes, TStage stage );
	
void astra_timer( BASIC::TDateTime utcdate );
void PrepCheckIn( int point_id );
void OpenCheckIn( int point_id );


#endif /*_STAGES_H_*/


