#include "apis_utils.h"
#include "misc.h"
#include "apps_interaction.h"
#include "httpClient.h"
#include "astra_service.h"
#include <pion/http/parser.hpp>

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace AstraLocale;
using namespace EXCEPTIONS;

const std::string PARAM_URL = "URL";
const std::string PARAM_ACTION_CODE = "ACTION_CODE";
const std::string PARAM_LOGIN = "LOGIN";
const std::string PARAM_PASSWORD = "PASSWORD";

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

TCompleteCheckDocInfo GetCheckDocInfo(const int point_dep, const string& airp_arv)
{
  set<string> apis_formats;
  return GetCheckDocInfo(point_dep, airp_arv, apis_formats);
}

TCompleteCheckDocInfo GetCheckDocInfo(const int point_dep, const string& airp_arv, set<string> &apis_formats)
{
  apis_formats.clear();
  TCompleteCheckDocInfo result;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.airline, points.flt_no, points.suffix, points.airp, points.scd_out, "
    "       trip_sets.pr_reg_with_doc "
    "FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id(+) AND points.point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();

  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("pr_reg_with_doc") &&
        Qry.FieldAsInteger("pr_reg_with_doc")!=0)
    {
      result.pass.doc.required_fields|=DOC_NO_FIELD;
      result.crew.doc.required_fields|=DOC_NO_FIELD;
    };

    TTripInfo fltInfo(Qry);
    if (GetTripSets(tsMintransFile, fltInfo))
    {
        result.pass.doc.required_fields|=DOC_MINTRANS_FIELDS;
        result.crew.doc.required_fields|=DOC_MINTRANS_FIELDS;
    };

    if(isNeedAPPSReq(point_dep, airp_arv)) {
      apis_formats.insert("APPS_SITA");
      result.pass.doc.required_fields|=DOC_APPS_SITA_FIELDS;
      result.crew.doc.required_fields|=DOC_APPS_SITA_FIELDS;
    }

    try
    {
      string airline, country_dep, country_arv, city;
      airline=Qry.FieldAsString("airline");
      city=base_tables.get("airps").get_row("code", Qry.FieldAsString("airp") ).AsString("city");
      country_dep=base_tables.get("cities").get_row("code", city).AsString("country");
      city=base_tables.get("airps").get_row("code", airp_arv ).AsString("city");
      country_arv=base_tables.get("cities").get_row("code", city).AsString("country");

      Qry.Clear();
      Qry.SQLText=
        "SELECT format FROM apis_sets "
        "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND "
        "      pr_denial=0";
      Qry.CreateVariable("airline", otString, airline);
      Qry.CreateVariable("country_dep", otString, country_dep);
      Qry.CreateVariable("country_arv", otString, country_arv);
      Qry.Execute();
      if (!Qry.Eof)
      {
        bool is_inter=!(country_dep=="��" && country_arv=="��");
        result.pass.doc.is_inter=is_inter;
        result.pass.doco.is_inter=is_inter;
        result.pass.docaB.is_inter=is_inter;
        result.pass.docaR.is_inter=is_inter;
        result.pass.docaD.is_inter=is_inter;
        result.crew.doc.is_inter=is_inter;
        result.crew.doco.is_inter=is_inter;
        result.crew.docaB.is_inter=is_inter;
        result.crew.docaR.is_inter=is_inter;
        result.crew.docaD.is_inter=is_inter;
        for(;!Qry.Eof;Qry.Next())
        {
          string fmt=Qry.FieldAsString("format");
          apis_formats.insert(fmt);
          if (fmt=="CSV_CZ")
          {
            result.pass.doc.required_fields|=DOC_CSV_CZ_FIELDS;
          };
          if (fmt=="EDI_CZ")
          {
            result.pass.doc.required_fields|=DOC_EDI_CZ_FIELDS;
          };
          if (fmt=="EDI_CN")
          {
            result.pass.doc.required_fields|=DOC_EDI_CN_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_CN_FIELDS;
          };
          if (fmt=="EDI_IN")
          {
            result.pass.doc.required_fields|=DOC_EDI_IN_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_IN_FIELDS;
          };
          if (fmt=="EDI_US")
          {
            result.pass.doc.required_fields|=DOC_EDI_US_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_US_FIELDS;
            result.crew.docaB.required_fields|=DOCA_B_CREW_EDI_US_FIELDS;
            result.pass.docaR.required_fields|=DOCA_R_PASS_EDI_US_FIELDS;
            result.crew.docaR.required_fields|=DOCA_R_CREW_EDI_US_FIELDS;
            result.pass.docaD.required_fields|=DOCA_D_PASS_EDI_US_FIELDS;
          };
          if (fmt=="EDI_USBACK")
          {
            result.pass.doc.required_fields|=DOC_EDI_USBACK_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_USBACK_FIELDS;
            result.crew.docaB.required_fields|=DOCA_B_CREW_EDI_USBACK_FIELDS;
            result.pass.docaR.required_fields|=DOCA_R_PASS_EDI_USBACK_FIELDS;
            result.crew.docaR.required_fields|=DOCA_R_CREW_EDI_USBACK_FIELDS;
            result.pass.docaD.required_fields|=DOCA_D_PASS_EDI_USBACK_FIELDS;
          };
          if (fmt=="EDI_UK")
          {
            result.pass.doc.required_fields|=DOC_EDI_UK_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_UK_FIELDS;
          };
          if (fmt=="EDI_ES")
          {
            result.pass.doc.required_fields|=DOC_EDI_ES_FIELDS;
            result.crew.doc.required_fields|=DOC_EDI_ES_FIELDS;
          };
          if (fmt=="CSV_DE")
          {
            result.pass.doc.required_fields|=DOC_CSV_DE_FIELDS;
            result.pass.doco.required_fields|=DOCO_CSV_DE_FIELDS;
          };
          if (fmt=="TXT_EE")
          {
            result.pass.doc.required_fields|=DOC_TXT_EE_FIELDS;
            result.pass.doco.required_fields|=DOCO_TXT_EE_FIELDS;
          };
          if (fmt=="XML_TR")
          {
            result.pass.doc.required_fields|=DOC_XML_TR_FIELDS;
            result.crew.doc.required_fields|=DOC_XML_TR_FIELDS;
          };
          if (fmt=="CSV_AE")
          {
            result.pass.doc.required_fields|=DOC_CSV_AE_FIELDS;
            result.crew.doc.required_fields|=DOC_CSV_AE_FIELDS;
          };
          if (fmt=="EDI_LT")
          {
            result.pass.doc.required_fields|=DOC_EDI_LT_FIELDS;
          };
        };
      };
      if (apis_formats.empty())
      {
          result.pass.doco.not_apis=true;
          result.crew.doco.not_apis=true;
          result.pass.docaB.not_apis=true;
          result.crew.docaB.not_apis=true;
          result.pass.docaR.not_apis=true;
          result.crew.docaR.not_apis=true;
          result.pass.docaD.not_apis=true;
          result.crew.docaD.not_apis=true;
      }
    }
    catch(EBaseTableError) {};

  };
  return result;
}

