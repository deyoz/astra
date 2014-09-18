#include "apis_utils.h"
#include "misc.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace AstraLocale;

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
        bool is_inter=!(country_dep=="РФ" && country_arv=="РФ");
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
    //проверим countries
    string country=ElemToElemId(etCountry,elem,fmt);
    if (fmt!=efmtUnknown)
    {
      fmt=efmtUnknown;
      //найдем в pax_doc_countries.country
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
  string::size_type errorIdx;

  modf(nowLocal, &nowLocal);

  if (doc.issue_date!=NoExists && doc.issue_date>nowLocal)
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_DATE", LParams()<<LParam("fieldname", "doco/issue_date" ));

  if (doc.expiry_date!=NoExists && doc.expiry_date<nowLocal)
    throw UserException("MSG.CHECK_DOCO.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "doco/expiry_date" ));

  if (!CheckLetDigSpaceDash(doc.birth_place, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> doco/birth_place: %s", doc.birth_place.c_str());
    throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" ));
  };

  if (!CheckLetDigSpace(doc.no, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> doco/no: %s", doc.no.c_str());
    throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" ));
  };

  if (!CheckLetDigSpaceDash(doc.issue_place, checkDocInfo, errorIdx))
  {
    ProgTrace(TRACE5, ">>>> doco/issue_place: %s", doc.issue_place.c_str());
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" ));
  };
}

CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  CheckIn::TPaxDocoItem result;
  TElemFmt fmt;

  result.birth_place = upperc(doc.birth_place);
  result.birth_place = TrimString(result.birth_place);
  if (result.birth_place.size()>35)
    throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" ));

  result.type = doc.type;
  result.type = TrimString(result.type);
  if (!result.type.empty())
  {
    result.type=ElemToElemId(etPaxDocType, upperc(result.type), fmt);
    if (fmt==efmtUnknown || result.type!="V")
      throw UserException("MSG.CHECK_DOCO.INVALID_TYPE", LParams()<<LParam("fieldname", "doco/type" ));
  };

  result.no = upperc(doc.no);
  result.no = TrimString(result.no);
  if (result.no.size()>25)
    throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" ));

  result.issue_place = upperc(doc.issue_place);
  result.issue_place = TrimString(result.issue_place);
  if (result.issue_place.size()>35)
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" ));

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
      throw UserException("MSG.CHECK_DOCO.INVALID_APPLIC_COUNTRY", LParams()<<LParam("fieldname", "doco/applic_country" ));
  };

  if (reqInfo->client_type == ctHTTP)
    result.ReplacePunctSymbols();

  return result;
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


