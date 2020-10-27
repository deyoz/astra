#ifndef COMPONCREATOR_H
#define COMPONCREATOR_H


#include "oralib.h"
//#include "salons.h"
#include "astra_misc.h"
#include "date_time.h"

namespace ComponCreator
{


struct TSetsCraftPoints: public std::vector<int> { //маршрут с одной компоновкой  comp_id
   void Clear() {
     clear();
   }
   TSetsCraftPoints() {
     Clear();
   }
};


class ComponFinder {
public:
  static bool isExistsCraft( const std::string& craft, TQuery& Qry );
  static int GetCompId( const std::string& craft, const std::string& bort, const std::string& airline,
                        const std::vector<std::string>& airps,  int f, int c, int y, bool pr_ignore_fcy,
                        TQuery& Qry );
  static int GetCompIdFromBort( const std::string& craft,
                                const std::string& bort,
                                TQuery& Qry );
};

class ComponLibraFinder {
public:
  class AstraSearchResult {
    private:
    public:
      int plan_id;
      int conf_id;
      int comp_id;
      BASIC::date_time::TDateTime time_create;
      BASIC::date_time::TDateTime time_change;

      AstraSearchResult( bool isLibraMode = true ) {
        clear();
      }
      void clear() {
        plan_id = -1;
        conf_id = -1;
        comp_id = -1;
        time_create = ASTRA::NoExists;
        time_change = ASTRA::NoExists;
      }

      bool isOk( ) {
        return ( comp_id >= 0 &&
                 time_change != ASTRA::NoExists );
      }
      void Write( TQuery& Qry );
      void ReadFromAHMIds( int plan_id, int conf_id, TQuery& Qry );
      void ReadFromAHMCompId( int comp_id, TQuery& Qry );
  };
  static std::string getConfigSQLText( bool withConfId );
  static std::string getConvertClassSQLText();
  static AstraSearchResult checkChangeAHMFromAHMIds( int plan_id, int conf_id, TQuery& Qry );
  static AstraSearchResult checkChangeAHMFromCompId( int comp_id, TQuery& Qry );
  static int getPlanId( const std::string& bort, TQuery& Qry );
  static int getConfig( int planId,
                        const std::string& airline, const std::string& bort,
                        int f, int c, int y,
                        bool pr_ignore_fcy, TQuery& Qry );
  static void SetChanges( int plan_id );
  static void SetChanges( int plan_id, int comp_id );
};

class ComponSetter: public TSetsCraftPoints {
public:
  enum TStatus { NotCrafts /* нет компоновок для типа ВС*/,
                 NotFound /* по условиям компоновка не найдена */,
                 NoChanges /* компоновка не изменилась */,
                 Created /* создана компоновка */ };

private:
  TAdvTripInfo fltInfo;
  struct LibraInfo {
    int planId;
    LibraInfo() {
      planId = -1;
    }
  } libraInfo;

  int fcomp_id;
  int SearchCompon( bool pr_tranzit_routes,
                    const std::vector<std::string>& airps,
                    TQuery &Qry );
  void Init( int point_id );
  TStatus IntSetCraftFreeSeating( );
  TStatus IntSetCraft( bool pr_tranzit_routes );
public:
  ComponSetter( int point_id ):TSetsCraftPoints() {
    Init( point_id );
  }
  TStatus SetCraft( bool pr_tranzit_routes );
  TStatus AutoSetCraft( bool pr_tranzit_routes );
  bool isLibraMode();
  int getCompId( ) {
    return fcomp_id;
  }
  void createBaseLibraCompon( ComponLibraFinder::AstraSearchResult& res, TQuery &Qry );
  void setCompId( int comp_id ) {
    fcomp_id = comp_id;
  }
};

ComponSetter::TStatus AutoSetCraft( int point_id, bool pr_tranzit_routes = true );
void InitVIP( const TTripInfo &fltInfo, TQuery &Qry );
void check_diffcomp_alarm( int point_id );
void setManualCompChg( int point_id );
void setFlightClasses( int point_id );

class LibraComps {
private:
  static bool isLibraComps( int id, bool isComps, BASIC::date_time::TDateTime& time_create );
public:
  enum CompType { ctFlight, ctComp };
  static bool isLibraComps( int id, CompType compType );
  static bool isLibraMode( const TTripInfo &info );
};

void signalChangesComp( TQuery &Qry, int plan_id, int conf_id = ASTRA::NoExists );

} //end namespace ComponCreator

#endif // COMPONCREATOR_H