TCompleteCheckTknInfo GetCheckTknInfo(const int point_dep)
{
  TCompleteCheckTknInfo result;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.airline, points.flt_no, points.suffix, points.airp, points.scd_out, "
    "       trip_sets.pr_reg_with_tkn "
    "FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id(+) AND points.point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();

  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("pr_reg_with_tkn") &&
        Qry.FieldAsInteger("pr_reg_with_tkn")!=0) result.pass.tkn.required_fields|=TKN_TICKET_NO_FIELD;

    TTripInfo fltInfo(Qry);
    if (GetTripSets(tsMintransFile, fltInfo)) result.pass.tkn.required_fields|=TKN_MINTRANS_FIELDS;
  };
  return result;
}

void GetAPISSets( const int point_id, TAPISMap &apis_map, set<string> &apis_formats)
{
  apis_map.clear();
  apis_formats.clear();

  TTripRoute route;
  route.GetRouteAfter(NoExists,point_id,trtNotCurrent,trtNotCancelled);
  for(TTripRoute::iterator r = route.begin(); r != route.end(); ++r)
  {
    set<string> formats;
    TCompleteCheckDocInfo check_info=GetCheckDocInfo(point_id, r->airp, formats);
    apis_map.insert( make_pair(r->airp, make_pair(check_info, formats)));
    apis_formats.insert(formats.begin(), formats.end());
  };
}

