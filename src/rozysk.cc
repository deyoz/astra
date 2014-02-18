#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

#include "basic.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "base_tables.h"
#include "http_io.h"
#include "passenger.h"
#include "file_queue.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const std::string ROZYSK_MAGISTRAL      = "ROZYSK_MAGISTRAL";
const std::string ROZYSK_MAGISTRAL_24   = "ROZYSK_MAGISTRAL_24";
const std::string ROZYSK_SIRENA         = "ROZYSK_SIR";
const std::string PARAM_ACTION_CODE     = "ACTION_CODE";
const std::string ROZYSK_MINTRANS       = "ROZYSK_MINTRANS";
const std::string ROZYSK_MINTRANS_24    = "ROZYSK_MINTRANS_24";
const std::string FILE_MINTRANS_TYPE    = "MINTRANS";
const std::string MINTRANS_ID           = "13001";

namespace rozysk
{

const std::string endl = "\r\n";

enum TRowType {rowMagistral, rowMintrans};

struct TRow {
    //дополнительно
    TDateTime time;
    string term;
    string user_descr;
    //рейс
    string airline;
    int flt_no;
    string suffix;
    TDateTime takeoff_utc;
    TDateTime takeoff_local;
    string airp_dep;
    //пассажир
    string airp_arv;
    string surname;
    string name;
    string seat_no;
    int bag_amount;
    int bag_weight;
    int rk_weight;
    string tags;
    string pnr;
    string operation;
    int transit_route;
    bool transfer_route;
    int reg_no;
    int pax_id;
    //документ
    string doc_type;
    string doc_issue_country;
    string doc_no;
    string doc_nationality;
    string doc_gender;
    TDateTime doc_birth_date;
    string doc_type_rcpt;
    //виза
    string visa_no;
    string visa_issue_place;
    TDateTime visa_issue_date;
    string visa_applic_country;
    string visa_birth_place;
    //билет
    string ticket_no;

