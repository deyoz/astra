#include "remarks.h"
#include <map>
#include "oralib.h"
#include "base_tables.h"
#include "qrys.h"
#include "serverlib/str_utils.h"
#include "astra_utils.h"
#include "term_version.h"
#include "date_time.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

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
      if (category=="ASVC") rem_cats[Qry.FieldAsString("rem_code")]=remASVC; else
      if (category=="CREW") rem_cats[Qry.FieldAsString("rem_code")]=remCREW;
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
  return cat==remTKN || cat==remDOC || cat==remDOCO || cat==remDOCA || cat==remASVC || cat==remFQT;
};

bool isDisabledRem( const string &rem_code, const string &rem_text)
{
  TRemCategory cat=getRemCategory(rem_code, rem_text);
  return isDisabledRemCategory(cat);
};

const char *ReadonlyRemCodes[]=
    {"TKNO","TKNA","TKNE","TKNM","TKN","TKT","TKTN","TKTNO","TTKNR","TTKNO","PSPT",
     "EXST","RQST","SEAT","NSST","NSSA","NSSB","NSSW","SMST","SMSA","SMSB","SMSW",
     "CHD","CHLD","INF","INFT","DOCS","DOCO","DOCA","ASVC",
     "ACLS","BCLS","CCLS","DCLS","ECLS","FCLS","GCLS","HCLS","ICLS","JCLS","KCLS","LCLS",
     "MCLS","NCLS","OCLS","PCLS","QCLS","RCLS","SCLS","TCLS","UCLS","VCLS","WCLS","XCLS",
     "YCLS","ZCLS",
     "PRSA"};

bool IsReadonlyRem( const string &rem_code, const string &rem_text )
{
  static set<string> rems;
  if (rems.empty())
  {
    int i=sizeof(ReadonlyRemCodes)/sizeof(ReadonlyRemCodes[0])-1;
    for(;i>=0;i--) rems.insert(ReadonlyRemCodes[i]);
  };

  if (!rem_code.empty()) return rems.find(rem_code)!=rems.end();

  if (!rem_text.empty())
  {
    for(set<string>::const_iterator iRem=rems.begin(); iRem!=rems.end(); ++iRem)
    {
      if (rem_text.substr(0,iRem->size())==*iRem) return true;
    };
  };

  return false;
}

static bool isAPPSRem( const std::string& rem )
{
  return ( rem == "RSIA" || rem == "SPIA" || rem == "SBIA" || rem == "SXIA" ||
           rem == "ATH" || rem == "GTH" || rem == "AAE" || rem == "GAE" );
}

void TRemGrp::Load(TRemEventType rem_set_type, int point_id)
{
    clear();
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
    clear();
    string event_type;
    switch(rem_set_type)
    {
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
      case retSERVICE_STAT:
        event_type = "SERVICE_STAT";
        break;
      case retLIMITED_CAPAB_STAT:
        event_type = "LIMITED_CAPAB_STAT";
        break;
      case retWEB:
        event_type = "WEB";
        break;
      case retKIOSK:
        event_type = "KIOSK";
        break;
      case retMOB:
        event_type = "MOB";
        break;
      case retTYPEB_LCI:
        event_type = "TYPEB_LCI";
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
      insert(Qry.FieldAsString("rem_code"));
}

CheckIn::TPaxRemItem getAPPSRem(const int pax_id, const std::string &lang )
{
  CheckIn::TPaxRemItem rem;
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT * FROM "
                "(SELECT status FROM apps_pax_data WHERE pax_id = :pax_id ORDER BY send_time DESC) "
                "WHERE rownum = 1";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();

  if( Qry.Eof )
    return rem;

  string status = Qry.FieldAsString("status");
  if( status == "B" )
    rem.code = "SBIA";
  else if( status == "P" )
    rem.code = "SPIA";
  else if( status == "X" )
    rem.code = "SXIA";
  if ( !rem.code.empty() )
    rem.text=ElemIdToPrefferedElem(etCkinRemType, rem.code, efmtNameLong, lang);
  return rem;
}

string GetRemarkStr(const TRemGrp &rem_grp, const multiset<CheckIn::TPaxRemItem> &rems, const string &term)
{
  string result;
  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();++r)
  {
    if (r->code.empty() || !rem_grp.exists(r->code)) continue;
    if (!result.empty()) result+=term;
    if ( r->code == "SBIA" || r->code == "SPIA" || r->code == "SXIA")
      result+=r->text;
    else
      result+=r->code;
  };
  return result;
};

