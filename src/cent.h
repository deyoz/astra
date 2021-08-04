#ifndef _CENT_H_
#define _CENT_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "jxtlib/JxtInterface.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "oralib.h"
#include "astra_misc.h"

enum TBalanceDataFlag { tdPass, tdBag, tdExcess, tdPad, tdCargo };


struct TPassenger {
  int pax_id;
  int parent_pax_id;
  int temp_parent_id;
  int grp_id;
  int reg_no;
  int point_dep;
  int point_arv;
  int seats;
  std::string pers_type;
  bool is_female;
  std::string surname;
  bool pr_pad;
  bool pr_wl;
  std::string zone;
  TPassenger() {
    pr_pad = false;
    pr_wl = false;
    pax_id = ASTRA::NoExists;
    temp_parent_id = ASTRA::NoExists;
    parent_pax_id = ASTRA::NoExists;
    reg_no = ASTRA::NoExists;
    seats = 0;
  };
};

struct TBalance {
  int male;
  int male_seats;
  int female;
  int female_seats;
  int chd;
  int chd_seats;
  int inf;
  int inf_seats;
  int rk_weight;
  int bag_amount;
  int bag_weight;
  int paybag_weight;
  int cargo;
  int mail;
  int pad_seats;
  int pad;
  TBalance() {
    male = 0;
    male_seats = 0;
    female = 0;
    female_seats = 0;
    chd = 0;
    chd_seats = 0;
    inf = 0;
    inf_seats = 0;
    rk_weight = 0;
    bag_amount = 0;
    bag_weight = 0;
    paybag_weight = 0;
    cargo = 0;
    mail = 0;
    pad_seats = 0;
    pad = 0;
  };
  void operator = (const TBalance &bal) {
    male = bal.male;
    male_seats = bal.male_seats;
    female = bal.female;
    female_seats = bal.female_seats;
    chd = bal.chd;
    chd_seats = bal.chd_seats;
    inf = bal.inf;
    inf_seats = bal.inf_seats;
    rk_weight = bal.rk_weight;
    bag_amount = bal.bag_amount;
    bag_weight = bal.bag_weight;
    paybag_weight = bal.paybag_weight;
    cargo = bal.cargo;
    mail = bal.mail;
    pad_seats = bal.pad_seats;
    pad = bal.pad;
  };
  void operator += (const TBalance &bal) {
    male += bal.male;
    male_seats += bal.male_seats;
    female += bal.female;
    female_seats += bal.female_seats;
    chd += bal.chd;
    chd_seats += bal.chd_seats;
    inf += bal.inf;
    inf_seats += bal.inf_seats;
    rk_weight += bal.rk_weight;
    bag_amount += bal.bag_amount;
    bag_weight += bal.bag_weight;
    paybag_weight += bal.paybag_weight;
    cargo += bal.cargo;
    mail += bal.mail;
    pad_seats += bal.pad_seats;
    pad += bal.pad;
  }
};

struct TDestBalance {
  int point_id;
  int num;
  std::string airp;
  TBalance tranzit;
  TBalance goshow;
  std::map<std::string,TBalance> total_classbal, tranzit_classbal, goshow_classbal;
};


class TBalanceData
{
  BitSet<TBalanceDataFlag> dataFlags;
  DB::TQuery *qTripSetsQry;
  DB::TQuery *qPassQry;
  DB::TQuery *qPassTQry;
  DB::TQuery *qBagQry;
  DB::TQuery *qBagTQry;
  DB::TQuery *qExcessBagQry;
  DB::TQuery *qExcessBagTQry;
  DB::TQuery *qCargoQry;
  DB::TQuery *qPADQry;
  void init();
  void getPassBalance( bool pr_tranzit_pass, int point_id, const TTripRoute &routesB, const TTripRoute &routesA, bool isLimitDest4 );
  public:
    TBalanceData( ) {
      BitSet<TBalanceDataFlag> indataFlags;
      for ( int i=(int)tdPass; i<=(int)tdCargo; i++ ) {
        dataFlags.setFlag( (TBalanceDataFlag)i );
      }
      init( );
    }
    TBalanceData( BitSet<TBalanceDataFlag> &indataFlags ) {
      dataFlags = indataFlags;
      init();
    }
    ~TBalanceData( ) {
      delete qTripSetsQry;
      if ( dataFlags.isFlag( tdPass ) ) {
        delete qPassQry;
        delete qPassTQry;
      }
      if ( dataFlags.isFlag( tdBag ) ) {
        delete qBagQry;
        delete qBagTQry;
      }
      if ( dataFlags.isFlag( tdExcess ) ) {
        delete qExcessBagQry;
        delete qExcessBagTQry;
      }
      if ( dataFlags.isFlag( tdPad ) ) {
        delete qPADQry;
      }
      if ( dataFlags.isFlag( tdCargo ) ) {
        delete qCargoQry;
      }
    }
    void get( int point_id, int pr_tranzit,
              const TTripRoute &routesB, const TTripRoute &routesA,
              bool isLimitDest4 );
    std::vector<TDestBalance> balances;
    std::map<int,TPassenger> passengers;
};

class CentInterface : public JxtInterface
{
private:
public:
  CentInterface() : JxtInterface("","cent")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CentInterface>::CreateHandler(&CentInterface::synchBalance);
     AddEvent("synchBalance",evHandle);
     evHandle=JxtHandler<CentInterface>::CreateHandler(&CentInterface::getDBFBalance);
     AddEvent("getDBFBalance",evHandle);
  };
  void synchBalance(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void getDBFBalance(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};


bool getBalanceFlightPermit( TQuery &FlightPermitQry,
                             int point_id,
                             const std::string &airline,
                             const std::string &airp,
                             int flt_no,
                             const std::string &bort,
                             const std::string &balance_type );

#endif /*_CENT_H_*/