    TRow()
    {
      time=NoExists;
      flt_no=NoExists;
      takeoff_utc=NoExists;
      takeoff_local=NoExists;
      bag_amount=NoExists;
      bag_weight=NoExists;
      rk_weight=NoExists;
      transit_route=NoExists;
      transfer_route=false;
      reg_no=NoExists;
      pax_id=NoExists;
      doc_birth_date=NoExists;
      visa_issue_date=NoExists;
    };
    TRow& fltFromDB(TQuery &Qry);
    TRow& paxFromDB(TQuery &Qry);
    TRow& setPnr(const vector<TPnrAddrItem> &pnrs);
    TRow& setDoc(const CheckIn::TPaxDocItem &doc);
    TRow& setVisa(const CheckIn::TPaxDocoItem &doc);
    TRow& setTkn(const CheckIn::TPaxTknItem &tkn);
    const TRow& toDB(TRowType type, TQuery &Qry, bool check_sql=true) const;
};

TRow& TRow::fltFromDB(TQuery &Qry)
{
  airline=Qry.FieldAsString("airline");
  flt_no=Qry.FieldIsNULL("flt_no")?NoExists:Qry.FieldAsInteger("flt_no");
  suffix=Qry.FieldAsString("suffix");
  airp_dep=Qry.FieldAsString("airp_dep");
  TDateTime takeoff=Qry.FieldIsNULL("takeoff")?NoExists:Qry.FieldAsDateTime("takeoff");
  bool is_utc=Qry.FieldAsInteger("is_utc")!=0;
  takeoff_utc=NoExists;
  takeoff_local=NoExists;
  if (takeoff!=NoExists)
  {
    if (is_utc)
    {
      takeoff_utc=takeoff;
      takeoff_local=UTCToLocal(takeoff, AirpTZRegion(airp_dep));
    }
    else
    {
      takeoff_utc=NoExists;
      takeoff_local=takeoff;
    };
  };
  return *this;
};

TRow& TRow::paxFromDB(TQuery &Qry)
{
  airp_arv=Qry.FieldAsString("airp_arv");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  seat_no=Qry.FieldAsString("seat_no");
  bag_amount=Qry.FieldIsNULL("bag_amount")?NoExists:Qry.FieldAsInteger("bag_amount");
  bag_weight=Qry.FieldIsNULL("bag_weight")?NoExists:Qry.FieldAsInteger("bag_weight");
  rk_weight=Qry.FieldIsNULL("rk_weight")?NoExists:Qry.FieldAsInteger("rk_weight");
  tags=Qry.FieldAsString("tags");
  operation=Qry.FieldAsString("operation");
  reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
  pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
  return *this;
};


TRow& TRow::setPnr(const vector<TPnrAddrItem> &pnrs)
{
  pnr.clear();
  if (!pnrs.empty()) pnr=pnrs.begin()->addr;
  return *this;
};

TRow& TRow::setDoc(const CheckIn::TPaxDocItem &doc)
{
  doc_type=doc.type;
  doc_issue_country=doc.issue_country;
  doc_no=doc.no;
  doc_nationality=doc.nationality;
  doc_gender=doc.gender;
  doc_birth_date=doc.birth_date;
  doc_type_rcpt=doc.type_rcpt;
  return *this;
};

TRow& TRow::setVisa(const CheckIn::TPaxDocoItem &visa)
{
  if (visa.type=="V")
  {
    visa_no=visa.no;
    visa_issue_place=visa.issue_place;
    visa_issue_date=visa.issue_date;
    visa_applic_country=visa.applic_country;
    visa_birth_place=visa.birth_place;
  }
  else
  {
    visa_no.clear();
    visa_issue_place.clear();
    visa_issue_date=NoExists;
    visa_applic_country.clear();
    visa_birth_place.clear();
  };
  return *this;
};

TRow& TRow::setTkn(const CheckIn::TPaxTknItem &tkn)
{
  ticket_no=tkn.no;
  return *this;
};

const TRow& TRow::toDB(TRowType type, TQuery &Qry, bool check_sql) const
{
  const char* sql=
    "DECLARE "
    "  pass BINARY_INTEGER; "
    "BEGIN "
    "  FOR pass IN 1..2 LOOP "
    "    IF :pax_id IS NOT NULL THEN "
    "      UPDATE rozysk "
    "      SET time=:time, "
    "          term=:term, "
    "          user_descr=:user_descr, "
    "          airline=:airline, "
    "          flt_no=:flt_no, "
    "          suffix=:suffix, "
    "          takeoff=:takeoff, "
    "          airp_dep=:airp_dep, "
    "          airp_arv=:airp_arv, "
    "          surname=:surname, "
    "          name=:name, "
    "          seat_no=:seat_no, "
    "          bag_amount=:bag_amount, "
    "          bag_weight=:bag_weight, "
    "          rk_weight=:rk_weight, "
    "          tags=:tags, "
    "          pnr=:pnr, "
    "          operation=:operation, "
    "          route_type=:route_type, "
    "          reg_no=:reg_no, "
    "          doc_type=:doc_type, "
    "          doc_issue_country=:doc_issue_country, "
    "          doc_no=:doc_no, "
    "          doc_nationality=:doc_nationality, "
    "          doc_gender=:doc_gender, "
    "          doc_birth_date=:doc_birth_date, "
    "          doc_type_rcpt=:doc_type_rcpt, "
    "          visa_no=:visa_no, "
    "          visa_issue_place=:visa_issue_place, "
    "          visa_issue_date=:visa_issue_date, "
    "          visa_applic_country=:visa_applic_country, "
    "          visa_birth_place=:visa_birth_place, "
    "          ticket_no=:ticket_no "
    "      WHERE pax_id=:pax_id; "
    "      IF SQL%FOUND THEN EXIT; END IF;"
    "    END IF; "
    "    BEGIN "
    "      INSERT INTO rozysk "
    "        (time, term, user_descr, "
    "         airline, flt_no, suffix, takeoff, airp_dep, "
    "         airp_arv, surname, name, seat_no, bag_amount, bag_weight, rk_weight, "
    "         tags, pnr, operation, route_type, reg_no, pax_id, "
    "         doc_type, doc_issue_country, doc_no, doc_nationality, doc_gender, doc_birth_date, doc_type_rcpt, "
    "         visa_no, visa_issue_place, visa_issue_date, visa_applic_country, visa_birth_place, "
    "         ticket_no) "
    "      VALUES "
    "        (:time, :term, :user_descr, "
    "         :airline, :flt_no, :suffix, :takeoff, :airp_dep, "
    "         :airp_arv, :surname, :name, :seat_no, :bag_amount, :bag_weight, :rk_weight, "
    "         :tags, :pnr, :operation, :route_type, :reg_no, :pax_id, "
    "         :doc_type, :doc_issue_country, :doc_no, :doc_nationality, :doc_gender, :doc_birth_date, :doc_type_rcpt, "
    "         :visa_no, :visa_issue_place, :visa_issue_date, :visa_applic_country, :visa_birth_place, "
    "         :ticket_no); "
    "      EXIT; "
    "    EXCEPTION "
    "      WHEN DUP_VAL_ON_INDEX THEN "
    "        IF pass=1 AND :pax_id IS NOT NULL THEN NULL; ELSE RAISE; END IF; "
    "    END; "
    "  END LOOP; "
    "END; ";
  if (check_sql)
  {
    if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
    {
      Qry.Clear();
      Qry.SQLText=sql;
      //дополнительно
      Qry.DeclareVariable("time", otDate);
      Qry.DeclareVariable("term", otString);
      Qry.DeclareVariable("user_descr", otString);
      //рейс
      Qry.DeclareVariable("airline", otString);
      Qry.DeclareVariable("flt_no", otInteger);
      Qry.DeclareVariable("suffix", otString);
      Qry.DeclareVariable("takeoff", otDate);
      Qry.DeclareVariable("airp_dep", otString);
      //пассажир
      Qry.DeclareVariable("airp_arv", otString);
      Qry.DeclareVariable("surname", otString);
      Qry.DeclareVariable("name", otString);
      Qry.DeclareVariable("seat_no", otString);
      Qry.DeclareVariable("bag_amount", otInteger);
      Qry.DeclareVariable("bag_weight", otInteger);
      Qry.DeclareVariable("rk_weight", otInteger);
      Qry.DeclareVariable("tags", otString);
      Qry.DeclareVariable("pnr", otString);
      Qry.DeclareVariable("operation", otString);
      Qry.DeclareVariable("route_type", otInteger);
      Qry.DeclareVariable("reg_no", otInteger);
      Qry.DeclareVariable("pax_id", otInteger);
      //документ
      Qry.DeclareVariable("doc_type", otString);
      Qry.DeclareVariable("doc_issue_country", otString);
      Qry.DeclareVariable("doc_no", otString);
      Qry.DeclareVariable("doc_nationality", otString);
      Qry.DeclareVariable("doc_gender", otString);
      Qry.DeclareVariable("doc_birth_date", otDate);
      Qry.DeclareVariable("doc_type_rcpt", otString);
      //виза
      Qry.DeclareVariable("visa_no", otString);
      Qry.DeclareVariable("visa_issue_place", otString);
      Qry.DeclareVariable("visa_issue_date", otDate);
      Qry.DeclareVariable("visa_applic_country", otString);
      Qry.DeclareVariable("visa_birth_place", otString);
      //билет
      Qry.DeclareVariable("ticket_no", otString);
    };
  };
  //дополнительно
  time!=NoExists?Qry.SetVariable("time", time):
                 Qry.SetVariable("time", FNull);
  Qry.SetVariable("term", term);
  Qry.SetVariable("user_descr", user_descr);
  //рейс
  Qry.SetVariable("airline", airline);
  flt_no!=NoExists?Qry.SetVariable("flt_no", flt_no):
                   Qry.SetVariable("flt_no", FNull);
  Qry.SetVariable("suffix", suffix);

  if (type==rowMagistral)
    takeoff_local!=NoExists?Qry.SetVariable("takeoff", takeoff_local):
                            Qry.SetVariable("takeoff", FNull);
  if (type==rowMintrans)
    takeoff_utc!=NoExists?Qry.SetVariable("takeoff", takeoff_utc):
                          Qry.SetVariable("takeoff", FNull);

  Qry.SetVariable("airp_dep", airp_dep);
  //пассажир
  Qry.SetVariable("airp_arv", airp_arv);
  Qry.SetVariable("surname", surname);
  Qry.SetVariable("name", name);
  Qry.SetVariable("seat_no", seat_no);
  bag_amount!=NoExists?Qry.SetVariable("bag_amount", bag_amount):
                       Qry.SetVariable("bag_amount", FNull);
  bag_weight!=NoExists?Qry.SetVariable("bag_weight", bag_weight):
                       Qry.SetVariable("bag_weight", FNull);
  rk_weight!=NoExists?Qry.SetVariable("rk_weight", rk_weight):
                      Qry.SetVariable("rk_weight", FNull);
  Qry.SetVariable("tags", tags.substr(0,250));
  Qry.SetVariable("pnr", pnr);
  if (type==rowMagistral)
    Qry.SetVariable("operation", operation);
  if (type==rowMintrans)
  {
    if (operation=="K1")
      Qry.SetVariable("operation", "04");
    else if (operation=="K2")
      Qry.SetVariable("operation", "05");
    else if (operation=="K3")
      Qry.SetVariable("operation", "06");
    else if (operation=="K0")
      Qry.SetVariable("operation", "16");
    else return *this;
  };
  if (transit_route==NoExists)
    Qry.SetVariable("route_type", FNull);
  else if (transit_route)
    Qry.SetVariable("route_type", 1);
  else if (transfer_route)
    Qry.SetVariable("route_type", 2);
  else
    Qry.SetVariable("route_type", 0);

  reg_no!=NoExists?Qry.SetVariable("reg_no", reg_no):
                   Qry.SetVariable("reg_no", FNull);
  pax_id!=NoExists?Qry.SetVariable("pax_id", pax_id):
                   Qry.SetVariable("pax_id", FNull);
  if (type==rowMagistral) Qry.SetVariable("pax_id", FNull);
  //документ
  Qry.SetVariable("doc_type", doc_type);
  Qry.SetVariable("doc_issue_country", doc_issue_country);
  if (doc_no.size()<=15)
    Qry.SetVariable("doc_no", doc_no);
  else
    Qry.SetVariable("doc_no", FNull);
  Qry.SetVariable("doc_nationality", doc_nationality);
  Qry.SetVariable("doc_gender", doc_gender);
  doc_birth_date!=NoExists?Qry.SetVariable("doc_birth_date", doc_birth_date):
                           Qry.SetVariable("doc_birth_date", FNull);
  Qry.SetVariable("doc_type_rcpt", doc_type_rcpt);
  //виза
  Qry.SetVariable("visa_no", visa_no);
  Qry.SetVariable("visa_issue_place", visa_issue_place);
  visa_issue_date!=NoExists?Qry.SetVariable("visa_issue_date", visa_issue_date):
                            Qry.SetVariable("visa_issue_date", FNull);
  Qry.SetVariable("visa_applic_country", visa_applic_country);
  Qry.SetVariable("visa_birth_place", visa_birth_place);
  //билет
  if (ticket_no.size()<=15)
    Qry.SetVariable("ticket_no", ticket_no);
  else
    Qry.SetVariable("ticket_no", FNull);
  Qry.Execute();
  return *this;
};

void check_flight(TQuery &Qry, int point_id)
{
  if (Qry.Eof) throw Exception("flight not found (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("airline")) throw Exception("empty airline (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("flt_no")) throw Exception("empty flt_no (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("takeoff")) throw Exception("empty takeoff (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("airp_dep")) throw Exception("empty airp_dep (point_id=%d)", point_id);
};

void get_flight(int point_id, TRow &r)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, "
    "       NVL(scd_out,NVL(est_out,act_out)) AS takeoff, 1 AS is_utc, "
    "       airp AS airp_dep "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  check_flight(Qry, point_id);
  r.fltFromDB(Qry);
};

void get_crs_flight(int point_id, TRow &r)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, scd AS takeoff, 0 AS is_utc, airp_dep "
    "FROM tlg_trips WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  check_flight(Qry, point_id);
  r.fltFromDB(Qry);
};

void get_transit_route(int point_dep, int point_arv, TRow &r)
{
  TTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtNotCurrent, trtNotCancelled);
  r.transit_route=NoExists;
  for(TTripRoute::const_iterator i=route.begin(); i!=route.end(); ++i)
    if (point_arv==i->point_id)
    {
      r.transit_route=(i==route.begin()?0:1);
      break;
    };
};

