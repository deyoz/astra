#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

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
#include "franchise.h"
#include "flt_settings.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace AstraLocale;

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
    //�������⥫쭮
    TDateTime time;
    string term;
    //३�
    string airline;
    int flt_no;
    string suffix;
    TDateTime takeoff_utc;
    TDateTime takeoff_local;
    string airp_dep;
    //���ᠦ��
    TDateTime landing_utc;
    TDateTime landing_local;
    string airp_arv;
    string surname;
    string name;
    string seat_no;
    int bag_weight;
    int rk_weight;
    string tags;
    string pnr;
    string operation;
    int whole_route_not_rus;
    bool transfer_route;
    int reg_no;
    int pax_id;
    //���㬥��
    CheckIn::TPaxDocItem doc;
    //����
    CheckIn::TPaxDocoItem visa;

    TRow()
    {
      time=NoExists;
      flt_no=NoExists;
      takeoff_utc=NoExists;
      takeoff_local=NoExists;
      landing_utc=NoExists;
      landing_local=NoExists;
      bag_weight=NoExists;
      rk_weight=NoExists;
      whole_route_not_rus=NoExists;
      transfer_route=false;
      reg_no=NoExists;
      pax_id=NoExists;
    };
    TRow& fltFromDB(TQuery &Qry);
    TRow& paxFromDB(TQuery &Qry);
    TRow& setPnr(const TPnrAddrs &pnrs);
    TRow& setDoc(const CheckIn::TPaxDocItem &_doc);
    TRow& setVisa(const CheckIn::TPaxDocoItem &_visa);
    const TRow& toDB(TRowType type, TQuery &Qry, bool check_sql=true) const;
    bool isCrew() const { return !(reg_no==NoExists || reg_no>0); }
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
    // �࠭砩����
    TTripInfo trip_info;
    trip_info.airp = airp_dep;
    trip_info.airline = airline;
    trip_info.flt_no = flt_no;
    trip_info.suffix = suffix;
    trip_info.scd_out = takeoff_local;
    Franchise::TProp franchise_prop;
    if (franchise_prop.get(trip_info, Franchise::TPropType::mintrans, true))
    {
      if (franchise_prop.val == Franchise::pvNo)
      {
        airline = franchise_prop.franchisee.airline;
        flt_no = franchise_prop.franchisee.flt_no;
        suffix = franchise_prop.franchisee.suffix;
      }
      else /*if (franchise_prop.val == Franchise::pvYes)*/
      {
        airline = franchise_prop.oper.airline;
        flt_no = franchise_prop.oper.flt_no;
        suffix = franchise_prop.oper.suffix;
      }
    }
  };
  return *this;
};

TRow& TRow::paxFromDB(TQuery &Qry)
{
  airp_arv=Qry.FieldAsString("airp_arv");
  TDateTime landing=Qry.FieldIsNULL("landing")?NoExists:Qry.FieldAsDateTime("landing");
  bool is_utc=Qry.FieldAsInteger("is_utc")!=0;

  landing_utc=NoExists;
  landing_local=NoExists;
  if (landing!=NoExists)
  {
    if (is_utc)
    {
      landing_utc=landing;
      landing_local=UTCToLocal(landing, AirpTZRegion(airp_arv));
    }
    else
    {
      landing_utc=NoExists;
      landing_local=landing;
    };
  };

  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  seat_no=Qry.FieldAsString("seat_no");
  bag_weight=Qry.FieldIsNULL("bag_weight")?NoExists:Qry.FieldAsInteger("bag_weight");
  rk_weight=Qry.FieldIsNULL("rk_weight")?NoExists:Qry.FieldAsInteger("rk_weight");
  tags=Qry.FieldAsString("tags");
  operation=Qry.FieldAsString("operation");
  reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
  pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
  return *this;
};


TRow& TRow::setPnr(const TPnrAddrs &pnrs)
{
  pnr.clear();
  if (!pnrs.empty()) pnr=pnrs.begin()->addr;
  return *this;
};

TRow& TRow::setDoc(const CheckIn::TPaxDocItem &_doc)
{
  doc=_doc;
  return *this;
};

