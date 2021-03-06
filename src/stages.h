#ifndef _STAGES_H_
#define _STAGES_H_

#include <map>
#include <vector>
#include <string>
#include "date_time.h"
#include "astra_consts.h"
#include <libxml/parser.h>
#include "oralib.h"
#include "astra_elems.h"
#include "astra_locale.h"
#include <serverlib/algo.h>

using BASIC::date_time::TDateTime;

enum TStage { sNoActive = 0, /*?? ??⨢??*/
              sPrepCheckIn = 10, /*?????⮢?? ? ॣ?????樨*/
              sOpenCheckIn = 20, /*?????⨥ ॣ?????樨*/
              sOpenWEBCheckIn = 25, /*?????⨥ WEB-ॣ?????樨*/
              sOpenKIOSKCheckIn = 26, /*?????⨥ ᠬ?-ॣ?????樨*/
              sCloseCheckIn = 30, /*?????⨥ ॣ?????樨*/
              sCloseWEBCancel = 31, /* ?????? ?⬥?? web-ॣ?????樨 */
              sCloseWEBCheckIn = 35, /*?????⨥ WEB-ॣ?????樨*/
              sCloseKIOSKCheckIn = 36, /*?????⨥ ᠬ?-ॣ?????樨*/
              sOpenBoarding = 40, /*??砫? ??ᠤ??*/
              sCloseBoarding = 50, /*????砭?? ??ᠤ??*/
              sRemovalGangWay = 70, /*???ઠ ?࠯?*/
              sTakeoff = 99 /*?뫥⥫*/ };

extern const char *TStageS[13];

TStage DecodeStage(const char* s);
const char* EncodeStage(TStage s);

enum TStage_Type { stCheckIn = 1,
                   stBoarding = 2,
                   stCraft = 3,
                   stWEBCheckIn = 4,
                   stKIOSKCheckIn = 5,
                   stWEBCancel = 6 };
enum TStageStep { stPrior, stNext };

class TTripStageTimes
{
  public:
    TDateTime scd;
    TDateTime est;
    TDateTime act;

    TTripStageTimes()
    {
      scd = ASTRA::NoExists;
      est = ASTRA::NoExists;
      act = ASTRA::NoExists;
    };

    TDateTime time() const
    {
      if ( act != ASTRA::NoExists )
        return act;
      else
        if ( est != ASTRA::NoExists )
          return est;
        else
          return scd;
    };
};

class TTripStage : public TTripStageTimes
{
  public:
    TStage stage;
    TDateTime old_est;
    TDateTime old_act;
    int pr_auto;
    void fromDB(TQuery &Qry);
    TTripStage() {
      old_est = ASTRA::NoExists;
      old_act = ASTRA::NoExists;
      pr_auto = 0;
    }
    bool equal( const TTripStage &stage ) const
    {
      return ( scd == stage.scd &&
               est == stage.est &&
               act == stage.act &&
               pr_auto == stage.pr_auto );
    }
};

typedef std::map<TStage, TTripStage> TMapTripStages;
typedef std::vector<std::string> TCkinClients;

class StageInitPropsContainer
{
  private:
    std::map<TStage, bool> autoAttribute_;
    std::map<TStage, int> timeBeforeScdOut_;
  public:
    void clear()
    {
      autoAttribute_.clear();
      timeBeforeScdOut_.clear();
    }
    void setAutoAttribute(const TStage stage, const bool value)
    {
      autoAttribute_.emplace(stage, value);
    }
    void setTimeBeforeScdOut(const TStage stage, const int value)
    {
      timeBeforeScdOut_.emplace(stage, value);
    }
    boost::optional<bool> getAutoAttribute(const TStage stage)
    {
      return algo::find_opt<boost::optional>(autoAttribute_, stage);
    }
    boost::optional<int> getTimeBeforeScdOut(const TStage stage)
    {
      return algo::find_opt<boost::optional>(timeBeforeScdOut_, stage);
    }
};

class TTripStages {
  private:
    int point_id;
    TMapTripStages tripstages;
    TCkinClients CkinClients;
  public:
    TTripStages( int vpoint_id );
    void LoadStages( int vpoint_id );
    static void LoadStages( int vpoint_id, TMapTripStages &ts );
    static void LoadStage( int vpoint_id, TStage stage, TTripStage &ts );
    static void ParseStages( xmlNodePtr tripNode, TMapTripStages&ts );
    static void WriteStages( int point_id, TMapTripStages &t );
    static void WriteStagesUTC( int point_id, TMapTripStages &ts );
    static void ReadCkinClients( int point_id, TCkinClients &ckin_clients );
    TDateTime time( TStage stage ) const;
    TTripStageTimes getStageTimes( TStage stage ) const;
    TStage getStage( TStage_Type stage_type );
    bool allowToBagCheckIn() { return getStage(stCheckIn) == sOpenCheckIn; }
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

typedef std::vector<TRule> vecRules; /* ???ᨢ ?ࠢ?? ??? ?????? stage*/
typedef std::map<TStage, vecRules> TMapRules; /* ???ᨢ stage ? ?ࠢ????? */
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
    std::string status( TStage_Type stage_type, TStage stage, bool is_lat );
    std::string status_view( TStage_Type stage_type, TStage stage,
                             boost::optional<AstraLocale::OutputLang> lang = boost::none);
    std::string stage_name( TStage stage, const std::string &airp, bool is_lat );
    std::string stage_name_view( TStage stage, const std::string &airp );
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
     TDateTime GetTime( const std::string &airline, const std::string &airp,
                               const std::string &craft, const std::string &triptype,
                                 TDateTime vtime );
};

bool CompatibleStage( TStage stage );

void astra_timer( TDateTime utcdate );
void exec_stage( int point_id, int stage_id );
void PrepCheckIn( int point_id );
void OpenCheckIn( int point_id );
void OpenWEBCheckIn( int point_id );
void CloseCheckIn( int point_id );
void CloseWEBCheckIn( int point_id );
void CloseBoarding( int point_id );
void Takeoff( int point_id );
void SetTripStages_IgnoreAuto( int point_id, bool ignore_auto );

bool CheckStageACT( int point_id, TStage stage );


#endif /*_STAGES_H_*/


