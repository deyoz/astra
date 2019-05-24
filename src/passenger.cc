#include "passenger.h"
#include "misc.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "term_version.h"
#include "baggage.h"
#include "qrys.h"
#include "exceptions.h"
#include "jxtlib/jxt_cont.h"
#include "astra_elem_utils.h"
#include "apis_utils.h"
#include "base_tables.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace AstraLocale;

namespace APIS
{

char ReplacePunctSymbol(char c)
{
  ByteReplace(&c,1,".,:;'\"\\/",
                   "      --");
  return c;
};

char ReplaceDigit(char c)
{
  ByteReplace(&c,1,"1234567890",
                   "----------");
  return c;
};

}; //namespace APIS

bool isTestPaxId(int id)
{
  return id!=ASTRA::NoExists && id>=TEST_ID_BASE && id<=TEST_ID_LAST;
}

int getEmptyPaxId()
{
  return EMPTY_ID;
}

bool isEmptyPaxId(int id)
{
  return id!=ASTRA::NoExists && id==EMPTY_ID;
}

namespace CheckIn
{

const TPaxGrpCategoriesView& PaxGrpCategories()
{
  static TPaxGrpCategoriesView paxGrpCategories;
  return paxGrpCategories;
}

const TPaxTknItem& TPaxTknItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"ticket_no",no);
  if (coupon!=ASTRA::NoExists)
    NewTextChild(node,"coupon_no",coupon);
  else
    NewTextChild(node,"coupon_no");
  NewTextChild(node,"ticket_rem",rem);
  NewTextChild(node,"ticket_confirm",(int)confirm);
  return *this;
};

TPaxTknItem& TPaxTknItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  no=NodeAsStringFast("ticket_no",node2);
  if (!NodeIsNULLFast("coupon_no",node2))
    coupon=NodeAsIntegerFast("coupon_no",node2);
  rem=NodeAsStringFast("ticket_rem",node2);
  confirm=NodeAsIntegerFast("ticket_confirm",node2)!=0;
  return *this;
};

const TPaxTknItem& TPaxTknItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("ticket_no", no);
  if (coupon!=ASTRA::NoExists)
    Qry.SetVariable("coupon_no", coupon);
  else
    Qry.SetVariable("coupon_no", FNull);
  Qry.SetVariable("ticket_rem", rem);
  Qry.SetVariable("ticket_confirm", (int)confirm);
  return *this;
};

TPaxTknItem& TPaxTknItem::fromDB(TQuery &Qry)
{
  clear();
  no=Qry.FieldAsString("ticket_no");
  if (!Qry.FieldIsNULL("coupon_no"))
    coupon=Qry.FieldAsInteger("coupon_no");
  else
    coupon=ASTRA::NoExists;
  rem=Qry.FieldAsString("ticket_rem");
  confirm=Qry.FieldAsInteger("ticket_confirm")!=0;
  return *this;
};

long int TPaxTknItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!no.empty()) result|=TKN_TICKET_NO_FIELD;
  return result;
};

std::string TPaxTknItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat,
                                      TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1 " << (inf_indicator?"INF":"")
         << (strictly_lat?transliter(convert_char_view(no, strictly_lat), 1, strictly_lat):no);
  if (coupon!=ASTRA::NoExists)
    result << "/" << coupon;
  return result.str();
}

int TPaxTknItem::checkedInETCount() const
{
  if (!validET()) return 0;
  TCachedQuery Qry("SELECT COUNT(*) AS num FROM pax WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no",
                   QParams() << QParam("ticket_no", otString, no)
                             << QParam("coupon_no", otInteger, coupon));
  Qry.get().Execute();
  if (Qry.get().Eof) return 0;
  return Qry.get().FieldAsInteger("num");
}

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn)
{
  return LoadPaxTkn(ASTRA::NoExists, pax_id, tkn);
};

bool LoadPaxTkn(TDateTime part_key, int pax_id, TPaxTknItem &tkn)
{
    tkn.clear();

    const char* sql=
        "SELECT ticket_no, coupon_no, ticket_rem, ticket_confirm "
        "FROM pax WHERE pax_id=:pax_id";
    const char* sql_arx=
        "SELECT ticket_no, coupon_no, ticket_rem, ticket_confirm "
        "FROM arx_pax WHERE part_key=:part_key AND pax_id=:pax_id";
    const char *result_sql = NULL;
    QParams QryParams;
    if (part_key!=ASTRA::NoExists)
    {
        result_sql = sql_arx;
        QryParams << QParam("part_key", otDate, part_key);
    }
    else
        result_sql = sql;
    QryParams << QParam("pax_id", otInteger, pax_id);
    TCachedQuery PaxTknQry(result_sql, QryParams);
    PaxTknQry.get().Execute();
    if (!PaxTknQry.get().Eof) tkn.fromDB(PaxTknQry.get());
    return !tkn.empty();
};

bool LoadCrsPaxTkn(int pax_id, TPaxTknItem &tkn)
{
  tkn.clear();
  const char* sql1=
    "SELECT ticket_no, coupon_no, rem_code AS ticket_rem, 0 AS ticket_confirm "
    "FROM crs_pax_tkn "
    "WHERE pax_id=:pax_id "
    "ORDER BY DECODE(rem_code,'TKNE',0,'TKNA',1,'TKNO',2,3),ticket_no,coupon_no ";
  const char* sql2=
    "SELECT report.get_TKNO2(:pax_id, '/') AS no FROM dual";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxTknQry(sql1, QryParams);
  PaxTknQry.get().Execute();
  if (!PaxTknQry.get().Eof) tkn.fromDB(PaxTknQry.get());
  else
  {
    TCachedQuery GetTKNO2Qry(sql2, QryParams);
    GetTKNO2Qry.get().Execute();
    if (!GetTKNO2Qry.get().Eof && !GetTKNO2Qry.get().FieldIsNULL("no"))
    {
      tkn.no=GetTKNO2Qry.get().FieldAsString("no");
      if (!tkn.no.empty())
      {
        string::size_type pos=tkn.no.find_last_of('/');
        if (pos!=string::npos)
        {
          if (StrToInt(tkn.no.substr(pos+1).c_str(),tkn.coupon)!=EOF)
            tkn.no.erase(pos);
          else
            tkn.coupon=ASTRA::NoExists;
          tkn.rem="TKNE";
        }
        else
          tkn.rem="TKNA";
      };
    };
  };
  return !tkn.empty();
};

string PaxDocGenderNormalize(const string &pax_doc_gender)
{
  if (pax_doc_gender.empty()) return "";
  int is_female=CheckIn::is_female(pax_doc_gender, "");
  if (is_female!=ASTRA::NoExists)
    return (is_female==0?"M":"F");
  else
    return "";
};

const TPaxDocCompoundType& TPaxDocCompoundType::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (TReqInfo::Instance()->client_type == ASTRA::ctTerm)
  {
    if (!TReqInfo::Instance()->desk.compatible(DOCO_ADD_TYPES_VERSION))
      NewTextChild(node, "type", type, "");
    else
      NewTextChild(node, "type", type.empty()?"":type+subtype, "");
  }
  else
  {
    NewTextChild(node, "type", type, "");
    NewTextChild(node, "subtype", subtype, "");
  };
  return *this;
}

std::string paxDocCountryToWebXML(const std::string &code,
                                  const boost::optional<AstraLocale::OutputLang>& lang)
{
  std::string result;
  if (!code.empty())
  {
    try
    {
      if (TReqInfo::Instance()->client_type == ASTRA::ctWeb ||
          TReqInfo::Instance()->client_type == ASTRA::ctMobile)
      {
        result=getBaseTable(etPaxDocCountry).get_row("code",code).AsString("country");
        result=lang?ElemIdToPrefferedElem(etCountry, result, efmtCodeNative, lang->get()):result;
      }
    }
    catch (EBaseTableError) {};
    if (result.empty()) result=lang?ElemIdToPrefferedElem(etPaxDocCountry, code, efmtCodeNative, lang->get()):code;
  };
  return result;
}

const TPaxDocCompoundType& TPaxDocCompoundType::toWebXML(xmlNodePtr node,
                                                         const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "type", lang?ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang->get()):type);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");

  TPaxDocCompoundType::toXML(docNode);
  NewTextChild(docNode, "issue_country", issue_country, "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "nationality", nationality, "");
  if (birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(birth_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "gender", gender, "");
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "surname", surname, "");
  NewTextChild(docNode, "first_name", first_name, "");
  NewTextChild(docNode, "second_name", second_name, "");
  NewTextChild(docNode, "pr_multi", (int)pr_multi, (int)false);
  NewTextChild(docNode, "scanned_attrs", scanned_attrs, (int)NO_FIELDS);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toWebXML(xmlNodePtr node,
                                         const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");

  TPaxDocCompoundType::toWebXML(docNode, lang);
  NewTextChild(docNode, "issue_country", paxDocCountryToWebXML(issue_country, lang));
  NewTextChild(docNode, "no", no);
  NewTextChild(docNode, "nationality", paxDocCountryToWebXML(nationality, lang));
  if (birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(birth_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "birth_date");
  NewTextChild(docNode, "gender", lang?ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang->get()):gender);
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "surname", surname);
  NewTextChild(docNode, "first_name", first_name);
  NewTextChild(docNode, "second_name", second_name);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  if (TReqInfo::Instance()->client_type == ASTRA::ctTerm)
  {
    string compound = NodeAsStringFast("type",node2,"");
    if (!compound.empty())
    {
      TElemFmt fmt;
      ElemToElemId(etPaxDocType, compound, fmt);
      if (fmt!=efmtUnknown) type=compound;
      else
      {
        type=compound.substr(0,1);
        subtype=compound.substr(1);
      };
    }
  }
  else
  {
    type=NodeAsStringFast("type",node2,"");
    subtype=NodeAsStringFast("subtype",node2,"");
  }
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=NodeAsNode("type", node);

  type=NodeAsStringFast("type",node2);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  type=NodeAsStringFast("TYPE", node2, "");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromXML(node);
  issue_country=NodeAsStringFast("issue_country",node2,"");
  no=NodeAsStringFast("no",node2,"");
  nationality=NodeAsStringFast("nationality",node2,"");
  birth_date = date_fromXML(NodeAsStringFast("birth_date",node2,""));
  gender=PaxDocGenderNormalize(NodeAsStringFast("gender",node2,""));
  expiry_date = date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  surname=NodeAsStringFast("surname",node2,"");
  first_name=NodeAsStringFast("first_name",node2,"");
  second_name=NodeAsStringFast("second_name",node2,"");
  pr_multi=NodeAsIntegerFast("pr_multi",node2,0)!=0;
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  return *this;
}

TScannedPaxDocItem& TScannedPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocItem::fromXML(node);
  extra=NodeAsStringFast("extra",node2,"");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  TPaxDocCompoundType::fromWebXML(node);

  xmlNodePtr node2=NodeAsNode("issue_country", node);
  issue_country=NodeAsStringFast("issue_country",node2);
  no=NodeAsStringFast("no",node2);
  nationality=NodeAsStringFast("nationality",node2);
  if (!NodeIsNULLFast("birth_date",node2))
    birth_date=NodeAsDateTimeFast("birth_date",node2);
  gender=NodeAsStringFast("gender",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  surname=NodeAsStringFast("surname",node2);
  first_name=NodeAsStringFast("first_name",node2);
  second_name=NodeAsStringFast("second_name",node2);
  return *this;
}

TPaxDocItem& TPaxDocItem::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromMeridianXML(node);
  issue_country=NodeAsStringFast("ISSUE_COUNTRY", node2, "");
  no=NodeAsStringFast("NO", node2, "");
  nationality=NodeAsStringFast("NATIONALITY",node2,"");
  birth_date = date_fromXML(NodeAsStringFast("BIRTH_DATE", node2, ""));
  gender=PaxDocGenderNormalize(NodeAsStringFast("GENDER", node2, ""));
  expiry_date = date_fromXML(NodeAsStringFast("EXPIRY_DATE", node2, ""));
  surname = upperc(NodeAsStringFast("SURNAME", node2, ""));
  first_name = upperc(NodeAsStringFast("FIRST_NAME", node2, ""));
  second_name = upperc(NodeAsStringFast("SECOND_NAME", node2, ""));
  return *this;
}

