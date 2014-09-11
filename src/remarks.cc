#include "remarks.h"
#include <map>
#include "oralib.h"
#include "base_tables.h"
#include "qrys.h"
#include "serverlib/str_utils.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;

TRemCategory getRemCategory( const string &rem_code, const string &rem_text )
{
  static map<string, TRemCategory> rem_cats;
  static bool init=false;
  if (!init)
  {
    TQuery Qry( &OraSession );
    Qry.Clear();
    Qry.SQLText =
      "SELECT rem_code, category FROM rem_cats";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      string category=Qry.FieldAsString("category");
      if (category=="TKN")  rem_cats[Qry.FieldAsString("rem_code")]=remTKN; else
      if (category=="DOC")  rem_cats[Qry.FieldAsString("rem_code")]=remDOC; else
      if (category=="DOCO") rem_cats[Qry.FieldAsString("rem_code")]=remDOCO; else
      if (category=="DOCA") rem_cats[Qry.FieldAsString("rem_code")]=remDOCA; else
      if (category=="FQT")  rem_cats[Qry.FieldAsString("rem_code")]=remFQT; else
      if (category=="ASVC") rem_cats[Qry.FieldAsString("rem_code")]=remASVC;
    };
    init=true;
  };
  if (!rem_code.empty())
  {
    //код ремарки не пустой
    map<string, TRemCategory>::const_iterator iRem=rem_cats.find(rem_code);
    if (iRem!=rem_cats.end()) return iRem->second;
  };
  if (!rem_text.empty())
  {
    for(map<string, TRemCategory>::const_iterator iRem=rem_cats.begin(); iRem!=rem_cats.end(); ++iRem)
    {
      if (rem_text.substr(0,iRem->first.size())==iRem->first) return iRem->second;
    };
  };
  return remUnknown;
};

bool isDisabledRemCategory( TRemCategory cat )
{
  return cat==remTKN || cat==remDOC || cat==remDOCO || cat==remDOCA || cat==remASVC;
};

bool isDisabledRem( const string &rem_code, const string &rem_text )
{
  return isDisabledRemCategory(getRemCategory(rem_code, rem_text));
};

void TRemGrp::Load(TRemEventType rem_set_type, int point_id)
{
    Clear();
    TQuery Qry(&OraSession);
    Qry.SQLText = "select airline from points where point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("TRemGrp::Load: point_id %d not found", point_id);
    Load(rem_set_type, Qry.FieldAsString("airline"));
}

void TRemGrp::Load(TRemEventType rem_set_type, const string &airline)
{
    Clear();
    string event_type;
    switch(rem_set_type) {
        case retBP:
            event_type = "BP";
            break;
        case retALARM_SS:
            event_type = "ALARM_SS";
            break;
        case retPNL_SEL:
            event_type = "PNL_SEL";
            break;
        case retBRD_VIEW:
            event_type = "BRD_VIEW";
            break;
        case retBRD_WARN:
            event_type = "BRD_WARN";
            break;
        case retRPT_SS:
            event_type = "RPT_SS";
            break;
        case retRPT_PM:
            event_type = "RPT_PM";
            break;
        case retCKIN_VIEW:
            event_type = "CKIN_VIEW";
            break;
        case retTYPEB_PSM:
            event_type = "TYPEB_PSM";
            break;
        case retTYPEB_PIL:
            event_type = "TYPEB_PIL";
            break;
        default:
            throw Exception("LoadRemGrp: unknown event type %d", rem_set_type);
    }
    
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT ckin_rem_types.code AS rem_code "
      "FROM ckin_rem_types, "
      "     rem_event_sets airline_rem_event_sets, "
      "     rem_event_sets basic_rem_event_sets "
      "WHERE ckin_rem_types.code=airline_rem_event_sets.rem_code(+) AND "
      "      airline_rem_event_sets.event_type(+)=:event_type AND "
      "      airline_rem_event_sets.airline(+)=:airline AND "
      "      ckin_rem_types.code=basic_rem_event_sets.rem_code(+) AND "
      "      basic_rem_event_sets.event_type(+)=:event_type AND "
      "      basic_rem_event_sets.airline(+) IS NULL AND "
      "      NVL(airline_rem_event_sets.event_value,NVL(basic_rem_event_sets.event_value,0))<>0";
    Qry.CreateVariable("event_type", otString, event_type);
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
      push_back(Qry.FieldAsString("rem_code"));
}