void get_transfer_route(int grp_id, TRow &r)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.SQLText = "SELECT grp_id FROM transfer WHERE grp_id=:grp_id AND rownum<2";
  Qry.Execute();
  if (!Qry.Eof)
  {
    r.transfer_route=true;
    return;
  };
  Qry.SQLText = "SELECT grp_id FROM tckin_segments WHERE grp_id=:grp_id AND rownum<2";
  Qry.Execute();
  if (!Qry.Eof)
  {
    r.transfer_route=true;
    return;
  };
  r.transfer_route=false;
};

const char* pax_sql=
  "SELECT "
  "  pax_grp.point_dep AS point_id, pax_grp.point_arv, pax_grp.airp_arv, pax_grp.grp_id, "
  "  pax.pax_id, pax.surname, pax.name, pax.reg_no, "
  "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
  "  NVL(ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
  "  NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
  "  NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight, "
  "  ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num) AS tags, "
  "  DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,DECODE(pax.pr_exam,0,'K1','K2'),'K3'),'K0') AS operation "
  "FROM pax_grp, pax "
  "WHERE pax_grp.grp_id=pax.grp_id";
  
const char* crs_pax_sql=
  "SELECT "
  "  crs_pnr.point_id, crs_pnr.airp_arv, crs_pnr.pnr_id, "
  "  crs_pax.pax_id, crs_pax.surname, crs_pax.name, NULL AS reg_no, "
  "  salons.get_crs_seat_no(seat_xname,seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
  "  NULL AS bag_amount, NULL AS bag_weight, NULL AS rk_weight, "
  "  NULL AS tags, "
  "  'K0' AS operation "
  "FROM crs_pnr,crs_pax,pax "
  "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pr_del=0 AND "
  "      crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL";

void check_pax(TQuery &Qry, int pax_id)
{
  if (Qry.Eof) throw Exception("passenger not found (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("airp_arv")) throw Exception("empty airp_arv (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("surname")) throw Exception("empty surname (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("operation")) throw Exception("empty operation(pax_id=%d)", pax_id);
};

void sync_pax_internal(int id,
                       const string &term,
                       const string &user_descr,
                       bool is_grp_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  if (is_grp_id)
    sql << pax_sql << " AND pax_grp.grp_id=:grp_id";
  else
    sql << pax_sql << " AND pax.pax_id=:pax_id";
  Qry.SQLText=sql.str().c_str();
  if (is_grp_id)
    Qry.CreateVariable("grp_id", otInteger, id);
  else
    Qry.CreateVariable("pax_id", otInteger, id);
  Qry.Execute();
  if (Qry.Eof)
  {
    if (is_grp_id)
      throw Exception("group not found (grp_id=%d)", id);
    else
      throw Exception("passenger not found (pax_id=%d)", id);
  };
  TQuery InsQry(&OraSession);
  TRow row;
  //дополнительно
  row.time=NowUTC();
  row.term=term;
  row.user_descr=user_descr;
  //рейс
  int point_id=Qry.FieldAsInteger("point_id");
  get_flight(point_id, row);

  bool check_sql=true;
  if (!Qry.Eof)
  {
    get_transit_route(point_id, Qry.FieldAsInteger("point_arv"), row);
    get_transfer_route(Qry.FieldAsInteger("grp_id"), row);
    for(;!Qry.Eof;Qry.Next())
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      //пассажир
      check_pax(Qry, pax_id);
      row.paxFromDB(Qry);
      //pnr
      vector<TPnrAddrItem> pnrs;
      GetPaxPnrAddr(pax_id, pnrs);
      row.setPnr(pnrs);
      //документ
      CheckIn::TPaxDocItem doc;
      LoadPaxDoc(pax_id, doc);
      row.setDoc(doc);
      //виза
      CheckIn::TPaxDocoItem doco;
      LoadPaxDoco(pax_id, doco);
      row.setVisa(doco);
      //билет
      CheckIn::TPaxTknItem tkn;
      LoadPaxTkn(pax_id, tkn);
      row.setTkn(tkn);

      for(int pass=0; pass<2; pass++)
      {
        TRowType type=(pass==0?rowMagistral:rowMintrans);
        row.toDB(type, InsQry, check_sql);
        check_sql=false;
      };
    };
  };
};

void sync_pax(int pax_id, const string &term, const string &user_descr)
{
  try
  {
    sync_pax_internal(pax_id, term, user_descr, false);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax: %s", e.what());
  };
};

void sync_pax_grp(int grp_id, const string &term, const string &user_descr)
{
  try
  {
    sync_pax_internal(grp_id, term, user_descr, true);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax_grp: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax_grp: %s", e.what());
  };
};

void sync_crs_pax_internal(int id,
                           const string &term,
                           const string &user_descr,
                           bool is_pnr_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  if (is_pnr_id)
    sql << crs_pax_sql << " AND crs_pnr.pnr_id=:pnr_id";
  else
    sql << crs_pax_sql << " AND crs_pax.pax_id=:pax_id";
  Qry.SQLText=sql.str().c_str();
  if (is_pnr_id)
    Qry.CreateVariable("pnr_id", otInteger, id);
  else
    Qry.CreateVariable("pax_id", otInteger, id);
  Qry.Execute();
  if (Qry.Eof) return;

  TQuery InsQry(&OraSession);
  TRow row;
  //дополнительно
  row.time=NowUTC();
  row.term=term;
  row.user_descr=user_descr;
  //рейс
  int point_id=Qry.FieldAsInteger("point_id");
  get_crs_flight(point_id, row);
  //pnr
  vector<TPnrAddrItem> pnrs;
  GetPnrAddr(Qry.FieldAsInteger("pnr_id"), pnrs);
  row.setPnr(pnrs);

  bool check_sql=true;
  for(;!Qry.Eof;Qry.Next())
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    //пассажир
    check_pax(Qry, pax_id);
    row.paxFromDB(Qry);
    //документ
    CheckIn::TPaxDocItem doc;
    LoadCrsPaxDoc(pax_id, doc);
    row.setDoc(doc);
    //виза
    CheckIn::TPaxDocoItem doco;
    LoadCrsPaxVisa(pax_id, doco);
    row.setVisa(doco);
    //билет
    CheckIn::TPaxTknItem tkn;
    LoadCrsPaxTkn(pax_id, tkn);
    row.setTkn(tkn);

    row.toDB(rowMagistral, InsQry, check_sql);
    check_sql=false;
  };
};

