#include "passenger.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "term_version.h"
#include "baggage.h"
#include "qrys.h"
#include "exceptions.h"
#include "jxtlib/jxt_cont.h"
#include "web_search.h"
#include "apis_utils.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;

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

namespace CheckIn
{

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

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn)
{
  return LoadPaxTkn(ASTRA::NoExists, pax_id, tkn);
};

bool LoadPaxTkn(TDateTime part_key, int pax_id, TPaxTknItem &tkn)
{
    tkn.clear();

    const char* sql=
        "SELECT ticket_no, coupon_no, ticket_rem, ticket_confirm, pers_type, seats "
        "FROM pax WHERE pax_id=:pax_id";
    const char* sql_arx=
        "SELECT ticket_no, coupon_no, ticket_rem, ticket_confirm, pers_type, seats "
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

string PaxDocCountryToTerm(const string &pax_doc_country)
{
  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(SCAN_DOC_VERSION) ||
      pax_doc_country.empty())
    return pax_doc_country;
  else
    return getBaseTable(etPaxDocCountry).get_row("code",pax_doc_country).AsString("country");
};

string PaxDocCountryFromTerm(const string &doc_code)
{
  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(SCAN_DOC_VERSION) ||
      doc_code.empty())
    return doc_code;
  else
    try
    {
      return getBaseTable(etPaxDocCountry).get_row("country",doc_code).AsString("code");
    }
    catch (EBaseTableError)
    {
      return "";
    };
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

const TPaxDocItem& TPaxDocItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");
  NewTextChild(docNode, "type", type, "");
  NewTextChild(docNode, "issue_country", PaxDocCountryToTerm(issue_country), "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "nationality", PaxDocCountryToTerm(nationality), "");
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
};

TPaxDocItem& TPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  
  type=NodeAsStringFast("type",node2,"");
  issue_country=PaxDocCountryFromTerm(NodeAsStringFast("issue_country",node2,""));
  no=NodeAsStringFast("no",node2,"");
  nationality=PaxDocCountryFromTerm(NodeAsStringFast("nationality",node2,""));
  if (!NodeIsNULLFast("birth_date",node2,true))
    birth_date = WebSearch::date_fromXML(NodeAsStringFast("birth_date",node2,""));
  gender=PaxDocGenderNormalize(NodeAsStringFast("gender",node2,""));
  if (!NodeIsNULLFast("expiry_date",node2,true))
    expiry_date = WebSearch::date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  surname=NodeAsStringFast("surname",node2,"");
  first_name=NodeAsStringFast("first_name",node2,"");
  second_name=NodeAsStringFast("second_name",node2,"");
  pr_multi=NodeAsIntegerFast("pr_multi",node2,0)!=0;
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  return *this;
};

const TPaxDocItem& TPaxDocItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("type", type);
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
};

TPaxDocItem& TPaxDocItem::fromDB(TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
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
};

long int TPaxDocItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;
  
  if (!type.empty())                result|=DOC_TYPE_FIELD;
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

const TPaxDocoItem& TPaxDocoItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place, "");
  NewTextChild(docNode, "type", type, "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "issue_place", issue_place, "");
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "applic_country", PaxDocCountryToTerm(applic_country), "");
  NewTextChild(docNode, "scanned_attrs", scanned_attrs, (int)NO_FIELDS);
  return *this;
};

TPaxDocoItem& TPaxDocoItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  birth_place=NodeAsStringFast("birth_place",node2,"");
  type=NodeAsStringFast("type",node2,"");
  no=NodeAsStringFast("no",node2,"");
  issue_place=NodeAsStringFast("issue_place",node2,"");
  if (!NodeIsNULLFast("issue_date",node2,true))
    issue_date = WebSearch::date_fromXML(NodeAsStringFast("issue_date",node2,""));
  if (!NodeIsNULLFast("expiry_date",node2,true))
    expiry_date=WebSearch::date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  applic_country=PaxDocCountryFromTerm(NodeAsStringFast("applic_country",node2,""));
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  return *this;
};

const TPaxDocoItem& TPaxDocoItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("birth_place", birth_place);
  Qry.SetVariable("type", type);
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
};

TPaxDocoItem& TPaxDocoItem::fromDB(TQuery &Qry)
{
  clear();
  birth_place=Qry.FieldAsString("birth_place");
  type=Qry.FieldAsString("type");
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
};