bool CheckLetDigSpace(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' )
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

bool CheckLetSpaceDash(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || *i==' ' || *i=='-' || (checkDocInfo.not_apis && *i=='.'))
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

bool CheckLetDigSpaceDash(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' || *i=='-' || (checkDocInfo.not_apis && (*i=='.' || *i=='/')))
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

string ElemToPaxDocCountryId(const string &elem, TElemFmt &fmt)
{
  string result=ElemToElemId(etPaxDocCountry,elem,fmt);
  if (fmt==efmtUnknown)
  {
    //�஢�ਬ countries
    string country=ElemToElemId(etCountry,elem,fmt);
    if (fmt!=efmtUnknown)
    {
      fmt=efmtUnknown;
      //������ � pax_doc_countries.country
      try
      {
        result=ElemToElemId(etPaxDocCountry,
                            getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code"),
                            fmt);
      }
      catch (EBaseTableError) {};
    };
  };
  return result;
}

void throwInvalidSymbol(const string &fieldname,
                        const TCheckDocTknInfo &checkDocInfo,
                        const string &symbol)
{
  (symbol.size()!=1?
    throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))):
    (checkDocInfo.is_inter && IsLetter(symbol[0]) && !IsAscii7(symbol[0])?
      throw UserException("WRAP.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))
                                                               <<LParam("text", LexemaData("MSG.FIELD_CONSIST_LAT_CHARS"))):
      throw UserException("WRAP.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))
                                                               <<LParam("text", LexemaData("MSG.INVALID_SYMBOL", LParams()<<LParam("symbol", symbol))))
    )
  );
}

