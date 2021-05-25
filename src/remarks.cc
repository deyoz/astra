#include "remarks.h"
#include <map>
#include <regex>
#include "oralib.h"
#include "base_tables.h"
#include "qrys.h"
#include "astra_utils.h"
#include "term_version.h"
#include "date_time.h"
#include "base_callbacks.h"
#include "passenger.h"

#include "iapi_interaction.h"
#include "apps_interaction.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

#include <serverlib/str_utils.h>

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

std::ostream& operator << (std::ostream& os, const TRemCategory& value)
{
  switch(value)
  {
    case remTKN:          os << "remTKN"; break;
    case remDOC:          os << "remDOC"; break;
    case remDOCO:         os << "remDOCO"; break;
    case remDOCA:         os << "remDOCA"; break;
    case remFQT:          os << "remFQT"; break;
    case remASVC:         os << "remASVC"; break;
    case remPD:           os << "remPD"; break;
    case remCREW:         os << "remCREW"; break;
    case remAPPSOverride: os << "remAPPSOverride"; break;
    case remAPPSStatus:   os << "remAPPSStatus"; break;
    case remUnknown:      os << "remUnknown"; break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const TierLevelKey& tierLevel)
{
  os << tierLevel.airline << ":" << tierLevel.code;
  return os;
}

const std::vector<std::string> appsStatusRem={"SXIA", "SPIA", "SBIA"}; //���冷� �����
const std::vector<std::string> appsOverrideRem={"RSIA", "OVRA", "OVRG"};

TRemCategory getRemCategory(const CheckIn::TPaxRemItem& rem)
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
    for(const string& rem_code : appsStatusRem) rem_cats[rem_code]=remAPPSStatus;
    for(const string& rem_code : appsOverrideRem) rem_cats[rem_code]=remAPPSOverride;

    init=true;
  };
  if (!rem.code.empty())
  {
    //��� ६�ન �� ���⮩
    map<string, TRemCategory>::const_iterator iRem=rem_cats.find(rem.code);
    if (iRem!=rem_cats.end()) return iRem->second;

    if (rem.code.size()==4 && rem.code.substr(0,2)=="PD")
    return remPD;
  };
  if (!rem.text.empty())
  {
    for(map<string, TRemCategory>::const_iterator iRem=rem_cats.begin(); iRem!=rem_cats.end(); ++iRem)
    {
      if (rem.text.substr(0,iRem->first.size())==iRem->first) return iRem->second;
    };
  };
  return remUnknown;
};

bool isDisabledRemCategory( TRemCategory cat )
{
  return cat==remTKN || cat==remDOC || cat==remDOCO || cat==remDOCA || cat==remASVC || cat==remFQT;
};