void sync_crs_pax(int pax_id, const string &term, const string &user_descr)
{
  try
  {
    sync_crs_pax_internal(pax_id, term, user_descr, false);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pax: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pax: %s", e.what());
  };
};

void sync_crs_pnr(int pnr_id, const string &term, const string &user_descr)
{
  try
  {
    sync_crs_pax_internal(pnr_id, term, user_descr, true);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pnr: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pnr: %s", e.what());
  };
};

struct TPax {
    TDateTime transactionDate;    //time
    string airline;               //airline
    int flightNumber;             //flt_no
    string flightSuffix;          //suffix
    TDateTime departureDate;      //takeoff
    string rackNumber;            //term
    string seatNumber;            //seat_no
    string firstName;             //name
    string lastName;              //surname
    string patronymic;            //patronymic
    string documentNumber;        //doc_no
    string operationType;         //operation
    string baggageReceiptsNumber; //tags
    string departureAirport;      //airp_dep
    string arrivalAirport;        //airp_arv
    string baggageWeight;         //bag_weight
    string PNR;                   //pnr
    string visaNumber;            //visa_no
    TDateTime visaDate;           //visa_issue_date
    string visaPlace;             //visa_issue_place
    string visaCountryCode;       //visa_applic_country
    string nationality;           //doc_nationality
    string gender;                //doc_gender+name
};

string make_soap_content(const vector<TPax> &paxs)
{
    ostringstream result;
    result <<
        "<soapenv:Envelope xmlns:soapenv='http://schemas.xmlsoap.org/soap/envelope/' xmlns:sir='http://vtsft.ru/sirenaSearchService/'>\n"
        "   <soapenv:Header/>\n"
        "   <soapenv:Body>\n"
        "      <sir:importASTDateRequest>\n"
        "         <sir:login>test</sir:login>\n";
    for(vector<TPax>::const_iterator iv = paxs.begin(); iv != paxs.end(); iv++)
        result <<
            "         <sir:policyParameters>\n"
            "            <sir:transactionDate>" << DateTimeToStr(iv->transactionDate, "yyyy-mm-dd") << "</sir:transactionDate>\n"
            "            <sir:transactionTime>" << DateTimeToStr(iv->transactionDate, "yyyymmddhhnnss") << "</sir:transactionTime>\n"
            "            <sir:flightNumber>" << iv->flightNumber << iv->flightSuffix << "</sir:flightNumber>\n"
            "            <sir:departureDate>" << (iv->departureDate==NoExists?"":DateTimeToStr(iv->departureDate, "yyyy-mm-dd")) << "</sir:departureDate>\n"
            "            <sir:rackNumber>" << iv->rackNumber << "</sir:rackNumber>\n"
            "            <sir:seatNumber>" << iv->seatNumber << "</sir:seatNumber>\n"
            "            <sir:firstName>" << iv->firstName << "</sir:firstName>\n"
            "            <sir:lastName>" << iv->lastName << "</sir:lastName>\n"
            "            <sir:patronymic>" << iv->patronymic << "</sir:patronymic>\n"
            "            <sir:documentNumber>" << iv->documentNumber << "</sir:documentNumber>\n"
            "            <sir:operationType>" << iv->operationType << "</sir:operationType>\n"
            "            <sir:baggageReceiptsNumber>" << iv->baggageReceiptsNumber << "</sir:baggageReceiptsNumber>\n"
            "            <sir:airline>" << iv->airline << "</sir:airline>\n"
            "            <sir:departureAirport>" << iv->departureAirport << "</sir:departureAirport>\n"
            "            <sir:arrivalAirport>" << iv->arrivalAirport << "</sir:arrivalAirport>\n"
            "            <sir:baggageWeight>" << iv->baggageWeight << "</sir:baggageWeight>\n"
            "            <sir:departureTime>" << (iv->departureDate == NoExists ? "" : DateTimeToStr(iv->departureDate, "yyyymmddhhnnss")) << "</sir:departureTime>\n"
            "            <sir:PNR>" << iv->PNR << "</sir:PNR>\n"
            "            <sir:visaNumber>" << iv->visaNumber << "</sir:visaNumber>\n"
            "            <sir:visaDate>" << (iv->visaDate==NoExists?"":DateTimeToStr(iv->visaDate, "yyyy-mm-dd")) << "</sir:visaDate>\n"
            "            <sir:visaPlace>" << iv->visaPlace << "</sir:visaPlace>\n"
            "            <sir:visaCountryCode>" << iv->visaCountryCode << "</sir:visaCountryCode>\n"
//            "            <sir:nationality>" << iv->nationality << "</sir:nationality>\n"
            "            <sir:sex>" << iv->gender << "</sir:sex>\n"
            "         </sir:policyParameters>\n";
    result <<
        "      </sir:importASTDateRequest>\n"
        "   </soapenv:Body>\n"
        "</soapenv:Envelope>";
    //ProgTrace( TRACE5, "make_soap_content: return %s", result.str().c_str() );
    return ConvertCodepage(result.str(), "CP866", "UTF-8");
}

class TPaxListFilter
{
  public:
    TDateTime first_time_utc, last_time_utc;
    string airline;
    int flt_no;
    string airp_dep;
    TPaxListFilter()
    {
      first_time_utc=NoExists;
      last_time_utc=NoExists;
      flt_no=NoExists;
    };
    virtual ~TPaxListFilter() {};
    virtual bool check_pax(const TPax &pax) const { return true; };
};

class TPaxListTJKFilter : public TPaxListFilter
{
  public:
    virtual bool check_pax(const TPax &pax) const
    {
/*
  1.        Всех, улетающих/прилетающих в аэропорты на территории Таджикистана вне зависимости от их подданства
  2.        Всех подданных Таджикистана, улетающих/прилетающих в аэропорты всего мира, которые проходят через систему Astra .
*/
      if (pax.departureAirport == "ДШБ" || pax.arrivalAirport == "ДШБ") return true;
/*
      string country_dep, country_arv;
      TBaseTable &basecities = base_tables.get( "cities" );
      TBaseTable &baseairps = base_tables.get( "airps" );
      try
      {
        country_dep=((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", pax.departureAirport, true )).city)).country;
      }
      catch(EBaseTableError) {};
      try
      {
        country_arv=((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", pax.departureAirport, true )).city)).country;
      }
      catch(EBaseTableError) {};


      if ( pax.nationality == "TJK" || country_dep == "ТД" || country_arv == "ТД" ) return true;
*/
      return false;
    };
};

