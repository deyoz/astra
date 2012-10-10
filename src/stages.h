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
              sOpenWEBCheckIn = 25, /*Открытие WEB-регистрации*/
              sOpenKIOSKCheckIn = 26, /*Открытие само-регистрации*/
              sCloseCheckIn = 30, /*Закрытие регистрации*/
              sCloseWEBCheckIn = 35, /*Закрытие WEB-регистрации*/
              sCloseKIOSKCheckIn = 36, /*Закрытие само-регистрации*/
              sOpenBoarding = 40, /*Начало посадки*/
              sCloseBoarding = 50, /*Окончание посадки*/
//              sRegDoc = 60, /*Оформление документации*/
              sRemovalGangWay = 70, /*Уборка трапа*/
              sTakeoff = 99 /*Вылетел*/ };

enum TStage_Type { stCheckIn = 1, stBoarding = 2, stCraft = 3, stWEB = 4, stKIOSK = 5 };
enum TStageStep { stPrior, stNext };

struct TTripStage {
  BASIC::TDateTime scd;
  BASIC::TDateTime est;
  BASIC::TDateTime act;
  BASIC::TDateTime old_est;
  BASIC::TDateTime old_act;
  int pr_auto;
  TTripStage() {
    scd = ASTRA::NoExists;
    est = ASTRA::NoExists;
    act = ASTRA::NoExists;
    old_est = ASTRA::NoExists;
    old_act = ASTRA::NoExists;
    pr_auto = 0;
  }
  bool equal( TTripStage stage ) {
    return ( scd == stage.scd &&
             est == stage.est &&
             act == stage.act &&
             pr_auto == stage.pr_auto );
  }
};

typedef std::map<TStage, TTripStage> TMapTripStages;
typedef std::vector<std::string> TCkinClients;


class TTripStages {
  private:
    int point_id;
    TMapTripStages tripstages;
    TCkinClients CkinClients;
  public:
    TTripStages( int vpoint_id );
    void LoadStages( int vpoint_id );
    static void LoadStages( int vpoint_id, TMapTripStages &ts );
    static void ParseStages( xmlNodePtr tripNode, TMapTripStages&ts );
    static void WriteStages( int point_id, TMapTripStages &t );
    static void WriteStagesUTC( int point_id, TMapTripStages &ts );
    static void ReadCkinClients( int point_id, TCkinClients &ckin_clients );
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
  std::string status_lat;
  int lvl;
};

typedef std::vector<TRule> vecRules; /* массив правил для одного stage*/
typedef std::map<TStage, vecRules> TMapRules; /* массив stage с правилами */
typedef std::vector<TStage_Status> TStage_Statuses;
typedef std::map<TStage_Type,TStage_Statuses> TMapStatuses;

struct TStage_name {
	TStage stage;
	std::string name;
	std::string name_lat;
	std::string airp;
};

class TStagesRules {
  private:
  	std::map<int,TCkinClients> ClientStages;
    std::vector<TStage_name> Graph_Stages;
    void Update();
  public:
    std::map<TStageStep,TMapRules> GrphRls;
    TGraph_Level GrphLvl;
    TMapStatuses StageStatuses;
    TStagesRules();
    bool CanStatus( TStage_Type stage_type, TStage stage );
    std::string status( TStage_Type stage_type, TStage stage, bool pr_locale );
    std::string stage_name( TStage stage, std::string airp, bool pr_locale );
    void Build( xmlNodePtr dataNode );
    void UpdateGraph_Stages( );
    void BuildGraph_Stages( const std::string airp, xmlNodePtr dataNode );
    bool canClientStage( const TCkinClients &ckin_clients, int stage_id );
    bool isClientStage( int stage_id );
    static TStagesRules *Instance();
};

struct TStageTime {
  std::string airline;
	std::string airp;
  std::string craft;
  std::string trip_type;
  int time;
  int priority;
};

class TStageTimes {
	 private:
	 	 TStage stage;
	 	 std::vector<TStageTime> times;
     void GetStageTimes( );
	 public:
	 	 TStageTimes( TStage istage );
     BASIC::TDateTime GetTime( const std::string &airline, const std::string &airp,
                               const std::string &craft, const std::string &triptype,
     	                         BASIC::TDateTime vtime );
};

void astra_timer( BASIC::TDateTime utcdate );
void exec_stage( int point_id, int stage_id );
void PrepCheckIn( int point_id );
void OpenCheckIn( int point_id );
void CloseCheckIn( int point_id );
void CloseBoarding( int point_id );
void Takeoff( int point_id );
void SetTripStages_IgnoreAuto( int point_id, bool ignore_auto );


#endif /*_STAGES_H_*/