string GetRemarkStr(const TRemGrp &rem_grp, const vector<CheckIn::TPaxRemItem> &rems, const string &term)
{
  string result;
  for(vector<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();++r)
  {
    if (r->code.empty() || !rem_grp.exists(r->code)) continue;
    if (!result.empty()) result+=term;
    result+=r->code;
  };
  return result;
};

string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const string &term)
{
    const char *sql =
        "SELECT TRIM(ticket_rem) AS rem_code, NULL AS rem FROM pax "
        "WHERE pax_id=:pax_id AND ticket_rem IS NOT NULL "
        "UNION "
        "SELECT 'DOCS', NULL FROM pax_doc WHERE pax_id=:pax_id "
        "UNION "
        "SELECT 'DOCO', NULL FROM pax_doco WHERE pax_id=:pax_id "
        "UNION "
        "SELECT 'DOCA', NULL FROM pax_doca WHERE pax_id=:pax_id "
        "UNION "
        "SELECT 'ASVC', NULL FROM pax_asvc WHERE pax_id=:pax_id "
        "UNION "
        "SELECT TRIM(rem_code), NULL FROM pax_rem "
        "WHERE pax_id=:pax_id AND "
        "      rem_code NOT IN (SELECT rem_code FROM rem_cats WHERE category IN ('DOC','DOCO','DOCA','TKN','ASVC'))";

    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
    vector<CheckIn::TPaxRemItem> rems;
    for(;!Qry.get().Eof;Qry.get().Next())
      rems.push_back(CheckIn::TPaxRemItem().fromDB(Qry.get()));
    sort(rems.begin(), rems.end());
    return GetRemarkStr(rem_grp, rems, term);
};

string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, const string &term)
{
  vector<CheckIn::TPaxRemItem> rems;
  LoadCrsPaxRem(pax_id, rems);
  sort(rems.begin(),rems.end());
  return GetRemarkStr(rem_grp, rems, term);
};

namespace CheckIn
{

TPaxRemItem::TPaxRemItem(const std::string &rem_code,
                         const std::string &rem_text)
{
  clear();
  code=rem_code;
  text=rem_text;
  calcPriority();
};

const TPaxRemItem& TPaxRemItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr remNode=NewTextChild(node,"rem");
  NewTextChild(remNode,"rem_code",code);
  NewTextChild(remNode,"rem_text",text);
  return *this;
};

TPaxRemItem& TPaxRemItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  code=NodeAsStringFast("rem_code",node2);
  text=NodeAsStringFast("rem_text",node2);
  calcPriority();
  return *this;
};

const TPaxRemItem& TPaxRemItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rem_code", code);
  Qry.SetVariable("rem", text);
  return *this;
};

TPaxRemItem& TPaxRemItem::fromDB(TQuery &Qry)
{
  clear();
  code=Qry.FieldAsString("rem_code");
  text=Qry.FieldAsString("rem");
  if (Qry.GetFieldIndex("priority")>=0)
  {
    if (!Qry.FieldIsNULL("priority")) priority=Qry.FieldAsInteger("priority");
  }
  else calcPriority();
  return *this;
};

void TPaxRemItem::calcPriority()
{
  priority=ASTRA::NoExists;
  try
  {
    priority=base_tables.get("CKIN_REM_TYPES").get_row("code",code).AsInteger("priority");
  }
  catch (EBaseTableError) {};
};

const TPaxFQTItem& TPaxFQTItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rem_code", rem);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("no", no);
  Qry.SetVariable("extra", extra);
  return *this;
};

TPaxFQTItem& TPaxFQTItem::fromDB(TQuery &Qry)
{
  clear();
  rem=Qry.FieldAsString("rem_code");
  airline=Qry.FieldAsString("airline");
  no=Qry.FieldAsString("no");
  extra=Qry.FieldAsString("extra");
  return *this;
};