long int TPaxDocoItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!birth_place.empty())         result|=DOCO_BIRTH_PLACE_FIELD;
  if (!type.empty())                result|=DOCO_TYPE_FIELD;
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

const TPaxDocaItem& TPaxDocaItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docaNode=NewTextChild(node,"doca");
  NewTextChild(docaNode, "type", type);
  NewTextChild(docaNode, "country", PaxDocCountryToTerm(country), "");
  NewTextChild(docaNode, "address", address, "");
  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(APIS_CITY_REGION_VERSION))
  {
    NewTextChild(docaNode, "city", city, "");
    NewTextChild(docaNode, "region", region, "");
  }
  else
  {
    string str;
    str.clear();
    transform(city.begin(), city.end(), back_inserter(str), APIS::ReplaceDigit);
    NewTextChild(docaNode, "city", str, "");
    str.clear();
    transform(region.begin(), region.end(), back_inserter(str), APIS::ReplaceDigit);
    NewTextChild(docaNode, "region", str, "");
  };

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
  country=PaxDocCountryFromTerm(NodeAsStringFast("country",node2,""));
  address=NodeAsStringFast("address",node2,"");
  city=NodeAsStringFast("city",node2,"");
  region=NodeAsStringFast("region",node2,"");
  postal_code=NodeAsStringFast("postal_code",node2,"");
  return *this;
};

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

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc, bool without_inf_indicator)
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
  if (!doc.gender.empty() && without_inf_indicator)      //!!!потом убрать
  {
    int is_female=CheckIn::is_female(doc.gender, "");
    if (is_female!=ASTRA::NoExists)
      doc.gender=(is_female==0?"M":"F");
    else
      doc.gender.clear();
  };                                                     //!!!потом убрать
  return !doc.empty();
};

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc)
{
  return LoadPaxDoco(ASTRA::NoExists, pax_id, doc);
};

bool LoadPaxDoco(TDateTime part_key, int pax_id, TPaxDocoItem &doc)
{
  doc.clear();
  const char* sql=
    "SELECT * FROM pax_doco WHERE pax_id=:pax_id";
  const char* sql_arx=
    "SELECT * FROM arx_pax_doco WHERE part_key=:part_key AND pax_id=:pax_id";
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

void ConvertDoca(const list<TPaxDocaItem> &doca,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD)
{
  docaB.clear();
  docaR.clear();
  docaD.clear();
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
  {
    if (d->type=="B") docaB=*d;
    if (d->type=="R") docaR=*d;
    if (d->type=="D") docaD=*d;
  };
};

bool LoadPaxDoca(int pax_id, list<TPaxDocaItem> &doca)
{
  return LoadPaxDoca(ASTRA::NoExists, pax_id, doca);
};

bool LoadPaxDoca(int pax_id, TDocaType type, TPaxDocaItem &doca)
{
  return LoadPaxDoca(ASTRA::NoExists, pax_id, type, doca);
};

bool LoadPaxDoca(TDateTime part_key, int pax_id, list<TPaxDocaItem> &doca)
{
  doca.clear();
  TPaxDocaItem docaItem;
  if (CheckIn::LoadPaxDoca(part_key, pax_id, docaDestination, docaItem))
    doca.push_back(docaItem);
  if (CheckIn::LoadPaxDoca(part_key, pax_id, docaResidence, docaItem))
    doca.push_back(docaItem);
  if (CheckIn::LoadPaxDoca(part_key, pax_id, docaBirth, docaItem))
    doca.push_back(docaItem);
  return !doca.empty();
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

bool LoadCrsPaxDoca(int pax_id, list<TPaxDocaItem> &doca)
{
  doca.clear();
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
      doca.push_back(TPaxDocaItem().fromDB(PaxDocaQry.get()));
      prior_type=PaxDocaQry.get().FieldAsString("type");
    };
  };
  return !doca.empty();
};

