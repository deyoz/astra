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

const std::string qryBalancePassWOCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, class, "
    "       pax.grp_id, pax.pax_id, pax.pers_type, pax_doc.gender, pax.surname, "
    "       crs_inf.pax_id AS parent_pax_id, seats, reg_no  "
    " FROM pax_grp, pax, pax_doc, crs_inf "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=pax_doc.pax_id(+) AND "
    "       pax.pax_id=crs_inf.inf_id(+) AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL";

    /*"SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, class, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,1,NULL,1,0),0)),0) AS male, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,1,0),0)),0) AS female, "
    "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS chd, "
    "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS inf, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,seats,NULL,seats,0),0)),0) AS male_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,seats,0),0)),0) AS female_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:chd,seats,0)),0) AS chd_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:inf,seats,0)),0) AS inf_seats "
    " FROM pax_grp, pax, pax_doc "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=pax_doc.pax_id(+) AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL "
    "GROUP BY point_dep, DECODE(point_dep,:point_dep,0,1), class"; */
const std::string qryBalancePassWithCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(status,:status_tranzit,1,0) as  pr_tranzit, class, "
    "       pax.grp_id, pax.pax_id, pax.pers_type, pax_doc.gender, pax.surname, "
    "       crs_inf.pax_id AS parent_pax_id, seats, reg_no  "
    " FROM pax_grp, pax, pax_doc, crs_inf "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=pax_doc.pax_id(+) AND "
    "       pax.pax_id=crs_inf.inf_id(+) AND "
    "       point_dep=:point_dep AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL ";
/*    "SELECT point_dep, "
    "       DECODE(status,:status_tranzit,1,0) as  pr_tranzit, class, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,1,NULL,1,0),0)),0) AS male, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,1,0),0)),0) AS female, "
    "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS chd, "
    "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS inf, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,seats,NULL,seats,0),0)),0) AS male_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,seats,0),0)),0) AS female_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:chd,seats,0)),0) AS chd_seats, "
    "       NVL(SUM(DECODE(pax.pers_type,:inf,seats,0)),0) AS inf_seats "
    " FROM pax_grp, pax, pax_doc "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=pax_doc.pax_id(+) AND "
    "       point_dep=:point_dep AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL "
    "GROUP BY point_dep, DECODE(status,:status_tranzit,1,0), class"; */
const std::string qryBalanceBagWOCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, class, "
    "       SUM(DECODE(bag2.pr_cabin, 0, amount, 0)) bag_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, weight, 0)) bag_weight, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, amount)) rk_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, weight)) rk_weight "
    "FROM pax_grp, bag2 "
    " WHERE pax_grp.grp_id = bag2.grp_id AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
    " GROUP BY point_dep, DECODE(point_dep,:point_dep,0,1), class";
const std::string qryBalanceBagWithCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(status,:status_tranzit,1,0) as  pr_tranzit, class, "
    "       SUM(DECODE(bag2.pr_cabin, 0, amount, 0)) bag_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, weight, 0)) bag_weight, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, amount)) rk_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, weight)) rk_weight "
    "FROM pax_grp, bag2 "
    " WHERE pax_grp.grp_id = bag2.grp_id AND "
    "       point_dep=:point_dep AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
    " GROUP BY point_dep, DECODE(status,:status_tranzit,1,0), class";
const std::string qryBalanceExcessBagWOCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, class, "
    "       SUM(NVL(pax_grp.excess,0)) paybag_weight "
    "FROM pax_grp "
    " WHERE point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pax_grp.bag_refuse = 0 "
    " GROUP BY point_dep, DECODE(point_dep,:point_dep,0,1), class";
const std::string qryBalanceExcessBagWithCheckinTranzit =
    "SELECT point_dep, "
    "       DECODE(status,:status_tranzit,1,0) as  pr_tranzit, class, "
    "       SUM(NVL(pax_grp.excess,0)) paybag_weight "
    "FROM pax_grp "
    " WHERE point_dep=:point_dep AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pax_grp.bag_refuse = 0 "
    " GROUP BY point_dep, DECODE(status,:status_tranzit,1,0), class";
/*const std::string qryBalancePad =
    "SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, pax_grp.class, "
    "       SUM(pax.seats) as count "
    " FROM pax_grp, pax, crs_pax, crs_pnr "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL AND "
    "       pax.pax_id=crs_pax.pax_id AND "
    "       crs_pax.pnr_id=crs_pnr.pnr_id AND "
    "       crs_pax.pr_del=0 AND "
    "       crs_pnr.status IN ('ID1','DG1','RG1','ID2','DG2','RG2') "
    "GROUP BY point_dep, DECODE(point_dep,:point_dep,0,1), pax_grp.class";*/