TRow& TRow::setVisa(const CheckIn::TPaxDocoItem &_visa)
{
  visa.clear();
  if (visa.type=="V") visa=_visa;
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
    "          airline=:airline, "
    "          flt_no=:flt_no, "
    "          suffix=:suffix, "
    "          takeoff=:takeoff, "
    "          landing=:landing, "
    "          airp_dep=:airp_dep, "
    "          airp_arv=:airp_arv, "
    "          surname=:surname, "
    "          name=:name, "
    "          seat_no=:seat_no, "
    "          bag_weight=:bag_weight, "
    "          rk_weight=:rk_weight, "
    "          tags=:tags, "
    "          pnr=:pnr, "
    "          operation=:operation, "
    "          route_type=:route_type, "
    "          route_type2=:route_type2, "
    "          reg_no=:reg_no, "
    "          doc_type=:doc_type, "
    "          doc_issue_country=:doc_issue_country, "
    "          doc_no=:doc_no, "
    "          doc_nationality=:doc_nationality, "
    "          doc_gender=:doc_gender, "
    "          doc_birth_date=:doc_birth_date, "
    "          doc_type_rcpt=:doc_type_rcpt, "
    "          doc_surname=:doc_surname, "
    "          doc_first_name=:doc_first_name, "
    "          doc_second_name=:doc_second_name, "
    "          visa_no=:visa_no, "
    "          visa_issue_place=:visa_issue_place, "
    "          visa_issue_date=:visa_issue_date, "
    "          visa_applic_country=:visa_applic_country "
    "      WHERE pax_id=:pax_id; "
    "      IF SQL%FOUND THEN EXIT; END IF;"
    "    END IF; "
    "    BEGIN "
    "      INSERT INTO rozysk "
    "        (time, term, "
    "         airline, flt_no, suffix, takeoff, landing, airp_dep, "
    "         airp_arv, surname, name, seat_no, bag_weight, rk_weight, "
    "         tags, pnr, operation, route_type, route_type2, reg_no, pax_id, "
    "         doc_type, doc_issue_country, doc_no, doc_nationality, doc_gender, doc_birth_date, doc_type_rcpt, "
    "         doc_surname, doc_first_name, doc_second_name, "
    "         visa_no, visa_issue_place, visa_issue_date, visa_applic_country) "
    "      VALUES "
    "        (:time, :term, "
    "         :airline, :flt_no, :suffix, :takeoff, :landing, :airp_dep, "
    "         :airp_arv, :surname, :name, :seat_no, :bag_weight, :rk_weight, "
    "         :tags, :pnr, :operation, :route_type, :route_type2, :reg_no, :pax_id, "
    "         :doc_type, :doc_issue_country, :doc_no, :doc_nationality, :doc_gender, :doc_birth_date, :doc_type_rcpt, "
    "         :doc_surname, :doc_first_name, :doc_second_name, "
    "         :visa_no, :visa_issue_place, :visa_issue_date, :visa_applic_country); "
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
      //�������⥫쭮
      Qry.DeclareVariable("time", otDate);
      Qry.DeclareVariable("term", otString);
      //३�
      Qry.DeclareVariable("airline", otString);
      Qry.DeclareVariable("flt_no", otInteger);
      Qry.DeclareVariable("suffix", otString);
      Qry.DeclareVariable("takeoff", otDate);
      Qry.DeclareVariable("airp_dep", otString);
      //���ᠦ��
      Qry.DeclareVariable("landing", otDate);
      Qry.DeclareVariable("airp_arv", otString);
      Qry.DeclareVariable("surname", otString);
      Qry.DeclareVariable("name", otString);
      Qry.DeclareVariable("seat_no", otString);
      Qry.DeclareVariable("bag_weight", otInteger);
      Qry.DeclareVariable("rk_weight", otInteger);
      Qry.DeclareVariable("tags", otString);
      Qry.DeclareVariable("pnr", otString);
      Qry.DeclareVariable("operation", otString);
      Qry.DeclareVariable("route_type", otInteger);
      Qry.DeclareVariable("route_type2", otInteger);
      Qry.DeclareVariable("reg_no", otInteger);
      Qry.DeclareVariable("pax_id", otInteger);
      //���㬥��
      Qry.DeclareVariable("doc_type", otString);
      Qry.DeclareVariable("doc_issue_country", otString);
      Qry.DeclareVariable("doc_no", otString);
      Qry.DeclareVariable("doc_nationality", otString);
      Qry.DeclareVariable("doc_gender", otString);
      Qry.DeclareVariable("doc_birth_date", otDate);
      Qry.DeclareVariable("doc_type_rcpt", otString);
      Qry.DeclareVariable("doc_surname", otString);
      Qry.DeclareVariable("doc_first_name", otString);
      Qry.DeclareVariable("doc_second_name", otString);
      //����
      Qry.DeclareVariable("visa_no", otString);
      Qry.DeclareVariable("visa_issue_place", otString);
      Qry.DeclareVariable("visa_issue_date", otDate);
      Qry.DeclareVariable("visa_applic_country", otString);
    };
  };
  //�������⥫쭮
  time!=NoExists?Qry.SetVariable("time", time):
                 Qry.SetVariable("time", FNull);
  Qry.SetVariable("term", term);
  //३�
  Qry.SetVariable("airline", airline);
  flt_no!=NoExists?Qry.SetVariable("flt_no", flt_no):
                   Qry.SetVariable("flt_no", FNull);
  Qry.SetVariable("suffix", suffix);

  if (type==rowMagistral)
  {
    takeoff_local!=NoExists?Qry.SetVariable("takeoff", takeoff_local):
                            Qry.SetVariable("takeoff", FNull);
    landing_local!=NoExists?Qry.SetVariable("landing", landing_local):
                            Qry.SetVariable("landing", FNull);
  };
  if (type==rowMintrans)
  {
    takeoff_utc!=NoExists?Qry.SetVariable("takeoff", takeoff_utc):
                          Qry.SetVariable("takeoff", FNull);
    landing_utc!=NoExists?Qry.SetVariable("landing", landing_utc):
                          Qry.SetVariable("landing", FNull);
  };

  Qry.SetVariable("airp_dep", airp_dep);
  //���ᠦ��
  Qry.SetVariable("airp_arv", airp_arv);
  Qry.SetVariable("surname", surname);
  Qry.SetVariable("name", name);
  Qry.SetVariable("seat_no", seat_no);
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
      Qry.SetVariable("operation", isCrew()?"50":"04");
    else if (operation=="K2")
      Qry.SetVariable("operation", isCrew()?"50":"05");
    else if (operation=="K3")
      Qry.SetVariable("operation", isCrew()?"50":"06");
    else if (operation=="K0")
      Qry.SetVariable("operation", isCrew()?"51":"16");
    else return *this;
  };
  if (transfer_route)
    Qry.SetVariable("route_type", 2);
  else
    Qry.SetVariable("route_type", 0);

  if (whole_route_not_rus==NoExists)
    Qry.SetVariable("route_type2", FNull);
  else
    Qry.SetVariable("route_type2", whole_route_not_rus);

  reg_no!=NoExists?Qry.SetVariable("reg_no", reg_no):
                   Qry.SetVariable("reg_no", FNull);
  pax_id!=NoExists?Qry.SetVariable("pax_id", pax_id):
                   Qry.SetVariable("pax_id", FNull);
  if (type==rowMagistral) Qry.SetVariable("pax_id", FNull);
  //���㬥��
  Qry.SetVariable("doc_type", doc.type);
  Qry.SetVariable("doc_issue_country", doc.issue_country);
  Qry.SetVariable("doc_no", doc.no);
  Qry.SetVariable("doc_nationality", doc.nationality);
  Qry.SetVariable("doc_gender", doc.gender);
  doc.birth_date!=NoExists?Qry.SetVariable("doc_birth_date", doc.birth_date):
                           Qry.SetVariable("doc_birth_date", FNull);
  Qry.SetVariable("doc_type_rcpt", doc.type_rcpt);
  Qry.SetVariable("doc_surname", doc.surname);
  Qry.SetVariable("doc_first_name", doc.first_name);
  Qry.SetVariable("doc_second_name", doc.second_name);

  //����
  Qry.SetVariable("visa_no", visa.no);
  Qry.SetVariable("visa_issue_place", visa.issue_place);
  visa.issue_date!=NoExists?Qry.SetVariable("visa_issue_date", visa.issue_date):
                            Qry.SetVariable("visa_issue_date", FNull);
  Qry.SetVariable("visa_applic_country", visa.applic_country);
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

