#include "remarks.h"
#include <map>
#include "oralib.h"
#include "base_tables.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;

TRemCategory getRemCategory( const string &rem_code )
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
  map<string, TRemCategory>::const_iterator iRem=rem_cats.find(rem_code);
  if (iRem!=rem_cats.end()) return iRem->second;
  return remUnknown;
};

bool isDisabledRemCategory( TRemCategory cat )
{
  return cat==remTKN || cat==remDOC || cat==remDOCO;
};

bool isDisabledRem( const string &rem_code )
{
  return isDisabledRemCategory(getRemCategory(rem_code));
};

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
    priority=base_tables.get("REM_TYPES").get_row("code",code).AsInteger("priority");
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
    if (getRemCategory(rem.fromDB(PaxRemQry).code)==remUnknown)
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