string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const string &lang, const string &term)
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
    multiset<CheckIn::TPaxRemItem> rems;
    for(;!Qry.get().Eof;Qry.get().Next())
      rems.insert(CheckIn::TPaxRemItem().fromDB(Qry.get()));
    CheckIn::TPaxRemItem apps_satus_rem = getAPPSRem( pax_id, lang );
    if ( !apps_satus_rem.empty() )
     rems.insert( apps_satus_rem );
    return GetRemarkStr(rem_grp, rems, term);
};

string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, const string &term)
{
  multiset<CheckIn::TPaxRemItem> rems;
  LoadCrsPaxRem(pax_id, rems);
  return GetRemarkStr(rem_grp, rems, term);
};

namespace CheckIn
{

std::string TPaxRemBasic::RemoveTrailingChars(const std::string &str, const std::string &chars) const
{
  size_t pos=str.find_last_not_of(chars);
  if (pos!=string::npos)
    return str.substr(0, pos+1);
  else
    return "";
}

std::string TPaxRemBasic::rem_text(bool inf_indicator, const std::string& lang, TLangApplying fmt) const
{
  if (empty()) return "";
  bool strictly_lat=lang!=AstraLocale::LANG_RU && fmt==applyLangForAll;
  bool translit_lat=lang!=AstraLocale::LANG_RU && (fmt==applyLangForAll || fmt==applyLangForTranslit);
  bool language_lat=lang!=AstraLocale::LANG_RU && (fmt==applyLangForAll || fmt==applyLangForTranslit || fmt==applyLang);
  return get_rem_text(inf_indicator, lang, strictly_lat, translit_lat, language_lat);
}

std::string TPaxRemBasic::rem_text(bool inf_indicator) const
{
  return rem_text(inf_indicator, AstraLocale::LANG_RU, applyLang);
}

int TPaxRemBasic::get_priority() const
{
  int priority=ASTRA::NoExists;
  try
  {
    priority=base_tables.get("CKIN_REM_TYPES").get_row("code",rem_code()).AsInteger("priority");
  }
  catch (EBaseTableError) {};
  return priority;
}

TPaxRemItem::TPaxRemItem(const std::string &rem_code,
                         const std::string &rem_text)
{
  clear();
  code=rem_code;
  text=rem_text;
  calcPriority();
}

TPaxRemItem::TPaxRemItem(const TPaxRemBasic &basic,
                         bool inf_indicator, const std::string &lang, TLangApplying fmt)
{
  clear();
  code=basic.rem_code();
  text=basic.rem_text(inf_indicator, lang, fmt);
  calcPriority();
}

TPaxRemItem::TPaxRemItem(const TPaxRemBasic &basic,
                         bool inf_indicator)
{
  clear();
  code=basic.rem_code();
  text=basic.rem_text(inf_indicator);
  calcPriority();
}

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
  priority=get_priority();
}

std::string TPaxRemItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat) const
{
  if (!code.empty() && text.substr(0,code.size())==code)
  {
    ostringstream result;
    result << ElemIdToPrefferedElem(etCkinRemType, code, efmtCodeNative, lang);
    if (!result.str().empty())
    {
      result << transliter(text.substr(code.size()), 1, translit_lat);
      return result.str();
    }
  }

  return transliter(text, 1, translit_lat);
}

const TPaxFQTItem& TPaxFQTItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rem_code", rem);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("no", no);
  Qry.SetVariable("extra", extra);
  Qry.SetVariable("tier_level", tier_level);
  Qry.SetVariable("tier_level_confirm", (int)tier_level_confirm);
  return *this;
};