const TPaxDocCompoundType& TPaxDocCompoundType::toDB(TQuery &Qry) const
{
  Qry.SetVariable("type", type);
  Qry.SetVariable("subtype", subtype);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toDB(TQuery &Qry) const
{
  TPaxDocCompoundType::toDB(Qry);
  Qry.SetVariable("issue_country", issue_country);
  Qry.SetVariable("no", no);
  Qry.SetVariable("nationality", nationality);
  if (birth_date!=ASTRA::NoExists)
    Qry.SetVariable("birth_date", birth_date);
  else
    Qry.SetVariable("birth_date", FNull);
  Qry.SetVariable("gender", gender);
  if (expiry_date!=ASTRA::NoExists)
    Qry.SetVariable("expiry_date", expiry_date);
  else
    Qry.SetVariable("expiry_date", FNull);
  Qry.SetVariable("surname", surname);
  Qry.SetVariable("first_name", first_name);
  Qry.SetVariable("second_name", second_name);
  Qry.SetVariable("pr_multi", (int)pr_multi);
  Qry.SetVariable("type_rcpt", type_rcpt);
  if (Qry.GetVariableIndex("scanned_attrs")>=0)
    Qry.SetVariable("scanned_attrs", (int)scanned_attrs);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromDB(TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
  if (Qry.GetFieldIndex("subtype")>=0)
    subtype=Qry.FieldAsString("subtype");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxDocCompoundType::fromDB(Qry);
  issue_country=Qry.FieldAsString("issue_country");
  no=Qry.FieldAsString("no");
  nationality=Qry.FieldAsString("nationality");
  if (!Qry.FieldIsNULL("birth_date"))
    birth_date=Qry.FieldAsDateTime("birth_date");
  else
    birth_date=ASTRA::NoExists;
  gender=PaxDocGenderNormalize(Qry.FieldAsString("gender"));
  if (!Qry.FieldIsNULL("expiry_date"))
    expiry_date=Qry.FieldAsDateTime("expiry_date");
  else
    expiry_date=ASTRA::NoExists;
  surname=Qry.FieldAsString("surname");
  first_name=Qry.FieldAsString("first_name");
  second_name=Qry.FieldAsString("second_name");
  pr_multi=Qry.FieldAsInteger("pr_multi")!=0;
  type_rcpt=Qry.FieldAsString("type_rcpt");
  if (Qry.GetFieldIndex("scanned_attrs")>=0)
    scanned_attrs=Qry.FieldAsInteger("scanned_attrs");
  return *this;
}

long int TPaxDocItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!type.empty())                result|=DOC_TYPE_FIELD;
  // if (!subtype.empty())             result|=DOC_SUBTYPE_FIELD;
  if (!issue_country.empty())       result|=DOC_ISSUE_COUNTRY_FIELD;
  if (!no.empty())                  result|=DOC_NO_FIELD;
  if (!nationality.empty())         result|=DOC_NATIONALITY_FIELD;
  if (birth_date!=ASTRA::NoExists)  result|=DOC_BIRTH_DATE_FIELD;
  if (!gender.empty())             result|=DOC_GENDER_FIELD;
  if (expiry_date!=ASTRA::NoExists) result|=DOC_EXPIRY_DATE_FIELD;
  if (!surname.empty())             result|=DOC_SURNAME_FIELD;
  if (!first_name.empty())          result|=DOC_FIRST_NAME_FIELD;
  if (!second_name.empty())         result|=DOC_SECOND_NAME_FIELD;
  return result;
};

long int TPaxDocItem::getEqualAttrsFieldsMask(const TPaxDocItem &item) const
{
  long int result=NO_FIELDS;

  if (type == item.type)                   result|=DOC_TYPE_FIELD;
  // if (subtype == item.subtype)             result|=DOC_SUBTYPE_FIELD;
  if (issue_country == item.issue_country) result|=DOC_ISSUE_COUNTRY_FIELD;
  if (no == item.no)                       result|=DOC_NO_FIELD;
  if (nationality == item.nationality)     result|=DOC_NATIONALITY_FIELD;
  if (birth_date == item.birth_date)       result|=DOC_BIRTH_DATE_FIELD;
  if (gender == item.gender)             result|=DOC_GENDER_FIELD;
  if (expiry_date == item.expiry_date)     result|=DOC_EXPIRY_DATE_FIELD;
  if (surname == item.surname)             result|=DOC_SURNAME_FIELD;
  if (first_name == item.first_name)       result|=DOC_FIRST_NAME_FIELD;
  if (second_name == item.second_name)     result|=DOC_SECOND_NAME_FIELD;
  return result;
};

std::string TPaxDocItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat,
                                      TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << (type.empty()?"":ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang))
         << "/" << (issue_country.empty()?"":PaxDocCountryIdToPrefferedElem(issue_country, efmtCodeISOInter, lang))
         << "/" << (strictly_lat?transliter(convert_char_view(no, strictly_lat), 1, strictly_lat):no)
         << "/" << (nationality.empty()?"":PaxDocCountryIdToPrefferedElem(nationality, efmtCodeISOInter, lang))
         << "/" << (birth_date!=ASTRA::NoExists?DateTimeToStr(birth_date, "ddmmmyy", language_lat):"")
         << "/" << (gender.empty()?"":ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang)) << (inf_indicator?"I":"")
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", language_lat):"")
         << "/" << transliter(surname, 1, translit_lat)
         << "/" << transliter(first_name, 1, translit_lat)
         << "/" << transliter(second_name, 1, translit_lat)
         << "/" << (pr_multi?"H":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang)
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, issue_country, efmtCodeNative, lang)
         << "/" << no
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, nationality, efmtCodeNative, lang)
         << "/" << (birth_date!=ASTRA::NoExists?DateTimeToStr(birth_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang)
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << surname
         << "/" << first_name
         << "/" << second_name;
  return result.str();
}

std::string TPaxDocItem::full_name() const
{
  ostringstream s;
  s << surname;
  if (!first_name.empty())
    s << " " << first_name;
  if (!second_name.empty())
    s << " " << second_name;
  return s.str();
}

std::string TPaxDocItem::getSurnameWithInitials() const
{
  ostringstream s;
  s << surname << " ";
  if (!first_name.empty())
    s << first_name.substr(0, 1) << ".";
  if (!second_name.empty())
    s << second_name.substr(0, 1) << ".";
  return s.str();
}

bool TPaxDocoItem::needPseudoType() const
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  return (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION) &&
          doco_confirm && getNotEmptyFieldsMask()==NO_FIELDS);
}

const TPaxDocoItem& TPaxDocoItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place, "");
  TPaxDocCompoundType::toXML(docNode);
  if (needPseudoType())
    ReplaceTextChild(docNode, "type", DOCO_PSEUDO_TYPE);
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "issue_place", issue_place, "");
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "applic_country", applic_country, "");
  NewTextChild(docNode, "scanned_attrs", scanned_attrs, (int)NO_FIELDS);
  return *this;
}

const TPaxDocoItem& TPaxDocoItem::toWebXML(xmlNodePtr node,
                                           const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;

  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place);
  TPaxDocCompoundType::toWebXML(docNode, lang);
  NewTextChild(docNode, "no", no);
  NewTextChild(docNode, "issue_place", issue_place);
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "issue_date");
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "applic_country", paxDocCountryToWebXML(applic_country, lang));
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromXML(xmlNodePtr node)
{
  clear();
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!(reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION)))
    doco_confirm=true;
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromXML(node);
  birth_place=NodeAsStringFast("birth_place",node2,"");
  no=NodeAsStringFast("no",node2,"");
  issue_place=NodeAsStringFast("issue_place",node2,"");
  issue_date = date_fromXML(NodeAsStringFast("issue_date",node2,""));
  expiry_date=date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  applic_country=NodeAsStringFast("applic_country",node2,"");
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION))
  {
    if (type==DOCO_PSEUDO_TYPE && getNotEmptyFieldsMask()==DOCO_TYPE_FIELD)
      doco_confirm=true;
    else
      doco_confirm=(getNotEmptyFieldsMask()!=NO_FIELDS);
    if (type==DOCO_PSEUDO_TYPE) type.clear();
  }
  else doco_confirm=true;
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromWebXML(const xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  TPaxDocCompoundType::fromWebXML(node);

  xmlNodePtr node2=NodeAsNode("birth_place", node);
  birth_place=NodeAsStringFast("birth_place",node2);
  no=NodeAsStringFast("no",node2);
  issue_place=NodeAsStringFast("issue_place",node2);
  if (!NodeIsNULLFast("issue_date",node2))
    issue_date=NodeAsDateTimeFast("issue_date",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  applic_country=NodeAsStringFast("applic_country",node2);
  return *this;
};

TPaxDocoItem& TPaxDocoItem::fromMeridianXML(const xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromMeridianXML(node);
  birth_place=NodeAsStringFast("BIRTH_PLACE", node2, "");
  no=NodeAsStringFast("NO", node2, "");
  issue_place=NodeAsStringFast("ISSUE_PLACE", node2, "");
  issue_date = date_fromXML(NodeAsStringFast("ISSUE_DATE", node2, ""));
  expiry_date = date_fromXML(NodeAsStringFast("EXPIRY_DATE", node2, ""));
  applic_country=NodeAsStringFast("APPLIC_COUNTRY", node2, "");
  doco_confirm=true;
  return *this;
}


const TPaxDocoItem& TPaxDocoItem::toDB(TQuery &Qry) const
{
  TPaxDocCompoundType::toDB(Qry);
  Qry.SetVariable("birth_place", birth_place);
  Qry.SetVariable("no", no);
  Qry.SetVariable("issue_place", issue_place);
  if (issue_date!=ASTRA::NoExists)
    Qry.SetVariable("issue_date", issue_date);
  else
    Qry.SetVariable("issue_date", FNull);
  if (expiry_date!=ASTRA::NoExists)
    Qry.SetVariable("expiry_date", expiry_date);
  else
    Qry.SetVariable("expiry_date", FNull);
  Qry.SetVariable("applic_country", applic_country);
  if (Qry.GetVariableIndex("scanned_attrs")>=0)
    Qry.SetVariable("scanned_attrs", (int)scanned_attrs);
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxDocCompoundType::fromDB(Qry);
  birth_place=Qry.FieldAsString("birth_place");
  no=Qry.FieldAsString("no");
  issue_place=Qry.FieldAsString("issue_place");
  if (!Qry.FieldIsNULL("issue_date"))
    issue_date=Qry.FieldAsDateTime("issue_date");
  else
    issue_date=ASTRA::NoExists;
  if (!Qry.FieldIsNULL("expiry_date"))
    expiry_date=Qry.FieldAsDateTime("expiry_date");
  else
    expiry_date=ASTRA::NoExists;
  applic_country=Qry.FieldAsString("applic_country");
  if (Qry.GetFieldIndex("scanned_attrs")>=0)
    scanned_attrs=Qry.FieldAsInteger("scanned_attrs");
  return *this;
}

long int TPaxDocoItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!birth_place.empty())         result|=DOCO_BIRTH_PLACE_FIELD;
  if (!type.empty())                result|=DOCO_TYPE_FIELD;
  // if (!subtype.empty())             result|=DOCO_SUBTYPE_FIELD;
  if (!no.empty())                  result|=DOCO_NO_FIELD;
  if (!issue_place.empty())         result|=DOCO_ISSUE_PLACE_FIELD;
  if (issue_date!=ASTRA::NoExists)  result|=DOCO_ISSUE_DATE_FIELD;
  if (expiry_date!=ASTRA::NoExists) result|=DOCO_EXPIRY_DATE_FIELD;
  if (!applic_country.empty())      result|=DOCO_APPLIC_COUNTRY_FIELD;
  return result;
};

long int TPaxDocoItem::getEqualAttrsFieldsMask(const TPaxDocoItem &item) const
{
  long int result=NO_FIELDS;

  if (birth_place == item.birth_place)       result|=DOCO_BIRTH_PLACE_FIELD;
  if (type == item.type)                     result|=DOCO_TYPE_FIELD;
  // if (subtype == item.subtype)               result|=DOCO_SUBTYPE_FIELD;
  if (no == item.no)                         result|=DOCO_NO_FIELD;
  if (issue_place == item.issue_place)       result|=DOCO_ISSUE_PLACE_FIELD;
  if (issue_date == item.issue_date)         result|=DOCO_ISSUE_DATE_FIELD;
  if (expiry_date == item.expiry_date)       result|=DOCO_EXPIRY_DATE_FIELD;
  if (applic_country == item.applic_country) result|=DOCO_APPLIC_COUNTRY_FIELD;
  return result;
};

void TPaxDocoItem::ReplacePunctSymbols()
{
  transform(birth_place.begin(), birth_place.end(), birth_place.begin(), APIS::ReplacePunctSymbol);
  transform(issue_place.begin(), issue_place.end(), issue_place.begin(), APIS::ReplacePunctSymbol);
}