void get_pax_list(const TPaxListFilter &filter,
                  bool with_crew,
                  vector<rozysk::TPax> &paxs)
{
  paxs.clear();
  if (filter.first_time_utc==NoExists ||
      filter.last_time_utc==NoExists)
    throw Exception("rozysk::get_pax_list: first_time_utc and last_time_utc not defined");

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT time, term, "
         "       airline, flt_no, suffix, takeoff, airp_dep, "
         "       airp_arv, surname, name, "
         "       seat_no, bag_weight+rk_weight AS bag_weight, tags, pnr, operation, "
         "       doc_no, doc_nationality, doc_gender, "
         "       visa_no, visa_issue_place, visa_issue_date, visa_applic_country "
         "FROM rozysk "
         "WHERE time>=:first_time AND time<:last_time AND pax_id IS NULL ";
  if (!with_crew)
    sql << "      AND reg_no>0 ";
  if (!filter.airline.empty())
  {
    sql << "      AND (airline=:airline) ";
    Qry.CreateVariable("airline", otString, filter.airline);
  };
  if (filter.flt_no!=NoExists)
  {
    sql << "      AND (flt_no=:flt_no) ";
    Qry.CreateVariable("flt_no", otInteger, filter.flt_no);
  };
  if (!filter.airp_dep.empty())
  {
    sql << "      AND (airp_dep=:airp_dep) ";
    Qry.CreateVariable("airp_dep", otString, filter.airp_dep);
  };
  sql << "ORDER BY time";
  Qry.SQLText=sql.str();
  Qry.CreateVariable( "first_time", otDate, filter.first_time_utc );
  Qry.CreateVariable( "last_time", otDate, filter.last_time_utc );
  Qry.Execute();
  if (!Qry.Eof)
  {
    int idx_time = Qry.FieldIndex( "time" );
    int idx_term = Qry.FieldIndex( "term" );

    int idx_airline = Qry.FieldIndex( "airline" );
    int idx_flt_no = Qry.FieldIndex( "flt_no" );
    int idx_suffix = Qry.FieldIndex( "suffix" );
    int idx_takeoff = Qry.FieldIndex( "takeoff" );
    int idx_airp_dep = Qry.FieldIndex( "airp_dep" );

    int idx_airp_arv = Qry.FieldIndex( "airp_arv" );
    int idx_surname = Qry.FieldIndex( "surname" );
    int idx_name = Qry.FieldIndex( "name" );
    int idx_seat_no = Qry.FieldIndex( "seat_no" );
    int idx_bag_weight = Qry.FieldIndex( "bag_weight" );
    int idx_tags = Qry.FieldIndex( "tags" );
    int idx_pnr = Qry.FieldIndex( "pnr" );
    int idx_operation = Qry.FieldIndex( "operation" );

    int idx_doc_no = Qry.FieldIndex( "doc_no" );
    int idx_doc_nationality = Qry.FieldIndex( "doc_nationality" );
    int idx_doc_gender = Qry.FieldIndex( "doc_gender" );
    int idx_visa_no = Qry.FieldIndex( "visa_no" );
    int idx_visa_issue_place = Qry.FieldIndex( "visa_issue_place" );
    int idx_visa_issue_date = Qry.FieldIndex( "visa_issue_date" );
    int idx_visa_applic_country = Qry.FieldIndex( "visa_applic_country" );

    for ( ;!Qry.Eof; Qry.Next() )
    {
      rozysk::TPax pax;
      pax.transactionDate = Qry.FieldAsDateTime( idx_time );
      pax.airline = Qry.FieldAsString( idx_airline );
      pax.flightNumber = Qry.FieldAsInteger( idx_flt_no );
      pax.flightSuffix = Qry.FieldAsString( idx_suffix );
      if ( Qry.FieldIsNULL( idx_takeoff ) )
        pax.departureDate = ASTRA::NoExists;
      else
        pax.departureDate = Qry.FieldAsDateTime( idx_takeoff );
      pax.rackNumber = Qry.FieldAsString( idx_term );
      pax.seatNumber = Qry.FieldAsString( idx_seat_no );
      pax.lastName = Qry.FieldAsString( idx_surname );
      pax.firstName = Qry.FieldAsString( idx_name );
      pax.patronymic = SeparateNames(pax.firstName);
      pax.documentNumber = Qry.FieldAsString( idx_doc_no );
      pax.operationType = Qry.FieldAsString( idx_operation );
      pax.baggageReceiptsNumber = Qry.FieldAsString( idx_tags );
      pax.departureAirport = Qry.FieldAsString( idx_airp_dep );
      pax.arrivalAirport = Qry.FieldAsString( idx_airp_arv );
      pax.baggageWeight = Qry.FieldAsString( idx_bag_weight );
      pax.PNR = Qry.FieldAsString( idx_pnr );
      pax.visaNumber = Qry.FieldAsString( idx_visa_no );
      if ( Qry.FieldIsNULL( idx_visa_issue_date ) )
        pax.visaDate = ASTRA::NoExists;
      else
        pax.visaDate = Qry.FieldAsDateTime( idx_visa_issue_date );
      pax.visaPlace = Qry.FieldAsString( idx_visa_issue_place );
      pax.visaCountryCode = Qry.FieldAsString( idx_visa_applic_country );
      pax.nationality = Qry.FieldAsString( idx_doc_nationality );
      int is_female=CheckIn::is_female(Qry.FieldAsString( idx_doc_gender ),
                                       Qry.FieldAsString( idx_name ));
      pax.gender = (is_female==ASTRA::NoExists?"N":(is_female==0?"M":"F"));

      if (filter.check_pax(pax)) paxs.push_back( pax );;
    }
  };
};

} //namespace rozysk