bool isDisabledRem(const CheckIn::TPaxRemItem& rem)
{
  TRemCategory cat=getRemCategory(rem);
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

bool isReadonlyRemCategory( TRemCategory cat )
{
  return cat==remTKN || cat==remDOC || cat==remDOCO || cat==remDOCA || cat==remASVC || cat==remPD;
};

bool isReadonlyRem(const CheckIn::TPaxRemItem& rem)
{
  static set<string> rems;
  if (rems.empty())
  {
    int i=sizeof(ReadonlyRemCodes)/sizeof(ReadonlyRemCodes[0])-1;
    for(;i>=0;i--) rems.insert(ReadonlyRemCodes[i]);
  };

  if (!rem.code.empty()) return rems.find(rem.code)!=rems.end();

  if (!rem.text.empty())
  {
    for(set<string>::const_iterator iRem=rems.begin(); iRem!=rems.end(); ++iRem)
    {
      if (rem.text.substr(0,iRem->size())==*iRem) return true;
    };
  };

  return false;
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
      case retREM_STAT:
        event_type = "REM_STAT";
        break;
      case retLIMITED_CAPAB_STAT:
        event_type = "LIMITED_CAPAB_STAT";
        break;
      case retSELF_CKIN_EXCHANGE:
        event_type = "SELF_CKIN_EXCHANGE";
        break;
      case retFORBIDDEN_WEB:
        event_type = "WEB";
        break;
      case retFORBIDDEN_KIOSK:
        event_type = "KIOSK";
        break;
      case retFORBIDDEN_MOB:
        event_type = "MOB";
        break;
      case retFORBIDDEN_FREE_SEAT:
        event_type = "FREE_SEAT";
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

static void getAPPSRemCodes(const int pax_id, set<string>& remCodes)
{
    if (pax_id == ASTRA::NoExists) {
        return;
    }
    for(const auto& st: APPS::statusesFromDb(PaxId_t(pax_id))) {
        if(st == "B") {
            remCodes.insert("SBIA");
        } else if(st == "P") {
            remCodes.insert("SPIA");
        } else if(st == "X") {
            remCodes.insert("SXIA");
        }
    }


    for(const auto& st: IAPI::statusesFromDb(PaxId_t(pax_id))) {
        if(st == "0Z") {
            remCodes.insert("SBIA");
        } else if (!st.empty()) {
            remCodes.insert("SXIA");
        }
    }
}

CheckIn::TPaxRemItem getAPPSRem(const int pax_id, const std::string &lang )
{
  set<string> remCodes;
  getAPPSRemCodes(pax_id, remCodes);

  if (!remCodes.empty())
  {
    for(const string& code : appsStatusRem)
      if (remCodes.find(code)!=remCodes.end())
      {
        CheckIn::TPaxRemItem rem;
        rem.code=code;
        rem.text=ElemIdToPrefferedElem(etCkinRemType, rem.code, efmtNameLong, lang);
        return rem;
      }
  }

  return CheckIn::TPaxRemItem();
}

string GetRemarkStr(const TRemGrp &rem_grp, const multiset<CheckIn::TPaxRemItem> &rems, const string &term)
{
  string result;
  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();++r)
  {
    if (r->code.empty() || !rem_grp.exists(r->code)) continue;
    if (std::find(appsStatusRem.begin(), appsStatusRem.end(), r->code)!=appsStatusRem.end())
    {
      if (!result.empty()) result.insert(0, term);
      result.insert(0, r->text);
    }
    else
    {
      if (!result.empty()) result+=term;
      result+=r->code;
    }
  };
  return result;
}

string GetRemarkMSGText(int pax_id, const string &rem_msg)
{
   string res;
   const char *sql =
       "SELECT TRIM(rem) rem FROM pax_rem "
       "WHERE pax_id=:pax_id AND rem_code=:rem_msg "
       "ORDER BY rem";
   QParams QryParams;
   QryParams << QParam("pax_id", otInteger, pax_id);
   QryParams << QParam("rem_msg", otString, rem_msg);
   TCachedQuery Qry(sql, QryParams);
   Qry.get().Execute();
   for(;!Qry.get().Eof;Qry.get().Next()) {
     string value = Qry.get().FieldAsString( "rem" );
     if ( rem_msg == value.substr( 0, 3 ) ) {
       value.erase( 0, 3 );
       value = TrimString(value);
     }
     if ( !res.empty() ) {
       res += "\n";
     }
     res += value;
   }
   return res;
}

void GetRemarks(int pax_id, const string &lang, std::multiset<CheckIn::TPaxRemItem> &rems)
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
        "SELECT TRIM(rem_code), NULL FROM pax_fqt where pax_id = :pax_id "
        "UNION "
        "SELECT TRIM(rem_code), NULL FROM pax_rem "
        "WHERE pax_id=:pax_id AND "
        "      rem_code NOT IN (SELECT rem_code FROM rem_cats WHERE category IN ('DOC','DOCO','DOCA','TKN','ASVC'))";

    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
    for(;!Qry.get().Eof;Qry.get().Next())
      rems.insert(CheckIn::TPaxRemItem().fromDB(Qry.get()));
    CheckIn::TPaxRemItem rem = getAPPSRem( pax_id, lang );
    if ( !rem.empty() )
     rems.insert( rem );
}