const TPaxASVCItem& TPaxASVCItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr remNode=NewTextChild(node, "asvc");
  NewTextChild(remNode, "rfic", RFIC);
  NewTextChild(remNode, "rfisc", RFISC);
  NewTextChild(remNode, "ssr_code", ssr_code, "");
  NewTextChild(remNode, "service_name", service_name);
  NewTextChild(remNode, "emd_type", emd_type);
  NewTextChild(remNode, "emd_no", emd_no);
  NewTextChild(remNode, "emd_coupon", emd_coupon);
  NewTextChild(remNode, "ssr_text", ssr_text, "");
  set<ASTRA::TRcptServiceType> service_types;
  rcpt_service_types(service_types);
  if (!service_types.empty())
  {
    xmlNodePtr serviceNode=NewTextChild(remNode, "rcpt_service_types");
    for(set<ASTRA::TRcptServiceType>::const_iterator t=service_types.begin(); t!=service_types.end(); ++t)
      NewTextChild(serviceNode, "type", *t);
  };
  return *this;
};

const TPaxASVCItem& TPaxASVCItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rfic", RFIC);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("ssr_code", ssr_code);
  Qry.SetVariable("service_name", service_name);
  Qry.SetVariable("emd_type", emd_type);
  Qry.SetVariable("emd_no", emd_no);
  Qry.SetVariable("emd_coupon", emd_coupon);
  return *this;
};

TPaxASVCItem& TPaxASVCItem::fromDB(TQuery &Qry)
{
  clear();
  RFIC=Qry.FieldAsString("rfic");
  RFISC=Qry.FieldAsString("rfisc");
  ssr_code=Qry.FieldAsString("ssr_code");
  service_name=Qry.FieldAsString("service_name");
  emd_type=Qry.FieldAsString("emd_type");
  emd_no=Qry.FieldAsString("emd_no");
  emd_coupon=Qry.FieldAsInteger("emd_coupon");
  return *this;
};

std::string TPaxASVCItem::text(const std::string &rem_status) const
{
  ostringstream s;
  s << "ASVC ";
  if (!rem_status.empty())
    s << rem_status << "1 ";
  s << RFIC << "/"
    << RFISC << "/"
    << ssr_code << "/"
    << service_name << "/"
    << emd_type << "/";
  if (!emd_no.empty())
  {
    s << emd_no;
    if (emd_coupon!=ASTRA::NoExists)
      s << "C" << emd_coupon;
  };
  return s.str();
};

void TPaxASVCItem::rcpt_service_types(set<ASTRA::TRcptServiceType> &service_types) const
{
  service_types.clear();
  if (emd_type!="A") return;
  if (RFIC=="C")
  {
    service_types.insert(ASTRA::rstExcess);
    service_types.insert(ASTRA::rstPaid);
  };
  if (RFIC=="D")
  {
    service_types.insert(ASTRA::rstDeclaredValue);
  };
};

bool LoadPaxRem(int pax_id, bool withFQTcat, vector<TPaxRemItem> &rems)
{
  rems.clear();
  const char* sql=
    "SELECT * FROM pax_rem WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxRemQry(sql, QryParams);
  PaxRemQry.get().Execute();
  for(;!PaxRemQry.get().Eof;PaxRemQry.get().Next())
  {
    TPaxRemItem rem;
    rem.fromDB(PaxRemQry.get());
    TRemCategory cat=getRemCategory(rem.code, rem.text);
    if (cat==remUnknown || (cat==remFQT && withFQTcat))
      rems.push_back(rem);
  };
  return !rems.empty();
};

bool LoadCrsPaxRem(int pax_id, vector<TPaxRemItem> &rems)
{
  rems.clear();
  const char* sql=
    "SELECT * FROM crs_pax_rem WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxRemQry(sql, QryParams);
  PaxRemQry.get().Execute();
  for(;!PaxRemQry.get().Eof;PaxRemQry.get().Next())
    rems.push_back(TPaxRemItem().fromDB(PaxRemQry.get()));
  return !rems.empty();
};