namespace mintrans
{
  struct TPax {
    //Персональные данные о пассажире
    string surname;            //фамилия
    string name;               //имя
    string patronymic;         //отчество
    TDateTime birthDate;       //дата рождения
    string birthPlace;         //место рождения
    string docType;            //тип документа
    string docNumber;          //номер документа
    string departPlace;        //лат. код а/п вылета
    string arrivePlace;        //лат. код а/п прилета
    int rtType;                //вид маршрута
    TDateTime departDateTime;  //время вылета UTC
    string typePDP;            //тип ПДП (член экипажа, пассажир)
    string gender;             //пол
    string nationalities;      //национальность
    //Данные о регистрируемой операции
    string operationType;      //тип операции
    TDateTime registerTimeIS;  //время операции UTC
    string airlineCode;        //код авиакомпании
    int flightNum;             //номер рейса
    string operSuff;           //суффикс рейса
    string actLoc;             //номер места в салоне
    string pnrId;              //номер PNR
    string ticket;             //номер билета
    string agentRegId;         //агент регистрации
    int baggageCnt;            //кол-во мест багажа
    int baggageHWeight;        //вес р/к
    int baggageWeight;         //вес багажа
    string gateReg;            //стойка регистрации
    int regControlNum;         //рег. номер
    string baggageLabel;       //номера баг. бирок
    //string selfRegID;          //киоск саморегистрации
};

void get_pax_list(int point_id,
                  vector<mintrans::TPax> &paxs)
{
  paxs.clear();
  if (point_id==NoExists)
    throw Exception("mintrans::get_pax_list: point_id not defined");

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT r.time, r.term, "
         "       TRIM(REPLACE(r.user_descr,';',' ')) AS user_descr, "
         "       r.airline, r.flt_no, r.suffix, r.takeoff, r.airp_dep, "
         "       r.airp_arv, r.surname, r.name, "
         "       r.seat_no, r.bag_amount, r.bag_weight, r.rk_weight, r.tags, r.pnr, r.operation, "
         "       r.route_type, r.reg_no, "
         "       r.doc_type, r.doc_issue_country, r.doc_no, r.doc_nationality, r.doc_gender, "
         "       r.doc_birth_date, r.doc_type_rcpt, "
         "       r.visa_birth_place, r.ticket_no "
         "FROM pax_grp, pax, rozysk r "
         "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=r.pax_id AND "
         "      pax_grp.point_dep=:point_id AND "
         "      (r.name IS NULL OR r.name<>'CBBG') "
         "ORDER BY time";
  Qry.SQLText=sql.str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (!Qry.Eof)
  {
    int idx_time = Qry.FieldIndex( "time" );
    int idx_term = Qry.FieldIndex( "term" );
    int idx_user_descr = Qry.FieldIndex( "user_descr" );

    int idx_airline = Qry.FieldIndex( "airline" );
    int idx_flt_no = Qry.FieldIndex( "flt_no" );
    int idx_suffix = Qry.FieldIndex( "suffix" );
    int idx_takeoff = Qry.FieldIndex( "takeoff" );
    int idx_airp_dep = Qry.FieldIndex( "airp_dep" );

    int idx_airp_arv = Qry.FieldIndex( "airp_arv" );
    int idx_surname = Qry.FieldIndex( "surname" );
    int idx_name = Qry.FieldIndex( "name" );
    int idx_seat_no = Qry.FieldIndex( "seat_no" );
    int idx_bag_amount = Qry.FieldIndex( "bag_amount" );
    int idx_bag_weight = Qry.FieldIndex( "bag_weight" );
    int idx_rk_weight = Qry.FieldIndex( "rk_weight" );
    int idx_tags = Qry.FieldIndex( "tags" );
    int idx_pnr = Qry.FieldIndex( "pnr" );
    int idx_operation = Qry.FieldIndex( "operation" );
    int idx_route_type = Qry.FieldIndex( "route_type" );
    int idx_reg_no = Qry.FieldIndex( "reg_no" );

    int idx_doc_type = Qry.FieldIndex( "doc_type" );
    int idx_doc_issue_country = Qry.FieldIndex( "doc_issue_country" );
    int idx_doc_no = Qry.FieldIndex( "doc_no" );
    int idx_doc_nationality = Qry.FieldIndex( "doc_nationality" );
    int idx_doc_gender = Qry.FieldIndex( "doc_gender" );
    int idx_doc_birth_date = Qry.FieldIndex( "doc_birth_date" );
    int idx_doc_type_rcpt = Qry.FieldIndex( "doc_type_rcpt" );

    int idx_visa_birth_place = Qry.FieldIndex( "visa_birth_place" );

    int idx_ticket_no = Qry.FieldIndex( "ticket_no" );

    vector< pair<TElemFmt,string> > fmts;
    getElemFmts(efmtCodeNative, AstraLocale::LANG_EN, fmts);

    for ( ;!Qry.Eof; Qry.Next() )
    {
      mintrans::TPax pax;
      //Персональные данные о пассажире
      pax.surname = Qry.FieldAsString( idx_surname );
      pax.name = Qry.FieldAsString( idx_name );
      pax.name = TruncNameTitles(TrimString(pax.name));
      pax.patronymic = SeparateNames(pax.name);
      if ( Qry.FieldIsNULL( idx_doc_birth_date ) )
        pax.birthDate = ASTRA::NoExists;
      else
        pax.birthDate = Qry.FieldAsDateTime( idx_doc_birth_date );
      pax.birthPlace = Qry.FieldAsString( idx_visa_birth_place );
      if (!Qry.FieldIsNULL( idx_doc_type_rcpt ))
      {
        try
        {
          TRcptDocTypesRow &doc_type_rcpt_row = (TRcptDocTypesRow&)base_tables.get("rcpt_doc_types").get_row("code",Qry.FieldAsString( idx_doc_type_rcpt ));
          pax.docType = doc_type_rcpt_row.code_mintrans;
          if (strcmp(Qry.FieldAsString( idx_doc_issue_country ),"RUS")!=0)
          {
            if (pax.docType=="00") pax.docType="99";
            if (pax.docType=="02")
            {
              if (Qry.FieldIsNULL( idx_doc_issue_country ))
                pax.docType="99";
              else
                pax.docType="03";
            };
          };
        }
        catch(EBaseTableError) {};
      }
      else if (!Qry.FieldIsNULL( idx_doc_type ))
      {
        try
        {
          TPaxDocTypesRow &doc_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",Qry.FieldAsString( idx_doc_type ));
          pax.docType = doc_type_row.code_mintrans;
        }
        catch(EBaseTableError) {};
      };

      pax.docNumber = Qry.FieldAsString( idx_doc_no );
      pax.departPlace = ElemIdToElem(etAirp, Qry.FieldAsString( idx_airp_dep ), fmts);
      if (pax.departPlace.empty()) pax.departPlace = Qry.FieldAsString( idx_airp_dep );
      pax.arrivePlace = ElemIdToElem(etAirp, Qry.FieldAsString( idx_airp_arv ), fmts);
      if (pax.arrivePlace.empty()) pax.arrivePlace = Qry.FieldAsString( idx_airp_arv );
      if ( Qry.FieldIsNULL( idx_route_type ) )
        pax.rtType = ASTRA::NoExists;
      else
        pax.rtType = Qry.FieldAsInteger( idx_route_type );
      if ( Qry.FieldIsNULL( idx_takeoff ) )
        pax.departDateTime = ASTRA::NoExists;
      else
        pax.departDateTime = Qry.FieldAsDateTime( idx_takeoff );
      pax.typePDP = (Qry.FieldIsNULL( idx_reg_no ) || Qry.FieldAsInteger( idx_reg_no )>0)?"1":"0";
      int is_female=CheckIn::is_female(Qry.FieldAsString( idx_doc_gender ),
                                       Qry.FieldAsString( idx_name ));
      pax.gender = (is_female==ASTRA::NoExists?"":(is_female==0?"M":"F"));
      pax.nationalities = Qry.FieldAsString( idx_doc_nationality );
      //Данные о регистрируемой операции
      pax.operationType = Qry.FieldAsString( idx_operation );
      pax.registerTimeIS = Qry.FieldAsDateTime( idx_time );
      pax.airlineCode = ElemIdToElem(etAirline, Qry.FieldAsString( idx_airline ), fmts);
      if (pax.airlineCode.empty()) pax.airlineCode = Qry.FieldAsString( idx_airline );
      pax.flightNum = Qry.FieldAsInteger( idx_flt_no );
      pax.operSuff = ElemIdToElem(etSuffix, Qry.FieldAsString( idx_suffix ), fmts);
      if (pax.operSuff.empty()) pax.operSuff = Qry.FieldAsString( idx_suffix );
      pax.actLoc = Qry.FieldAsString( idx_seat_no );
      pax.pnrId = Qry.FieldAsString( idx_pnr );
      pax.ticket = Qry.FieldAsString( idx_ticket_no );
      pax.agentRegId = Qry.FieldAsString( idx_user_descr );
      if ( Qry.FieldIsNULL( idx_bag_amount ) )
        pax.baggageCnt = ASTRA::NoExists;
      else
        pax.baggageCnt = Qry.FieldAsInteger( idx_bag_amount );
      if ( Qry.FieldIsNULL( idx_rk_weight ) )
        pax.baggageHWeight = ASTRA::NoExists;
      else
        pax.baggageHWeight = Qry.FieldAsInteger( idx_rk_weight );
      if ( Qry.FieldIsNULL( idx_bag_weight ) )
        pax.baggageWeight = ASTRA::NoExists;
      else
        pax.baggageWeight = Qry.FieldAsInteger( idx_bag_weight );
      pax.gateReg = Qry.FieldAsString( idx_term );
      if ( Qry.FieldIsNULL( idx_reg_no ) || Qry.FieldAsInteger( idx_reg_no )<0 )
        pax.regControlNum = ASTRA::NoExists;
      else
        pax.regControlNum = Qry.FieldAsInteger( idx_reg_no );
      pax.baggageLabel = Qry.FieldAsString( idx_tags );
      paxs.push_back( pax );;
    }
  };
};

} //namespace mintrans