string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const string &lang, const string &term)
{
    multiset<CheckIn::TPaxRemItem> rems;
    GetRemarks(pax_id, lang, rems);
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

std::string TPaxRemBasic::rem_text(bool inf_indicator, const std::string& lang, TLangApplying fmt, TOutput output) const
{
  if (empty()) return "";
  bool strictly_lat=lang!=AstraLocale::LANG_RU && fmt==applyLangForAll;
  bool translit_lat=lang!=AstraLocale::LANG_RU && (fmt==applyLangForAll || fmt==applyLangForTranslit);
  bool language_lat=lang!=AstraLocale::LANG_RU && (fmt==applyLangForAll || fmt==applyLangForTranslit || fmt==applyLang);
  return get_rem_text(inf_indicator, lang, strictly_lat, translit_lat, language_lat, output);
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
  catch (const EBaseTableError&) {};
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
                         bool inf_indicator,
                         const std::string &lang,
                         TLangApplying fmt,
                         TOutput output)
{
  clear();
  code=basic.rem_code();
  text=basic.rem_text(inf_indicator, lang, fmt, output);
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

TPaxRemItem& TPaxRemItem::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  text=upperc(NodeAsStringFast("rem_text",node2));
  TrimString(text);

  //�뤥��� ��� ६�ન
  std::smatch sm;
  if (std::regex_search(text, sm, std::regex("^[A-Z�-��0-9]{3,5}\\b")))
    code=sm[0];

  calcPriority();
  return *this;
}

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
                                      bool language_lat,
                                      TOutput output) const
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

TPaxFQTItem& TPaxFQTItem::fromDB(DB::TQuery &Qry)
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

const TPaxFQTItem& TPaxFQTItem::toXML(xmlNodePtr node,
                                      const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;
  xmlNodePtr remNode=NewTextChild(node,"fqt_rem");
  NewTextChild(remNode, "rem_code", lang?ElemIdToPrefferedElem(etCkinRemType, rem, efmtCodeNative, lang->get()):rem);
  NewTextChild(remNode, "airline", lang?airlineToPrefferedCode(airline, lang.get()):airline);
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

TPaxFQTItem& TPaxFQTItem::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  rem="FQTV";
  TElemFmt fmt;
  airline = ElemToElemId( etAirline, NodeAsStringFast("airline", node2), fmt );
  if (fmt==efmtUnknown)
    airline=NodeAsStringFast("airline", node2);
  no=NodeAsStringFast("no", node2);
  return *this;
};

std::string TPaxFQTItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat,
                                      TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem, efmtCodeNative, lang)
         << " " << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative, lang)
         << " " << (strictly_lat?transliter(convert_char_view(no, strictly_lat), 1, strictly_lat):no);
  if (!extra.empty())
    result << "/" << transliter(extra, 1, translit_lat);
  if (output == outputReport and !tier_level.empty())
    result << "/" << transliter(tier_level, 1, translit_lat);

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
  NewTextChild(remNode, "paid_weight", ASTRA::NoExists, ASTRA::NoExists); //!!! ��������
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

const TServiceBasic& TServiceBasic::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("rfic", RFIC);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("service_quantity", service_quantity);
  Qry.SetVariable("ssr_code", ssr_code);
  Qry.SetVariable("service_name", service_name);
  Qry.SetVariable("emd_type", emd_type);
  return *this;
}

const TPaxASVCItem& TPaxASVCItem::toDB(DB::TQuery &Qry) const
{
  TServiceBasic::toDB(Qry);
  Qry.SetVariable("emd_no", emd_no);
  emd_coupon==ASTRA::NoExists?Qry.SetVariable("emd_coupon", FNull):
                              Qry.SetVariable("emd_coupon", emd_coupon);
  return *this;
}