void SavePaxDoc(int pax_id, const TPaxDocItem &doc, TQuery& PaxDocQry)
{
  const char* sql=
        "BEGIN "
        "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
        "  IF :only_delete=0 THEN "
        "    INSERT INTO pax_doc "
        "      (pax_id,type,issue_country,no,nationality,birth_date,gender,expiry_date, "
        "       surname,first_name,second_name,pr_multi,type_rcpt,scanned_attrs) "
        "    VALUES "
        "      (:pax_id,:type,:issue_country,:no,:nationality,:birth_date,:gender,:expiry_date, "
        "       :surname,:first_name,:second_name,:pr_multi,:type_rcpt,:scanned_attrs); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("type",otString);
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
        "      (pax_id,birth_place,type,no,issue_place,issue_date,expiry_date,applic_country,scanned_attrs) "
        "    VALUES "
        "      (:pax_id,:birth_place,:type,:no,:issue_place,:issue_date,:expiry_date,:applic_country,:scanned_attrs); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("birth_place",otString);
    PaxDocQry.DeclareVariable("type",otString);
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
};

void SavePaxDoca(int pax_id, const list<TPaxDocaItem> &doca, TQuery& PaxDocaQry, bool new_checkin)
{
  list<TPaxDocaItem> doca2=doca;
  if (!doca2.empty() &&
      !(TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
        TReqInfo::Instance()->desk.compatible(APIS_CITY_REGION_VERSION)))
  {
    list<TPaxDocaItem> old_doca;
    if (new_checkin)
      LoadCrsPaxDoca(pax_id, old_doca); //данные из бронирования
    else
      LoadPaxDoca(ASTRA::NoExists, pax_id, old_doca);

    TPaxDocaItem old_docaB, old_docaR, old_docaD;
    ConvertDoca(old_doca, old_docaB, old_docaR, old_docaD);
    for(list<CheckIn::TPaxDocaItem>::iterator d=doca2.begin(); d!=doca2.end(); ++d)
    {
      CheckIn::TPaxDocaItem old_doca;
      if (d->type=="B") old_doca=old_docaB;
      if (d->type=="R") old_doca=old_docaR;
      if (d->type=="D") old_doca=old_docaD;

      string city, region;
      transform(old_doca.city.begin(), old_doca.city.end(), back_inserter(city), APIS::ReplaceDigit);
      transform(old_doca.region.begin(), old_doca.region.end(), back_inserter(region), APIS::ReplaceDigit);

      if (d->city==city) d->city=old_doca.city;
      if (d->region==region) d->region=old_doca.region;
    };
  };

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

bool LoadPaxNorms(int pax_id, vector< pair<TPaxNormItem, TNormItem> > &norms)
{
  norms.clear();
  const char* sql=
    "SELECT pax_norms.bag_type, pax_norms.norm_id, pax_norms.norm_trfer, "
    "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
    "FROM pax_norms,bag_norms "
    "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery NormQry(sql, QryParams);
  NormQry.get().Execute();
  for(;!NormQry.get().Eof;NormQry.get().Next())
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry.get()),
                               TNormItem().fromDB(NormQry.get())) );
  return !norms.empty();
};

bool LoadGrpNorms(int grp_id, vector< pair<TPaxNormItem, TNormItem> > &norms)
{
  norms.clear();
  const char* sql=
    "SELECT grp_norms.bag_type, grp_norms.norm_id, grp_norms.norm_trfer, "
    "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
    "FROM grp_norms,bag_norms "
    "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";

  QParams QryParams;
  QryParams << QParam("grp_id", otInteger, grp_id);
  TCachedQuery NormQry(sql, QryParams);
  NormQry.get().Execute();
  for(;!NormQry.get().Eof;NormQry.get().Next())
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry.get()),
                               TNormItem().fromDB(NormQry.get())) );
  return !norms.empty();
};

void LoadNorms(xmlNodePtr node, bool pr_unaccomp)
{
  if (node==NULL) return;
  xmlNodePtr node2=node->children;

  vector< pair<TPaxNormItem, TNormItem> > norms;
  if (!pr_unaccomp)
  {
    int pax_id=NodeAsIntegerFast("pax_id",node2);
    LoadPaxNorms(pax_id, norms);
  }
  else
  {
    int grp_id=NodeAsIntegerFast("grp_id",node2);
    LoadGrpNorms(grp_id, norms);
  };
  
  xmlNodePtr normsNode=NewTextChild(node,"norms");
  vector< pair<TPaxNormItem, TNormItem> >::const_iterator i=norms.begin();
  for(;i!=norms.end();++i)
  {
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    i->first.toXML(normNode);
    i->second.toXML(normNode);
  };
};