bool LoadPaxFQT(int pax_id, vector<TPaxFQTItem> &fqts)
{
  fqts.clear();
  const char* sql=
    "SELECT * FROM pax_fqt WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxFQTQry(sql, QryParams);
  PaxFQTQry.get().Execute();
  for(;!PaxFQTQry.get().Eof;PaxFQTQry.get().Next())
    fqts.push_back(TPaxFQTItem().fromDB(PaxFQTQry.get()));
  return !fqts.empty();
};

bool LoadPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc, bool from_crs)
{
  asvc.clear();
  const char* sql=
    "SELECT * FROM pax_asvc WHERE pax_id=:pax_id";
  const char* crs_sql=
    "SELECT * FROM crs_pax_asvc "
    "WHERE pax_id=:pax_id AND rem_status='HI' AND "
    "      rfic IS NOT NULL AND "
    "      rfisc IS NOT NULL AND "
    "      service_name IS NOT NULL AND "
    "      emd_type IS NOT NULL AND "
    "      emd_no IS NOT NULL AND "
    "      emd_coupon IS NOT NULL ";

  QParams ASVCQryParams;
  ASVCQryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxASVCQry(from_crs?crs_sql:sql, ASVCQryParams);
  PaxASVCQry.get().Execute();
  if (!PaxASVCQry.get().Eof)
  {
    const char* rem_sql=
      "SELECT rem FROM pax_rem WHERE pax_id=:pax_id AND rem_code=:rem_code";
    const char* crs_rem_sql=
      "SELECT rem FROM crs_pax_rem WHERE pax_id=:pax_id AND rem_code=:rem_code";

    QParams RemQryParams;
    RemQryParams << QParam("pax_id", otInteger, pax_id)
                 << QParam("rem_code", otString);
    TCachedQuery PaxRemQry(from_crs?crs_rem_sql:rem_sql, RemQryParams);

    for(;!PaxASVCQry.get().Eof;PaxASVCQry.get().Next())
    {
      TPaxASVCItem ASVCItem;
      ASVCItem.fromDB(PaxASVCQry.get());
      if (!ASVCItem.ssr_code.empty())
      {
        PaxRemQry.get().SetVariable("rem_code", ASVCItem.ssr_code);
        PaxRemQry.get().Execute();
        if (!PaxRemQry.get().Eof)
        {
          ASVCItem.ssr_text=PaxRemQry.get().FieldAsString("rem");
          PaxRemQry.get().Next();
          if (!PaxRemQry.get().Eof) ASVCItem.ssr_text.clear();
        };
      };
      asvc.push_back(ASVCItem);
    };
  };
  return !asvc.empty();
};

bool LoadPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc)
{
  return LoadPaxASVC(pax_id, asvc, false);
};

bool LoadCrsPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc)
{
  return LoadPaxASVC(pax_id, asvc, true);
};

void SavePaxRem(int pax_id, const vector<TPaxRemItem> &rems)
{
  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="DELETE FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES(:pax_id,:rem,:rem_code)";
  RemQry.DeclareVariable("rem",otString);
  RemQry.DeclareVariable("rem_code",otString);
  for(vector<TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    if (r->text.empty()) continue; //защита от пустой ремарки (иногда может почему то приходить с терминала)
    r->toDB(RemQry);
    RemQry.Execute();
  };
};

void SavePaxFQT(int pax_id, const vector<TPaxFQTItem> &fqts)
{
  TQuery FQTQry(&OraSession);
  FQTQry.Clear();
  FQTQry.SQLText="DELETE FROM pax_fqt WHERE pax_id=:pax_id";
  FQTQry.CreateVariable("pax_id",otInteger,pax_id);
  FQTQry.Execute();

  FQTQry.SQLText=
    "INSERT INTO pax_fqt(pax_id,rem_code,airline,no,extra) "
    "VALUES(:pax_id,:rem_code,:airline,:no,:extra)";
  FQTQry.DeclareVariable("rem_code",otString);
  FQTQry.DeclareVariable("airline",otString);
  FQTQry.DeclareVariable("no",otString);
  FQTQry.DeclareVariable("extra",otString);
  for(vector<TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
  {
    r->toDB(FQTQry);
    FQTQry.Execute();
  };
};

};