TServiceBasic& TServiceBasic::fromDB(DB::TQuery &Qry)
{
  clear();
  RFIC=Qry.FieldAsString("rfic");
  RFISC=Qry.FieldAsString("rfisc");
  service_quantity=Qry.FieldAsInteger("service_quantity");
  ssr_code=Qry.FieldAsString("ssr_code");
  service_name=Qry.FieldAsString("service_name");
  emd_type=Qry.FieldAsString("emd_type");
  return *this;
}

TPaxASVCItem& TPaxASVCItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TServiceBasic::fromDB(Qry);
  emd_no=Qry.FieldAsString("emd_no");
  emd_coupon=Qry.FieldIsNULL("emd_coupon")?ASTRA::NoExists:
                                           Qry.FieldAsInteger("emd_coupon");
  return *this;
};

std::string TPaxASVCItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat,
                                       TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " " << rem_status() << service_quantity
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

void TServiceBasic::rcpt_service_types(set<ASTRA::TRcptServiceType> &service_types) const
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
    TRemCategory cat=getRemCategory(rem);
    if (isDisabledRemCategory(cat)) continue;
    rems.insert(rem);
  };

  return !rems.empty();
};

bool LoadPaxRem(bool is_crs, int pax_id, multiset<TPaxRemItem> &rems, bool onlyPD,
                const std::string& code)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id
                   << ", is_crs=" << is_crs;
  rems.clear();
  std::string rem_code;
  std::string rem_text;
  auto cur = make_db_curs(
        "SELECT "
        "rem, rem_code "
        "FROM " + std::string(is_crs ? "CRS_PAX_REM" : "PAX_REM") + " "
        "WHERE pax_id=:pax_id "
        + std::string(onlyPD ? "AND rem_code LIKE 'PD__' " : "")
        + std::string(!code.empty() ? "AND rem_code = :rem_code " : ""),
        PgOra::getROSession(is_crs ? "CRS_PAX_REM" : "PAX_REM"));

  cur.stb()
      .def(rem_text)
      .defNull(rem_code, "")
      .bind(":pax_id", pax_id);
  if (!code.empty()) {
    cur.bind(":rem_code", code);
  }
  cur.exec();

  while (!cur.fen()) {
    rems.insert(TPaxRemItem(rem_code, rem_text));
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << rems.size();
  return !rems.empty();
}

bool LoadCrsPaxRem(int pax_id, multiset<TPaxRemItem> &rems, bool onlyPD,
                   const std::string& code)
{
  return LoadPaxRem(true /*is_crs*/, pax_id, rems, onlyPD, code);
}

bool LoadCrsPaxFQT(int pax_id, std::set<TPaxFQTItem> &fqts)
{
  fqts.clear();
  const char* sql=
    "SELECT crs_pax_fqt.*, "
    "(CASE WHEN tier_level IS NULL THEN NULL ELSE 1 END) AS tier_level_confirm "
    "FROM crs_pax_fqt WHERE pax_id=:pax_id";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  DB::TCachedQuery PaxFQTQry(PgOra::getROSession("CRS_PAX_FQT"), sql, QryParams, STDLOG);
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
  DB::TCachedQuery PaxFQTQry(PgOra::getROSession("PAX_FQT"), sql, QryParams, STDLOG);
  PaxFQTQry.get().Execute();
  for(;!PaxFQTQry.get().Eof;PaxFQTQry.get().Next())
    fqts.insert(TPaxFQTItem().fromDB(PaxFQTQry.get()));
  return !fqts.empty();
};

