#include "remarks.h"
#include <map>
#include "oralib.h"
#include "base_tables.h"
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
      if (category=="FQT")  rem_cats[Qry.FieldAsString("rem_code")]=remFQT;
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
  return cat==remTKN || cat==remDOC || cat==remDOCO;
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

string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, TQuery &Qry, const string &term)
{
    const char *sql =
        "SELECT TRIM(ticket_rem) AS rem_code, NULL AS rem FROM pax "
        "WHERE pax_id=:pax_id AND ticket_rem IS NOT NULL "
        "UNION "
        "SELECT 'DOCS', NULL FROM pax_doc WHERE pax_id=:pax_id "
        "UNION "
        "SELECT 'DOCO', NULL FROM pax_doco WHERE pax_id=:pax_id "
        "UNION "
        "SELECT TRIM(rem_code), NULL FROM pax_rem "
        "WHERE pax_id=:pax_id AND "
        "      rem_code NOT IN (SELECT rem_code FROM rem_cats WHERE category IN ('DOC','DOCO','TKN'))";
    if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
    {
        Qry.Clear();
        Qry.SQLText=sql;
        Qry.DeclareVariable("pax_id",otInteger);
    };
    Qry.SetVariable("pax_id", pax_id);
    Qry.Execute();
    vector<CheckIn::TPaxRemItem> rems;
    for(;!Qry.Eof;Qry.Next())
      rems.push_back(CheckIn::TPaxRemItem().fromDB(Qry));
    sort(rems.begin(), rems.end());
    return GetRemarkStr(rem_grp, rems, term);
};

string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, TQuery &Qry, const string &term)
{
  vector<CheckIn::TPaxRemItem> rems;
  LoadCrsPaxRem(pax_id, rems, Qry);
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

bool LoadPaxRem(int pax_id, bool withFQTcat, vector<TPaxRemItem> &rems, TQuery& PaxRemQry)
{
  rems.clear();
  const char* sql=
    "SELECT * FROM pax_rem WHERE pax_id=:pax_id";
  if (strcmp(PaxRemQry.SQLText.SQLText(),sql)!=0)
  {
    PaxRemQry.Clear();
    PaxRemQry.SQLText=sql;
    PaxRemQry.DeclareVariable("pax_id",otInteger);
  };
  PaxRemQry.SetVariable("pax_id",pax_id);
  PaxRemQry.Execute();
  for(;!PaxRemQry.Eof;PaxRemQry.Next())
  {
    TPaxRemItem rem;
    rem.fromDB(PaxRemQry);
    TRemCategory cat=getRemCategory(rem.code, rem.text);
    if (cat==remUnknown || (cat==remFQT && withFQTcat))
      rems.push_back(rem);
  };
  return !rems.empty();
};

bool LoadCrsPaxRem(int pax_id, vector<TPaxRemItem> &rems, TQuery& PaxRemQry)
{
  rems.clear();
  const char* sql=
    "SELECT * FROM crs_pax_rem WHERE pax_id=:pax_id";
  if (strcmp(PaxRemQry.SQLText.SQLText(),sql)!=0)
  {
    PaxRemQry.Clear();
    PaxRemQry.SQLText=sql;
    PaxRemQry.DeclareVariable("pax_id",otInteger);
  };
  PaxRemQry.SetVariable("pax_id",pax_id);
  PaxRemQry.Execute();
  for(;!PaxRemQry.Eof;PaxRemQry.Next())
    rems.push_back(TPaxRemItem().fromDB(PaxRemQry));
  return !rems.empty();
};

bool LoadPaxFQT(int pax_id, vector<TPaxFQTItem> &fqts, TQuery& PaxFQTQry)
{
  fqts.clear();
  const char* sql=
    "SELECT * FROM pax_fqt WHERE pax_id=:pax_id";
  if (strcmp(PaxFQTQry.SQLText.SQLText(),sql)!=0)
  {
    PaxFQTQry.Clear();
    PaxFQTQry.SQLText=sql;
    PaxFQTQry.DeclareVariable("pax_id",otInteger);
  };
  PaxFQTQry.SetVariable("pax_id",pax_id);
  PaxFQTQry.Execute();
  for(;!PaxFQTQry.Eof;PaxFQTQry.Next())
    fqts.push_back(TPaxFQTItem().fromDB(PaxFQTQry));
  return !fqts.empty();
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



