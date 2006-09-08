#ifndef _STAGES_H_
#define _STAGES_H_

#include <map>
#include <vector>
#include <string>
#include "basic.h"

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
  TTripStage() {
    scd = 0;
    est = 0;
    act = 0;
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
    TStagesRules();
    void Update();
    bool CanStatus( TStage_Type stage_type, TStage stage );
    std::string TStagesRules::status( TStage_Type stage_type, TStage stage );
    static TStagesRules *Instance();

};

struct TStageTimes {
  std::string craft;
  std::string trip_type;
  int time;
};

void GetStageTimes( std::vector<TStageTimes> &stagetimes, TStage stage );


#endif /*_STAGES_H_*/