std::set<TPaxFQTItem> getPaxFQTNotEmptyTierLevel(const PaxOrigin& origin, const PaxId_t& paxId, bool onlyFQTV)
{
  std::set<TPaxFQTItem> result;
  switch(origin)
  {
    case paxCheckIn:
      LoadPaxFQT(paxId.get(), result);
      break;
    case paxPnl:
      LoadCrsPaxFQT(paxId.get(), result);
      break;
    default:
      break;
  }

  //������ std::remove_id �ਬ����� ��� std::set ����������, ��� �⥫��� ��
  //��稭�� � �++20 ��� std::set ���� ��⮤ erase_if ��� ��� 楫��
  for(set<TPaxFQTItem>::iterator i=result.begin(); i!=result.end();)
  {
    if (i->tier_level.empty() or (onlyFQTV and i->rem != "FQTV"))
    {
      i=result.erase(i);
      continue;
    }
    ++i;
  }

  return result;
}

boost::optional<TPaxFQTItem> TPaxFQTItem::getNotEmptyTierLevel(const PaxOrigin& origin, const PaxId_t& paxId, bool onlyFQTV)
{
  std::set<TPaxFQTItem> fqts=getPaxFQTNotEmptyTierLevel(origin, paxId, onlyFQTV);
  if (fqts.empty()) return boost::none;
  return *fqts.begin();
}

bool DeletePaxASVC(int pax_id)
{
  TCachedQuery Qry("DELETE FROM pax_asvc WHERE pax_id=:id",
                   QParams() << QParam("id", otInteger, pax_id));
  Qry.get().Execute();
  return (Qry.get().RowsProcessed()>0);
}

bool DeletePaxPD(int pax_id)
{
  TCachedQuery Qry("DELETE FROM pax_rem WHERE pax_id=:id AND rem_code LIKE 'PD__'",
                   QParams() << QParam("id", otInteger, pax_id));
  Qry.get().Execute();
  return (Qry.get().RowsProcessed()>0);
}

TGrpServiceAutoList loadCrsPaxAsvc(PaxId_t pax_id, const std::optional<TTripInfo>& flt = {});

bool AddPaxASVC(const TGrpServiceAutoItem& item)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << item.pax_id;
  auto cur = make_db_curs(
        "INSERT INTO pax_asvc ("
        "pax_id, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon "
        ") VALUES ( "
        ":pax_id, :rfic, :rfisc, :service_quantity, :ssr_code, :service_name, :emd_type, :emd_no, :emd_coupon "
        ")",
        PgOra::getRWSession("PAX_ASVC"));
  short notNull = 0;
  short null = -1;
  cur.stb()
      .bind(":pax_id", item.pax_id)
      .bind(":rfic", item.RFIC)
      .bind(":rfisc", item.RFISC)
      .bind(":service_quantity", item.service_quantity)
      .bind(":ssr_code", item.ssr_code)
      .bind(":service_name", item.service_name)
      .bind(":emd_type", item.emd_type)
      .bind(":emd_no", item.emd_no)
      .bind(":emd_coupon", item.emd_coupon,
            item.emd_coupon != ASTRA::NoExists ? &notNull : &null);
  cur.exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool AddPaxASVC(int pax_id)
{
  if (pax_id == ASTRA::NoExists) {
    return false;
  }
  bool result = false;
  const TGrpServiceAutoList asvcs = loadCrsPaxAsvc(PaxId_t(pax_id));
  for (const TGrpServiceAutoItem& asvc: asvcs) {
    result = AddPaxASVC(asvc);
  }
  return result;
}

bool AddPaxASVC(int id, bool is_grp_id)
{
  if (id == ASTRA::NoExists) {
    return false;
  }
  bool result = false;
  if (is_grp_id) {
    const std::set<PaxId_t> paxIdSet = loadPaxIdSet(GrpId_t(id));
    for (PaxId_t pax_id: paxIdSet) {
      result = AddPaxASVC(pax_id.get());
    }
  } else {
    if (existsPax(PaxId_t(id))) {
      result = AddPaxASVC(id);
    }
  }
  return result;
}