TPaxFQTItem& TPaxFQTItem::fromDB(TQuery &Qry)
{
  clear();
  rem=Qry.FieldAsString("rem_code");
  airline=Qry.FieldAsString("airline");
  no=Qry.FieldAsString("no");
  extra=Qry.FieldAsString("extra");
  tier_level=Qry.FieldAsString("tier_level");
  tier_level_confirm=!Qry.FieldIsNULL("tier_level_confirm") &&
                     Qry.FieldAsInteger("tier_level_confirm")!=0;
  return *this;
};

const TPaxFQTItem& TPaxFQTItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr remNode=NewTextChild(node,"fqt_rem");
  NewTextChild(remNode, "rem_code", rem);
  NewTextChild(remNode, "airline", airline);
  NewTextChild(remNode, "no", no);
  NewTextChild(remNode, "extra", extra, "");
  NewTextChild(remNode, "tier_level", tier_level, "");
  NewTextChild(remNode, "tier_level_confirm", (int)tier_level_confirm, (int)false);
  return *this;
};

TPaxFQTItem& TPaxFQTItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  rem=NodeAsStringFast("rem_code", node2);
  airline=NodeAsStringFast("airline", node2);
  no=NodeAsStringFast("no", node2);
  extra=NodeAsStringFast("extra", node2, "");
  tier_level=NodeAsStringFast("tier_level", node2, "");
  tier_level_confirm=NodeAsIntegerFast("tier_level_confirm", node2, (int)false)!=0;
  return *this;
};

std::string TPaxFQTItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem, efmtCodeNative, lang)
         << " " << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative, lang)
         << " " << (strictly_lat?transliter(convert_char_view(no, strictly_lat), 1, strictly_lat):no);
  if (!extra.empty())
    result << "/" << transliter(extra, 1, translit_lat);

  return RemoveTrailingChars(result.str(), " ");
}

std::string TPaxFQTItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative, lang)
         << "/" << no;
  if (!extra.empty())
    result << "/" << extra;
  return result.str();
}

std::string TPaxFQTCard::no_str(const std::string &lang) const
{
  ostringstream s;
  s << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative, lang) << "/" << no;
  return s.str();
}

const TPaxASVCItem& TPaxASVCItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr remNode=NewTextChild(node, "asvc");
  NewTextChild(remNode, "rfic", RFIC);
  NewTextChild(remNode, "rfisc", RFISC);
  NewTextChild(remNode, "service_quantity", service_quantity, 1);
  NewTextChild(remNode, "ssr_code", ssr_code, "");
  NewTextChild(remNode, "service_name", service_name);
  NewTextChild(remNode, "emd_type", emd_type);
  NewTextChild(remNode, "emd_no", emd_no);
  NewTextChild(remNode, "emd_coupon", emd_coupon);
  NewTextChild(remNode, "ssr_text", ssr_text, "");
  NewTextChild(remNode, "paid_weight", ASTRA::NoExists, ASTRA::NoExists); //!!! докрутить
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
  if (service_quantity!=ASTRA::NoExists)
    Qry.SetVariable("service_quantity", service_quantity);
  else
    Qry.SetVariable("service_quantity", FNull); //!!! потом удалить
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
  if (!Qry.FieldIsNULL("service_quantity"))
    service_quantity=Qry.FieldAsInteger("service_quantity");
  else
    service_quantity=1;  //!!! потом удалить
  ssr_code=Qry.FieldAsString("ssr_code");
  service_name=Qry.FieldAsString("service_name");
  emd_type=Qry.FieldAsString("emd_type");
  emd_no=Qry.FieldAsString("emd_no");
  emd_coupon=Qry.FieldAsInteger("emd_coupon");
  return *this;
};

std::string TPaxASVCItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HI" << service_quantity
         << " " << RFIC
         << "/" << RFISC
         << "/" << (strictly_lat && !IsAscii7(ssr_code)?"":ssr_code)
         << "/" << transliter(service_name, 1, translit_lat)
         << "/" << emd_type
         << "/" << (strictly_lat?transliter(convert_char_view(emd_no, strictly_lat), 1, strictly_lat):emd_no);
  if (emd_coupon!=ASTRA::NoExists)
    result << "C" << emd_coupon;

  return result.str();
}