std::string TPaxDocoItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat,
                                       TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << transliter(birth_place, 1, translit_lat)
         << "/" << (type.empty()?"":ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang))
         << "/" << (strictly_lat?transliter(convert_char_view(no, strictly_lat), 1, strictly_lat):no)
         << "/" << transliter(issue_place, 1, translit_lat)
         << "/" << (issue_date!=ASTRA::NoExists?DateTimeToStr(issue_date, "ddmmmyy", language_lat):"")
         << "/" << (applic_country.empty()?"":PaxDocCountryIdToPrefferedElem(applic_country, efmtCodeISOInter, lang))
         << "/" << (inf_indicator?"I":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocoItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << birth_place
         << "/" << ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang)
         << "/" << no
         << "/" << issue_place
         << "/" << (issue_date!=ASTRA::NoExists?DateTimeToStr(issue_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, applic_country, efmtCodeNative, lang)
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"");
  return result.str();
}

const TPaxDocaItem& TPaxDocaItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docaNode=NewTextChild(node,"doca");
  NewTextChild(docaNode, "type", type);
  NewTextChild(docaNode, "country", country, "");
  NewTextChild(docaNode, "address", address, "");
  NewTextChild(docaNode, "city", city, "");
  NewTextChild(docaNode, "region", region, "");
  NewTextChild(docaNode, "postal_code", postal_code, "");
  return *this;
};

TPaxDocaItem& TPaxDocaItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  type=NodeAsStringFast("type",node2);
  country=NodeAsStringFast("country",node2,"");
  address=NodeAsStringFast("address",node2,"");
  city=NodeAsStringFast("city",node2,"");
  region=NodeAsStringFast("region",node2,"");
  postal_code=NodeAsStringFast("postal_code",node2,"");
  return *this;
};

TPaxDocaItem& TPaxDocaItem::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  type = upperc(NodeAsStringFast("TYPE", node2, ""));
  country=NodeAsStringFast("COUNTRY", node2, "");
  address=NodeAsStringFast("ADDRESS", node2, "");
  city=NodeAsStringFast("CITY", node2, "");
  region=NodeAsStringFast("REGION", node2, "");
  postal_code=NodeAsStringFast("POSTAL_CODE", node2, "");
  return *this;
}

const TPaxDocaItem& TPaxDocaItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("type", type);
  Qry.SetVariable("country", country);
  Qry.SetVariable("address", address);
  Qry.SetVariable("city", city);
  Qry.SetVariable("region", region);
  Qry.SetVariable("postal_code", postal_code);
  return *this;
};

TPaxDocaItem& TPaxDocaItem::fromDB(TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
  country=Qry.FieldAsString("country");
  address=Qry.FieldAsString("address");
  city=Qry.FieldAsString("city");
  region=Qry.FieldAsString("region");
  postal_code=Qry.FieldAsString("postal_code");
  return *this;
};

long int TPaxDocaItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!type.empty())        result|=DOCA_TYPE_FIELD;
  if (!country.empty())     result|=DOCA_COUNTRY_FIELD;
  if (!address.empty())     result|=DOCA_ADDRESS_FIELD;
  if (!city.empty())        result|=DOCA_CITY_FIELD;
  if (!region.empty())      result|=DOCA_REGION_FIELD;
  if (!postal_code.empty()) result|=DOCA_POSTAL_CODE_FIELD;
  return result;
};

long int TPaxDocaItem::getEqualAttrsFieldsMask(const TPaxDocaItem &item) const
{
  long int result=NO_FIELDS;

  if (type == item.type)               result|=DOCA_TYPE_FIELD;
  if (country == item.country)         result|=DOCA_COUNTRY_FIELD;
  if (address == item.address)         result|=DOCA_ADDRESS_FIELD;
  if (city == item.city)               result|=DOCA_CITY_FIELD;
  if (region == item.region)           result|=DOCA_REGION_FIELD;
  if (postal_code == item.postal_code) result|=DOCA_POSTAL_CODE_FIELD;
  return result;
};

void TPaxDocaItem::ReplacePunctSymbols()
{
  transform(address.begin(), address.end(), address.begin(), APIS::ReplacePunctSymbol);
  transform(city.begin(), city.end(), city.begin(), APIS::ReplacePunctSymbol);
  transform(region.begin(), region.end(), region.begin(), APIS::ReplacePunctSymbol);
  transform(postal_code.begin(), postal_code.end(), postal_code.begin(), APIS::ReplacePunctSymbol);
}

std::string TPaxDocaItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat,
                                       TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << type
         << "/" << (country.empty()?"":PaxDocCountryIdToPrefferedElem(country, efmtCodeISOInter, lang))
         << "/" << transliter(address, 1, translit_lat)
         << "/" << transliter(city, 1, translit_lat)
         << "/" << transliter(region, 1, translit_lat)
         << "/" << transliter(postal_code, 1, translit_lat)
         << "/" << (inf_indicator?"I":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocaItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << type
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, country, efmtCodeNative, lang)
         << "/" << address
         << "/" << city
         << "/" << region
         << "/" << postal_code;
  return result.str();
}

TAPIType TPaxDocaItem::apiType() const
{
  // throw exception вместо apiUnknown?
  TAPIType result = apiUnknown;
  if (type == "B") result = apiDocaB;
  if (type == "R") result = apiDocaR;
  if (type == "D") result = apiDocaD;
  return result;
}

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc)
{
  return LoadPaxDoc(ASTRA::NoExists, pax_id, doc);
};

bool LoadPaxDoc(TDateTime part_key, int pax_id, TPaxDocItem &doc)
{
  doc.clear();
  const char* sql=
    "SELECT * FROM pax_doc WHERE pax_id=:pax_id";
  const char* sql_arx=
    "SELECT * FROM arx_pax_doc WHERE part_key=:part_key AND pax_id=:pax_id";
  const char *sql_result = NULL;
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
  {
      QryParams << QParam("part_key", otDate, part_key);
      sql_result = sql_arx;
  }
  else
      sql_result = sql;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxDocQry(sql_result, QryParams);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  return !doc.empty();
};

std::string GetPaxDocStr(TDateTime part_key,
                         int pax_id,
                         bool with_issue_country,
                         const string &lang)
{
  ostringstream result;

  TPaxDocItem doc;
  if (LoadPaxDoc(part_key, pax_id, doc) && !doc.no.empty())
  {
    result << doc.no;
    if (with_issue_country && !doc.issue_country.empty())
    {
      if (lang.empty())
        result << " " << ElemIdToPrefferedElem(etPaxDocCountry, doc.issue_country, efmtCodeNative, TReqInfo::Instance()->desk.lang, true);
      else
        result << " " << ElemIdToPrefferedElem(etPaxDocCountry, doc.issue_country, efmtCodeNative, lang, true);
    };
  };
  return result.str();
};

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc)
{
  doc.clear();
  const char* sql1=
    "SELECT * "
    "FROM crs_pax_doc "
    "WHERE pax_id=:pax_id "
    "ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no NULLS LAST";
  const char* sql2=
    "SELECT report.get_PSPT2(:pax_id) AS no FROM dual";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxDocQry(sql1, QryParams);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  else
  {
    TCachedQuery GetPSPT2Qry(sql2, QryParams);
    GetPSPT2Qry.get().Execute();
    if (!GetPSPT2Qry.get().Eof && !GetPSPT2Qry.get().FieldIsNULL("no"))
    {
      doc.no=GetPSPT2Qry.get().FieldAsString("no");
    };
  };
  return !doc.empty();
};

boost::optional<TPaxDocoItem> LoadPaxDoco(int pax_id)
{
  TPaxDocoItem doc;
  if (LoadPaxDoco(pax_id, doc))
    return doc;
  else
    return boost::none;
}

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc)
{
  return LoadPaxDoco(ASTRA::NoExists, pax_id, doc);
};

bool LoadPaxDoco(TDateTime part_key, int pax_id, TPaxDocoItem &doc)
{
  doc.clear();
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
    QryParams << QParam("part_key", otDate, part_key);
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxQry(part_key!=ASTRA::NoExists?
                      "SELECT doco_confirm FROM arx_pax WHERE part_key=:part_key AND pax_id=:pax_id":
                      "SELECT doco_confirm FROM pax WHERE pax_id=:pax_id",
                      QryParams);
  doc.doco_confirm=false;
  PaxQry.get().Execute();
  if (!PaxQry.get().Eof)
    doc.doco_confirm=PaxQry.get().FieldIsNULL("doco_confirm") ||
                 PaxQry.get().FieldAsInteger("doco_confirm")!=0;

  TCachedQuery PaxDocQry(part_key!=ASTRA::NoExists?
                         "SELECT * FROM arx_pax_doco WHERE part_key=:part_key AND pax_id=:pax_id":
                         "SELECT * FROM pax_doco WHERE pax_id=:pax_id",
                         QryParams);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  return !doc.empty();
};

bool LoadCrsPaxVisa(int pax_id, TPaxDocoItem &doc)
{
  doc.clear();
  const char* sql=
    "SELECT birth_place, type, no, issue_place, issue_date, NULL AS expiry_date, applic_country "
    "FROM crs_pax_doco "
    "WHERE pax_id=:pax_id AND rem_code='DOCO' AND type='V' "
    "ORDER BY no ";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxDocQry(sql, QryParams);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  return !doc.empty();
};

void ConvertDoca(CheckIn::TDocaMap doca_map,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD)
{
  // присвоение полю type было в старом коде!
  docaB.clear();
  docaB.type="B";
  docaR.clear();
  docaR.type="R";
  docaD.clear();
  docaD.type="D";
  if (doca_map.count(apiDocaB)) docaB = doca_map[apiDocaB];
  if (doca_map.count(apiDocaR)) docaR = doca_map[apiDocaR];
  if (doca_map.count(apiDocaD)) docaD = doca_map[apiDocaD];
};

boost::optional<TPaxDocaItem> LoadPaxDoca(int pax_id, TDocaType type)
{
  TPaxDocaItem doc;
  if (LoadPaxDoca(pax_id, type, doc))
    return doc;
  else
    return boost::none;
}

bool LoadPaxDoca(int pax_id, CheckIn::TDocaMap &doca_map)
{
  return LoadPaxDoca(ASTRA::NoExists, pax_id, doca_map);
};

bool LoadPaxDoca(int pax_id, TDocaType type, TPaxDocaItem &doca)
{
  return LoadPaxDoca(ASTRA::NoExists, pax_id, type, doca);
};

bool LoadPaxDoca(TDateTime part_key, int pax_id, CheckIn::TDocaMap &doca_map)
{
  doca_map.Clear();
  const char* sql = "SELECT * FROM pax_doca WHERE pax_id=:pax_id";
  const char* sql_arx = "SELECT * FROM arx_pax_doca WHERE part_key=:part_key AND pax_id=:pax_id";
  const char* sql_result = nullptr;
  QParams QryParams;
  if (part_key != ASTRA::NoExists)
  {
    QryParams << QParam("part_key", otDate, part_key);
    sql_result = sql_arx;
  }
  else
  {
    sql_result = sql;
  }
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxDocQry(sql_result, QryParams);
  for(PaxDocQry.get().Execute(); !PaxDocQry.get().Eof; PaxDocQry.get().Next())
  {
    TPaxDocaItem docaItem;
    docaItem.fromDB(PaxDocQry.get());
    if (!docaItem.empty_without_type() && docaItem.apiType() != apiUnknown) doca_map[docaItem.apiType()] = docaItem;
  }
  for (CheckIn::TDocaMap::const_iterator i = doca_map.begin(); i != doca_map.end(); ++i)
    if (not i->second.empty()) return true;
  return false;
};

bool LoadPaxDoca(TDateTime part_key, int pax_id, TDocaType type, TPaxDocaItem &doca)
{
  doca.clear();
  const char* sql=
    "SELECT * FROM pax_doca WHERE pax_id=:pax_id AND type=:type";
  const char* sql_arx=
    "SELECT * FROM arx_pax_doca WHERE part_key=:part_key AND pax_id=:pax_id AND type=:type";
  const char *sql_result = NULL;
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
  {
      QryParams << QParam("part_key", otDate, part_key);
      sql_result = sql_arx;
  }
  else
      sql_result = sql;
  QryParams << QParam("pax_id", otInteger, pax_id);
  switch(type)
  {
    case docaDestination: QryParams << QParam("type", otString, "D"); break;
    case docaResidence:   QryParams << QParam("type", otString, "R"); break;
    case docaBirth:       QryParams << QParam("type", otString, "B"); break;
  };
  TCachedQuery PaxDocQry(sql_result, QryParams);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doca.fromDB(PaxDocQry.get());
  return !doca.empty();
};