bool AddPaxRem(int pax_id, const CheckIn::TPaxRemItem& item)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "INSERT INTO pax_rem ( "
        "pax_id, rem, rem_code "
        ") VALUES ( "
        ":pax_id, :rem, :rem_code "
        ")",
        PgOra::getRWSession("PAX_REM"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .bind(":rem", item.text)
      .bind(":rem_code", item.code);
  cur.exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool AddPaxPD(int pax_id)
{
  bool result = false;
  multiset<CheckIn::TPaxRemItem> rems;
  LoadCrsPaxRem(pax_id, rems, true /*onlyPD*/);
  for (const CheckIn::TPaxRemItem& rem: rems) {
    result = AddPaxRem(pax_id, rem);
  }
  return result;
}

bool AddPaxPD(int id, bool is_grp_id)
{
  if (id == ASTRA::NoExists) {
    return false;
  }
  bool result = false;
  if (is_grp_id) {
    const std::set<PaxId_t> paxIdSet = loadPaxIdSet(GrpId_t(id));
    for (PaxId_t pax_id: paxIdSet) {
      result = AddPaxPD(pax_id.get());
    }
  } else {
    if (existsPax(PaxId_t(id))) {
      result = AddPaxPD(id);
    }
  }
  return result;
}

std::string LoadCrsPaxRem(int pax_id, const std::string& ssr_code)
{
  multiset<CheckIn::TPaxRemItem> rems;
  LoadCrsPaxRem(pax_id, rems);
  std::string ssr_text;
  for (const CheckIn::TPaxRemItem& rem: rems) {
    if (rem.code == ssr_code) {
      if (!ssr_text.empty()) {
        ssr_text.clear();
        break;
      }
      ssr_text = rem.text;
    }
  }
  return ssr_text;
}

bool PaxASVC_CrsFromDB(PaxId_t pax_id, vector<TPaxASVCItem> &result)
{
  result.clear();
  const TGrpServiceAutoList asvcs = loadCrsPaxAsvc(pax_id);
  for (const TGrpServiceAutoItem& asvc: asvcs) {
    TPaxASVCItem item;
    item.clear();
    item.emd_no=asvc.emd_no;
    item.emd_coupon=asvc.emd_coupon;
    item.RFIC=asvc.RFIC;
    item.RFISC=asvc.RFISC;
    item.service_quantity=asvc.service_quantity;
    item.ssr_code=asvc.ssr_code;
    item.service_name=asvc.service_name;
    item.emd_type=asvc.emd_type;
    if (!item.ssr_code.empty()) {
      item.ssr_text = LoadCrsPaxRem(pax_id.get(), item.ssr_code);
    }
    result.push_back(item);
  }
  return !result.empty();
}

bool PaxASVCFromDB(PaxId_t pax_id, vector<TPaxASVCItem> &asvc, bool from_crs)
{
  if (from_crs) {
    return PaxASVC_CrsFromDB(pax_id, asvc);
  }
  asvc.clear();

  QParams ASVCQryParams;
  ASVCQryParams << QParam("pax_id", otInteger, pax_id.get());
  DB::TCachedQuery PaxASVCQry(
        PgOra::getROSession("PAX_ASVC"),
        "SELECT * FROM pax_asvc "
        "WHERE pax_id=:pax_id",
        ASVCQryParams,
        STDLOG);
  PaxASVCQry.get().Execute();
  if (!PaxASVCQry.get().Eof)
  {
    QParams RemQryParams;
    RemQryParams << QParam("pax_id", otInteger, pax_id.get())
                 << QParam("rem_code", otString);
    DB::TCachedQuery PaxRemQry(
          PgOra::getROSession("PAX_ASVC"),
          "SELECT rem FROM pax_rem "
          "WHERE pax_id=:pax_id "
          "AND rem_code=:rem_code",
          RemQryParams,
          STDLOG);

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
}

bool LoadPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc)
{
  if (pax_id == ASTRA::NoExists) {
    return false;
  }
  return PaxASVCFromDB(PaxId_t(pax_id), asvc, false);
}

bool LoadCrsPaxASVC(int pax_id, vector<TPaxASVCItem> &asvc)
{
  if (pax_id == ASTRA::NoExists) {
    return false;
  }
  return PaxASVCFromDB(PaxId_t(pax_id), asvc, true);
}

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
    //��࠭塞 tier_level
    set<TPaxFQTItem> prior_fqts;
    if (from_crs)
      LoadCrsPaxFQT(pax_id, prior_fqts);
    else
      LoadPaxFQT(pax_id, prior_fqts);

    TPaxFQTCards cards;
    GetPaxFQTCards(prior_fqts, cards);

    //���⠭���� tier_level
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
        if (pass==0 && (i->user_id!=user_id || i->desk!=desk)) continue; //1 ��室 - ᮢ������� ���짮��⥫� � ����
        if (pass==1 && i->user_id!=user_id) continue;                    //2 ��室 - ᮢ������� ���짮��⥫�
                                                                         //3 ��室 - ���� 㤠����� ��᫥���� ���������
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
                         const PaxRems &prior_rems,
                         const PaxRems &curr_rems,
                         PaxRems &added,
                         PaxRems &deleted,
                         list<std::pair<TPaxRemItem, TPaxRemItem> > &modified)
{
  GetPaxRemDifference(rem_grp, prior_rems, curr_rems, added, deleted);

  modified.clear();
  for(PaxRems::iterator a=added.begin(); a!=added.end();)
  {
    PaxRems::iterator d=deleted.begin();
    for(; d!=deleted.end();)
      if (a->code==d->code) break;
    if (d!=deleted.end())
    {
      modified.emplace_back(*d, *a);
      a=added.erase(a);
      d=deleted.erase(d);
      continue;
    }
    ++a;
  }
}