std::string TPaxASVCItem::no_str() const
{
  ostringstream s;
  s << emd_no;
  if (emd_coupon!=ASTRA::NoExists)
    s << "/" << emd_coupon;
  else
    s << "/?";
  return s.str();
};

void TPaxASVCItem::rcpt_service_types(set<ASTRA::TRcptServiceType> &service_types) const
{
  service_types.clear();
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

bool LoadPaxRem(int pax_id, std::multiset<TPaxRemItem> &rems)
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
    if (isDisabledRemCategory(cat)) continue;
    rems.insert(rem);
  };

  return !rems.empty();
};

bool LoadCrsPaxRem(int pax_id, multiset<TPaxRemItem> &rems)
{
  rems.clear();
  const char* sql=
    "SELECT * FROM crs_pax_rem WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxRemQry(sql, QryParams);
  PaxRemQry.get().Execute();
  for(;!PaxRemQry.get().Eof;PaxRemQry.get().Next())
    rems.insert(TPaxRemItem().fromDB(PaxRemQry.get()));
  return !rems.empty();
};

bool LoadCrsPaxFQT(int pax_id, std::set<TPaxFQTItem> &fqts)
{
  fqts.clear();
  const char* sql=
    "SELECT crs_pax_fqt.*, DECODE(tier_level, NULL, NULL, 1) AS tier_level_confirm "
    "FROM crs_pax_fqt WHERE pax_id=:pax_id";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxFQTQry(sql, QryParams);
  PaxFQTQry.get().Execute();
  for(;!PaxFQTQry.get().Eof;PaxFQTQry.get().Next())
    fqts.insert(TPaxFQTItem().fromDB(PaxFQTQry.get()));
  return !fqts.empty();
};

bool LoadPaxFQT(int pax_id, std::set<TPaxFQTItem> &fqts)
{
  fqts.clear();
  const char* sql=
    "SELECT * FROM pax_fqt WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxFQTQry(sql, QryParams);
  PaxFQTQry.get().Execute();
  for(;!PaxFQTQry.get().Eof;PaxFQTQry.get().Next())
    fqts.insert(TPaxFQTItem().fromDB(PaxFQTQry.get()));
  return !fqts.empty();
};

bool SyncPaxASVC(int id, bool is_grp_id)
{
  ostringstream sql;
  sql <<
    "INSERT INTO pax_asvc(pax_id, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon) "
    "SELECT pax.pax_id, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon "
    "FROM pax, crs_pax_asvc "
    "WHERE pax.pax_id=crs_pax_asvc.pax_id AND "
    "      rem_status='HI' AND "
    "      rfic IS NOT NULL AND "
    "      rfisc IS NOT NULL AND "
    "      service_name IS NOT NULL AND "
    "      emd_type IS NOT NULL AND "
    "      emd_no IS NOT NULL AND "
    "      emd_coupon IS NOT NULL AND ";
  if (is_grp_id)
    sql <<  "      pax.grp_id=:id ";
  else
    sql <<  "      pax.pax_id=:id ";

  QParams QryParams;
  QryParams << QParam("id", otInteger, id);
  TCachedQuery Qry(sql.str().c_str(), QryParams);
  Qry.get().Execute();
  return (Qry.get().RowsProcessed()>0);
};

bool LoadPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc, bool from_crs)
{
  asvc.clear();
  const char* sql=
    "SELECT * FROM pax_asvc WHERE pax_id=:pax_id";
  const char* crs_sql=
    "SELECT * FROM crs_pax_asvc "
    "WHERE pax_id=:pax_id AND "
    "      rem_status='HI' AND "
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


void SavePaxRem(int pax_id, const multiset<TPaxRemItem> &rems)
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
  for(multiset<TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    if (r->text.empty()) continue; //защита от пустой ремарки (иногда может почему то приходить с терминала)
    if ( isAPPSRem( r->code) )
      continue; // не храним информацию о перезаписи и повторной отправке в APPS
    r->toDB(RemQry);
    RemQry.Execute();
  };
};