bool LoadCrsPaxDoca(int pax_id, CheckIn::TDocaMap &doca_map)
{
  doca_map.Clear();
  const char* sql=
    "SELECT type, country, address, city, region, postal_code "
    "FROM crs_pax_doca "
    "WHERE pax_id=:pax_id AND rem_code='DOCA' AND type IS NOT NULL "
    "ORDER BY type, address ";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery PaxDocaQry(sql, QryParams);
  PaxDocaQry.get().Execute();
  string prior_type;
  for(;!PaxDocaQry.get().Eof;PaxDocaQry.get().Next())
  {
    if (prior_type!=PaxDocaQry.get().FieldAsString("type"))
    {
      TPaxDocaItem doca_item;
      doca_item.fromDB(PaxDocaQry.get());
      if (!doca_item.empty_without_type() && doca_item.apiType() != apiUnknown) doca_map[doca_item.apiType()] = doca_item;
      prior_type=PaxDocaQry.get().FieldAsString("type");
    };
  };
  for (CheckIn::TDocaMap::const_iterator i = doca_map.begin(); i != doca_map.end(); ++i)
    if (not i->second.empty()) return true;
  return false;
};

void SavePaxDoc(int pax_id, const TPaxDocItem &doc, TQuery& PaxDocQry)
{
  const char* sql=
        "BEGIN "
        "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
        "  IF :only_delete=0 THEN "
        "    SELECT MAX(type_rcpt) INTO :type_rcpt "
        "    FROM crs_pax_doc "
        "    WHERE pax_id=:pax_id AND no=:no AND type_rcpt IS NOT NULL AND rownum<2; "
        "    INSERT INTO pax_doc "
        "      (pax_id,type,subtype,issue_country,no,nationality,birth_date,gender,expiry_date, "
        "       surname,first_name,second_name,pr_multi,type_rcpt,scanned_attrs) "
        "    VALUES "
        "      (:pax_id,:type,:subtype,:issue_country,:no,:nationality,:birth_date,:gender,:expiry_date, "
        "       :surname,:first_name,:second_name,:pr_multi,:type_rcpt,:scanned_attrs); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("type",otString);
    PaxDocQry.DeclareVariable("subtype",otString);
    PaxDocQry.DeclareVariable("issue_country",otString);
    PaxDocQry.DeclareVariable("no",otString);
    PaxDocQry.DeclareVariable("nationality",otString);
    PaxDocQry.DeclareVariable("birth_date",otDate);
    PaxDocQry.DeclareVariable("gender",otString);
    PaxDocQry.DeclareVariable("expiry_date",otDate);
    PaxDocQry.DeclareVariable("surname",otString);
    PaxDocQry.DeclareVariable("first_name",otString);
    PaxDocQry.DeclareVariable("second_name",otString);
    PaxDocQry.DeclareVariable("pr_multi",otInteger);
    PaxDocQry.DeclareVariable("type_rcpt",otString);
    PaxDocQry.DeclareVariable("scanned_attrs",otInteger);
    PaxDocQry.DeclareVariable("only_delete",otInteger);
  };

  doc.toDB(PaxDocQry);
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.SetVariable("only_delete",(int)doc.empty());
  PaxDocQry.Execute();
};

void SavePaxDoco(int pax_id, const TPaxDocoItem &doc, TQuery& PaxDocQry)
{
  const char* sql=
        "BEGIN "
        "  DELETE FROM pax_doco WHERE pax_id=:pax_id; "
        "  IF :only_delete=0 THEN "
        "    INSERT INTO pax_doco "
        "      (pax_id,birth_place,type,subtype,no,issue_place,issue_date,expiry_date,applic_country,scanned_attrs) "
        "    VALUES "
        "      (:pax_id,:birth_place,:type,:subtype,:no,:issue_place,:issue_date,:expiry_date,:applic_country,:scanned_attrs); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("birth_place",otString);
    PaxDocQry.DeclareVariable("type",otString);
    PaxDocQry.DeclareVariable("subtype",otString);
    PaxDocQry.DeclareVariable("no",otString);
    PaxDocQry.DeclareVariable("issue_place",otString);
    PaxDocQry.DeclareVariable("issue_date",otDate);
    PaxDocQry.DeclareVariable("expiry_date",otDate);
    PaxDocQry.DeclareVariable("applic_country",otString);
    PaxDocQry.DeclareVariable("scanned_attrs",otInteger);
    PaxDocQry.DeclareVariable("only_delete",otInteger);
  };

  doc.toDB(PaxDocQry);
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.SetVariable("only_delete",(int)doc.empty());
  PaxDocQry.Execute();

  TCachedQuery PaxQry("UPDATE pax SET doco_confirm=:doco_confirm WHERE pax_id=:pax_id",
                      QParams() << QParam("pax_id", otInteger, pax_id)
                                << QParam("doco_confirm", otInteger, (int)doc.doco_confirm));
  PaxQry.get().Execute();
};

void SavePaxDoca(int pax_id, const CheckIn::TDocaMap &doca_map, TQuery& PaxDocaQry)
{
  list<TPaxDocaItem> doca2;
  for (CheckIn::TDocaMap::const_iterator idm = doca_map.begin(); idm != doca_map.end(); ++idm)
    if (!idm->second.empty()) doca2.push_back(idm->second);

  const char* sql=
        "BEGIN "
        "  IF :first_iteration<>0 THEN "
        "    DELETE FROM pax_doca WHERE pax_id=:pax_id; "
        "    :first_iteration:=0; "
        "  END IF; "
        "  IF :only_delete=0 THEN "
        "    INSERT INTO pax_doca "
        "      (pax_id,type,country,address,city,region,postal_code) "
        "    VALUES "
        "      (:pax_id,:type,:country,:address,:city,:region,:postal_code); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocaQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocaQry.Clear();
    PaxDocaQry.SQLText=sql;
    PaxDocaQry.DeclareVariable("pax_id",otInteger);
    PaxDocaQry.DeclareVariable("type",otString);
    PaxDocaQry.DeclareVariable("country",otString);
    PaxDocaQry.DeclareVariable("address",otString);
    PaxDocaQry.DeclareVariable("city",otString);
    PaxDocaQry.DeclareVariable("region",otString);
    PaxDocaQry.DeclareVariable("postal_code",otString);
    PaxDocaQry.DeclareVariable("only_delete",otInteger);
    PaxDocaQry.DeclareVariable("first_iteration",otInteger);
  };

  PaxDocaQry.SetVariable("pax_id",pax_id);
  PaxDocaQry.SetVariable("first_iteration",(int)true);
  if (!doca2.empty())
  {
    for(list<TPaxDocaItem>::const_iterator d=doca2.begin(); d!=doca2.end(); ++d)
    {
      d->toDB(PaxDocaQry);
      PaxDocaQry.SetVariable("only_delete",(int)d->empty());
      PaxDocaQry.Execute();
    };
  }
  else
  {
    TPaxDocaItem().toDB(PaxDocaQry);
    PaxDocaQry.SetVariable("only_delete",(int)true);
    PaxDocaQry.Execute();
  };

};

const TPaxItem& TPaxItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr paxNode=node;
  NewTextChild(paxNode, "pax_id", id);
  NewTextChild(paxNode, "surname", surname);
  NewTextChild(paxNode, "name", name);
  NewTextChild(paxNode, "pers_type", EncodePerson(pers_type));
  NewTextChild(paxNode, "crew_type", CrewTypes().encode(crew_type));
  NewTextChild(paxNode, "seat_no", seat_no);
  NewTextChild(paxNode, "seat_type", seat_type);
  NewTextChild(paxNode, "seats", seats);
  NewTextChild(paxNode, "refuse", refuse);
  NewTextChild(paxNode, "reg_no", reg_no);
  NewTextChild(paxNode, "subclass", subcl);
  if (bag_pool_num!=ASTRA::NoExists)
    NewTextChild(paxNode, "bag_pool_num", bag_pool_num);
  else
    NewTextChild(paxNode, "bag_pool_num");
  NewTextChild(paxNode, "tid", tid);

  if (TknExists) tkn.toXML(paxNode);
  if (DocExists) doc.toXML(paxNode);
  if (DocoExists || doco.needPseudoType()) doco.toXML(paxNode);
  if (DocaExists)
  {
    xmlNodePtr docaNode=NewTextChild(paxNode, "addresses");
    for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
      d->second.toXML(docaNode);
  };
  return *this;
};

TPaxItem& TPaxItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  TReqInfo *reqInfo=TReqInfo::Instance();

  if (!NodeIsNULLFast("pax_id",node2))
    id=NodeAsIntegerFast("pax_id",node2);
  surname=NodeAsStringFast("surname",node2);
  name=NodeAsStringFast("name",node2);
  tid=NodeAsIntegerFast("tid",node2,ASTRA::NoExists);
  PaxUpdatesPending=GetNodeFast("refuse",node2)!=NULL;
  if (tid==ASTRA::NoExists)
  {
    //новая регистрация
    seat_no=NodeAsStringFast("seat_no",node2);
    seat_type=NodeAsStringFast("seat_type",node2);
    seats=NodeAsIntegerFast("seats",node2);
    if (reqInfo->client_type==ASTRA::ctPNL)
      reg_no=NodeAsIntegerFast("reg_no",node2);
  };

  if (tid==ASTRA::NoExists || PaxUpdatesPending)
  {
    pers_type=DecodePerson(NodeAsStringFast("pers_type",node2));
    if (!NodeIsNULLFast("crew_type",node2, true))
      crew_type=CrewTypes().decode(NodeAsStringFast("crew_type",node2));
    if (PaxUpdatesPending)
      refuse=NodeAsStringFast("refuse",node2);

    if (tid==ASTRA::NoExists ||
        (PaxUpdatesPending && reqInfo->client_type==ASTRA::ctTerm))
    {
      tkn.fromXML(node);
      TknExists=true;
      if (!NodeIsNULLFast("bag_pool_num",node2))
        bag_pool_num=NodeAsIntegerFast("bag_pool_num",node2);
    };

    if (refuse.empty() && api_doc_applied())
    {
      if (reqInfo->client_type==ASTRA::ctTerm)
      {
        //терминал
        doc.fromXML(GetNodeFast("document",node2));
        doco.fromXML(GetNodeFast("doco",node2));
        DocExists=true;
        DocoExists=true;

        xmlNodePtr docaNode=GetNodeFast("addresses",node2);
        if (docaNode!=NULL)
        {
          for(docaNode=docaNode->children; docaNode!=NULL; docaNode=docaNode->next)
          {
            TPaxDocaItem docaItem;
            docaItem.fromXML(docaNode);
            if (docaItem.empty()) continue;
            if (docaItem.apiType() != apiUnknown) doca_map[docaItem.apiType()] = docaItem;
          };
        };
        DocaExists=true;
      }
      else
      {
        //киоски и веб
        xmlNodePtr docNode=GetNodeFast("document",node2);
        doc.fromXML(docNode);
        xmlNodePtr docoNode=GetNodeFast("doco",node2);
        doco.fromXML(docoNode);
        xmlNodePtr docaNode=GetNodeFast("addresses",node2);
        if (docaNode!=NULL)
        {
          for(docaNode=docaNode->children; docaNode!=NULL; docaNode=docaNode->next)
          {
            TPaxDocaItem docaItem;
            docaItem.fromXML(docaNode);
            if (docaItem.empty()) continue;
            if (docaItem.apiType() != apiUnknown) doca_map[docaItem.apiType()] = docaItem;
          };
        };

        DocExists=(tid==ASTRA::NoExists || docNode!=NULL);
        DocoExists=(tid==ASTRA::NoExists || docoNode!=NULL);
        DocaExists=(tid==ASTRA::NoExists || docaNode!=NULL);
      };
    };
  };

  subcl=NodeAsStringFast("subclass",node2,"");
  return *this;
};

const TComplexClass& TComplexClass::toDB(TQuery &Qry, const std::string& fieldPrefix) const
{
  if (Qry.GetVariableIndex(fieldPrefix+"subclass")>=0)
    Qry.SetVariable(fieldPrefix+"subclass", subcl);
  if (Qry.GetVariableIndex(fieldPrefix+"class")>=0)
    Qry.SetVariable(fieldPrefix+"class", cl);
  if (Qry.GetVariableIndex(fieldPrefix+"class_grp")>=0)
    cl_grp!=ASTRA::NoExists?Qry.SetVariable(fieldPrefix+"class_grp", cl_grp):
                            Qry.SetVariable(fieldPrefix+"class_grp", FNull);
  return *this;
}