void SaveNorms(xmlNodePtr node, bool pr_unaccomp)
{
  if (node==NULL) return;
  xmlNodePtr node2=node->children;

  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode==NULL) return;

  TQuery NormQry(&OraSession);
  NormQry.Clear();
  if (!pr_unaccomp)
  {
    int pax_id;
    if (GetNodeFast("generated_pax_id",node2)!=NULL)
      pax_id=NodeAsIntegerFast("generated_pax_id",node2);
    else
      pax_id=NodeAsIntegerFast("pax_id",node2);
    NormQry.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
    NormQry.CreateVariable("pax_id",otInteger,pax_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO pax_norms(pax_id,bag_type,norm_id,norm_trfer) "
      "VALUES(:pax_id,:bag_type,:norm_id,:norm_trfer)";
  }
  else
  {
    int grp_id;
    if (GetNodeFast("generated_grp_id",node2)!=NULL)
      grp_id=NodeAsIntegerFast("generated_grp_id",node2);
    else
      grp_id=NodeAsIntegerFast("grp_id",node2);
    NormQry.SQLText="DELETE FROM grp_norms WHERE grp_id=:grp_id";
    NormQry.CreateVariable("grp_id",otInteger,grp_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO grp_norms(grp_id,bag_type,norm_id,norm_trfer) "
      "VALUES(:grp_id,:bag_type,:norm_id,:norm_trfer)";
  };
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  NormQry.DeclareVariable("norm_trfer",otInteger);
  for(normNode=normNode->children;normNode!=NULL;normNode=normNode->next)
  {
    TPaxNormItem().fromXML(normNode).toDB(NormQry);
    NormQry.Execute();
  };
};

const TPaxItem& TPaxItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  TReqInfo *reqInfo=TReqInfo::Instance();

  xmlNodePtr paxNode=node;
  NewTextChild(paxNode, "pax_id", id);
  NewTextChild(paxNode, "surname", surname);
  NewTextChild(paxNode, "name", name);
  NewTextChild(paxNode, "pers_type", EncodePerson(pers_type));
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
  if (reqInfo->desk.compatible(DOCS_VERSION))
  {
    if (DocExists) doc.toXML(paxNode);
    if (DocoExists) doco.toXML(paxNode);
  }
  else
  {
    NewTextChild(paxNode, "document", doc.no);
  };
  if (reqInfo->desk.compatible(DOCA_VERSION))
  {
    if (DocaExists)
    {
      xmlNodePtr docaNode=NewTextChild(paxNode, "addresses");
      for(list<TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
        d->toXML(docaNode);
    };
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
    if (PaxUpdatesPending)
      refuse=NodeAsStringFast("refuse",node2);

    if (tid==ASTRA::NoExists ||
        (PaxUpdatesPending && reqInfo->client_type==ASTRA::ctTerm))
    {
      tkn.fromXML(node);
      TknExists=true;
      if (reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS) &&
          !NodeIsNULLFast("bag_pool_num",node2))
        bag_pool_num=NodeAsIntegerFast("bag_pool_num",node2);
    };

    if (reqInfo->client_type==ASTRA::ctTerm)
    {
      //терминал
      if (reqInfo->desk.compatible(DOCS_VERSION))
      {
        doc.fromXML(GetNodeFast("document",node2));
        doco.fromXML(GetNodeFast("doco",node2));
        DocExists=true;
        DocoExists=true;
      }
      else
      {
        doc.no=NodeAsStringFast("document",node2);
        DocExists=true;
      };
      if (reqInfo->desk.compatible(DOCA_VERSION))
      {
        xmlNodePtr docaNode=GetNodeFast("addresses",node2);
        if (docaNode!=NULL)
        {
          for(docaNode=docaNode->children; docaNode!=NULL; docaNode=docaNode->next)
          {
            TPaxDocaItem docaItem;
            docaItem.fromXML(docaNode);
            if (docaItem.empty()) continue;
            doca.push_back(docaItem);
          };
        };
        DocaExists=true;
      };
    }
    else
    {
      //киоски и веб
      xmlNodePtr docNode=GetNodeFast("document",node2);
      if (docNode!=NULL) doc.fromXML(docNode);
      xmlNodePtr docoNode=GetNodeFast("doco",node2);
      if (docoNode!=NULL) doco.fromXML(docoNode);
      xmlNodePtr docaNode=GetNodeFast("addresses",node2);
      if (docaNode!=NULL)
      {
        for(docaNode=docaNode->children; docaNode!=NULL; docaNode=docaNode->next)
        {
          TPaxDocaItem docaItem;
          docaItem.fromXML(docaNode);
          if (docaItem.empty()) continue;
          doca.push_back(docaItem);
        };
      };

      DocExists=(tid==ASTRA::NoExists || docNode!=NULL);
      DocoExists=(tid==ASTRA::NoExists || docoNode!=NULL);
      DocaExists=(tid==ASTRA::NoExists || docaNode!=NULL);
    };
  };

  subcl=NodeAsStringFast("subclass",node2,"");
  return *this;
};

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
  if (Qry.GetVariableIndex("seat_type")>=0)
    Qry.SetVariable("seat_type", seat_type);
  if (Qry.GetVariableIndex("seats")>=0)
    Qry.SetVariable("seats", seats);
  if (Qry.GetVariableIndex("refuse")>=0)
    Qry.SetVariable("refuse", refuse);
  if (Qry.GetVariableIndex("pr_brd")>=0)
    pr_brd!=ASTRA::NoExists?Qry.SetVariable("pr_brd", (int)pr_brd):
                            Qry.SetVariable("pr_brd", FNull);
  if (Qry.GetVariableIndex("pr_exam")>=0)
    Qry.SetVariable("pr_exam", (int)pr_exam);
  if (Qry.GetVariableIndex("wl_type")>=0)
    Qry.SetVariable("wl_type", wl_type);
  if (Qry.GetVariableIndex("reg_no")>=0)
    Qry.SetVariable("reg_no", reg_no);
  if (Qry.GetVariableIndex("subclass")>=0)
    Qry.SetVariable("subclass", subcl);
  if (Qry.GetVariableIndex("bag_pool_num")>=0)
    bag_pool_num!=ASTRA::NoExists?Qry.SetVariable("bag_pool_num", bag_pool_num):
                                  Qry.SetVariable("bag_pool_num", FNull);
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  if (Qry.GetVariableIndex("ticket_no")>=0)
    tkn.toDB(Qry);
  return *this;
};