void get_route_not_rus(int point_dep, int point_arv, TRow &r)
{
  TTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);
  r.whole_route_not_rus=NoExists;
  bool rus_exists=false;
  TTripRoute::const_iterator i=route.begin();
  for(; i!=route.end(); ++i)
  {
    try
    {
      string city = base_tables.get("AIRPS").get_row("code", i->airp).AsString("city");
      if (base_tables.get("CITIES").get_row("code", city).AsString("country") == "��") rus_exists=true;
    }
    catch(const EBaseTableError&) {};
    if (point_arv==i->point_id) break;
  };
  if (i!=route.end())
  {
    //��諨 ������ �㭪�
    r.whole_route_not_rus=rus_exists?0:1;
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
  "  salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
  "  NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
  "  NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight, "
  "  ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num) AS tags, "
  "  DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,DECODE(pax.pr_exam,0,'K1','K2'),'K3'),'K0') AS operation, "
  "  NVL(scd_in,NVL(est_in,act_in)) AS landing, 1 AS is_utc "
  "FROM pax_grp, pax, points "
  "WHERE pax_grp.grp_id=pax.grp_id AND pax_grp.point_arv=points.point_id ";

const char* crs_pax_sql=
  "SELECT "
  "  crs_pnr.point_id, crs_pnr.airp_arv, crs_pnr.pnr_id, "
  "  crs_pax.pax_id, crs_pax.surname, crs_pax.name, NULL AS reg_no, "
  "  salons.get_crs_seat_no(seat_xname,seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
  "  NULL AS bag_weight, NULL AS rk_weight, "
  "  NULL AS tags, "
  "  'K0' AS operation, "
  "  NULL AS landing, 0 AS is_utc "
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
  //�������⥫쭮
  row.time=NowUTC();
  row.term=term;
  //३�
  int point_id=Qry.FieldAsInteger("point_id");
  get_flight(point_id, row);

  bool check_sql=true;
  if (!Qry.Eof)
  {
    get_route_not_rus(point_id, Qry.FieldAsInteger("point_arv"), row);
    get_transfer_route(Qry.FieldAsInteger("grp_id"), row);
    for(;!Qry.Eof;Qry.Next())
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      //���ᠦ��
      check_pax(Qry, pax_id);
      row.paxFromDB(Qry);
      //pnr
      TPnrAddrs pnrs;
      pnrs.getByPaxId(pax_id);
      row.setPnr(pnrs);
      //���㬥��
      CheckIn::TPaxDocItem doc;
      LoadPaxDoc(pax_id, doc);
      row.setDoc(doc);
      //����
      CheckIn::TPaxDocoItem doco;
      LoadPaxDoco(pax_id, doco);
      row.setVisa(doco);

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
  catch(const Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax: %s", e.what());
  }
  catch(const std::exception &e)
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
  catch(const Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax_grp: %s", e.what());
  }
  catch(const std::exception &e)
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
  //�������⥫쭮
  row.time=NowUTC();
  row.term=term;
  //३�
  int point_id=Qry.FieldAsInteger("point_id");
  get_crs_flight(point_id, row);
  //pnr
  TPnrAddrs pnrs;
  pnrs.getByPnrId(Qry.FieldAsInteger("pnr_id"));
  row.setPnr(pnrs);

  bool check_sql=true;
  for(;!Qry.Eof;Qry.Next())
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    //���ᠦ��
    check_pax(Qry, pax_id);
    row.paxFromDB(Qry);
    //���㬥��
    CheckIn::TPaxDocItem doc;
    LoadCrsPaxDoc(pax_id, doc);
    row.setDoc(doc);
    //����
    CheckIn::TPaxDocoItem doco;
    LoadCrsPaxVisa(pax_id, doco);
    row.setVisa(doco);

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
  catch(const Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pax: %s", e.what());
  }
  catch(const std::exception &e)
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
  catch(const Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pnr: %s", e.what());
  }
  catch(const std::exception &e)
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
  1.        ���, 㫥����/�ਫ����� � ��ய���� �� ����ਨ ��������⠭� ��� ����ᨬ��� �� �� �������⢠
  2.        ��� ��������� ��������⠭�, 㫥����/�ਫ����� � ��ய���� �ᥣ� ���, ����� ��室�� �१ ��⥬� Astra .
*/
      if (pax.departureAirport == "���" || pax.arrivalAirport == "���") return true;
/*
      string country_dep, country_arv;
      TBaseTable &basecities = base_tables.get( "cities" );
      TBaseTable &baseairps = base_tables.get( "airps" );
      try
      {
        country_dep=((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", pax.departureAirport, true )).city)).country;
      }
      catch(const EBaseTableError&) {};
      try
      {
        country_arv=((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", pax.departureAirport, true )).city)).country;
      }
      catch(const EBaseTableError&) {};


      if ( pax.nationality == "TJK" || country_dep == "��" || country_arv == "��" ) return true;
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
    //���ᮭ���� ����� � ���ᠦ��
    string surname;            //䠬����
    string name;               //���
    string patronymic;         //����⢮
    TDateTime birthDate;       //��� ஦�����
    string docType;            //⨯ ���㬥��
    string docNumber;          //����� ���㬥��
    string departPlace;        //���. ��� �/� �뫥�
    string arrivePlace;        //���. ��� �/� �ਫ��
    int transfer;              //��� �������
    int overFlight;            //�ਧ��� ��ᯮᠤ�筮�� ३� � ��
    TDateTime departDateTime;  //�६� ��ࠢ����� UTC
    TDateTime arriveDateTime;  //�६� �ਡ��� UTC
    string typePDP;            //⨯ ���ᮭ����� ������ (童� �����, ���ᠦ��)
    string crewRoleCode;       //��� ��⥣�ਨ 童�� �����
    string gender;             //���
    string citizenship;        //��樮���쭮���
    //����� � ॣ�����㥬�� ����樨
    string operationType;      //⨯ ����樨
    TDateTime registerTimeIS;  //�६� ����樨 UTC
    string airlineCode;        //��� ������������
    int flightNum;             //����� ३�
    string operSuff;           //���䨪� ३�
    string actLoc;             //����� ���� � ᠫ���
    string pnrId;              //����� PNR
    //string selfRegID;          //���� ᠬ�ॣ����樨
};

void get_pax_list(int point_id,
                  vector<mintrans::TPax> &paxs)
{
  paxs.clear();
  if (point_id==NoExists)
    throw Exception("mintrans::get_pax_list: point_id not defined");

  OutputLang outputLang(LANG_EN, {OutputLang::OnlyTrueIataCodes});

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT r.time, "
         "       r.airline, r.flt_no, r.suffix, r.takeoff, r.airp_dep, "
         "       r.landing, r.airp_arv, r.surname, r.name, "
         "       r.seat_no, r.pnr, r.operation, "
         "       r.route_type, r.route_type2, r.reg_no, "
         "       r.doc_type, r.doc_issue_country, r.doc_no, r.doc_nationality, r.doc_gender, "
         "       r.doc_birth_date, r.doc_type_rcpt, r.doc_surname, r.doc_first_name, r.doc_second_name "
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

    int idx_airline = Qry.FieldIndex( "airline" );
    int idx_flt_no = Qry.FieldIndex( "flt_no" );
    int idx_suffix = Qry.FieldIndex( "suffix" );
    int idx_takeoff = Qry.FieldIndex( "takeoff" );
    int idx_airp_dep = Qry.FieldIndex( "airp_dep" );

    int idx_landing = Qry.FieldIndex( "landing" );
    int idx_airp_arv = Qry.FieldIndex( "airp_arv" );
    int idx_surname = Qry.FieldIndex( "surname" );
    int idx_name = Qry.FieldIndex( "name" );
    int idx_seat_no = Qry.FieldIndex( "seat_no" );
    int idx_pnr = Qry.FieldIndex( "pnr" );
    int idx_operation = Qry.FieldIndex( "operation" );
    int idx_route_type = Qry.FieldIndex( "route_type" );
    int idx_route_type2 = Qry.FieldIndex( "route_type2" );
    int idx_reg_no = Qry.FieldIndex( "reg_no" );

    int idx_doc_type = Qry.FieldIndex( "doc_type" );
    int idx_doc_issue_country = Qry.FieldIndex( "doc_issue_country" );
    int idx_doc_no = Qry.FieldIndex( "doc_no" );
    int idx_doc_nationality = Qry.FieldIndex( "doc_nationality" );
    int idx_doc_gender = Qry.FieldIndex( "doc_gender" );
    int idx_doc_birth_date = Qry.FieldIndex( "doc_birth_date" );
    int idx_doc_type_rcpt = Qry.FieldIndex( "doc_type_rcpt" );
    int idx_doc_surname = Qry.FieldIndex( "doc_surname" );
    int idx_doc_first_name = Qry.FieldIndex( "doc_first_name" );
    int idx_doc_second_name = Qry.FieldIndex( "doc_second_name" );

    for ( ;!Qry.Eof; Qry.Next() )
    {
      mintrans::TPax pax;
      bool isNotCrew=(Qry.FieldIsNULL( idx_reg_no ) || Qry.FieldAsInteger( idx_reg_no )>0);
      //���ᮭ���� ����� � ���ᠦ��
      pax.surname = Qry.FieldAsString(idx_doc_surname);
      pax.name = Qry.FieldAsString(idx_doc_first_name);
      pax.patronymic = Qry.FieldAsString(idx_doc_second_name);
      if (pax.surname.empty() || pax.name.empty())
      {
        pax.surname = Qry.FieldAsString( idx_surname );
        pax.name = Qry.FieldAsString( idx_name );
        pax.name = TruncNameTitles(TrimString(pax.name));
        pax.patronymic = SeparateNames(pax.name);
      };
      if (pax.patronymic.empty()) pax.patronymic="NA";
      if ( Qry.FieldIsNULL( idx_doc_birth_date ) )
        pax.birthDate = ASTRA::NoExists;
      else
        pax.birthDate = Qry.FieldAsDateTime( idx_doc_birth_date );
      if (!Qry.FieldIsNULL( idx_doc_type_rcpt ))
      {
        try
        {
          const TRcptDocTypesRow &doc_type_rcpt_row = (const TRcptDocTypesRow&)base_tables.get("rcpt_doc_types").get_row("code",Qry.FieldAsString( idx_doc_type_rcpt ));
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
        catch(const EBaseTableError&) {};
      }
      else if (!Qry.FieldIsNULL( idx_doc_type ))
      {
        try
        {
          const TPaxDocTypesRow &doc_type_row = (const TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",Qry.FieldAsString( idx_doc_type ));
          pax.docType = doc_type_row.code_mintrans;
        }
        catch(const EBaseTableError&) {};
      };

      pax.docNumber = Qry.FieldAsString( idx_doc_no );
      pax.departPlace = /*airpToPrefferedCode(*/ Qry.FieldAsString( idx_airp_dep )/*, outputLang)*/;
      if (pax.departPlace.empty()) pax.departPlace = Qry.FieldAsString( idx_airp_dep );
      pax.arrivePlace = /*airpToPrefferedCode(*/ Qry.FieldAsString( idx_airp_arv )/*, outputLang)*/;
      if (pax.arrivePlace.empty()) pax.arrivePlace = Qry.FieldAsString( idx_airp_arv );
      if ( Qry.FieldIsNULL( idx_route_type ) )
        pax.transfer = ASTRA::NoExists;
      else
        pax.transfer = Qry.FieldAsInteger( idx_route_type );
      if ( Qry.FieldIsNULL( idx_route_type2 ) )
        pax.overFlight = ASTRA::NoExists;
      else
        pax.overFlight = Qry.FieldAsInteger( idx_route_type2 );
      if ( Qry.FieldIsNULL( idx_takeoff ) )
        pax.departDateTime = ASTRA::NoExists;
      else
        pax.departDateTime = Qry.FieldAsDateTime( idx_takeoff );
      if ( Qry.FieldIsNULL( idx_landing ) )
        pax.arriveDateTime = ASTRA::NoExists;
      else
        pax.arriveDateTime = Qry.FieldAsDateTime( idx_landing );
      pax.typePDP = isNotCrew?"1":"0";
      pax.crewRoleCode = isNotCrew?"":"1"; //�ᥣ�� 1, ⠪ ��� �� �࠭��, ��� 童� ����� ��室����
      int is_female=CheckIn::is_female(Qry.FieldAsString( idx_doc_gender ),
                                       Qry.FieldAsString( idx_name ));
      pax.gender = (is_female==ASTRA::NoExists?"U":(is_female==0?"M":"F"));
      pax.citizenship = Qry.FieldAsString( idx_doc_nationality );
      //����� � ॣ�����㥬�� ����樨
      pax.operationType = Qry.FieldAsString( idx_operation );
      pax.registerTimeIS = Qry.FieldAsDateTime( idx_time );
      pax.airlineCode = airlineToPrefferedCode( Qry.FieldAsString( idx_airline ), outputLang);
      if (pax.airlineCode.empty()) pax.airlineCode = Qry.FieldAsString( idx_airline );
      pax.flightNum = Qry.FieldAsInteger( idx_flt_no );
      pax.operSuff = ElemIdToPrefferedElem(etSuffix, Qry.FieldAsString( idx_suffix ), efmtCodeNative, LANG_EN);
      if (pax.operSuff.empty()) pax.operSuff = Qry.FieldAsString( idx_suffix );
      pax.actLoc = Qry.FieldAsString( idx_seat_no );
      pax.pnrId = Qry.FieldAsString( idx_pnr );
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
    TDateTime last_time=NowUTC() - 1.0/1440.0; //�⬠�뢠�� ������, ⠪ ��� ����� ��᫥���� ᥪ㭤 ����� ���� �� �� �������祭� � rozysk
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
     << "docType" << ";"
     << "docNumber" << ";"
     << "departPlace" << ";"
     << "arrivePlace" << ";"
     << "transfer" << ";"
     << "overFlight" << ";"
     << "departDateTime" << ";"
     << "arriveDateTime" << ";"
     << "typePDP" << ";"
     << "crewRoleCode" << ";"
     << "gender" << ";"
     << "citizenship" << ";"
     << "operationType" << ";"
     << "registerTimeIS" << ";"
     << "airlineCode" << ";"
     << "flightNum" << ";"
     << "operSuff" << ";"
     << "actLoc" << ";"
     << "pnrId"
     << endl;
  for(vector<mintrans::TPax>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    f << p->surname << ";"
      << p->name << ";"
      << p->patronymic << ";"
      << (p->birthDate==NoExists?"":DateTimeToStr(p->birthDate, "dd.mm.yyyy")) << ";"
      << p->docType << ";"
      << p->docNumber << ";"
      << p->departPlace << ";"
      << p->arrivePlace << ";"
      << (p->transfer==NoExists?"":IntToString(p->transfer)) << ";"
      << (p->overFlight==NoExists?"":IntToString(p->overFlight)) << ";"
      << (p->departDateTime==NoExists?"":DateTimeToStr(p->departDateTime, "yyyy-mm-ddThh:nnZ")) << ";"
      << (p->arriveDateTime==NoExists?"":DateTimeToStr(p->arriveDateTime, "yyyy-mm-ddThh:nnZ")) << ";"
      << p->typePDP << ";"
      << p->crewRoleCode << ";"
      << p->gender << ";"
      << p->citizenship << ";"
      << (departure && p->operationType=="06"?"14":p->operationType) << ";"
      << (p->registerTimeIS==NoExists?"":DateTimeToStr(p->registerTimeIS, "yyyy-mm-ddThh:nnZ")) << ";"
      << p->airlineCode << ";"
      << setw(3) << setfill('0') << p->flightNum << ";"
      << p->operSuff << ";"
      << p->actLoc << ";"
      << p->pnrId
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

  TDateTime last_time=NowUTC() - 1.0/1440.0; //�⬠�뢠�� ������, ⠪ ��� ����� ��᫥���� ᥪ㭤 ����� ���� �� �� �������祭� � rozysk

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
            //� ��砥 �訡�� ����襬 ���⮩ 䠩�
            f.open(file_name.str().c_str());
            if (f.is_open()) f.close();
          }
          catch( ... ) { };
          throw;
        };
      }
      catch(const Exception &E)
      {
          OraSession.Rollback();
          try
          {
              if (isIgnoredEOracleError(E)) continue;
              ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
          }
          catch(...) {};

      }
      catch(const std::exception &E)
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
      //� ��砥 �訡�� ����襬 ���⮩ 䠩�
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
      //������塞 ��� � ������
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