void sirena_rozysk_send()
{
    ProgTrace(TRACE5,"sirena_rozysk_send started");
    TFileQueue file_queue;
    file_queue.get( TFilterQueue( OWN_POINT_ADDR(), ROZYSK_SIRENA ) );
    for ( TFileQueue::iterator item=file_queue.begin(); item!=file_queue.end(); item++) {
        if ( item->params.find( PARAM_HTTP_ADDR ) == item->params.end() ||
                item->params[ PARAM_HTTP_ADDR ].empty() )
            throw Exception("http_addr not specified");
        if ( item->params.find( PARAM_ACTION_CODE ) == item->params.end() ||
                item->params[ PARAM_ACTION_CODE ].empty() )
            throw Exception("action_code not specified");
        HTTPRequestInfo request;
        request.resource = item->params[PARAM_HTTP_ADDR];
        request.action = item->params[PARAM_ACTION_CODE];
        request.content = item->data;
        sirena_rozysk_send(request);
        TFileQueue::deleteFile(item->id);
        ProgTrace(TRACE5, "sirena_rozysk_send: id = %d completed", item->id);
    }
}

void sync_sirena_rozysk( TDateTime utcdate )
{
    ProgTrace(TRACE5,"sync_sirena_rozysk started");

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
        "SELECT last_create FROM http_sets WHERE code=:code AND pr_denial=0 FOR UPDATE";
    Qry.CreateVariable("code", otString, ROZYSK_SIRENA);
    Qry.Execute();
    if (Qry.Eof) return;
    TDateTime last_time=NowUTC() - 1.0/1440.0; //отматываем минуту, так как данные последних секунд могут быть еще не закоммичены в rozysk
    TDateTime first_time = Qry.FieldIsNULL("last_create") ? last_time - 10.0/1440.0 :
        Qry.FieldAsDateTime("last_create");

    if (first_time>=last_time) return;

    rozysk::TPaxListTJKFilter filter;
    filter.first_time_utc=first_time;
    filter.last_time_utc=last_time;

    vector<rozysk::TPax> paxs;
    rozysk::get_pax_list(filter, false, paxs);

    Qry.Clear();
    Qry.SQLText =
        "UPDATE http_sets SET last_create=:last_time WHERE code=:code";
    Qry.CreateVariable("code", otString, ROZYSK_SIRENA);
    Qry.CreateVariable("last_time", otDate, last_time);
    Qry.Execute();

    ProgTrace( TRACE5, "pax.size()=%zu", paxs.size() );
    if(not paxs.empty()) {

        TTripInfo flt;
        map<string, string> file_params;
        TFileQueue::add_sets_params( flt.airp,
                flt.airline,
                IntToString(flt.flt_no),
                OWN_POINT_ADDR(),
                ROZYSK_SIRENA,
                1,
                file_params );
        if(not file_params.empty()) {
            TFileQueue::putFile( OWN_POINT_ADDR(),
                    OWN_POINT_ADDR(),
                    ROZYSK_SIRENA,
                    file_params,
                    make_soap_content(paxs));
            ProgTrace(TRACE5, "sync_sirena_rozysk completed");
        }
    }
}

void create_mintrans_file(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline, flt_no, suffix, airp, scd_out, act_out "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  if (Qry.FieldIsNULL("airline") ||
      Qry.FieldIsNULL("flt_no") ||
      Qry.FieldIsNULL("airp")) return;

  bool departure=!Qry.FieldIsNULL("act_out");

  TTripInfo fltInfo(Qry);
  if (!GetTripSets(tsMintransFile, fltInfo)) return;

  map<string, string> fileparams;
  TFileQueue::add_sets_params( Qry.FieldAsString("airp"),
                               Qry.FieldAsString("airline"),
                               Qry.FieldAsString("flt_no"),
                               OWN_POINT_ADDR(),
                               FILE_MINTRANS_TYPE,
                               true,
                               fileparams );

  if (fileparams[PARAM_WORK_DIR].empty()) return;

  string encoding=TFileQueue::getEncoding(FILE_MINTRANS_TYPE, OWN_POINT_ADDR(), true);
  if (encoding.empty()) encoding="UTF-8";

  vector<mintrans::TPax> paxs;
  mintrans::get_pax_list(point_id, paxs);

  if (paxs.empty()) return;

  ostringstream f;
  f  << "surname" << ";"
     << "name" << ";"
     << "patronymic" << ";"
     << "birthDate" << ";"
     << "birthPlace" << ";"
     << "docType" << ";"
     << "docNumber" << ";"
     << "departPlace" << ";"
     << "arrivePlace" << ";"
     << "rtType" << ";"
     << "departDateTime" << ";"
     << "typePDP" << ";"
     << "gender" << ";"
     << "nationalities" << ";"
     << "operationType" << ";"
     << "registerTimeIS" << ";"
     << "airlineCode" << ";"
     << "flightNum" << ";"
     << "operSuff" << ";"
     << "actLoc" << ";"
     << "pnrId" << ";"
     << "ticket" << ";"
     << "agentRegId" << ";"
     << "baggageCnt" << ";"
     << "baggageHWeight" << ";"
     << "baggageWeight" << ";"
     << "gateReg" << ";"
     << "regControlNum" << ";"
     << "baggageLabel"
     << endl;
  for(vector<mintrans::TPax>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    f << p->surname << ";"
      << p->name << ";"
      << p->patronymic << ";"
      << (p->birthDate==NoExists?"":DateTimeToStr(p->birthDate, "dd.mm.yyyy")) << ";"
      << p->birthPlace << ";"
      << p->docType << ";"
      << p->docNumber << ";"
      << p->departPlace << ";"
      << p->arrivePlace << ";"
      << (p->rtType==NoExists?"":IntToString(p->rtType)) << ";"
      << (p->departDateTime==NoExists?"":DateTimeToStr(p->departDateTime, "dd.mm.yyyy hh:nn:ss")) << ";"
      << p->typePDP << ";"
      << p->gender << ";"
      << p->nationalities << ";"
      << (departure && p->operationType=="06"?"14":p->operationType) << ";"
      << (p->registerTimeIS==NoExists?"":DateTimeToStr(p->registerTimeIS, "dd.mm.yyyy hh:nn:ss")) << ";"
      << p->airlineCode << ";"
      << setw(3) << setfill('0') << p->flightNum << ";"
      << p->operSuff << ";"
      << p->actLoc << ";"
      << p->pnrId << ";"
      << p->ticket << ";"
      << p->agentRegId << ";"
      << (p->baggageCnt==NoExists?"":IntToString(p->baggageCnt)) << ";"
      << (p->baggageHWeight==NoExists?"":IntToString(p->baggageHWeight)) << ";"
      << (p->baggageWeight==NoExists?"":IntToString(p->baggageWeight)) << ";"
      << p->gateReg << ";"
      << (p->regControlNum==NoExists?"":IntToString(p->regControlNum)) << ";"
      << p->baggageLabel
      << endl;
  };

  TFileQueue::putFile(OWN_POINT_ADDR(),
                      OWN_POINT_ADDR(),
                      FILE_MINTRANS_TYPE,
                      fileparams,
                     (encoding=="CP866"?f.str():ConvertCodepage(f.str(), "CP866", encoding)));
};

