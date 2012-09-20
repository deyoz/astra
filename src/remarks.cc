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
    //��� ६�ન �� ���⮩
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
      "SELECT NVL(airline_rem_event_sets.rem_code,basic_rem_event_sets.rem_code) AS rem_code, "
      "       NVL(airline_rem_event_sets.event_value,basic_rem_event_sets.event_value) AS event_value "
      "FROM "
      "  (SELECT * FROM rem_event_sets WHERE event_type=:event_type AND airline=:airline) airline_rem_event_sets "
      "  FULL OUTER JOIN "
      "  (SELECT * FROM rem_event_sets WHERE event_type=:event_type AND airline IS NULL) basic_rem_event_sets "
      "ON basic_rem_event_sets.event_type=airline_rem_event_sets.event_type AND "
      "   basic_rem_event_sets.rem_code=airline_rem_event_sets.rem_code ";
    Qry.CreateVariable("event_type", otString, event_type);
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      if (Qry.FieldAsInteger("event_value")!=0)
        push_back(Qry.FieldAsString("rem_code"));
    };
}

string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, TQuery &Qry, const string &term)
{
    const char *sql =
        "SELECT rem_code "
        "FROM ckin_rem_types, rem_grp, "
        "     (SELECT ticket_rem AS rem_code FROM pax  "
        "      WHERE pax_id=:pax_id AND ticket_rem IS NOT NULL "
        "      UNION "
        "      SELECT 'DOCS' FROM pax_doc WHERE pax_id=:pax_id "
        "      UNION "
        "      SELECT 'DOCO' FROM pax_doco WHERE pax_id=:pax_id "
        "      UNION "
        "      SELECT rem_code FROM pax_rem  "
        "      WHERE pax_id=:pax_id AND  "
        "            rem_code NOT IN (SELECT rem_code FROM rem_cats WHERE category IN ('DOC','DOCO','TKN'))) pax_rem "
        "WHERE pax_rem.rem_code=ckin_rem_types.code(+) AND ckin_rem_types.grp_id=rem_grp.id(+) "
        "ORDER BY rem_grp.priority NULLS LAST, rem_code ";
    if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
    {
        Qry.Clear();
        Qry.SQLText=sql;
        Qry.DeclareVariable("pax_id",otInteger);
    };
    Qry.SetVariable("pax_id", pax_id);
    Qry.Execute();
    string result;
    for(; not Qry.Eof; Qry.Next()) {
        string rem = StrUtils::rtrim(Qry.FieldAsString("rem_code"));
        if(rem.empty() or not rem_grp.exists(rem)) continue;
        if(not result.empty())
            result += term;
        result += rem;
    }
    return result;
}

namespace CheckIn
{

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

TPaxFQTItem& TPaxFQTItem::fromDB(TQuery &Qry)
{
  clear();
  rem=Qry.FieldAsString("rem_code");
  airline=Qry.FieldAsString("airline");
  no=Qry.FieldAsString("no");
  extra=Qry.FieldAsString("extra");
  return *this;
};

bool LoadPaxRem(int pax_id, vector<TPaxRemItem> &rems, TQuery& PaxRemQry)
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
    if (getRemCategory(rem.code, rem.text)==remUnknown)
      rems.push_back(rem);
  };
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

};