void GetPaxRemDifference(const boost::optional<TRemGrp> &rem_grp,
                         const PaxRems &prior_rems,
                         const PaxRems &curr_rems,
                         PaxRems &added,
                         PaxRems &deleted)
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
                         const boost::optional<std::set<TPaxFQTItem>>& add_fqts,
                         std::multiset<TPaxRemItem> &rems_and_asvc)
{
  if (pax_id == ASTRA::NoExists) {
    return;
  }
  TReqInfo *reqInfo = TReqInfo::Instance();
  rems_and_asvc.clear();

  if (from_crs)
    LoadCrsPaxRem(pax_id, rems_and_asvc);
  else
    LoadPaxRem(pax_id, rems_and_asvc);

  for(multiset<TPaxRemItem>::iterator r=rems_and_asvc.begin(); r!=rems_and_asvc.end();)
  {
    if (isDisabledRem(*r))
      r=Erase(rems_and_asvc, r);
    else
      ++r;
  };

  std::vector<TPaxASVCItem> asvc;
  PaxASVCFromDB(PaxId_t(pax_id), asvc, from_crs);
  for(vector<TPaxASVCItem>::const_iterator r=asvc.begin(); r!=asvc.end(); ++r)
  {
    rems_and_asvc.insert(TPaxRemItem(*r, false, reqInfo->desk.lang, applyLang));
  };

  std::set<TPaxFQTItem> fqts;
  if (!add_fqts)
  {
    if (!(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
      PaxFQTFromDB(pax_id, from_crs, fqts);
  }
  else
    fqts=add_fqts.get();

  for(set<TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
  {
    rems_and_asvc.insert(TPaxRemItem(*r, false, reqInfo->desk.lang, applyLang));
  };
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

bool forbiddenRemExists(const TRemGrp& forbiddenRemGrp,
                        const multiset<CheckIn::TPaxRemItem> &rems)
{
  if (forbiddenRemGrp.empty()) return false;

  for(const CheckIn::TPaxRemItem& rem : rems)
    if (forbiddenRemGrp.exists(rem.code))
    {
      ProgTrace(TRACE5, "%s: forbidden rem code %s", __FUNCTION__, rem.code.c_str());
      return true;
    }

  return false;
}