void save_mintrans_files()
{
  ProgTrace(TRACE5,"save_mintrans_files started");

  TDateTime last_time=NowUTC() - 1.0/1440.0; //отматываем минуту, так как данные последних секунд могут быть еще не закоммичены в rozysk

  TFileQueue file_queue;
  file_queue.get( TFilterQueue( OWN_POINT_ADDR(),
                                FILE_MINTRANS_TYPE,
                                last_time ) );
  TDateTime prior_time=NoExists;
  int msec=0;
  try {
    for ( TFileQueue::iterator item=file_queue.begin(); item!=file_queue.end(); item++,OraSession.Commit() ) {
      try {
        map<string,string>::const_iterator work_dir_param=item->params.find( PARAM_WORK_DIR );
        if ( work_dir_param == item->params.end() || work_dir_param->second.empty() )
        {
          TFileQueue::deleteFile(item->id);
          continue;
        }
    
        if (prior_time==NoExists || item->time!=prior_time)
          msec=0;
        else
          msec++;
        prior_time=item->time;

        ostringstream file_name;
        file_name << work_dir_param->second
                  << MINTRANS_ID
                  << DateTimeToStr(item->time, "_yyyy_mm_dd_hh_nn_ss_")
                  << setw(3) << setfill('0') << msec
                  << ".csv";
        ofstream f;
        f.open(file_name.str().c_str());
        if (!f.is_open()) throw Exception("Can't open file '%s'",file_name.str().c_str());
        try
        {
          f << item->data;
          f.close();
          TFileQueue::deleteFile(item->id);
        }
        catch(...)
        {
          try { f.close(); } catch( ... ) { };
          try
          {
            //в случае ошибки запишем пустой файл
            f.open(file_name.str().c_str());
            if (f.is_open()) f.close();
          }
          catch( ... ) { };
          throw;
        };
      }
      catch(Exception &E)
      {
          OraSession.Rollback();
          try
          {
              EOracleError *orae=dynamic_cast<EOracleError*>(&E);
              if (orae!=NULL&&
                      (orae->Code==4061||orae->Code==4068)) continue;
              ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
          }
          catch(...) {};

      }
      catch(std::exception &E)
      {
          OraSession.Rollback();
          ProgError(STDLOG,"std::exception: %s (file id=%d)",E.what(),item->id);
      }
      catch(...)
      {
          OraSession.Rollback();
          ProgError(STDLOG,"Something goes wrong");
      };
    }; //end for
  }
  catch(...)
  {
    throw;
  };
  ProgTrace(TRACE5,"save_mintrans_files finished");
};

void create_file(const string &format,
                 const TDateTime first_time_utc, const TDateTime last_time_utc,
                 const char* airp, const char* tz_region, const char* file_name)
{
  if (!(format==ROZYSK_MAGISTRAL ||
        format==ROZYSK_MAGISTRAL_24)) return;

  rozysk::TPaxListFilter filter;
  filter.first_time_utc=first_time_utc;
  filter.last_time_utc=last_time_utc;
  filter.airp_dep=airp;

  vector<rozysk::TPax> paxs;
  rozysk::get_pax_list(filter, false, paxs);


  ofstream f;
  f.open(file_name);
  if (!f.is_open()) throw Exception("Can't open file '%s'",file_name);
  try
  {
    if (format==ROZYSK_MAGISTRAL ||
        format==ROZYSK_MAGISTRAL_24)
    {
      TDateTime first_time_title=first_time_utc;
      TDateTime last_time_title=last_time_utc;
      if (tz_region!=NULL&&*tz_region!=0)
      {
        first_time_title = UTCToLocal(first_time_utc, tz_region);
        last_time_title = UTCToLocal(last_time_utc, tz_region);
      };

      f << DateTimeToStr(first_time_title,"ddmmyyhhnn") << '-'
        << DateTimeToStr(last_time_title-1.0/1440,"ddmmyyhhnn") << rozysk::endl;
    };
    for(vector<rozysk::TPax>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    {
      TDateTime time_local=UTCToLocal(p->transactionDate, AirpTZRegion(p->departureAirport));

      if (format==ROZYSK_MAGISTRAL ||
          format==ROZYSK_MAGISTRAL_24)
      {
        f << DateTimeToStr(time_local,"dd.mm.yyyy") << '|'
          << DateTimeToStr(time_local,"hh:nn") << '|'
          << setw(3) << setfill('0') << p->flightNumber << p->flightSuffix << '|'
          << DateTimeToStr(p->departureDate,"dd.mm.yyyy") << '|'
          << p->rackNumber.substr(0,6) << '|'
          << p->seatNumber << '|'
          << p->lastName.substr(0,20) << '|'
          << p->firstName.substr(0,20) << '|'
          << p->patronymic.substr(0,20) << '|'
          << p->documentNumber << '|'
          << p->operationType << '|'
          << p->baggageReceiptsNumber << '|'
          << p->airline << '|' << '|'
          << p->departureAirport << '|'
          << p->arrivalAirport << '|'
          << p->baggageWeight << '|' << '|'
          << DateTimeToStr(p->departureDate,"hh:nn") << '|'
          << p->PNR.substr(0,12) << '|' << rozysk::endl;
      };
    };
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //в случае ошибки запишем пустой файл
      f.open(file_name);
      if (f.is_open()) f.close();
    }
    catch( ... ) { };
    throw;
  };
};

void sync_mvd(void)
{
  int Hour,Min,Sec;
  TDateTime now=NowUTC();
  DecodeTime(now,Hour,Min,Sec);
  if (Min%15!=2) return;
  Min-=2;

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE file_sets SET last_create=:now WHERE code=:code AND "
    "                 (airp=:airp OR airp IS NULL AND :airp IS NULL)";
  Qry.DeclareVariable("code",otString);
  Qry.DeclareVariable("airp",otString);
  Qry.DeclareVariable("now",otDate);

  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT name,dir,last_create,airp "
    "FROM file_sets "
    "WHERE code=:code AND pr_denial=0";
  FilesQry.DeclareVariable("code",otString);

  for(int pass=0;pass<=3;pass++)
  {
    string format;
    switch (pass)
    {
      case 0: format=ROZYSK_MAGISTRAL; break;
      case 1: format=ROZYSK_MAGISTRAL_24; break;
      case 2: format=ROZYSK_MINTRANS; break;
      case 3: format=ROZYSK_MINTRANS_24; break;
    };

    modf(now,&now);
    if (format==ROZYSK_MAGISTRAL ||
        format==ROZYSK_MINTRANS)
    {
      //добавляем часы и минуты
      TDateTime now_time;
      EncodeTime(Hour,Min,0,now_time);
      now+=now_time;
    };

    FilesQry.SetVariable("code",format);

    Qry.SetVariable("code",FilesQry.GetVariableAsString("code"));
    Qry.SetVariable("now",now);

    FilesQry.Execute();
    for(;!FilesQry.Eof;FilesQry.Next())
    {
      if (!FilesQry.FieldIsNULL("last_create")&&
         (now-FilesQry.FieldAsDateTime("last_create"))<1.0/1440) continue;

      TDateTime local;
      string tz_region;
      if (!FilesQry.FieldIsNULL("airp"))
      {
        tz_region=AirpTZRegion(FilesQry.FieldAsString("airp"));
        local=UTCToLocal(now,tz_region);
      }
      else
      {
        tz_region.clear();
        local=now;
      };
      ostringstream file_name;
      file_name << FilesQry.FieldAsString("dir")
                << DateTimeToStr(local,FilesQry.FieldAsString("name"));

      if (FilesQry.FieldIsNULL("last_create"))
        create_file(format,
                    now-1,now,
                    FilesQry.FieldAsString("airp"),
                    tz_region.c_str(),
                    file_name.str().c_str());
      else
        create_file(format,
                    FilesQry.FieldAsDateTime("last_create"),now,
                    FilesQry.FieldAsString("airp"),
                    tz_region.c_str(),
                    file_name.str().c_str());

      Qry.SetVariable("airp",FilesQry.FieldAsString("airp"));
      Qry.Execute();
      OraSession.Commit();
    };
  };
};
