#ifndef _STAGES_H_
#define _STAGES_H_

#include <map>
#include <vector>
#include <string>
#include "basic.h"
#include "astra_consts.h"
#include <libxml/parser.h>

enum TStage { sNoActive = 0, /*не активен*/
              sPrepCheckIn = 10, /*Подготовка к регистрации*/
              sOpenCheckIn = 20, /*Открытие регистрации*/
              sCloseCheckIn = 30, /*Закрытие регистрации*/
              sOpenBoarding = 40, /*Начало посадки*/
              sCloseBoarding = 50, /*Окончание посадки*/
              sRegDoc = 60, /*Оформление документации*/
              sRemovalGangWay = 70, /*Уборка трапа*/
              sTakeoff = 99 /*Вылетел*/ };

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

typedef std::vector<TRule> vecRules; /* массив правил для одного stage*/
typedef std::map<TStage, vecRules> TMapRules; /* массив stage с правилами */
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
void exec_stage( int point_id, int stage_id );
void PrepCheckIn( int point_id );
void OpenCheckIn( int point_id );
void CloseCheckIn( int point_id );
void CloseBoarding( int point_id );
void Takeoff( int point_id );


#endif /*_STAGES_H_*/