const TPaxItem& TPaxItem::toDB(TQuery &Qry) const
{
  id!=ASTRA::NoExists?Qry.SetVariable("pax_id", id):
                      Qry.SetVariable("pax_id", FNull);
  if (Qry.GetVariableIndex("surname")>=0)
    Qry.SetVariable("surname", surname);
  if (Qry.GetVariableIndex("name")>=0)
    Qry.SetVariable("name", name);
  if (Qry.GetVariableIndex("pers_type")>=0)
    Qry.SetVariable("pers_type", EncodePerson(pers_type));
  if (Qry.GetVariableIndex("crew_type")>=0)
    Qry.SetVariable("crew_type", CrewTypes().encode(crew_type));
  if (Qry.GetVariableIndex("is_jmp")>=0)
    Qry.SetVariable("is_jmp", (int)is_jmp);
  if (Qry.GetVariableIndex("seat_type")>=0)
    Qry.SetVariable("seat_type", seat_type);
  if (Qry.GetVariableIndex("seats")>=0)
    Qry.SetVariable("seats", seats);
  if (Qry.GetVariableIndex("refuse")>=0)
    Qry.SetVariable("refuse", refuse);
  if (Qry.GetVariableIndex("pr_brd")>=0)
    Qry.SetVariable("pr_brd", (int)pr_brd);
  if (Qry.GetVariableIndex("pr_exam")>=0)
    Qry.SetVariable("pr_exam", (int)pr_exam);
  if (Qry.GetVariableIndex("wl_type")>=0)
    Qry.SetVariable("wl_type", wl_type);
  if (Qry.GetVariableIndex("reg_no")>=0)
    Qry.SetVariable("reg_no", reg_no);
  if (Qry.GetVariableIndex("subclass")>=0)
    Qry.SetVariable("subclass", subcl);
  cabin.toDB(Qry, "cabin_");
  if (Qry.GetVariableIndex("bag_pool_num")>=0)
    bag_pool_num!=ASTRA::NoExists?Qry.SetVariable("bag_pool_num", bag_pool_num):
                                  Qry.SetVariable("bag_pool_num", FNull);
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  if (Qry.GetVariableIndex("ticket_no")>=0)
    tkn.toDB(Qry);
  return *this;
};

ASTRA::TGender::Enum TSimplePaxItem::genderFromDB(TQuery &Qry)
{
  if (Qry.FieldIsNULL("is_female"))
    return ASTRA::TGender::Unknown;
  else if (Qry.FieldAsInteger("is_female")!=0)
    return ASTRA::TGender::Female;
  else
    return ASTRA::TGender::Male;
}

ASTRA::TTrickyGender::Enum TSimplePaxItem::getTrickyGender(ASTRA::TPerson pers_type, ASTRA::TGender::Enum gender)
{
  ASTRA::TTrickyGender::Enum result=ASTRA::TTrickyGender::Male;
  switch(pers_type)
  {
    case ASTRA::adult:
      if (gender==ASTRA::TGender::Female) result=ASTRA::TTrickyGender::Female;
      break;
    case ASTRA::child:
      result=ASTRA::TTrickyGender::Child;
      break;
    case ASTRA::baby:
      result=ASTRA::TTrickyGender::Infant;
      break;
    default:
      break;
  };
  return result;
}

TComplexClass& TComplexClass::fromDB(TQuery &Qry, const std::string& fieldPrefix)
{
  clear();
  subcl=Qry.FieldAsString(fieldPrefix+"subclass");
  cl=Qry.FieldAsString(fieldPrefix+"class");
  cl_grp=Qry.FieldIsNULL(fieldPrefix+"class_grp")?ASTRA::NoExists:Qry.FieldAsInteger(fieldPrefix+"class_grp");
  return *this;
}

TSimplePaxItem& TSimplePaxItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  grp_id=Qry.FieldAsInteger("grp_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
  if(Qry.GetFieldIndex("crew_type") >= 0)
      crew_type = CrewTypes().decode(Qry.FieldAsString("crew_type"));
  is_jmp=Qry.FieldAsInteger("is_jmp")!=0;
  if (Qry.GetFieldIndex("seat_no") >= 0)
    seat_no=Qry.FieldAsString("seat_no");
  seat_type=Qry.FieldAsString("seat_type");
  seats=Qry.FieldAsInteger("seats");
  refuse=Qry.FieldAsString("refuse");
  pr_brd=Qry.FieldAsInteger("pr_brd")!=0;
  pr_exam=Qry.FieldAsInteger("pr_exam")!=0;
  reg_no=Qry.FieldAsInteger("reg_no");
  subcl=Qry.FieldAsString("subclass");
  cabin.fromDB(Qry, "cabin_");
  bag_pool_num=Qry.FieldIsNULL("bag_pool_num")?ASTRA::NoExists:Qry.FieldAsInteger("bag_pool_num");
  tid=Qry.FieldAsInteger("tid");
  tkn.fromDB(Qry);
  TknExists=true;
  gender=genderFromDB(Qry);
  return *this;
}

TSimplePaxItem& TSimplePaxItem::fromDBCrs(TQuery &Qry, bool withTkn)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (isTest())
  {
    pers_type=ASTRA::adult;
    seats=1;
  }
  else
  {
    pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
    seat_type=Qry.FieldAsString("seat_type");
    seats=Qry.FieldAsInteger("seats");
  }
  if (Qry.GetFieldIndex("seat_no")>=0)
    seat_no = Qry.FieldAsString("seat_no");
  if (Qry.GetFieldIndex("subclass")>=0)
    subcl = Qry.FieldAsString("subclass"); //!!!vlad upgrade
  if (Qry.GetFieldIndex("reg_no")>=0)
    reg_no = Qry.FieldIsNULL("reg_no")?ASTRA::NoExists:Qry.FieldAsInteger("reg_no");
  if (withTkn)
  {
    if (isTest())
    {
      tkn.no=Qry.FieldAsString("tkn_no");
      if (!tkn.no.empty())
      {
        tkn.coupon=1;
        tkn.rem="TKNE";
      };
    }
    else LoadCrsPaxTkn(id, tkn);
  }
  TknExists=withTkn;
  return *this;
}

bool TSimplePaxItem::getByPaxId(int pax_id, TDateTime part_key)
{
  clear();
  QParams QryParams;
  string SQLText;
  if(part_key == ASTRA::NoExists) {
      SQLText = "SELECT * FROM pax WHERE pax_id=:pax_id";
      QryParams << QParam("pax_id", otInteger, pax_id);
  } else {
      SQLText = "SELECT * FROM arx_pax WHERE part_key = :part_key and pax_id=:pax_id";
      QryParams
          << QParam("pax_id", otInteger, pax_id)
          << QParam("part_key", otDate, part_key);
  }
  TCachedQuery PaxQry(SQLText, QryParams);
  PaxQry.get().Execute();
  if (PaxQry.get().Eof) return false;
  fromDB(PaxQry.get());
  return true;
}

TPaxItem& TPaxItem::fromDB(TQuery &Qry)
{
  clear();
  TSimplePaxItem::fromDB(Qry);
  DocExists=CheckIn::LoadPaxDoc(id, doc);
  DocoExists=CheckIn::LoadPaxDoco(id, doco);
  DocaExists=CheckIn::LoadPaxDoca(id, doca_map);
  return *this;
};

int TPaxItem::is_female() const
{
  if (!DocExists) return ASTRA::NoExists;
  return CheckIn::is_female(doc.gender, name);
};

int is_female(const string &pax_doc_gender, const string &pax_name)
{
  int result=ASTRA::NoExists;
  if (!pax_doc_gender.empty())
  {
    if (pax_doc_gender.substr(0,1)=="M") result=0;
    if (pax_doc_gender.substr(0,1)=="F") result=1;
  };
  if (result==ASTRA::NoExists)
  {
    TPaxNameTitle info;
    string name_tmp(pax_name);
    GetPaxNameTitle(name_tmp, false, info);
    if (!info.empty()) result=(int)info.is_female;
  };
  return result;
};

std::string TSimplePaxItem::full_name() const
{
  ostringstream s;
  s << surname;
  if (!name.empty())
    s << " " << name;
  return s.str();
};

bool TSimplePaxItem::isCBBG() const
{
  return name=="CBBG";
}

bool TSimplePaxItem::api_doc_applied() const
{
  return !isCBBG();
}

bool TSimplePaxItem::upward_within_bag_pool(const TSimplePaxItem& pax) const
{
  int res;
  res=int(refuse!=ASTRA::refuseAgentError)-int(pax.refuse!=ASTRA::refuseAgentError);
  if (res==0) res=int(pers_type==ASTRA::adult || pers_type==ASTRA::child)-
                  int(pax.pers_type==ASTRA::adult || pax.pers_type==ASTRA::child);
  if (res==0) res=int(seats>0)-int(pax.seats>0);
  if (res==0) res=int(refuse.empty())-int(pax.refuse.empty());
  //специально чтобы при прочих равных выбрать ВЗ:
  if (res==0) res=-(int(pers_type)-int(pax.pers_type));

  return res>0;
}

void TSimplePaxItem::UpdTid(int pax_id)
{
  TCachedQuery Qry("UPDATE pax SET tid=cycle_tid__seq.nextval WHERE pax_id=:pax_id",
                   QParams() << QParam("pax_id", otInteger, pax_id));
  Qry.get().Execute();
}

std::string TSimplePaxItem::checkInStatus() const
{
  if (id==ASTRA::NoExists) return "unknown";
  if (!refuse.empty()) return "refused";
  if (pr_brd) return "boarded";
  if (grp_id!=ASTRA::NoExists) return "checked";
  return "not_checked";
}

TComplexClass TSimplePaxItem::getCrsClass(bool onlyIfClassChange) const
{
  TComplexClass result;

  if (id==ASTRA::NoExists) return result;

  TCachedQuery Qry(
    "SELECT NVL(crs_pax.etick_class, NVL(crs_pax.orig_class, crs_pnr.class)) AS orig_class, "
    "       crs_pnr.subclass, crs_pnr.class "
    "FROM crs_pnr, crs_pax "
    "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pax_id=:pax_id AND "
    "      crs_pax.pr_del=0",
    QParams() << QParam("pax_id", otInteger, id));
  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    std::string currentClass=Qry.get().FieldAsString("class");
    std::string originalClass=Qry.get().FieldAsString("orig_class");
    if (!onlyIfClassChange || currentClass!=originalClass)
    {
      result.subcl=Qry.get().FieldAsString("subclass");
      result.cl=currentClass;
    }
  }
  return result;
}

std::string TSimplePaxItem::getCabinClass() const
{
  if (cabin.cl.empty() && grp_id!=ASTRA::NoExists)
  {
    TSimplePaxGrpItem grp;
    if (grp.getByGrpId(grp_id)) return grp.cl;
  }

  return cabin.cl;
}

std::string TSimplePaxItem::getCabinSubclass() const
{
  return cabin.subcl.empty()?subcl:cabin.subcl;
}

std::string TSimplePaxItem::getSeatNo(const std::string& fmt) const
{
  if (id==ASTRA::NoExists) return "";
  TCachedQuery Qry(
    "SELECT salons.get_seat_no(:pax_id, :seats, :is_jmp, pax_grp.status, pax_grp.point_dep, :fmt, rownum) seat_no "
    "FROM pax_grp WHERE grp_id=:grp_id",
    QParams() << QParam("pax_id", otInteger, id)
              << QParam("grp_id", otInteger, grp_id)
              << QParam("seats", otInteger, seats)
              << QParam("is_jmp", otInteger, (int)is_jmp)
              << QParam("fmt", otString, fmt));
  Qry.get().Execute();
  if (!Qry.get().Eof)
    return Qry.get().FieldAsString("seat_no");
  return "";
}

bool TSimplePaxItem::cabinClassToDB() const
{
  if (id==ASTRA::NoExists) return false;

  TCachedQuery Qry("UPDATE pax "
                   "SET cabin_subclass=:cabin_subclass, "
                   "    cabin_class=:cabin_class, "
                   "    cabin_class_grp=:cabin_class_grp, "
                   "    tid=cycle_tid__seq.nextval "
                   "WHERE pax_id=:pax_id AND tid=:tid",
                   QParams() << QParam("pax_id", otInteger, id)
                             << QParam("tid", otInteger, tid)
                             << QParam("cabin_subclass", otString)
                             << QParam("cabin_class", otString)
                             << QParam("cabin_class_grp", otInteger));
  cabin.toDB(Qry.get(), "cabin_");
  Qry.get().Execute();

  return Qry.get().RowsProcessed()>0;
}

bool TSimplePaxItem::getBaggageInHoldTotals(TBagTotals& totals) const
{
  totals.clear();
  if (id==ASTRA::NoExists) return false;

  TCachedQuery Qry(
    "SELECT NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS amount, "
    "       NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS weight "
    "FROM pax "
    "WHERE pax_id=:pax_id",
    QParams() << QParam("pax_id", otInteger, id));
  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    totals.amount=Qry.get().FieldAsInteger("amount");
    totals.weight=Qry.get().FieldAsInteger("weight");
  }

  return !Qry.get().Eof;
}