void GetPaxFQTCards(const std::set<TPaxFQTItem> &fqts, TPaxFQTCards &cards)
{
  cards.clear();
  for(set<TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f)
  {
    pair< TPaxFQTCards::iterator, bool > res=cards.insert(make_pair(*f, *f));
    if (!res.second && res.first!=cards.end())
      res.first->second.copyIfBetter(*f);
  };
}

void SyncFQTTierLevel(int pax_id, bool from_crs, set<TPaxFQTItem> &fqts)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
  {
    if (pax_id==ASTRA::NoExists) return;
    if (fqts.empty()) return;
    //сохраняем tier_level
    set<TPaxFQTItem> prior_fqts;
    if (from_crs)
      LoadCrsPaxFQT(pax_id, prior_fqts);
    else
      LoadPaxFQT(pax_id, prior_fqts);

    TPaxFQTCards cards;
    GetPaxFQTCards(prior_fqts, cards);

    //простановка tier_level
    set<TPaxFQTItem> tmp_fqts;
    for(set<TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f)
    {
      TPaxFQTItem item=*f;
      TPaxFQTCards::const_iterator i=cards.find(*f);
      if (i!=cards.end()) item.copyIfBetter(i->second);
      tmp_fqts.insert(item);
    };

    fqts=tmp_fqts;
  }
}

void SavePaxFQT(int pax_id, const std::set<TPaxFQTItem> &fqts)
{
  TQuery FQTQry(&OraSession);
  FQTQry.Clear();
  FQTQry.SQLText="DELETE FROM pax_fqt WHERE pax_id=:pax_id";
  FQTQry.CreateVariable("pax_id",otInteger,pax_id);
  FQTQry.Execute();

  FQTQry.SQLText=
    "INSERT INTO pax_fqt(pax_id,rem_code,airline,no,extra,tier_level,tier_level_confirm) "
    "VALUES(:pax_id,:rem_code,:airline,:no,:extra,:tier_level,:tier_level_confirm)";
  FQTQry.DeclareVariable("rem_code",otString);
  FQTQry.DeclareVariable("airline",otString);
  FQTQry.DeclareVariable("no",otString);
  FQTQry.DeclareVariable("extra",otString);
  FQTQry.DeclareVariable("tier_level",otString);
  FQTQry.DeclareVariable("tier_level_confirm",otInteger);
  for(set<TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
  {
    r->toDB(FQTQry);
    FQTQry.Execute();
  };
};

class TPaxRemOriginItem
{
  public:
    TPaxRemItem rem;
    int user_id;
    string desk;
    TDateTime time_create;
    TPaxRemOriginItem()
    {
      clear();
    }
    void clear()
    {
      rem.clear();
      user_id=ASTRA::NoExists;
      desk.clear();
      time_create=ASTRA::NoExists;
    }
    const TPaxRemOriginItem& toDB(TQuery &Qry) const;
    TPaxRemOriginItem& fromDB(TQuery &Qry);
};

const TPaxRemOriginItem& TPaxRemOriginItem::toDB(TQuery &Qry) const
{
  rem.toDB(Qry);
  Qry.SetVariable("user_id", user_id);
  Qry.SetVariable("desk", desk);
  Qry.SetVariable("time_create", time_create);
  return *this;
};

TPaxRemOriginItem& TPaxRemOriginItem::fromDB(TQuery &Qry)
{
  clear();
  rem.fromDB(Qry);
  user_id=Qry.FieldAsInteger("user_id");
  desk=Qry.FieldAsString("desk");
  time_create=Qry.FieldAsDateTime("time_create");
  return *this;
};

class TPaxRemOriginList : public std::vector<TPaxRemOriginItem>
{
  private:
    bool modified;
  public:
    TPaxRemOriginList() : modified(false) {}
    void load(int pax_id);
    void save(int pax_id) const;
    void add(const multiset<TPaxRemItem> &rems, const int &user_id, const string &desk);
    void del(const multiset<TPaxRemItem> &rems, const int &user_id, const string &desk);
    void clear()
    {
      std::vector<TPaxRemOriginItem>::clear();
      modified=false;
    }
};

void TPaxRemOriginList::load(int pax_id)
{
  clear();
  const char* sql=
    "SELECT * FROM pax_rem_origin WHERE pax_id=:pax_id";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxRemQry(sql, QryParams);
  PaxRemQry.get().Execute();
  for(;!PaxRemQry.get().Eof;PaxRemQry.get().Next())
    push_back(TPaxRemOriginItem().fromDB(PaxRemQry.get()));
}

void TPaxRemOriginList::save(int pax_id) const
{
  if (!modified) return;

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="DELETE FROM pax_rem_origin WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id", otInteger, pax_id);
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem_origin(pax_id, rem, rem_code, user_id, desk, time_create) "
    "VALUES(:pax_id, :rem, :rem_code, :user_id, :desk, :time_create)";
  RemQry.DeclareVariable("rem", otString);
  RemQry.DeclareVariable("rem_code", otString);
  RemQry.DeclareVariable("user_id", otInteger);
  RemQry.DeclareVariable("desk", otString);
  RemQry.DeclareVariable("time_create", otDate);
  for(TPaxRemOriginList::const_iterator r=begin(); r!=end(); ++r)
  {
    r->toDB(RemQry);
    RemQry.Execute();
  };
}

