#ifndef _PERS_WEIGHTS_H_
#define _PERS_WEIGHTS_H_

#include <string>
#include <vector>
#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "events.h"

struct ClassesPersWeight {
  int id;
  int priority;
  std::string cl;
  std::string subcl;
  int male;
  int female;
  int child;
  int infant;
  ClassesPersWeight() {
    id = -1;
    priority = 0;
    male = 0;
    female = ASTRA::NoExists;
    child = 0;
    infant = 0;
  }
};

class PersWeightRules {
  private:
    std::vector<ClassesPersWeight> weights;
    bool intequal( PersWeightRules *p );
  public:
    void Clear() {
      weights.clear();
    }
    bool isFemale() {
      for ( std::vector<ClassesPersWeight>::iterator i=weights.begin(); i!=weights.end(); i++ ) {
        if ( i->female != ASTRA::NoExists && i->female != i->male )
          return true;
      }
      return false;
    }
    void Add( ClassesPersWeight &p ) {
      weights.push_back( p );
    }
    bool weight( std::string cl, std::string subcl, ClassesPersWeight &weight );
    void read( int point_id );
    bool equal( PersWeightRules *p ) {
      return ( intequal( p ) && p->intequal( this ) );
    }
    void write( int point_id );
    bool empty() {
      return weights.empty();
    }
};


struct TPerTypeWeight {
  int id;
  BASIC::TDateTime first_date;
  BASIC::TDateTime last_date;
  bool pr_summer;
  std::string airline;
  std::string craft;
  std::string bort;
  ClassesPersWeight weight;
  TPerTypeWeight() {
    id = 0;
    first_date = ASTRA::NoExists;
    last_date = ASTRA::NoExists;
    pr_summer = true;
  }
};

class TPersWeights
{
  public:
    std::vector<TPerTypeWeight> weights;
    //static TPersWeights *Instance();
    TPersWeights();
    void Update();
    void getRules( const BASIC::TDateTime &scd_utc, const std::string &airline,
                   const std::string &craft, const std::string &bort,
                   PersWeightRules &weights );
    void getRules( int point_id, PersWeightRules &rweights );
};

enum TTypeFlightWeight { withBrd, onlyCheckin };
enum TTypeCalcCommerceWeight { CWTotal, CWResidual };

class TFlightWeights
{
  public:
    int male;
    int female;
    int child;
    int infant;
    int weight_male;
    int weight_female;
    int weight_child;
    int weight_infant;
    int weight_bag;
    int weight_cabin_bag;
    int weight_cabin_bag_wo_check;
    void Clear() {
      male = 0;
      female = 0;
      child = 0;
      infant = 0;
      weight_male = 0;
      weight_female = 0;
      weight_child = 0;
      weight_infant = 0;
      weight_bag = 0;
      weight_cabin_bag = 0;
      weight_cabin_bag_wo_check = 0;
    };
    TFlightWeights(){
      Clear();
    };
    void read( int point_id, TTypeFlightWeight weight_type, bool include_wait_list );
};

int getCommerceWeight( int point_id, TTypeFlightWeight weight_type, TTypeCalcCommerceWeight calc_type );
void recountBySubcls(int point_id,
                     const TGrpToLogInfo &grpInfoBefore,
                     const TGrpToLogInfo &grpInfoAfter);
void recountBySubcls(int point_id);

int check_counters_by_subcls(int argc,char **argv);

#endif /*_PERS_WEIGHTS_H_*/