boost::optional<WeightConcept::TNormItem> TSimplePaxItem::getRegularNorm() const
{
  if (id==ASTRA::NoExists) return boost::none;

  std::list< std::pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
  PaxNormsFromDB(ASTRA::NoExists, id, norms);
  for(const auto& norm : norms)
    if (norm.first.isRegularBagType()) return norm.second;

  return boost::none;
}

template<class T>
static void getBaggageInHoldList(int id, T& list)
{
  list.clear();

  if (id==ASTRA::NoExists) return;

  TPaxServiceLists serviceLists;
  serviceLists.fromDB(id, false);
  for(const TPaxServiceListsItem& i : serviceLists)
    if (i.trfer_num==0 && i.category==TServiceCategory::BaggageInHold)
    {
      list.fromDB(i.primary(), true);
      break;
    }

  for(typename T::iterator i=list.begin(); i!=list.end();)
  {
    auto& item=i->second;
    item.isBaggageInHold()?++i:i=list.erase(i);
  }
}

void TSimplePaxItem::getBaggageListForSBDO(TRFISCListWithProps& list) const
{
  getBaggageInHoldList(id, list);
}

void TSimplePaxItem::getBaggageListForSBDO(TBagTypeList& list) const
{
  getBaggageInHoldList(id, list);
}

TAPISItem& TAPISItem::fromDB(int pax_id)
{
  clear();
  CheckIn::LoadPaxDoc(pax_id, doc);
  CheckIn::LoadPaxDoco(pax_id, doco);
  CheckIn::LoadPaxDoca(pax_id, doca_map);
  return *this;
};

const TAPISItem& TAPISItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr apisNode=NewTextChild(node, "apis");
  doc.toXML(apisNode);
  doco.toXML(apisNode);
  xmlNodePtr docaNode=NewTextChild(apisNode, "addresses");
  for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
    d->second.toXML(docaNode);
  return *this;
};

TPaxListItem& TPaxListItem::fromXML(xmlNodePtr paxNode)
{
  TReqInfo *reqInfo=TReqInfo::Instance();

  clear();
  if (paxNode==NULL) return *this;
  xmlNodePtr node2=paxNode->children;

  node=paxNode;
  pax.fromXML(paxNode);
  dont_check_payment=NodeAsIntegerFast("dont_check_payment", node2, 0)!=0;
  crs_seat_no=NodeAsStringFast("preseat_no", node2, "");
  //ремарки
  xmlNodePtr remNode=GetNodeFast("rems",node2);
  if (remNode!=NULL)
  {
    remsExists=true;
    for(remNode=remNode->children; remNode!=NULL; remNode=remNode->next)
    {
      CheckIn::TPaxRemItem rem;
      rem.fromXML(remNode);
      TRemCategory cat=getRemCategory(rem.code, rem.text);
      if (cat==remASVC) continue; //пропускаем переданные ASVC
      rems.insert(rem);
    };
    if (reqInfo->api_mode ||
            (reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
    {
      //ремарки FQT
      for(remNode=NodeAsNodeFast("fqt_rems",node2)->children; remNode!=NULL; remNode=remNode->next)
        addFQT(CheckIn::TPaxFQTItem().fromXML(remNode));
      checkFQTTierLevel();
    };
  };
  //нормы
  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode!=NULL)
  {
    norms=list<WeightConcept::TPaxNormItem>();
    for(normNode=normNode->children; normNode!=NULL; normNode=normNode->next)
      norms.get().push_back(WeightConcept::TPaxNormItem().fromXML(normNode));
  };

  return *this;
};

void TPaxListItem::addFQT(const CheckIn::TPaxFQTItem &fqt)
{
  if (!fqts.insert(fqt).second)
  {
    ostringstream rem;
    rem << fqt.rem_code() << " " << fqt.logStr(TReqInfo::Instance()->desk.lang);
    throw UserException("MSG.REMARK.DUPLICATED",
                        LParams() << LParam("surname", pax.full_name())
                                  << LParam("remark", rem.str()));
  };
}

void TPaxListItem::checkFQTTierLevel()
{
  TPaxFQTCards cards;
  GetPaxFQTCards(fqts, cards);

  //простановка tier_level
  set<TPaxFQTItem> tmp_fqts;
  for(set<TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f)
  {
    TPaxFQTItem item=*f;
    TPaxFQTCards::const_iterator i=cards.find(*f);
    if (i==cards.end())
      throw EXCEPTIONS::Exception("%s: i==cards.end() strange situation!", __FUNCTION__);
    if (!item.tier_level.empty() &&
        !i->second.tier_level.empty() &&
        item.tier_level!=i->second.tier_level)
      throw UserException("MSG.REMARK.DIFFERENT_TIER_LEVEL",
                          LParams() << LParam("surname", pax.full_name())
                                    << LParam("fqt_card", i->first.no_str(TReqInfo::Instance()->desk.lang)));
    item.copyIfBetter(i->second);
    tmp_fqts.insert(item);
  };

  fqts=tmp_fqts;
}

void TPaxListItem::checkImportantRems(bool new_checkin, ASTRA::TPaxStatus grp_status)
{
  for(int pass=0; pass<2; pass++)
  {
    Statistic<string> crewRemsStat;
    for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end(); ++r)
    {
      if (pass==0)
      {
        TRemCategory cat=getRemCategory(r->code, r->text);
        if (!r->code.empty() && cat==remCREW)
          crewRemsStat.add(r->code);
      }
      else
      {
        if (r->code=="JMP")
          crewRemsStat.add(r->code);
      }
    };
    if (!crewRemsStat.empty())
    {
      //есть ремарки, связанные с экипажем
      //пороверяем дублирование ремарок и взаимоислючающие ремарки
      if (crewRemsStat.size()>1)
      {
        //взаимоисключающие
        throw UserException("MSG.REMARK.MUTUALLY_EXCLUSIVE",
                            LParams() << LParam("surname", pax.full_name())
                            << LParam("remark1", crewRemsStat.begin()->first)
                            << LParam("remark2", crewRemsStat.rbegin()->first));
      }
      else
      {
        if (crewRemsStat.begin()->second>1)
          //повторяющиеся
          throw UserException("MSG.REMARK.DUPLICATED",
                              LParams() << LParam("surname", pax.full_name())
                              << LParam("remark", crewRemsStat.begin()->first));

        if (new_checkin)
        {
          if (pass==0)
          {
            if (grp_status==ASTRA::psCrew)
              pax.crew_type=ASTRA::TCrewType::Unknown;
            else
              pax.crew_type=CrewTypes().decode(crewRemsStat.begin()->first);
          }
          else
          {
            if (grp_status==ASTRA::psCrew || pax.pers_type==ASTRA::baby)
              pax.is_jmp=false;
            else
              pax.is_jmp=(crewRemsStat.begin()->first=="JMP");
          }
        };

        CheckIn::TPaxRemItem rem=(pass==0?CalcCrewRem(grp_status, pax.crew_type):
                                          CalcJmpRem(grp_status, pax.is_jmp));
        if (crewRemsStat.begin()->first!=rem.code)
        {
          //удаляем неправильную ремарку
          for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end();)
          {
            if (r->code==crewRemsStat.begin()->first) r=Erase(rems, r); else ++r;
          }
          //добавляем правильную ремарку
          if (!rem.code.empty())
            rems.insert(CheckIn::TPaxRemItem(rem.code,rem.text));
        }
      };
    }
    else
    {
      //нет ремарок, связанных с экипажем
      CheckIn::TPaxRemItem rem=(pass==0?CalcCrewRem(grp_status, pax.crew_type):
                                        CalcJmpRem(grp_status, pax.is_jmp));
      if (!rem.code.empty())
        rems.insert(CheckIn::TPaxRemItem(rem.code,rem.text));
    };
  };
}

int TPaxListItem::getExistingPaxIdOrSwear() const
{
  int pax_id=(pax.id==ASTRA::NoExists?generated_pax_id:pax.id);
  if (pax_id==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("%s: strange situation!", __FUNCTION__);
  return pax_id;
}

int TPaxList::getBagPoolMainPaxId(int bag_pool_num) const
{
  if (bag_pool_num==ASTRA::NoExists) return ASTRA::NoExists;
  boost::optional<TSimplePaxItem> pax;
  for(TPaxList::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->pax.bag_pool_num!=bag_pool_num) continue;
    if (!pax || i->pax.upward_within_bag_pool(pax.get())) pax=i->pax;
  };
  return pax?pax.get().id:ASTRA::NoExists;
}

const TPaxGrpItem& TPaxGrpItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr grpNode=node;
  NewTextChild(grpNode, "grp_id", id);
  NewTextChild(grpNode, "point_dep", point_dep);
  NewTextChild(grpNode, "airp_dep", airp_dep);
  NewTextChild(grpNode, "point_arv", point_arv);
  NewTextChild(grpNode, "airp_arv", airp_arv);
  NewTextChild(grpNode, "class", cl);
  NewTextChild(grpNode, "status", EncodePaxStatus(status));
  NewTextChild(grpNode, "bag_refuse", bag_refuse);
  if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    NewTextChild(grpNode, "bag_types_id", id);
    NewTextChild(grpNode, "piece_concept", (int)baggage_pc);
  };
  NewTextChild(grpNode, "tid", tid);

  NewTextChild(grpNode, "show_ticket_norms", (int)pc);
  NewTextChild(grpNode, "show_wt_norms", (int)wt);
  return *this;
};

bool TPaxGrpItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return true;
  xmlNodePtr node2=node->children;

  int grp_id=NodeAsIntegerFast("grp_id",node2,ASTRA::NoExists);
  if (grp_id!=ASTRA::NoExists)
  {
    //запись изменений
    if (!getByGrpIdWithBagConcepts(grp_id)) return false;
    if (tid!=NodeAsIntegerFast("tid",node2)) return false;
  };

  id=NodeAsIntegerFast("grp_id",node2,ASTRA::NoExists);
  point_dep=NodeAsIntegerFast("point_dep",node2);
  airp_dep=NodeAsStringFast("airp_dep",node2);
  point_arv=NodeAsIntegerFast("point_arv",node2);
  airp_arv=NodeAsStringFast("airp_arv",node2);
  cl=NodeAsStringFast("class",node2);
  if (id==ASTRA::NoExists)
    status=DecodePaxStatus(NodeAsStringFast("status",node2));

  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode!=NULL)
  {
    norms=list<WeightConcept::TPaxNormItem>();
    for(normNode=normNode->children; normNode!=NULL; normNode=normNode->next)
      norms.get().push_back(WeightConcept::TPaxNormItem().fromXML(normNode));
  };
  return true;
};

TPaxGrpItem& TPaxGrpItem::fromXMLadditional(xmlNodePtr node, xmlNodePtr firstSegNode, bool is_unaccomp)
{
  hall=ASTRA::NoExists;
  bag_refuse.clear();

  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  TReqInfo *reqInfo=TReqInfo::Instance();

  if (tid!=ASTRA::NoExists)
  {
    //запись изменений
    bag_refuse=NodeAsStringFast("bag_refuse",node2);
  };

  //зал
  if (reqInfo->client_type == ASTRA::ctTerm)
  {
    hall=NodeAsIntegerFast("hall",node2);
  };

  if (wt) WeightConcept::PaidBagFromXML(node, id, is_unaccomp, trfer_confirm, paid);

  group_bag=TGroupBagItem();
  if (!group_bag.get().fromXML(node, id, hall, is_unaccomp, baggage_pc, trfer_confirm)) group_bag=boost::none;

  TGrpServiceListWithAuto list;
  if (!list.fromXML(node))
  {
    svc=boost::none;
    svc_auto=boost::none;
  }
  else
  {
    svc=TGrpServiceList();
    svc_auto=TGrpServiceAutoList();
    list.split(id, svc.get(), svc_auto.get());
  }

  ServicePaymentFromXML(firstSegNode, id, is_unaccomp, baggage_pc, payment);

  return *this;
};