void TPaxRemOriginList::add(const multiset<TPaxRemItem> &rems,
                            const int &user_id,
                            const string &desk)
{
  if (rems.empty()) return;
  TPaxRemOriginItem item;
  item.user_id=user_id;
  item.desk=desk;
  item.time_create=NowUTC();
  for(multiset<TPaxRemItem>::const_iterator i=rems.begin(); i!=rems.end(); ++i)
  {
    item.rem=*i;
    push_back(item);
    modified=true;
  }
}

void TPaxRemOriginList::del(const multiset<TPaxRemItem> &rems,
                            const int &user_id,
                            const string &desk)
{
  if (rems.empty()) return;
  for(multiset<TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    for(int pass=0; pass<3; pass++)
    {
      TPaxRemOriginList::iterator j=end();
      for(TPaxRemOriginList::iterator i=begin(); i!=end(); ++i)
      {
        if (!(i->rem==*r)) continue;
        if (pass==0 && (i->user_id!=user_id || i->desk!=desk)) continue; //1 проход - совпадение пользователя и пульта
        if (pass==1 && i->user_id!=user_id) continue;                    //2 проход - совпадение пользователя
                                                                         //3 проход - просто удаление последней введенной
        if (j==end() || j->time_create<i->time_create) j=i;
      }
      if (j!=end())
      {
        erase(j);
        modified=true;
        break;
      };
    }
  }
}

void GetPaxRemDifference(const boost::optional<TRemGrp> &rem_grp,
                         const multiset<TPaxRemItem> &prior_rems,
                         const multiset<TPaxRemItem> &curr_rems,
                         multiset<TPaxRemItem> &added,
                         multiset<TPaxRemItem> &deleted)
{
  added.clear();
  deleted.clear();

  multiset<TPaxRemItem> prior_rems_filtered, curr_rems_filtered;
  for(multiset<TPaxRemItem>::const_iterator i=prior_rems.begin(); i!=prior_rems.end(); ++i)
    if (!rem_grp || rem_grp.get().exists(i->code)) prior_rems_filtered.insert(*i);
  for(multiset<TPaxRemItem>::const_iterator i=curr_rems.begin(); i!=curr_rems.end(); ++i)
    if (!rem_grp || rem_grp.get().exists(i->code)) curr_rems_filtered.insert(*i);

  set_difference(prior_rems_filtered.begin(), prior_rems_filtered.end(),
                 curr_rems_filtered.begin(), curr_rems_filtered.end(),
                 inserter(deleted, deleted.end()));

  set_difference(curr_rems_filtered.begin(), curr_rems_filtered.end(),
                 prior_rems_filtered.begin(), prior_rems_filtered.end(),
                 inserter(added, added.end()));
}