TPaxItem& TPaxItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
  seat_no=Qry.FieldAsString("seat_no");
  seat_type=Qry.FieldAsString("seat_type");
  seats=Qry.FieldAsInteger("seats");
  refuse=Qry.FieldAsString("refuse");
  reg_no=Qry.FieldAsInteger("reg_no");
  subcl=Qry.FieldAsString("subclass");
  bag_pool_num=Qry.FieldIsNULL("bag_pool_num")?ASTRA::NoExists:Qry.FieldAsInteger("bag_pool_num");
  tid=Qry.FieldAsInteger("tid");
  tkn.fromDB(Qry);
  TknExists=true;
  DocExists=CheckIn::LoadPaxDoc(id, doc);
  DocoExists=CheckIn::LoadPaxDoco(id, doco);
  DocaExists=CheckIn::LoadPaxDoca(id, doca);
  return *this;
};

int TPaxItem::is_female() const
{
  if (!DocExists) return ASTRA::NoExists;

  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(DOCS_VERSION))
  {
    return CheckIn::is_female(doc.gender, name);
  }
  else
  {
    TPaxDocItem crs_doc;
    if (id!=ASTRA::NoExists) LoadCrsPaxDoc(id, crs_doc);
    return CheckIn::is_female(crs_doc.gender, name);
  };
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

TAPISItem& TAPISItem::fromDB(int pax_id)
{
  clear();
  CheckIn::LoadPaxDoc(pax_id, doc);
  CheckIn::LoadPaxDoco(pax_id, doco);
  CheckIn::LoadPaxDoca(pax_id, doca);
  return *this;
};

const TAPISItem& TAPISItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr apisNode=NewTextChild(node, "apis");
  doc.toXML(apisNode);
  doco.toXML(apisNode);
  xmlNodePtr docaNode=NewTextChild(apisNode, "addresses");
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
    d->toXML(docaNode);
  return *this;
};

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
  NewTextChild(grpNode, "tid", tid);
  return *this;
};