const TPaxGrpItem& TPaxGrpItem::toDB(TQuery &Qry) const
{
  id!=ASTRA::NoExists?Qry.SetVariable("grp_id", id):
                      Qry.SetVariable("grp_id", FNull);
  if (Qry.GetVariableIndex("point_dep")>=0)
    Qry.SetVariable("point_dep", point_dep);
  if (Qry.GetVariableIndex("point_arv")>=0)
    Qry.SetVariable("point_arv", point_arv);
  if (Qry.GetVariableIndex("airp_dep")>=0)
    Qry.SetVariable("airp_dep", airp_dep);
  if (Qry.GetVariableIndex("airp_arv")>=0)
    Qry.SetVariable("airp_arv", airp_arv);
  if (Qry.GetVariableIndex("class")>=0)
    Qry.SetVariable("class", cl);
  if (Qry.GetVariableIndex("status")>=0)
    Qry.SetVariable("status", EncodePaxStatus(status));
  if (Qry.GetVariableIndex("hall")>=0)
    hall!=ASTRA::NoExists?Qry.SetVariable("hall", hall):
                          Qry.SetVariable("hall", FNull);
  if (Qry.GetVariableIndex("bag_refuse")>=0)
    Qry.SetVariable("bag_refuse",(int)(!bag_refuse.empty()));
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  return *this;
};

TSimplePnrItem& TSimplePnrItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pnr_id");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  status=Qry.FieldAsString("status");
  return *this;
}

TSimplePaxGrpItem& TSimplePaxGrpItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("grp_id");
  point_dep=Qry.FieldAsInteger("point_dep");
  point_arv=Qry.FieldAsInteger("point_arv");
  airp_dep=Qry.FieldAsString("airp_dep");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  status=DecodePaxStatus(Qry.FieldAsString("status"));
  if (!Qry.FieldIsNULL("hall"))
    hall=Qry.FieldAsInteger("hall");
  if (Qry.FieldAsInteger("bag_refuse")!=0)
    bag_refuse=ASTRA::refuseAgentError;
  trfer_confirm=Qry.FieldAsInteger("trfer_confirm")!=0;
  is_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
  if (Qry.GetFieldIndex("client_type")>=0)
    client_type = DecodeClientType(Qry.FieldAsString("client_type"));
  tid=Qry.FieldAsInteger("tid");

  baggage_pc=Qry.FieldAsInteger("piece_concept")!=0;
  return *this;
}

bool TSimplePaxGrpItem::getByGrpId(int grp_id)
{
  clear();
  TCachedQuery Qry("SELECT * FROM pax_grp WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

ASTRA::TCompLayerType TSimplePaxGrpItem::getCheckInLayerType() const
{
  return DecodeCompLayerType(getBaseTable(etGrpStatusType).get_row("code", EncodePaxStatus(status)).AsString("layer_type").c_str());
}

bool TSimplePnrItem::getByPaxId(int pax_id)
{
  clear();
  TCachedQuery Qry("SELECT crs_pnr.* FROM crs_pnr, crs_pax WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:pax_id",
                   QParams() << QParam("pax_id", otInteger, pax_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

TPaxGrpItem& TPaxGrpItem::fromDBWithBagConcepts(TQuery &Qry)
{
  clear();
  TSimplePaxGrpItem::fromDB(Qry);
  GetBagConcepts(id, pc, wt, rfisc_used);
  return *this;
};

bool TPaxGrpItem::getByGrpIdWithBagConcepts(int grp_id)
{
  clear();
  if (!TSimplePaxGrpItem::getByGrpId(grp_id)) return false;
  GetBagConcepts(id, pc, wt, rfisc_used);
  return true;
}

TPaxGrpCategory::Enum TSimplePaxGrpItem::grpCategory() const
{
  if (status==ASTRA::psCrew)
    return TPaxGrpCategory::Crew;
  else if (cl.empty())
    return TPaxGrpCategory::UnnacompBag;
  else
    return TPaxGrpCategory::Passenges;
}

void TPaxGrpItem::SyncServiceAuto(const TTripInfo& flt)
{
  ostringstream sql;
  sql <<
    "SELECT pax.pax_id, 0 AS transfer_num, "
    "       rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon "
    "FROM pax, crs_pax_asvc "
    "WHERE pax.pax_id=crs_pax_asvc.pax_id AND "
    "      rfic IS NOT NULL AND "
    "      rfisc IS NOT NULL AND "
    "      service_quantity IS NOT NULL AND "
    "      service_name IS NOT NULL AND "
    "      (rem_status='HI' AND "
    "       emd_type IS NOT NULL AND "
    "       emd_no IS NOT NULL AND "
    "       emd_coupon IS NOT NULL OR "
    "       rem_status='HK' AND "
    "       emd_type IS NULL AND "
    "       emd_no IS NULL AND "
    "       emd_coupon IS NULL) AND "
    "      pax.grp_id=:id ";
  QParams QryParams;
  QryParams << QParam("id", otInteger, id);
  TCachedQuery Qry(sql.str().c_str(), QryParams);
  Qry.get().Execute();
  for(;!Qry.get().Eof; Qry.get().Next())
  {
    TGrpServiceAutoItem item;
    item.fromDB(Qry.get());
    if (!item.isSuitableForAutoCheckin()) continue;
    if (!item.permittedForAutoCheckin(flt)) continue;

    if (!svc_auto) svc_auto=TGrpServiceAutoList();
    svc_auto.get().push_back(item);
  }
}

void TPaxGrpItem::checkInfantsCount(const TSimplePaxList &prior_paxs,
                                    const TSimplePaxList &curr_paxs) const
{
  if (prior_paxs.empty() && !curr_paxs.empty())
  {
    //new_checkin
    if (curr_paxs.infantsMoreThanAdults())
    {
      LogTrace(TRACE0) << "curr_paxs.infantsMoreThanAdults()";
      throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
    }
    return;
  }

  bool needCheck=false;
  if (prior_paxs.size()!=curr_paxs.size())
    throw EXCEPTIONS::Exception("%s: strange situation: prior_paxs.size()!=curr_paxs.size()!", __FUNCTION__);

  TSimplePaxList::const_iterator iPrior=prior_paxs.begin();
  for(; iPrior!=prior_paxs.end(); ++iPrior)
  {
    TSimplePaxList::const_iterator iCurr=curr_paxs.begin();
    for(; iCurr!=curr_paxs.end(); ++iCurr)
      if (iCurr->id==iPrior->id) break;
    if (iCurr==curr_paxs.end()) break;
    if (iCurr->pers_type!=iPrior->pers_type ||
        iCurr->seats!=iPrior->seats ||
        iCurr->refuse!=iPrior->refuse) break;
  }
  if (iPrior!=prior_paxs.end()) needCheck=true;


  if (!needCheck) return;

  TCachedQuery Qry("SELECT SUM(CASE WHEN pers_type=:adult THEN 1 ELSE 0 END) AS adults, "
                   "       SUM(CASE WHEN pers_type=:infant AND seats=0 THEN 1 ELSE 0 END) AS infants "
                   "FROM pax "
                   "WHERE grp_id=:grp_id AND refuse IS NULL ",
                   QParams() << QParam("grp_id", otInteger, id)
                             << QParam("adult", otString, EncodePerson(ASTRA::adult))
                             << QParam("infant", otString, EncodePerson(ASTRA::baby)));

  Qry.get().Execute();
  if (Qry.get().Eof ||
      Qry.get().FieldIsNULL("adults") ||
      Qry.get().FieldIsNULL("infants")) return;

  int adults=Qry.get().FieldAsInteger("adults");
  int infants=Qry.get().FieldAsInteger("infants");

  if (adults < infants)
  {
    LogTrace(TRACE0) << "adults=" << adults << " infants=" << infants;
    throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
  }
}

void TPaxGrpItem::UpdTid(int grp_id)
{
  TCachedQuery Qry("UPDATE pax_grp SET tid=cycle_tid__seq.nextval WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
}

TCkinPaxTknItem& TCkinPaxTknItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxTknItem::fromDB(Qry);
  if (!Qry.FieldIsNULL("grp_id"))
    grp_id=Qry.FieldAsInteger("grp_id");
  if (!Qry.FieldIsNULL("pax_id"))
    pax_id=Qry.FieldAsInteger("pax_id");
  return *this;
}

void GetTCkinTickets(int pax_id, map<int, TCkinPaxTknItem> &tkns)
{
  tkns.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm, "
    "       pax.grp_id, pax.pax_id, tckin_pax_grp.seg_no "
    "FROM pax, tckin_pax_grp, "
    "     (SELECT tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
    "      FROM pax, tckin_pax_grp "
    "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.pax_id=:pax_id) p "
    "WHERE tckin_pax_grp.tckin_id=p.tckin_id AND "
    "      pax.grp_id=tckin_pax_grp.grp_id AND "
    "      tckin_pax_grp.first_reg_no-pax.reg_no=p.distance ";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    tkns.insert(make_pair(Qry.FieldAsInteger("seg_no"), TCkinPaxTknItem().fromDB(Qry)));
}

// Ф-я возвращает непрерывный список сегментов до или после pax_id, в зав-ти от after_current
// Если встречается номер сегмента, не идущий подряд, он и следующие за ним элементы отсекаются.
// Напр. если список такой 1, 2, 4, 5, 7, 8
// То в случае after_current = true вернется 7, 8 (непрерывность с конца)
// При after_current = false вернется 1, 2 (с начала)
void GetTCkinTickets(int pax_id, map<int, TCkinPaxTknItem> &tkns, bool after_current)
{
    tkns.clear();
    map<int, TCkinPaxTknItem> _tkns;
    GetTCkinTickets(pax_id, _tkns);

    // Находим в мепе элемент с тек. pax_id
    auto current = _tkns.begin();
    for(; current != _tkns.end(); current++) {
        if(current->second.pax_id == pax_id) break;
    }

    if(current != _tkns.end()) {
        auto curr_idx = current->first;
        // в зав-ти от after_current в result помещаем часть слева или справа от pax_id
        if(after_current) {
            current++;
            tkns.insert(current, _tkns.end());
        } else {
            tkns.insert(_tkns.begin(), current);
        }

        // а здесь происходит удаление элементов, seg_no которых не подряд
        // Напр.:
        // (1,2,3,5,6,10) -> (1,2,3)
        if(after_current) {
            auto ri = tkns.begin();
            curr_idx++;
            for(; ri != tkns.end(); ri++, curr_idx++) {
                if(ri->first != curr_idx) break;
            }
            if(ri != tkns.end())
                tkns.erase(ri, tkns.end());
        } else {
            auto ri = tkns.rbegin();
            curr_idx--;
            for(; ri != tkns.rend(); ri++, curr_idx--) {
                if(ri->first != curr_idx) break;
            }
            if(ri != tkns.rend()) {
                auto i = ri.base();
                tkns.erase(tkns.begin(), i);
            }
        }
    }
}

void GetTCkinTicketsBefore(int pax_id, map<int, TCkinPaxTknItem> &tkns)
{
    GetTCkinTickets(pax_id, tkns, false);
}

void GetTCkinTicketsAfter(int pax_id, map<int, TCkinPaxTknItem> &tkns)
{
    GetTCkinTickets(pax_id, tkns, true);
}

std::string isFemaleStr( int is_female )
{
  switch (is_female) {
    case ASTRA::NoExists:
      return "";
      break;
    case 0:
      return "M";
      break;
    default:
      return "F";
      break;
  };
}

bool TPaxTknItem::validForSearch() const
{
  return !no.empty();
}

void TPaxTknItem::addSQLTablesForSearch(const PaxOrigin& origin, std::list<std::string>& tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      break;
    case paxPnl:
      tables.push_back("crs_pax_tkn");
      break;
    case paxTest:
      break;
  }
}

void TPaxTknItem::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      if (no.size()==14)
        conditions.push_back("pax.ticket_no IN (:tkn_no, :tkn_no_13)");
      else
        conditions.push_back("pax.ticket_no=:tkn_no");
      if (coupon!=ASTRA::NoExists)
        conditions.push_back("pax.coupon_no=:tkn_coupon");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id=crs_pax_tkn.pax_id");
      if (no.size()==14)
        conditions.push_back("crs_pax_tkn.ticket_no IN (:tkn_no, :tkn_no_13)");
      else
        conditions.push_back("crs_pax_tkn.ticket_no=:tkn_no");
      if (coupon!=ASTRA::NoExists)
        conditions.push_back("crs_pax_tkn.coupon_no=:tkn_coupon");
      break;
    case paxTest:
      if (no.size()==14)
        conditions.push_back("test_pax.tkn_no IN (:tkn_no, :tkn_no_13)");
      else
        conditions.push_back("test_pax.tkn_no=:tkn_no");
      break;
  }
}

void TPaxTknItem::addSQLParamsForSearch(QParams& params) const
{
  params << QParam("tkn_no", otString, no);
  if (no.size()==14)
    params << QParam("tkn_no_13", otString, no.substr(0,13));
  if (coupon!=ASTRA::NoExists)
    params << QParam("tkn_coupon", otInteger, coupon);
}

bool TPaxDocItem::validForSearch() const
{
  return !no.empty();
}

void TPaxDocItem::addSQLTablesForSearch(const PaxOrigin& origin, std::list<std::string>& tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      tables.push_back("pax_doc");
      break;
    case paxPnl:
      tables.push_back("crs_pax_doc");
      break;
    case paxTest:
      break;
  }
}

void TPaxDocItem::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax.pax_id=pax_doc.pax_id");
      conditions.push_back("pax_doc.no=:doc_no");
      if (birth_date!=ASTRA::NoExists)
        conditions.push_back("(pax_doc.birth_date=:doc_birth_date OR pax_doc.birth_date IS NULL)");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id=crs_pax_doc.pax_id");
      conditions.push_back("crs_pax_doc.no=:doc_no");
      if (birth_date!=ASTRA::NoExists)
        conditions.push_back("(crs_pax_doc.birth_date=:doc_birth_date OR crs_pax_doc.birth_date IS NULL)");
      break;
    case paxTest:
      conditions.push_back("test_pax.doc_no=:doc_no");
      break;
  }
}