void CheckDoc(const CheckIn::TPaxDocItem &doc,
              const TCheckDocTknInfo &checkDocInfo,
              TDateTime nowLocal)
{
  string::size_type errorIdx;

  modf(nowLocal, &nowLocal);

  if (doc.birth_date!=NoExists && doc.birth_date>nowLocal)
    throw UserException("MSG.CHECK_DOC.INVALID_BIRTH_DATE", LParams()<<LParam("fieldname", "document/birth_date" ));

  if (doc.expiry_date!=NoExists && doc.expiry_date<nowLocal)
    throw UserException("MSG.CHECK_DOC.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "document/expiry_date" ));

  if (!CheckLetDigSpace(doc.no, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> document/no: %s", doc.no.c_str());
    throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" ));
  };

  if (!CheckLetSpaceDash(doc.surname, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> document/surname: %s", doc.surname.c_str());
    throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" ));
  };

  if (!CheckLetSpaceDash(doc.first_name, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> document/first_name: %s", doc.first_name.c_str());
    throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" ));
  };

  if (!CheckLetSpaceDash(doc.second_name, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> document/second_name: %s", doc.second_name.c_str());
    throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" ));
  };
}

CheckIn::TPaxDocItem NormalizeDoc(const CheckIn::TPaxDocItem &doc)
{
  CheckIn::TPaxDocItem result;
  TElemFmt fmt;

  result.type = doc.type;
  result.type = TrimString(result.type);
  if (!result.type.empty())
  {
    result.type=ElemToElemId(etPaxDocType, upperc(result.type), fmt);
    if (fmt==efmtUnknown || result.type=="V")
      throw UserException("MSG.CHECK_DOC.INVALID_TYPE", LParams()<<LParam("fieldname", "document/type" ));
  };
  result.issue_country = doc.issue_country;
  result.issue_country = TrimString(result.issue_country);
  if (!result.issue_country.empty())
  {
    result.issue_country=ElemToPaxDocCountryId(upperc(result.issue_country), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_ISSUE_COUNTRY", LParams()<<LParam("fieldname", "document/issue_country" ));
  };

  result.no = upperc(doc.no);
  result.no = TrimString(result.no);
  if (result.no.size()>15)
    throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" ));

  result.nationality = doc.nationality;
  result.nationality = TrimString(result.nationality);
  if (!result.nationality.empty())
  {
    result.nationality=ElemToPaxDocCountryId(upperc(result.nationality), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_NATIONALITY", LParams()<<LParam("fieldname", "document/nationality" ));
  };

  if (doc.birth_date!=NoExists)
    modf(doc.birth_date, &result.birth_date);

  result.gender = doc.gender;
  result.gender = TrimString(result.gender);
  if (!result.gender.empty())
  {
    result.gender=ElemToElemId(etGenderType, upperc(result.gender), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_GENDER", LParams()<<LParam("fieldname", "document/gender" ));
  };

  if (doc.expiry_date!=NoExists)
    modf(doc.expiry_date, &result.expiry_date);

  result.surname = upperc(doc.surname);
  result.surname = TrimString(result.surname);
  if (result.surname.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" ));

  result.first_name = upperc(doc.first_name);
  result.first_name = TrimString(result.first_name);
  if (result.first_name.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" ));

  result.second_name = upperc(doc.second_name);
  result.second_name = TrimString(result.second_name);
  if (result.second_name.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" ));

  return result;
}

void CheckDoco(const CheckIn::TPaxDocoItem &doc,
               const TCheckDocTknInfo &checkDocInfo,
               TDateTime nowLocal)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    string::size_type errorIdx;

    modf(nowLocal, &nowLocal);

    if (doc.issue_date!=NoExists && doc.issue_date>nowLocal)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_DATE", LParams()<<LParam("fieldname", "doco/issue_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.ISSUE_DATE")));

    if (doc.expiry_date!=NoExists && doc.expiry_date<nowLocal)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "doco/expiry_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.EXPIRY_DATE")));

    if (!CheckLetDigSpaceDash(doc.birth_place, checkDocInfo, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/birth_place: %s", doc.birth_place.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" )):
        throwInvalidSymbol("CAP.PAX_DOCO.BIRTH_PLACE", checkDocInfo, (errorIdx==string::npos?"":doc.birth_place.substr(errorIdx, 1)));
    };

    if (!CheckLetDigSpace(doc.no, checkDocInfo, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/no: %s", doc.no.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" )):
        throwInvalidSymbol("CAP.PAX_DOCO.NO", checkDocInfo, (errorIdx==string::npos?"":doc.no.substr(errorIdx, 1)));
    };

    if (!CheckLetDigSpaceDash(doc.issue_place, checkDocInfo, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/issue_place: %s", doc.issue_place.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" )):
        throwInvalidSymbol("CAP.PAX_DOCO.ISSUE_PLACE", checkDocInfo, (errorIdx==string::npos?"":doc.issue_place.substr(errorIdx, 1)));
    };

    //�஢��塞 ������
    if (reqInfo->client_type==ctTerm)
    {
      long int mask=doc.getNotEmptyFieldsMask();

      for(int pass=0; pass<7; pass++)
      {
        long int FIELD=NO_FIELDS;
        string CAP;
        switch(pass)
        {
          case 0:
            FIELD=DOCO_BIRTH_PLACE_FIELD;
            CAP="CAP.PAX_DOCO.BIRTH_PLACE";
            break;
          case 1:
            FIELD=DOCO_TYPE_FIELD;
            CAP="CAP.PAX_DOCO.TYPE";
            break;
          case 2:
            FIELD=DOCO_NO_FIELD;
            CAP="CAP.PAX_DOCO.NO";
            break;
          case 3:
            FIELD=DOCO_ISSUE_PLACE_FIELD;
            CAP="CAP.PAX_DOCO.ISSUE_PLACE";
            break;
          case 4:
            FIELD=DOCO_ISSUE_DATE_FIELD;
            CAP="CAP.PAX_DOCO.ISSUE_DATE";
            break;
          case 5:
            FIELD=DOCO_APPLIC_COUNTRY_FIELD;
            CAP="CAP.PAX_DOCO.APPLIC_COUNTRY";
            break;
          case 6:
            FIELD=DOCO_EXPIRY_DATE_FIELD;
            CAP="CAP.PAX_DOCO.EXPIRY_DATE";
            break;
        };
        if ((mask & FIELD) == 0 && (checkDocInfo.required_fields & FIELD) != 0)
          throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(CAP)));
      };
    };
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      throw UserException("WRAP.PAX_DOCO.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
    else
      throw;
  };
}

CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    CheckIn::TPaxDocoItem result;
    TElemFmt fmt;

    result.birth_place = upperc(doc.birth_place);
    result.birth_place = TrimString(result.birth_place);
    if (result.birth_place.size()>35)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.BIRTH_PLACE")));

    result.type = doc.type;
    result.type = TrimString(result.type);
    if (!result.type.empty())
    {
      result.type=ElemToElemId(etPaxDocType, upperc(result.type), fmt);
      if (fmt==efmtUnknown ||
          ((reqInfo->client_type == ctWeb ||
            reqInfo->client_type == ctKiosk ||
            reqInfo->client_type == ctMobile) && result.type!="V"))
        reqInfo->client_type!=ctTerm?
              throw UserException("MSG.CHECK_DOCO.INVALID_TYPE", LParams()<<LParam("fieldname", "doco/type" )):
              throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.TYPE")));
    };

    result.no = upperc(doc.no);
    result.no = TrimString(result.no);
    if (result.no.size()>25)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.NO")));

    result.issue_place = upperc(doc.issue_place);
    result.issue_place = TrimString(result.issue_place);
    if (result.issue_place.size()>35)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.ISSUE_PLACE")));

    if (doc.issue_date!=NoExists)
      modf(doc.issue_date, &result.issue_date);

    if (doc.expiry_date!=NoExists)
      modf(doc.expiry_date, &result.expiry_date);

    result.applic_country = doc.applic_country;
    result.applic_country = TrimString(result.applic_country);
    if (!result.applic_country.empty())
    {
      result.applic_country=ElemToPaxDocCountryId(upperc(result.applic_country), fmt);
      if (fmt==efmtUnknown)
        reqInfo->client_type!=ctTerm?
              throw UserException("MSG.CHECK_DOCO.INVALID_APPLIC_COUNTRY", LParams()<<LParam("fieldname", "doco/applic_country" )):
              throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.APPLIC_COUNTRY")));
    };

    if (reqInfo->client_type == ctHTTP)
      result.ReplacePunctSymbols();

    return result;
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      throw UserException("WRAP.PAX_DOCO.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
    else
      throw;
  };
}

void CheckDoca(const CheckIn::TPaxDocaItem &doc,
               const TCheckDocTknInfo &checkDocInfo)
{
    string::size_type errorIdx;
    if (!CheckLetDigSpaceDash(doc.address, checkDocInfo, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/address: %s", doc.address.c_str());
        throw UserException("MSG.CHECK_DOCA.INVALID_ADDRESS", LParams()<<LParam("fieldname", "doca/address" ));
    };

    if (!CheckLetDigSpaceDash(doc.city, checkDocInfo, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/city: %s", doc.city.c_str());
        throw UserException("MSG.CHECK_DOCA.INVALID_CITY", LParams()<<LParam("fieldname", "doca/city" ));
    };

    if (!CheckLetDigSpaceDash(doc.region, checkDocInfo, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/region: %s", doc.region.c_str());
        throw UserException("MSG.CHECK_DOCA.INVALID_REGION", LParams()<<LParam("fieldname", "doca/region" ));
    };

    if (!CheckLetDigSpaceDash(doc.postal_code, checkDocInfo, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/postal_code: %s", doc.postal_code.c_str());
        throw UserException("MSG.CHECK_DOCA.INVALID_POSTAL_CODE", LParams()<<LParam("fieldname", "doca/postal_code" ));
    };
}

CheckIn::TPaxDocaItem NormalizeDoca(const CheckIn::TPaxDocaItem &doc)
{
    CheckIn::TPaxDocaItem result;
    TElemFmt fmt;

    result.type = upperc(doc.type);
    result.type = TrimString(result.type);
    if (!(result.type == "B" || result.type == "R" || result.type == "D"))
        throw EXCEPTIONS::Exception("Invalid <doca> type. B, R or D is required");

    result.country = doc.country;
    result.country = TrimString(result.country);
    if (!result.country.empty())
    {
      result.country = ElemToPaxDocCountryId(upperc(result.country), fmt);
      if (fmt==efmtUnknown)
        throw UserException("MSG.CHECK_DOCA.INVALID_COUNTRY", LParams()<<LParam("fieldname", "doca/country"));
    }
    result.address = upperc(doc.address);
    result.address = TrimString(result.address);
    if (result.address.size() > 35) {
        throw UserException("MSG.CHECK_DOCO.INVALID_ADDRESS", LParams()<<LParam("fieldname", "doca/address"));
    }
    result.city = upperc(doc.city);
    result.city = TrimString(result.city);
    if (result.city.size() > 35) {
        throw UserException("MSG.CHECK_DOCO.INVALID_SITY", LParams()<<LParam("fieldname", "doca/city"));
    }
    result.region = upperc(doc.region);
    result.region = TrimString(result.region);
    if (result.region.size() > 35) {
        throw UserException("MSG.CHECK_DOCO.INVALID_REGION", LParams()<<LParam("fieldname", "doca/region"));
    }
    result.postal_code = doc.postal_code;
    result.postal_code = TrimString(result.postal_code);
    if (result.postal_code.size() > 17) {
        throw UserException("MSG.CHECK_DOCO.INVALID_POSTAL_CODE", LParams()<<LParam("fieldname", "doca/postal_code"));
    }

    if (TReqInfo::Instance()->client_type == ctHTTP)
      result.ReplacePunctSymbols();

    return result;
}

namespace APIS
{
const char *AlarmTypeS[] = {
    "APIS_DIFFERS_FROM_BOOKING",
    "APIS_INCOMPLETE",
    "APIS_MANUAL_INPUT"
};

string EncodeAlarmType(const TAlarmType alarm )
{
    if(alarm < 0 or alarm >= atLength)
        throw EXCEPTIONS::Exception("InboundTrfer::EncodeAlarmType: wrong alarm type %d", alarm);
    return AlarmTypeS[ alarm ];
};

}; //namespace APIS

const TFilterQueue& getApisFilter( const std::string& type ) {
  static std::map<std::string, TFilterQueue> filter;
  std::map<std::string, TFilterQueue>::const_iterator it = filter.find( type );
  if ( it == filter.end() ) {
    it = filter.insert( make_pair( type,
          TFilterQueue( OWN_POINT_ADDR(), type, ASTRA::NoExists,
                        ASTRA::NoExists, false, 10 ) ) ).first;
  }
  return it->second;
}

std::string toSoapReq( const std::string& text, const std::string& login, const std::string& pswd, const std::string& type )
{
  XMLDoc soap_reqDoc;
  soap_reqDoc.set("soapenv:Envelope");
  if (soap_reqDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("toSoapReq: CreateXMLDoc failed");
  xmlNodePtr soapNode=xmlDocGetRootElement(soap_reqDoc.docPtr());
  SetProp(soapNode, "xmlns:soapenv", "http://schemas.xmlsoap.org/soap/envelope/");
  xmlNodePtr headerNode = NewTextChild(soapNode, "soapenv:Header");
  if ( type == APIS_LT ) {
    xmlNodePtr securityNode = NewTextChild(headerNode, "Security");
    SetProp(securityNode, "xmlns", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd");
    xmlNodePtr tokenNode = NewTextChild(securityNode, "UsernameToken");
    NewTextChild(tokenNode, "Username", login);
    xmlNodePtr pswdNode = NewTextChild(tokenNode, "Password", pswd);
    SetProp(pswdNode, "Type", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordText");
  }
  xmlNodePtr bodyNode = NewTextChild(soapNode, "soapenv:Body");
  if ( type == APIS_TR ) {
    xmlNodePtr operationNode = NewTextChild( bodyNode, "voy:getFlightMessageSIN" );
    SetProp( operationNode, "xmlns:voy", "http://www.gtb.gov.tr/voy.xml.webservices" );
    xmlNodePtr apisNode = NewTextChild( operationNode, "FlightMessage" );
    xmlDocPtr xml_text = NULL;
    try {
        xml_text = TextToXMLTree( text );
        if(xml_text == NULL)
            throw Exception( "TextToXMLTree failed" );
        xmlNodePtr srcNode = xmlDocGetRootElement( xml_text );
        CopyNodeList( apisNode,srcNode );
        xmlFreeDoc( xml_text );
    } catch( Exception &E ) {
        if( xml_text )
            xmlFreeDoc( xml_text );
        ProgError( STDLOG, "�訡�� ࠧ��� XML. '%s' : %s", text.c_str(), E.what() );
        throw;
    } catch(...) {
        if( xml_text )
            xmlFreeDoc( xml_text );
        ProgError( STDLOG, "�訡�� ࠧ��� XML. '%s'", text.c_str() );
        throw;
    }
  }
  else if ( type == APIS_LT ) {
    xmlNodePtr operationNode = NewTextChild( bodyNode, "Push" );
    SetProp( operationNode, "xmlns", "https://www.pnr.lt/ns/2014" );
    NewTextChild( operationNode, "pPnrXml", text );
  }
  return GetXMLDocText( soap_reqDoc.docPtr() );
}

void send_apis_tr ()
{
  send_apis( APIS_TR );
}

void send_apis_lt ()
{
  send_apis( APIS_LT );
}

void send_apis ( const std::string type )
{
  TFileQueue file_queue;
  file_queue.get( getApisFilter( type ) );
  ProgTrace(TRACE5, "send_apis: Num of %s items in queue: %zu \n", type.c_str(), file_queue.size());
  for ( TFileQueue::iterator item=file_queue.begin(); item!=file_queue.end(); item++) {
      if ( item->params.find( PARAM_URL ) == item->params.end() ||
              item->params[ PARAM_URL ].empty() )
          throw Exception("url not specified");
      if ( item->params.find( PARAM_ACTION_CODE ) == item->params.end() ||
              item->params[ PARAM_ACTION_CODE ].empty() )
          throw Exception("action_code not specified");
      if ( item->params.find( PARAM_LOGIN ) == item->params.end() ||
              item->params[ PARAM_LOGIN ].empty() )
          throw Exception("login not specified");
      if ( item->params.find( PARAM_PASSWORD ) == item->params.end() ||
              item->params[ PARAM_PASSWORD ].empty() )
          throw Exception("password not specified");
      RequestInfo request;
      std::string proto;
      std::string query;
      if(not pion::http::parser::parse_uri(item->params[PARAM_URL], proto, request.host, request.port, request.path, query))
        throw Exception("parse_uri failed for '%s'", item->params[PARAM_URL].c_str());
      request.action = item->params[PARAM_ACTION_CODE];
      request.login = item->params[PARAM_LOGIN];
      request.pswd = item->params[PARAM_PASSWORD];
      request.content = toSoapReq( item->data, request.login, request.pswd, type );
      request.using_ssl = (proto=="https")?true:false;
      request.timeout = 60000;
      TFileQueue::sendFile(item->id);
      ResponseInfo response;
      httpClient_main(request, response);
      process_reply(response.content, type);
      TFileQueue::doneFile(item->id);
      createMsg( *item, evCommit );
  }
}

void process_reply( const std::string& result, const std::string& type )
{
  if( result.empty() )
    throw Exception( "Result is empty" );

  xmlDocPtr doc = NULL;
  doc = TextToXMLTree( result );
  if( doc == NULL )
    throw Exception( "Wrong answer XML" );
  try
  {
    xmlNodePtr rootNode=xmlDocGetRootElement( doc );
    xmlNodePtr node = rootNode->children;
    node = NodeAsNodeFast( "Body", node );
    if( !node )
      throw Exception( "Body tag not found" );
    node = node->children;
    string name = ( type == APIS_TR )?"getFlightMessageResponse":"PushResponse";
    node = NodeAsNodeFast( name.c_str(), node );
    if( !node )
      throw Exception( "%s tag not found", name.c_str() );
    if ( type == APIS_TR ) {
      node = node->children;
      node = NodeAsNodeFast( "Statu", node );
      if( !node )
        throw Exception( "Statu tag not found" );
      node = node->children;
      node = NodeAsNodeFast( "explanation", node );
      std::string status = ( node ? NodeAsString(node) : "" );
      if( status != "OK" )
        throw Exception( "Return status not OK: '%s'", status.c_str() );
    }
    ProgTrace(TRACE5, "process_reply: successful");
  }
  catch(...)
  {
    xmlFreeDoc( doc );
    throw;
  }
  xmlFreeDoc( doc );
}