const std::string qryBalancePad =
    "SELECT pax.pax_id, point_dep, point_arv, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, pax_grp.class, "
    "       pax.seats "
    " FROM pax_grp, pax, crs_pax, crs_pnr "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       point_arv=:point_arv AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pr_brd IS NOT NULL AND "
    "       pax.pax_id=crs_pax.pax_id AND "
    "       crs_pax.pnr_id=crs_pnr.pnr_id AND "
    "       crs_pax.pr_del=0 AND "
    "       crs_pnr.status IN ('ID1','DG1','RG1','ID2','DG2','RG2') ";
const std::string qryBalanceCargo =
    "SELECT point_dep, "
    "       DECODE(point_dep,:point_dep,0,1) as  pr_tranzit, "
    "       cargo, mail "
    "FROM trip_load "
    " WHERE point_arv=:point_arv ";
    

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
  std::string gender;
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
  TQuery *qTripSetsQry;
  TQuery *qPassQry;
  TQuery *qPassTQry;
  TQuery *qBagQry;
  TQuery *qBagTQry;
  TQuery *qExcessBagQry;
  TQuery *qExcessBagTQry;
  TQuery *qCargoQry;
  TQuery *qPADQry;
  void init() {
    qTripSetsQry = new TQuery( &OraSession );
    qTripSetsQry->SQLText =
      "SELECT pr_tranz_reg FROM trip_sets WHERE point_id=:point_id";
    qTripSetsQry->DeclareVariable( "point_id", otInteger );

    balances.clear();
    //total_classpads.clear();
    passengers.clear();
    qPassQry = NULL;
    qPassTQry = NULL;
    qBagQry = NULL;
    qBagTQry = NULL;
    qExcessBagQry = NULL;
    qExcessBagTQry = NULL;
    qCargoQry = NULL;
    qPADQry = NULL;

    if ( dataFlags.isFlag( tdPass ) ) {
      qPassQry = new TQuery( &OraSession );
      qPassQry->SQLText = qryBalancePassWOCheckinTranzit.c_str();
      qPassQry->DeclareVariable( "point_dep", otInteger );
      qPassQry->DeclareVariable( "point_arv", otInteger );

      qPassTQry = new TQuery( &OraSession );
      qPassTQry->SQLText = qryBalancePassWithCheckinTranzit.c_str();
      qPassTQry->DeclareVariable( "point_dep", otInteger );
      qPassTQry->DeclareVariable( "point_arv", otInteger );
      qPassTQry->CreateVariable( "status_tranzit", otString, EncodePaxStatus( ASTRA::psTransit ) );
    }
    if ( dataFlags.isFlag( tdBag ) ) {
      qBagQry = new TQuery( &OraSession );
      qBagQry->SQLText = qryBalanceBagWOCheckinTranzit.c_str();
      qBagQry->DeclareVariable( "point_dep", otInteger );
      qBagQry->DeclareVariable( "point_arv", otInteger );

      qBagTQry = new TQuery( &OraSession );
      qBagTQry->SQLText = qryBalanceBagWithCheckinTranzit.c_str();
      qBagTQry->DeclareVariable( "point_dep", otInteger );
      qBagTQry->DeclareVariable( "point_arv", otInteger );
      qBagTQry->CreateVariable( "status_tranzit", otString, EncodePaxStatus( ASTRA::psTransit ) );
    }
    if ( dataFlags.isFlag( tdExcess ) ) {
      qExcessBagQry = new TQuery( &OraSession );
      qExcessBagQry->SQLText = qryBalanceExcessBagWOCheckinTranzit.c_str();
      qExcessBagQry->DeclareVariable( "point_dep", otInteger );
      qExcessBagQry->DeclareVariable( "point_arv", otInteger );

      qExcessBagTQry = new TQuery( &OraSession );
      qExcessBagTQry->SQLText = qryBalanceExcessBagWithCheckinTranzit.c_str();
      qExcessBagTQry->DeclareVariable( "point_dep", otInteger );
      qExcessBagTQry->DeclareVariable( "point_arv", otInteger );
      qExcessBagTQry->CreateVariable( "status_tranzit", otString, EncodePaxStatus( ASTRA::psTransit ) );
    }
    if ( dataFlags.isFlag( tdPad ) ) {
      qPADQry = new TQuery( &OraSession );
      qPADQry->SQLText = qryBalancePad.c_str();
      qPADQry->DeclareVariable( "point_dep", otInteger );
      qPADQry->DeclareVariable( "point_arv", otInteger );
    }
    if ( dataFlags.isFlag( tdCargo ) ) {
      qCargoQry = new TQuery( &OraSession );
      qCargoQry->SQLText = qryBalanceCargo.c_str();
      qCargoQry->DeclareVariable( "point_dep", otInteger );
      qCargoQry->DeclareVariable( "point_arv", otInteger );
    }
  };
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