TPaxGrpItem& TPaxGrpItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  id=NodeAsIntegerFast("grp_id",node2,ASTRA::NoExists);
  point_dep=NodeAsIntegerFast("point_dep",node2);
  airp_dep=NodeAsStringFast("airp_dep",node2);
  point_arv=NodeAsIntegerFast("point_arv",node2);
  airp_arv=NodeAsStringFast("airp_arv",node2);
  cl=NodeAsStringFast("class",node2);
  if (NodeIsNULLFast("status",node2,true))
  {
    if (id!=ASTRA::NoExists)
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText="SELECT status FROM pax_grp WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id", otInteger, id);
      Qry.Execute();
      if (!Qry.Eof && !Qry.FieldIsNULL("status"))
        status=DecodePaxStatus(Qry.FieldAsString("status"));
    };
  }
  else status=DecodePaxStatus(NodeAsStringFast("status",node2));
  tid=NodeAsIntegerFast("tid",node2,ASTRA::NoExists);
  return *this;
};

TPaxGrpItem& TPaxGrpItem::fromXMLadditional(xmlNodePtr node)
{
  excess=ASTRA::NoExists;
  hall=ASTRA::NoExists;
  bag_refuse.clear();

  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  excess=NodeAsIntegerFast("excess",node2);
  if (tid!=ASTRA::NoExists)
  {
    //запись изменений
    bag_refuse=NodeAsStringFast("bag_refuse",node2);
  };

  //зал
  TReqInfo *reqInfo=TReqInfo::Instance();
  if (reqInfo->client_type == ASTRA::ctTerm)
  {
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
    {
      hall=NodeAsIntegerFast("hall",node2);
    }
    else
    {
      if (tid==ASTRA::NoExists)
      {
        //новая регистрация
        hall=NodeAsIntegerFast("hall",node2);
        JxtContext::getJxtContHandler()->sysContext()->write("last_hall_id", hall);
      }
      else
      {
        //запись изменений
        TQuery Qry(&OraSession);
        //попробуем считать зал из контекста пульта
        hall=JxtContext::getJxtContHandler()->sysContext()->readInt("last_hall_id", ASTRA::NoExists);
        if (hall==ASTRA::NoExists)
        {
          //ничего в контексте нет - берем зал из station_halls, но только если он один
          Qry.Clear();
          Qry.SQLText=
            "SELECT station_halls.hall "
            "FROM station_halls,stations "
            "WHERE station_halls.airp=stations.airp AND "
            "      station_halls.station=stations.name AND "
            "      stations.desk=:desk AND stations.work_mode=:work_mode";
          Qry.CreateVariable("desk",otString, TReqInfo::Instance()->desk.code);
          Qry.CreateVariable("work_mode",otString,"Р");
          Qry.Execute();
          if (!Qry.Eof)
          {
            hall=Qry.FieldAsInteger("hall");
            Qry.Next();
            if (!Qry.Eof) hall=ASTRA::NoExists;
          };
        };
        if (hall==ASTRA::NoExists)
        {
          //берем зал из pax_grp для этой группы
          Qry.Clear();
          Qry.SQLText="SELECT hall FROM pax_grp WHERE grp_id=:grp_id";
          Qry.CreateVariable("grp_id", otInteger, id);
          Qry.Execute();
          if (!Qry.Eof && !Qry.FieldIsNULL("hall"))
            hall=Qry.FieldAsInteger("hall");
        };
      };
    };
  };


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
  Qry.SetVariable("excess", excess);
  if (Qry.GetVariableIndex("hall")>=0)
    hall!=ASTRA::NoExists?Qry.SetVariable("hall", hall):
                          Qry.SetVariable("hall", FNull);
  if (Qry.GetVariableIndex("bag_refuse")>=0)
    Qry.SetVariable("bag_refuse",(int)(!bag_refuse.empty()));
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  return *this;
};

TPaxGrpItem& TPaxGrpItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("grp_id");
  point_dep=Qry.FieldAsInteger("point_dep");
  point_arv=Qry.FieldAsInteger("point_arv");
  airp_dep=Qry.FieldAsString("airp_dep");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  status=DecodePaxStatus(Qry.FieldAsString("status"));
  excess=Qry.FieldAsInteger("excess");
  if (!Qry.FieldIsNULL("hall"))
    hall=Qry.FieldAsInteger("hall");
  if (Qry.FieldAsInteger("bag_refuse")!=0)
    bag_refuse=ASTRA::refuseAgentError;
  tid=Qry.FieldAsInteger("tid");
  return *this;
};

}; //namespace CheckIn