void SyncPaxRemOrigin(const boost::optional<TRemGrp> &rem_grp,
                      const int &pax_id,
                      const multiset<TPaxRemItem> &prior_rems,
                      const multiset<TPaxRemItem> &curr_rems,
                      const int &user_id,
                      const string &desk)
{
  multiset<TPaxRemItem> added, deleted;

  GetPaxRemDifference(rem_grp, prior_rems, curr_rems, added, deleted);

  if (deleted.empty() && added.empty()) return;

  TPaxRemOriginList rems_origin;
  rems_origin.load(pax_id);
  rems_origin.del(deleted, user_id, desk);
  rems_origin.add(added, user_id, desk);
  rems_origin.save(pax_id);
}

void PaxRemAndASVCFromDB(int pax_id,
                         bool from_crs,
                         std::multiset<TPaxRemItem> &rems_and_asvc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  rems_and_asvc.clear();
  std::vector<TPaxASVCItem> asvc;

  if (from_crs)
  {
    LoadCrsPaxRem(pax_id, rems_and_asvc);
    LoadCrsPaxASVC(pax_id, asvc);
  }
  else
  {
    LoadPaxRem(pax_id, rems_and_asvc);
    LoadPaxASVC(pax_id, asvc);
  };

  for(multiset<TPaxRemItem>::iterator r=rems_and_asvc.begin(); r!=rems_and_asvc.end();)
  {
    if (isDisabledRem(r->code, r->text))
      r=Erase(rems_and_asvc, r);
    else
      ++r;
  };
  for(vector<TPaxASVCItem>::const_iterator r=asvc.begin(); r!=asvc.end(); ++r)
  {
    rems_and_asvc.insert(TPaxRemItem(*r, false, reqInfo->desk.lang, applyLang));
  };

  if (!(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
  {
    std::set<TPaxFQTItem> fqts;

    if (from_crs)
      LoadCrsPaxFQT(pax_id, fqts);
    else
      LoadPaxFQT(pax_id, fqts);

    for(set<TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
    {
      rems_and_asvc.insert(TPaxRemItem(*r, false, reqInfo->desk.lang, applyLang));
    };
  }
}

void PaxFQTFromDB(int pax_id,
                  bool from_crs,
                  std::set<TPaxFQTItem> &fqts)
{
  fqts.clear();

  if (from_crs)
    LoadCrsPaxFQT(pax_id, fqts);
  else
    LoadPaxFQT(pax_id, fqts);
}

void PaxRemAndASVCToXML(const std::multiset<TPaxRemItem> &rems_and_asvc,
                        xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr remsNode=NewTextChild(node,"rems");
  for(multiset<TPaxRemItem>::const_iterator r=rems_and_asvc.begin(); r!=rems_and_asvc.end(); ++r)
    r->toXML(remsNode);
}

void PaxFQTToXML(const std::set<TPaxFQTItem> &fqts,
                 xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr remsNode=NewTextChild(node,"fqt_rems");
  for(set<TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
    r->toXML(remsNode);
}

}; //namespace CheckIn

CheckIn::TPaxRemItem CalcCrewRem(const ASTRA::TPaxStatus grp_status,
                                 const ASTRA::TCrewType::Enum crew_type)
{
  if (grp_status==ASTRA::psCrew)
    return CheckIn::TPaxRemItem("CREW", "CREW");
  else if (crew_type==ASTRA::TCrewType::ExtraCrew)
    return CheckIn::TPaxRemItem(CrewTypes().encode(crew_type), CrewTypes().encode(crew_type)+" 2");
  else if (crew_type==ASTRA::TCrewType::DeadHeadCrew ||
           crew_type==ASTRA::TCrewType::MiscOperStaff)
    return CheckIn::TPaxRemItem(CrewTypes().encode(crew_type), CrewTypes().encode(crew_type));
  return CheckIn::TPaxRemItem();
}

CheckIn::TPaxRemItem CalcJmpRem(const ASTRA::TPaxStatus grp_status,
                                const bool is_jmp)
{
  if (grp_status!=ASTRA::psCrew && is_jmp)
    return CheckIn::TPaxRemItem("JMP", "JMP");
  return CheckIn::TPaxRemItem();
}