void TPaxDocItem::addSQLParamsForSearch(QParams& params) const
{
  params << QParam("doc_no", otString, no);
  if (birth_date!=ASTRA::NoExists)
    params << QParam("doc_birth_date", otDate, birth_date);
}

std::string TScannedPaxDocItem::getTrueNo() const
{
  if (isNationalRussianPassport() && no.size()==9 && !extra.empty())
    return no.substr(0,3)+extra.substr(0,1)+no.substr(3);

  return no;
}

void TScannedPaxDocItem::addSQLParamsForSearch(QParams& params) const
{
  LogTrace(TRACE5) << __FUNCTION__ << ": doc_no=" << getTrueNo();
  params << QParam("doc_no", otString, getTrueNo());
  if (birth_date!=ASTRA::NoExists)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": doc_birth_date=" << DateTimeToStr(birth_date);
    params << QParam("doc_birth_date", otDate, birth_date);
  }
}

bool TSimplePaxList::infantsMoreThanAdults() const
{
  return CheckIn::infantsMoreThanAdults(*this);
}

}; //namespace CheckIn

namespace Sirena
{

void PaxBrandsNormsToStream(const TTrferRoute &trfer, const CheckIn::TPaxItem &pax, ostringstream &s)
{
  s << "#" << setw(3) << setfill('0') << pax.reg_no << " "
    << pax.full_name() << "(" << ElemIdToCodeNative(etPersType, EncodePerson(pax.pers_type)) << "):" << endl;

  TPaxNormList norms;
  PaxNormsFromDB(pax.id, norms);
  TPaxBrandList brands;
  PaxBrandsFromDB(pax.id, brands);

  for(int pass=0; pass<2; pass++)
  {
    string prior_text;
    int trfer_num=0;
    for(TTrferRoute::const_iterator t=trfer.begin(); ; ++t, trfer_num++)
    {
      //ищем норму, соответствующую сегменту
      string curr_text;
      if (t!=trfer.end())
      {
        if (pass==0)
        {
          //бренды

          for(TPaxBrandList::const_iterator b=brands.begin(); b!=brands.end(); ++b)
          {
            if (b->first.trfer_num!=trfer_num) continue;

            const TSimplePaxBrandItem &brand=b->second;

            TSimplePaxBrandItem::const_iterator i=brand.find(TReqInfo::Instance()->desk.lang);
            if (i==brand.end() && !brand.empty()) i=brand.begin(); //первая попавшаяся

            if (i!=brand.end()) curr_text=i->second.text;
            break;
          }
        }
        else
        {
          boost::optional<bool> carry_on=false;
          carry_on=boost::none;
          //нормы
          for(TPaxNormList::const_iterator n=norms.begin(); n!=norms.end(); ++n)
          {
            if (n->first.trfer_num!=trfer_num) continue;

            const TSimplePaxNormItem &norm=n->second;

            TSimplePaxNormItem::const_iterator i=norm.find(TReqInfo::Instance()->desk.lang);
            if (i==norm.end() && !norm.empty()) i=norm.begin(); //первая попавшаяся

            if (i!=norm.end())
            {
              if (!curr_text.empty()) curr_text+='\n';

              if (!carry_on || carry_on.get()!=norm.carry_on)
              {
                curr_text+=getLocaleText(norm.carry_on?"MSG.CARRY_ON_NORM_OF_THE_AIRLINE_APPLIES":
                                                       "MSG.BAGGAGE_NORM_OF_THE_AIRLINE_APPLIES",
                                         LParams() << LParam("airline", ElemIdToCodeNative(etAirline, norm.airline)));
                curr_text+='\n';
                carry_on=norm.carry_on;
              };

              curr_text+=i->second.text;
            };
          };
        };
      };

      if (t!=trfer.begin())
      {
        if (t==trfer.end() || curr_text!=prior_text)
        {
          if (pass==1 || !prior_text.empty())
          {
            s << ":" << endl //закончили секцию сегментов
              << (prior_text.empty()?getLocaleText(pass==0?"MSG.UNKNOWN_BRAND":
                                                           "MSG.LUGGAGE.UNKNOWN_BAG_NORM"):prior_text) << endl;  //записали соответствующую норму
          };
        }
        else
        {
          if (pass==1 || !prior_text.empty()) s << " -> ";
        }
      };

      if (t==trfer.end()) break;

      if (t==trfer.begin() || curr_text!=prior_text)
      {
        if (pass==1 || t==trfer.begin() || !prior_text.empty()) s << endl;
      };

      if (pass==1 || !curr_text.empty())
      {
        const TTrferRouteItem &item=*t;
        s << ElemIdToCodeNative(etAirline, item.operFlt.airline)
          << setw(2) << setfill('0') << item.operFlt.flt_no
          << ElemIdToCodeNative(etSuffix, item.operFlt.suffix)
          << " " << ElemIdToCodeNative(etAirp, item.operFlt.airp);
      };

      prior_text=curr_text;
    }
  }

  s << string(100,'=') << endl; //подведем черту :)
}

} //namespace Sirena

const char rus_pnr[]="0123456789БВГДКЛМНПРСТФХЦЖШ";
const char lat_pnr[]="0123456789BVGDKLMNPRSTFXCZW";

static char toLatPnrCharStrictly(char c)
{
  if (!IsAscii7(c))
  {
    ByteReplace(&c,1,rus_pnr,lat_pnr);
    if (!IsAscii7(c)) c='?';
  };
  return c;
}

static char toLatPnrChar(char c)
{
  ByteReplace(&c,1,rus_pnr,lat_pnr);
  return c;
}

static char toRusPnrChar(char c)
{
  ByteReplace(&c,1,lat_pnr,rus_pnr);
  return c;
}

string convert_pnr_addr(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), toLatPnrCharStrictly);
  return result;

}

const TPnrAddrInfo& TPnrAddrInfo::toXML(xmlNodePtr addrParentNode,
                                        const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (addrParentNode==nullptr) return *this;

  xmlNodePtr addrNode=NewTextChild(addrParentNode, "pnr_addr");
  NewTextChild(addrNode, "airline", lang?airlineToPrefferedCode(airline, lang.get()):airline);
  NewTextChild(addrNode, "addr", lang?convert_pnr_addr(addr, lang->isLatin()):addr);

  return *this;
}

bool TPnrAddrInfo::validForSearch() const
{
  return !addr.empty();
}

void TPnrAddrInfo::addSQLTablesForSearch(const PaxOrigin& origin, std::list<std::string>& tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      tables.push_back("crs_pax");
      tables.push_back("crs_pnr");
      tables.push_back("pnr_addrs");
      break;
    case paxPnl:
      tables.push_back("pnr_addrs");
      break;
    case paxTest:
      break;
  }
}

void TPnrAddrInfo::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax.pax_id=crs_pax.pax_id");
      conditions.push_back("crs_pax.pnr_id=crs_pnr.pnr_id");
      conditions.push_back("crs_pnr.system='CRS'");
      conditions.push_back("crs_pax.pr_del=0");
      conditions.push_back("pnr_addrs.pnr_id=crs_pax.pnr_id");
      conditions.push_back("pnr_addrs.addr IN (:addr_rus, :addr_lat)");
      break;
    case paxPnl:
      conditions.push_back("pnr_addrs.pnr_id=crs_pax.pnr_id");
      conditions.push_back("pnr_addrs.addr IN (:addr_rus, :addr_lat)");
      break;
    case paxTest:
      conditions.push_back("test_pax.pnr_addr IN (:addr_rus, :addr_lat)");
      break;
  }
}

void TPnrAddrInfo::addSQLParamsForSearch(QParams& params) const
{
  std::string addr_rus(addr), addr_lat(addr);
  transform(addr_rus.begin(), addr_rus.end(), addr_rus.begin(), toRusPnrChar);
  transform(addr_lat.begin(), addr_lat.end(), addr_lat.begin(), toLatPnrChar);
  params << QParam("addr_rus", otString, addr_rus)
         << QParam("addr_lat", otString, addr_lat);
}

const TPnrAddrs& TPnrAddrs::toXML(xmlNodePtr addrsParentNode) const
{
  if (addrsParentNode==nullptr) return *this;

  if (!empty())
  {
    if (size()==1)
      front().toXML(addrsParentNode, boost::none);
    else
    {
      xmlNodePtr addrsNode=NewTextChild(addrsParentNode, "pnr_addrs");
      for(const TPnrAddrInfo& addr : *this) addr.toXML(addrsNode, boost::none);
    }
  }

  return *this;
}

const TPnrAddrs& TPnrAddrs::toWebXML(xmlNodePtr addrsParentNode,
                                     const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (addrsParentNode==nullptr) return *this;

  xmlNodePtr addrsNode=NewTextChild(addrsParentNode, "pnr_addrs");
  for(const TPnrAddrInfo& addr : *this) addr.toXML(addrsNode, lang);

  return *this;
}
const TPnrAddrs& TPnrAddrs::toSirenaXML(xmlNodePtr addrParentNode,
                                        const AstraLocale::OutputLang& lang) const
{
  if (addrParentNode==nullptr) return *this;

  for(const TPnrAddrInfo& addr : *this)
    SetProp(NewTextChild(addrParentNode, "recloc", addr.addr), "crs", airlineToPrefferedCode(addr.airline, lang));

  return *this;
}

std::string TPnrAddrs::getByPnrId(int pnr_id)
{
  string airline;
  return getByPnrId(pnr_id, airline);
}

void TPnrAddrs::getByPnrIdFast(int pnr_id)
{
  string airline("No matter");
  getByPnrId(pnr_id, airline);
}

std::string TPnrAddrs::getByPnrId(int pnr_id, string &airline)
{
  clear();

  QParams QryParams;
  QryParams << QParam("pnr_id", otInteger, pnr_id);
  if (!airline.empty())
    QryParams << QParam("airline", otString, airline);

  TCachedQuery CachedQry(airline.empty()?
                         "SELECT pnr_addrs.airline, pnr_addrs.addr, tlg_trips.airline AS priority_airline "
                         "FROM pnr_addrs, crs_pnr, tlg_trips "
                         "WHERE pnr_addrs.pnr_id=crs_pnr.pnr_id AND "
                         "      crs_pnr.point_id=tlg_trips.point_id AND "
                         "      pnr_addrs.pnr_id=:pnr_id "
                         "ORDER BY DECODE(pnr_addrs.airline, tlg_trips.airline, 0, 1), pnr_addrs.airline":
                         "SELECT pnr_addrs.airline, pnr_addrs.addr, :airline AS priority_airline "
                         "FROM pnr_addrs "
                         "WHERE pnr_addrs.pnr_id=:pnr_id "
                         "ORDER BY DECODE(pnr_addrs.airline, :airline, 0, 1), pnr_addrs.airline",
                         QryParams);

  TQuery &Qry=CachedQry.get();
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (airline.empty())
      airline=Qry.FieldAsString("priority_airline");
    emplace_back(Qry.FieldAsString("airline"),
                 Qry.FieldAsString("addr"));
  }

  if (!empty() && begin()->airline==airline)
    return begin()->addr;
  else
    return "";
}

std::string TPnrAddrs::getByPaxId(int pax_id)
{
  string airline;
  return getByPaxId(pax_id, airline);
}

void TPnrAddrs::getByPaxIdFast(int pax_id)
{
  string airline("No matter");
  getByPaxId(pax_id, airline);
}

std::string TPnrAddrs::getByPaxId(int pax_id, string &airline)
{
  clear();
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);

  TCachedQuery CachedQry(
    "SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0",
    QryParams);
  TQuery &Qry=CachedQry.get();
  Qry.Execute();
  if (!Qry.Eof)
    return getByPnrId(Qry.FieldAsInteger("pnr_id"), airline);
  else
    return "";
};
