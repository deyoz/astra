#include "apis_utils.h"
#include "misc.h"
#include "apps_interaction.h"
#include "httpClient.h"
#include "astra_service.h"
#include "sopp.h"
#include <boost/scoped_ptr.hpp>

#include "apis_creator.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace EXCEPTIONS;

const std::string PARAM_URL = "URL";
const std::string PARAM_ACTION_CODE = "ACTION_CODE";
const std::string PARAM_LOGIN = "LOGIN";
const std::string PARAM_PASSWORD = "PASSWORD";

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

bool TAPICheckInfo::CheckLet(const std::string &str, std::string::size_type &errorIdx) const
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) )
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

bool TAPICheckInfo::CheckLetDigSpace(const std::string &str, std::string::size_type &errorIdx) const
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' )
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

bool TAPICheckInfo::CheckLetSpaceDash(const std::string &str, std::string::size_type &errorIdx) const
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || *i==' ' || *i=='-' || (not_apis && *i=='.'))
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

bool TAPICheckInfo::CheckLetDigSpaceDash(const std::string &str, std::string::size_type &errorIdx) const
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' || *i=='-' || (not_apis && (*i=='.' || *i=='/')))
         ))
      return false;
  errorIdx=string::npos;
  return true;
}

void TAPICheckInfoList::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  for(TAPICheckInfoList::const_iterator i=begin(); i!=end(); ++i)
    switch(i->first)
    {
      case apiDoc:   i->second.toXML(NewTextChild(node, "doc"));    break;
      case apiDoco:
      {
        TReqInfo *reqInfo = TReqInfo::Instance();
        if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_ADD_TYPES_VERSION) &&
            i->second.required_fields!=NO_FIELDS)
        {
          TAPICheckInfo checkInfo=i->second;
          checkInfo.required_fields=DOCO_TYPE_FIELD;
          checkInfo.toXML(NewTextChild(node, "doco"));
        }
        else i->second.toXML(NewTextChild(node, "doco"));
        break;
      }
      case apiDocaB: i->second.toXML(NewTextChild(node, "doca_b")); break;
      case apiDocaR: i->second.toXML(NewTextChild(node, "doca_r")); break;
      case apiDocaD: i->second.toXML(NewTextChild(node, "doca_d")); break;
      case apiTkn:   i->second.toXML(NewTextChild(node, "tkn"));    break;
      case apiUnknown: throw Exception("TAPICheckInfoList::toXML: apiUnknown!");
    };
}

void TAPICheckInfoList::toWebXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  for(TAPICheckInfoList::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first==apiDoc)
    {
      xmlNodePtr fieldsNode=NewTextChild(node, "doc_required_fields");
      SetProp(fieldsNode, "is_inter", i->second.is_inter);
      if ((i->second.required_fields&DOC_TYPE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "type");
      if ((i->second.required_fields&DOC_ISSUE_COUNTRY_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "issue_country");
      if ((i->second.required_fields&DOC_NO_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "no");
      if ((i->second.required_fields&DOC_NATIONALITY_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "nationality");
      if ((i->second.required_fields&DOC_BIRTH_DATE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "birth_date");
      if ((i->second.required_fields&DOC_GENDER_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "gender");
      if ((i->second.required_fields&DOC_EXPIRY_DATE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "expiry_date");
      if ((i->second.required_fields&DOC_SURNAME_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "surname");
      if ((i->second.required_fields&DOC_FIRST_NAME_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "first_name");
      if ((i->second.required_fields&DOC_SECOND_NAME_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "second_name");
    }
    if (i->first==apiDoco)
    {
      xmlNodePtr fieldsNode=NewTextChild(node, "doco_required_fields");
      SetProp(fieldsNode, "is_inter", i->second.is_inter);
      if ((i->second.required_fields&DOCO_BIRTH_PLACE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "birth_place");
      if ((i->second.required_fields&DOCO_TYPE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "type");
      if ((i->second.required_fields&DOCO_NO_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "no");
      if ((i->second.required_fields&DOCO_ISSUE_PLACE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "issue_place");
      if ((i->second.required_fields&DOCO_ISSUE_DATE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "issue_date");
      if ((i->second.required_fields&DOCO_EXPIRY_DATE_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "expiry_date");
      if ((i->second.required_fields&DOCO_APPLIC_COUNTRY_FIELD) != 0x0000)
        NewTextChild(fieldsNode, "field", "applic_country");
    }
  }

}

const TAPICheckInfo& TAPICheckInfoList::get(TAPIType apiType) const
{
  TAPICheckInfoList::const_iterator i=find(apiType);
  if (i==end()) throw Exception("APICheckInfoList::get: i==end()!");
  return i->second;
}

TAPICheckInfo& TAPICheckInfoList::get(TAPIType apiType)
{
  return const_cast<TAPICheckInfo&>(static_cast<const TAPICheckInfoList&>(*this).get(apiType));
}

TCompleteAPICheckInfo::TCompleteAPICheckInfo(const int point_dep, const std::string& airp_arv)
{
  set(point_dep, airp_arv);
}

void TCompleteAPICheckInfo::set(const int point_dep, const std::string& airp_arv)
{
  clear();
  TTripInfo fltInfo;
  if (fltInfo.getByPointId(point_dep))
  {
    TTripSetList setList;
    setList.fromDB(point_dep);

    if (setList.value(tsRegWithTkn, false))
      _pass.get(apiTkn).required_fields|=TKN_TICKET_NO_FIELD;

    if (setList.value(tsRegWithDoc, false))
    {
      _pass.get(apiDoc).required_fields|=DOC_NO_FIELD;
      _crew.get(apiDoc).required_fields|=DOC_NO_FIELD;
    };

    if (GetTripSets(tsMintransFile, fltInfo))
    {
      _pass.get(apiTkn).required_fields|=TKN_MINTRANS_FIELDS;
      _pass.get(apiDoc).required_fields|=DOC_MINTRANS_FIELDS;
      _crew.get(apiDoc).required_fields|=DOC_MINTRANS_FIELDS;
    };

    bool is_inter=false;
    try
    {
      string airline, country_dep, country_arv, city;
      airline=fltInfo.airline;
      city=base_tables.get("airps").get_row("code", fltInfo.airp ).AsString("city");
      country_dep=base_tables.get("cities").get_row("code", city).AsString("country");
      city=base_tables.get("airps").get_row("code", airp_arv ).AsString("city");
      country_arv=base_tables.get("cities").get_row("code", city).AsString("country");

      TQuery Qry( &OraSession );
      Qry.Clear();
      Qry.SQLText=
        "SELECT format FROM apis_sets "
        "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND "
        "      pr_denial=0";
      Qry.CreateVariable("airline", otString, airline);
      Qry.CreateVariable("country_dep", otString, country_dep);
      Qry.CreateVariable("country_arv", otString, country_arv);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
        _apis_formats.insert(Qry.FieldAsString("format"));

      is_inter=!(country_dep=="РФ" && country_arv=="РФ");
    }
    catch(EBaseTableError) {};

    // if(checkAPPSSets(point_dep, airp_arv))
    // {
    //   _apis_formats.insert("APPS_SITA");
    //   is_inter=true;
    // };

    std::set<std::string> apps_formats;
    if (checkAPPSSets(point_dep, airp_arv, &apps_formats))
    {
      _apis_formats.insert(apps_formats.begin(), apps_formats.end());
      is_inter=true;
    }

    if (!_apis_formats.empty())
    {
      //здесь имеем ненулевой apis_formats
      set_is_inter(is_inter);
      set_not_apis(false);
      for(std::set<std::string>::const_iterator f=_apis_formats.begin(); f!=_apis_formats.end(); ++f)
      {
        boost::scoped_ptr<TAPISFormat> pAPISFormat(SpawnAPISFormat(*f));
        for (std::set<TAPIType>::const_iterator api = get_apis_doc_set().begin(); api != get_apis_doc_set().end(); ++api)
        {
          _pass.get(*api).required_fields |= pAPISFormat->required_fields(TAPISFormat::pass, *api);
          _crew.get(*api).required_fields |= pAPISFormat->required_fields(TAPISFormat::crew, *api);
        }
      }
    }
    _extra_crew = _pass;
    // применение настроек "не контролировать документ"
    if (GetTripSets(tsNoCtrlDocsCrew, fltInfo))
      for (std::set<TAPIType>::const_iterator api = get_apis_doc_set().begin(); api != get_apis_doc_set().end(); ++api)
        _crew.get(*api).required_fields = NO_FIELDS;
    if (GetTripSets(tsNoCtrlDocsExtraCrew, fltInfo))
      for (std::set<TAPIType>::const_iterator api = get_apis_doc_set().begin(); api != get_apis_doc_set().end(); ++api)
        _extra_crew.get(*api).required_fields = NO_FIELDS;
  };
}

TRouteAPICheckInfo::TRouteAPICheckInfo(const int point_id)
{
  clear();
  TTripRoute route;
  route.GetRouteAfter(NoExists,point_id,trtNotCurrent,trtNotCancelled);
  for(TTripRoute::iterator r = route.begin(); r != route.end(); ++r)
    insert( make_pair(r->airp, TCompleteAPICheckInfo(point_id, r->airp)));
}

bool TRouteAPICheckInfo::apis_generation() const
{
  for(TRouteAPICheckInfo::const_iterator i=begin(); i!=end(); ++i)
    if (!i->second.apis_formats().empty()) return true;
  return false;
}

boost::optional<const TCompleteAPICheckInfo &> TRouteAPICheckInfo::get(const std::string &airp_arv) const
{
  TRouteAPICheckInfo::const_iterator i=find(airp_arv);
  if (i!=end()) return i->second;
  return boost::none;
}

void throwInvalidSymbol(const string &fieldname,
                        const TAPICheckInfo &checkInfo,
                        const string &symbol)
{
  (symbol.size()!=1?
    throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))):
    (checkInfo.is_inter && IsLetter(symbol[0]) && !IsAscii7(symbol[0])?
      throw UserException("WRAP.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))
                                                               <<LParam("text", LexemaData("MSG.FIELD_CONSIST_LAT_CHARS"))):
      throw UserException("WRAP.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(fieldname))
                                                               <<LParam("text", LexemaData("MSG.INVALID_SYMBOL", LParams()<<LParam("symbol", symbol))))
    )
  );
}

void CheckDoc(const CheckIn::TPaxDocItem &doc,
              TPaxTypeExt pax_type_ext,
              const std::string &pax_surname,
              const TCompleteAPICheckInfo &checkInfo,
              TDateTime nowLocal)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);

  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    string::size_type errorIdx;

    modf(nowLocal, &nowLocal);

    if (doc.birth_date!=NoExists && (doc.birth_date>nowLocal || doc.birth_date<IncMonth(nowLocal, -130*12))) //до 130 лет назад
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_BIRTH_DATE", LParams()<<LParam("fieldname", "document/birth_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.BIRTH_DATE")));

    if (doc.expiry_date!=NoExists && (doc.expiry_date<nowLocal || doc.expiry_date>IncMonth(nowLocal, 100*12))) //до 100 лет вперед
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "document/expiry_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.EXPIRY_DATE")));

    if (!ci.CheckLet(doc.subtype, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> document/subtype: %s", doc.subtype.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SUBTYPE", LParams()<<LParam("fieldname", "document/subtype" )):
        throwInvalidSymbol("CAP.PAX_DOC.TYPE", ci, (errorIdx==string::npos?"":doc.subtype.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpace(doc.no, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> document/no: %s", doc.no.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" )):
        throwInvalidSymbol("CAP.PAX_DOC.NO", ci, (errorIdx==string::npos?"":doc.no.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetSpaceDash(doc.surname, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> document/surname: %s", doc.surname.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" )):
        throwInvalidSymbol("CAP.PAX_DOC.SURNAME", ci, (errorIdx==string::npos?"":doc.surname.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetSpaceDash(doc.first_name, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> document/first_name: %s", doc.first_name.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" )):
        throwInvalidSymbol("CAP.PAX_DOC.FIRST_NAME", ci, (errorIdx==string::npos?"":doc.first_name.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetSpaceDash(doc.second_name, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> document/second_name: %s", doc.second_name.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" )):
        throwInvalidSymbol("CAP.PAX_DOC.SECOND_NAME", ci, (errorIdx==string::npos?"":doc.second_name.substr(errorIdx, 1)));
    };

    //проверяем похожесть фамилий
    if (!pax_surname.empty() && !doc.surname.empty() &&
        best_transliter_similarity(pax_surname, doc.surname)<70)  //не менее 70% схожести
    {
      ProgTrace(TRACE5, ">>>> document/surname: %s, pax.surname=%s", doc.surname.c_str(), pax_surname.c_str());
//      reqInfo->client_type!=ctTerm?
//        throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" )):
        throw UserException("MSG.CHECK_DOC.DOC_SURNAME_DIFFERS_FROM_PAX_SURNAME");
    };

    //проверяем пустоту
    if (reqInfo->client_type==ctTerm)
    {
      long int mask=doc.getNotEmptyFieldsMask();

      for(int pass=0; pass<10; pass++)
      {
        long int FIELD=NO_FIELDS;
        string CAP;
        switch(pass)
        {
          case 0:
            FIELD=DOC_TYPE_FIELD;
            CAP="CAP.PAX_DOC.TYPE";
            break;
          case 1:
            FIELD=DOC_ISSUE_COUNTRY_FIELD;
            CAP="CAP.PAX_DOC.ISSUE_COUNTRY";
            break;
          case 2:
            FIELD=DOC_NO_FIELD;
            CAP="CAP.PAX_DOC.NO";
            break;
          case 3:
            FIELD=DOC_NATIONALITY_FIELD;
            CAP="CAP.PAX_DOC.NATIONALITY";
            break;
          case 4:
            FIELD=DOC_BIRTH_DATE_FIELD;
            CAP="CAP.PAX_DOC.BIRTH_DATE";
            break;
          case 5:
            FIELD=DOC_GENDER_FIELD;
            CAP="CAP.PAX_DOC.GENDER";
            break;
          case 6:
            FIELD=DOC_EXPIRY_DATE_FIELD;
            CAP="CAP.PAX_DOC.EXPIRY_DATE";
            break;
          case 7:
            FIELD=DOC_SURNAME_FIELD;
            CAP="CAP.PAX_DOC.SURNAME";
            break;
          case 8:
            FIELD=DOC_FIRST_NAME_FIELD;
            CAP="CAP.PAX_DOC.FIRST_NAME";
            break;
          case 9:
            FIELD=DOC_SECOND_NAME_FIELD;
            CAP="CAP.PAX_DOC.SECOND_NAME";
            break;
        };
        if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
          throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(CAP)));
      };
    };
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      throw UserException("WRAP.PAX_DOC.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
    else
      throw;
  };
}

void CheckDoco(const CheckIn::TPaxDocoItem &doc,
               TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo,
               TDateTime nowLocal)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);

  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    string::size_type errorIdx;

    modf(nowLocal, &nowLocal);

    if (doc.issue_date!=NoExists && (doc.issue_date>nowLocal || doc.issue_date<IncMonth(nowLocal, -70*12))) //до 70 лет назад
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_DATE", LParams()<<LParam("fieldname", "doco/issue_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.ISSUE_DATE")));

    if (doc.expiry_date!=NoExists && (doc.expiry_date<nowLocal || doc.expiry_date>IncMonth(nowLocal, 70*12))) //до 70 лет вперед
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "doco/expiry_date" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.EXPIRY_DATE")));

    if (!ci.CheckLetDigSpaceDash(doc.birth_place, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/birth_place: %s", doc.birth_place.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" )):
        throwInvalidSymbol("CAP.PAX_DOCO.BIRTH_PLACE", ci, (errorIdx==string::npos?"":doc.birth_place.substr(errorIdx, 1)));
    };

    if (!ci.CheckLet(doc.subtype, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/subtype: %s", doc.subtype.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_SUBTYPE", LParams()<<LParam("fieldname", "doco/subtype" )):
        throwInvalidSymbol("CAP.PAX_DOCO.TYPE", ci, (errorIdx==string::npos?"":doc.subtype.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpace(doc.no, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/no: %s", doc.no.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" )):
        throwInvalidSymbol("CAP.PAX_DOCO.NO", ci, (errorIdx==string::npos?"":doc.no.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpaceDash(doc.issue_place, errorIdx))
    {
      ProgTrace(TRACE5, ">>>> doco/issue_place: %s", doc.issue_place.c_str());
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" )):
        throwInvalidSymbol("CAP.PAX_DOCO.ISSUE_PLACE", ci, (errorIdx==string::npos?"":doc.issue_place.substr(errorIdx, 1)));
    };

    //проверяем пустоту
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
        if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
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

void CheckDoca(const CheckIn::TPaxDocaItem &doc,
               TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);

  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    string::size_type errorIdx;
    if (!ci.CheckLetDigSpaceDash(doc.address, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/address: %s", doc.address.c_str());
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCA.INVALID_ADDRESS", LParams()<<LParam("fieldname", "doca/address" )):
          throwInvalidSymbol("CAP.PAX_DOCA.ADDRESS", ci, (errorIdx==string::npos?"":doc.address.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpaceDash(doc.city, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/city: %s", doc.city.c_str());
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCA.INVALID_CITY", LParams()<<LParam("fieldname", "doca/city" )):
          throwInvalidSymbol("CAP.PAX_DOCA.CITY", ci, (errorIdx==string::npos?"":doc.city.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpaceDash(doc.region, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/region: %s", doc.region.c_str());
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCA.INVALID_REGION", LParams()<<LParam("fieldname", "doca/region" )):
          throwInvalidSymbol("CAP.PAX_DOCA.REGION", ci, (errorIdx==string::npos?"":doc.region.substr(errorIdx, 1)));
    };

    if (!ci.CheckLetDigSpaceDash(doc.postal_code, errorIdx))
    {
        ProgTrace(TRACE5, ">>>> doca/postal_code: %s", doc.postal_code.c_str());
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCA.INVALID_POSTAL_CODE", LParams()<<LParam("fieldname", "doca/postal_code" )):
          throwInvalidSymbol("CAP.PAX_DOCA.POSTAL_CODE", ci, (errorIdx==string::npos?"":doc.postal_code.substr(errorIdx, 1)));
    };

    //проверяем пустоту
    if (reqInfo->client_type==ctTerm)
    {
      long int mask=doc.getNotEmptyFieldsMask();

      for(int pass=0; pass<5; pass++)
      {
        long int FIELD=NO_FIELDS;
        string CAP;
        switch(pass)
        {
          case 0:
            FIELD=DOCA_COUNTRY_FIELD;
            CAP="CAP.PAX_DOCA.COUNTRY";
            break;
          case 1:
            FIELD=DOCA_ADDRESS_FIELD;
            CAP="CAP.PAX_DOCA.ADDRESS";
            break;
          case 2:
            FIELD=DOCA_CITY_FIELD;
            CAP="CAP.PAX_DOCA.CITY";
            break;
          case 3:
            FIELD=DOCA_REGION_FIELD;
            CAP="CAP.PAX_DOCA.REGION";
            break;
          case 4:
            FIELD=DOCA_POSTAL_CODE_FIELD;
            CAP="CAP.PAX_DOCA.POSTAL_CODE";
            break;
        };
        if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
          throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText(CAP)));
      };
    };
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      switch(doc.apiType())
      {
        case apiDocaB:
          throw UserException("WRAP.PAX_DOCA_B.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        case apiDocaR:
          throw UserException("WRAP.PAX_DOCA_R.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        case apiDocaD:
          throw UserException("WRAP.PAX_DOCA_D.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        default:
          throw;
      }
    else
      throw;
  };
}

CheckIn::TPaxDocItem NormalizeDoc(const CheckIn::TPaxDocItem &doc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    CheckIn::TPaxDocItem result(doc);
    TElemFmt fmt;

    result.type = doc.type;
    result.type = TrimString(result.type);
    if (!result.type.empty())
    {
      result.type=ElemToElemId(etPaxDocType, upperc(result.type), fmt);
      bool is_docs_type=result.type.empty()?false:getBaseTable(etPaxDocType).get_row("code", result.type).AsBoolean("is_docs_type");
      if (fmt==efmtUnknown || !is_docs_type)
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOC.INVALID_TYPE", LParams()<<LParam("fieldname", "document/type" )):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.TYPE")));
    };

    result.subtype = upperc(doc.subtype);
    result.subtype = TrimString(result.subtype);
    if (result.subtype.size()>1)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SUBTYPE", LParams()<<LParam("fieldname", "document/subtype" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.TYPE")));

    result.issue_country = doc.issue_country;
    result.issue_country = TrimString(result.issue_country);
    if (!result.issue_country.empty())
    {
      result.issue_country=ElemToPaxDocCountryId(upperc(result.issue_country), fmt);
      if (fmt==efmtUnknown)
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOC.INVALID_ISSUE_COUNTRY", LParams()<<LParam("fieldname", "document/issue_country" )):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.ISSUE_COUNTRY")));
    };

    result.no = upperc(doc.no);
    result.no = TrimString(result.no);
    if (result.no.size()>15)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.NO")));

    result.nationality = doc.nationality;
    result.nationality = TrimString(result.nationality);
    if (!result.nationality.empty())
    {
      result.nationality=ElemToPaxDocCountryId(upperc(result.nationality), fmt);
      if (fmt==efmtUnknown)
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOC.INVALID_NATIONALITY", LParams()<<LParam("fieldname", "document/nationality" )):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.NATIONALITY")));
    };

    if (doc.birth_date!=NoExists)
      modf(doc.birth_date, &result.birth_date);

    result.gender = doc.gender;
    result.gender = TrimString(result.gender);
    if (!result.gender.empty())
    {
      result.gender=ElemToElemId(etGenderType, upperc(result.gender), fmt);
      if (fmt==efmtUnknown)
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOC.INVALID_GENDER", LParams()<<LParam("fieldname", "document/gender" )):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.GENDER")));
    };

    if (doc.expiry_date!=NoExists)
      modf(doc.expiry_date, &result.expiry_date);

    result.surname = upperc(doc.surname);
    result.surname = TrimString(result.surname);
    if (result.surname.size()>64)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.SURNAME")));

    result.first_name = upperc(doc.first_name);
    result.first_name = TrimString(result.first_name);
    if (result.first_name.size()>64)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.FIRST_NAME")));

    result.second_name = upperc(doc.second_name);
    result.second_name = TrimString(result.second_name);
    if (result.second_name.size()>64)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOC.SECOND_NAME")));

    return result;
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      throw UserException("WRAP.PAX_DOC.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
    else
      throw;
  };
}

CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    CheckIn::TPaxDocoItem result(doc);
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
      bool is_doco_type=result.type.empty()?false:getBaseTable(etPaxDocType).get_row("code", result.type).AsBoolean("is_doco_type");
      if (fmt==efmtUnknown || !is_doco_type ||
          ((reqInfo->client_type == ctWeb ||
            reqInfo->client_type == ctKiosk ||
            reqInfo->client_type == ctMobile) && result.type!="V"))
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCO.INVALID_TYPE", LParams()<<LParam("fieldname", "doco/type" )):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.TYPE")));
    };

    result.subtype = upperc(doc.subtype);
    result.subtype = TrimString(result.subtype);
    if (result.subtype.size()>1)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCO.INVALID_SUBTYPE", LParams()<<LParam("fieldname", "doco/subtype" )):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.TYPE")));

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

CheckIn::TPaxDocaItem NormalizeDoca(const CheckIn::TPaxDocaItem &doc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  try
  {
    CheckIn::TPaxDocaItem result(doc);
    TElemFmt fmt;

    result.type = upperc(doc.type);
    result.type = TrimString(result.type);

    if (result.apiType()==apiUnknown)
      throw Exception("NormalizeDoca: result.apiType()==apiUnknown!");

    result.country = doc.country;
    result.country = TrimString(result.country);
    if (!result.country.empty())
    {
      result.country = ElemToPaxDocCountryId(upperc(result.country), fmt);
      if (fmt==efmtUnknown)
        reqInfo->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCA.INVALID_COUNTRY", LParams()<<LParam("fieldname", "doca/country")):
          throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCA.COUNTRY")));
    }
    result.address = upperc(doc.address);
    result.address = TrimString(result.address);
    if (result.address.size() > 35)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCA.INVALID_ADDRESS", LParams()<<LParam("fieldname", "doca/address")):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCA.ADDRESS")));

    result.city = upperc(doc.city);
    result.city = TrimString(result.city);
    if (result.city.size() > 35)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCA.INVALID_CITY", LParams()<<LParam("fieldname", "doca/city")):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCA.CITY")));

    result.region = upperc(doc.region);
    result.region = TrimString(result.region);
    if (result.region.size() > 35)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCA.INVALID_REGION", LParams()<<LParam("fieldname", "doca/region")):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCA.REGION")));

    result.postal_code = doc.postal_code;
    result.postal_code = TrimString(result.postal_code);
    if (result.postal_code.size() > 17)
      reqInfo->client_type!=ctTerm?
        throw UserException("MSG.CHECK_DOCA.INVALID_POSTAL_CODE", LParams()<<LParam("fieldname", "doca/postal_code")):
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCA.POSTAL_CODE")));

    if (TReqInfo::Instance()->client_type == ctHTTP)
      result.ReplacePunctSymbols();

    return result;
  }
  catch(UserException &e)
  {
    if (reqInfo->client_type==ctTerm)
      switch(doc.apiType())
      {
        case apiDocaB:
          throw UserException("WRAP.PAX_DOCA_B.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        case apiDocaR:
          throw UserException("WRAP.PAX_DOCA_R.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        case apiDocaD:
          throw UserException("WRAP.PAX_DOCA_D.DETAILS", LParams()<<LParam("text" ,e.getLexemaData()));
        default:
          throw;
      }
    else
      throw;
  };
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

bool isValidGender(const string &fmt, const string &pax_doc_gender, const string &pax_name)
{
  if (fmt=="CSV_CZ" || fmt=="EDI_CZ" || fmt=="EDI_US" || fmt=="EDI_USBACK"|| fmt=="EDI_LT")
  {
    int is_female=CheckIn::is_female(pax_doc_gender, pax_name);
    if (is_female==NoExists) return false;
  };
  return true;
};

bool isValidDocType(const string &fmt, const TPaxStatus &status, const string &doc_type)
{
  if (fmt=="EDI_CZ" || fmt=="EDI_LT")
  {
    if (!(doc_type=="P" ||
          doc_type=="A" ||
          doc_type=="C" ||
          (status!=psCrew &&
           (doc_type=="B" ||
            doc_type=="T" ||
            doc_type=="N" ||
            doc_type=="M")) ||
          (status==psCrew &&
           (doc_type=="L")))) return false;
  };
  if (fmt=="EDI_CN")
  {
    /*
    1. Official travel documents:
      P for Passport
      PT for P.R. China Travel Permit
      PL for P.R. China Exit and Entry Permit
      W for Travel Permit To and From HK and Macao
      A for Travel Permit To and From HK and Macao for Public Affairs
      Q for Travel Permit To HK and Macao
      C for Travel Permit of HK and Macao Residents To and From Mainland
      D for Travel Permit of Mainland Residents To and From Taiwan
      T for Travel Permit of Taiwan Residents To and From Mainland
      S for Seafarer's Passport
      F for approved non-standard identity documents used for travel
    2. Other documents:
      V for Visa
      AC for Crew Member Certificate

    Visa and Crew Member Certificate are optional
    */
    if (!(doc_type=="P" ||
          doc_type=="PT" ||
          doc_type=="PL" ||
          doc_type=="W" ||
          doc_type=="A" ||
          doc_type=="Q" ||
          doc_type=="C" ||
          doc_type=="D" ||
          doc_type=="T" ||
          doc_type=="S" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };
  if (fmt=="EDI_IN" || fmt=="EDI_KR" || fmt=="EDI_AZ")
  {
    /*
    ICAO 9303 Document Types
      P Passport
      V Visa
      A Identity Card (exact use defined by the Issuing State)
      C Identity Card (exact use defined by the Issuing State)
      I Identity Card (exact use defined by the Issuing State)
      AC Crew Member Certificate
      IP Passport Card
    Other Document Types
      F Approved non-standard identity documents used for travel
      (exact use defined by the Issuing State).
    */
    if (!(doc_type=="P" ||
          doc_type=="A" ||
          doc_type=="C" ||
          doc_type=="I" ||
          doc_type=="IP" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };
  if (fmt=="EDI_US" || fmt=="EDI_USBACK" || fmt=="XML_TR")
  {
    /*
    P - Passport
    C - Permanent resident card
    A - Resident alien card
    M - US military ID.
    T - Re-entry permit or refugee permit
    IN - NEXUS card
    IS - SENTRI card
    F - Facilitation card
    V - U.S. Non-Immigrant Visa (Secondary Document Only)
    L - Pilots license (crew members only)
    */
    if (!(doc_type=="P" ||
          doc_type=="C" ||
          doc_type=="A" ||
          (status!=psCrew &&
           (doc_type=="M" ||
            doc_type=="T" ||
            doc_type=="IN" ||
            doc_type=="IS" ||
            doc_type=="F")) ||
          (status==psCrew &&
           (doc_type=="L")))) return false;
  };

  if (fmt=="EDI_UK")
  {
    /*
    P - Passport
    G - Group Passport
    A - National Identity Card or Resident Card. Exact use defined by issuing state
    C - National Identity Card or Resident Card. Exact use defined by issuing state
    I - National Identity Card or Resident Card. Exact use defined by issuing state
    M - Military Identification
    D - Diplomatic Identification
    AC - Crew Member Certificate
    IP - Passport Card
    F -  Other approved non-standard identity documents used for travel (as per Authority regulations)
    */
    if (!(doc_type=="P" ||
          doc_type=="G" ||
          doc_type=="A" ||
          doc_type=="C" ||
          doc_type=="I" ||
          doc_type=="M" ||
          doc_type=="D" ||
          doc_type=="IP" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };

  if (fmt=="EDI_ES")
  {
    /*
    I (C,A) National Id Docs and Residence cards C and A admitted though main type being I.
    P Passport
    G group Passport
    V Visa
    R Travel title
    */
    if (!(doc_type=="I" ||
          doc_type=="C" ||
          doc_type=="A" ||
          doc_type=="P" ||
          doc_type=="G" ||
          doc_type=="R")) return false;
  };
  return true;
};

}; //namespace APIS

void HandleDoc(const CheckIn::TPaxGrpItem &grp,
               const CheckIn::TSimplePaxItem &pax,
               const TCompleteAPICheckInfo &checkInfo,
               const TDateTime &checkDate,
               CheckIn::TPaxDocItem &doc)
{
  ASTRA::TPaxTypeExt pax_type_ext(grp.status, pax.crew_type);
  doc=NormalizeDoc(doc);
  CheckDoc(doc, pax_type_ext, pax.surname, checkInfo, checkDate);

  if (checkInfo.incomplete(doc, pax_type_ext))
    throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOC_INFO_NOT_SET");

  for(set<string>::const_iterator f=checkInfo.apis_formats().begin(); f!=checkInfo.apis_formats().end(); ++f)
  {
    if (!APIS::isValidDocType(*f, grp.status, doc.type))
    {
      if (!doc.type.empty())
        throw UserException("MSG.PASSENGER.INVALID_DOC_TYPE_FOR_APIS",
                            LParams() << LParam("surname", pax.full_name())
                            << LParam("code", ElemIdToCodeNative(etPaxDocType, doc.type)));
    };
    if (!APIS::isValidGender(*f, doc.gender, pax.name))
    {
      if (!doc.gender.empty())
        throw UserException("MSG.PASSENGER.INVALID_GENDER_FOR_APIS",
                            LParams() << LParam("surname", pax.full_name())
                            << LParam("code", ElemIdToCodeNative(etGenderType, doc.gender)));
    };    
  };
};

void HandleDoco(const CheckIn::TPaxGrpItem &grp,
                const CheckIn::TSimplePaxItem &pax,
                const TCompleteAPICheckInfo &checkInfo,
                const TDateTime &checkDate,
                CheckIn::TPaxDocoItem &doco)
{
  if (!(doco.getNotEmptyFieldsMask()==NO_FIELDS && doco.doco_confirm)) //пришла непустая информация о визе
  {
    ASTRA::TPaxTypeExt pax_type_ext(grp.status, pax.crew_type);
    doco=NormalizeDoco(doco);
    CheckDoco(doco, pax_type_ext, checkInfo, checkDate);

    if (checkInfo.incomplete(doco, pax_type_ext))
      throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOCO_INFO_NOT_SET");

    for (auto fmt : checkInfo.apis_formats())
    {
      boost::scoped_ptr<TAPISFormat> pFormat(SpawnAPISFormat(fmt));
      if (!pFormat->CheckDocoIssueCountry(doco.issue_place))
        TReqInfo::Instance()->client_type!=ctTerm?
          throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" )):
          throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_COUNTRY", LParams()<<LParam("fieldname", getLocaleText("CAP.PAX_DOCO.ISSUE_PLACE")));

    }
  };
};

void HandleDoca(const CheckIn::TPaxGrpItem &grp,
                const CheckIn::TSimplePaxItem &pax,
                const TCompleteAPICheckInfo &checkInfo,
                const CheckIn::TDocaMap &doca_map)
{
  ASTRA::TPaxTypeExt pax_type_ext(grp.status, pax.crew_type);
  // При вкл. настройке "не контролировать документ у экипажа / доп. экипажа"
  // не производится проверка заполнения обязательных полей для используемого APIS,
  // таким образом, становится возможным отсутствие документа в контейнере,
  // из-за чего проверка incomplete не производится.
  // Для обхода этой ситуации введён дополнительный контейнер, содержащий в том числе и пустые документы
  // (с заполненным типом, во избежание Exception: NormalizeDoca: result.apiType()==apiUnknown!)
  CheckIn::TDocaMap doca_map_2(doca_map);
  if (!doca_map_2.count(apiDocaB)) doca_map_2[apiDocaB].type = "B";
  if (!doca_map_2.count(apiDocaR)) doca_map_2[apiDocaR].type = "R";
  if (!doca_map_2.count(apiDocaD)) doca_map_2[apiDocaD].type = "D";
  for(CheckIn::TDocaMap::iterator d = doca_map_2.begin(); d != doca_map_2.end(); ++d)
  {
    CheckIn::TPaxDocaItem &doc = d->second;
    doc=NormalizeDoca(doc);
    CheckDoca(doc, pax_type_ext, checkInfo);
    if (checkInfo.incomplete(doc, pax_type_ext))
    {
      switch(doc.apiType())
      {
        case apiDocaB:
          throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOCA_B_INFO_NOT_SET");
        case apiDocaR:
          throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOCA_R_INFO_NOT_SET");
        case apiDocaD:
          throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOCA_D_INFO_NOT_SET");
        default: ;
      }
    };
  }
};

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
        ProgError( STDLOG, "Ошибка разбора XML. '%s' : %s", text.c_str(), E.what() );
        throw;
    } catch(...) {
        if( xml_text )
            xmlFreeDoc( xml_text );
        ProgError( STDLOG, "Ошибка разбора XML. '%s'", text.c_str() );
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

static bool pion_parse_uri(const std::string& uri, std::string& proto,
                           std::string& host, boost::uint16_t& port,
                           std::string& path, std::string& query)
{
    size_t proto_end = uri.find("://");
    size_t proto_len = 0;

    if(proto_end != std::string::npos) {
        proto = uri.substr(0, proto_end);
        proto_len = proto_end + 3; // add ://
    } else {
        proto.clear();
    }

    // find a first slash charact
    // that indicates the end of the <server>:<port> part
    size_t server_port_end = uri.find('/', proto_len);
    if(server_port_end == std::string::npos) {
        return false;
    }

    // copy <server>:<port> into temp string
    std::string t;
    t = uri.substr(proto_len, server_port_end - proto_len);
    size_t port_pos = t.find(':', 0);

    // assign output host and port parameters

    host = t.substr(0, port_pos); // if port_pos == npos, copy whole string
    if(host.length() == 0) {
        return false;
    }

    // parse the port, if it's not empty
    if(port_pos != std::string::npos) {
        try {
            port = boost::lexical_cast<int>(t.substr(port_pos+1));
        } catch (boost::bad_lexical_cast &) {
            return false;
        }
    } else if (proto == "http" || proto == "HTTP") {
        port = 80;
    } else if (proto == "https" || proto == "HTTPS") {
        port = 443;
    } else {
        port = 0;
    }

    // copy the rest of the URI into path part
    path = uri.substr(server_port_end);

    // split the path and the query string parts
    size_t query_pos = path.find('?', 0);

    if(query_pos != std::string::npos) {
        query = path.substr(query_pos + 1, path.length() - query_pos - 1);
        path = path.substr(0, query_pos);
    } else {
        query.clear();
    }

    return true;
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
      if(not pion_parse_uri(item->params[PARAM_URL], proto, request.host, request.port, request.path, query))
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
  if( doc == NULL ) {
      LogTrace(TRACE5) << "process_reply: result: '" << result << "'";
      throw Exception( "Wrong answer XML" );
  }
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ФУНКЦИИ ДЛЯ МЕРИДИАНА (for HTTP)

void CheckDocHttp(const CheckIn::TPaxDocItem &doc,
                  TPaxTypeExt pax_type_ext,
                  const std::string &pax_surname,
                  const TCompleteAPICheckInfo &checkInfo,
                  TDateTime nowLocal,
                  const std::string full_name)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);
  try
  {
    string::size_type errorIdx;
    modf(nowLocal, &nowLocal);

    if (doc.birth_date!=NoExists && (doc.birth_date>nowLocal || doc.birth_date<IncMonth(nowLocal, -130*12))) //до 130 лет назад
      throw UserException("MSG.CHECK_DOC.INVALID_BIRTH_DATE", LParams()<<LParam("fieldname", "document/birth_date" ));

    if (doc.expiry_date!=NoExists && (doc.expiry_date<nowLocal || doc.expiry_date>IncMonth(nowLocal, 100*12))) //до 100 лет вперед
      throw UserException("MSG.CHECK_DOC.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "document/expiry_date" ));

    if (!ci.CheckLetDigSpace(doc.no, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOC.NO", ci, (errorIdx==string::npos?"":doc.no.substr(errorIdx, 1)));

    if (!ci.CheckLetSpaceDash(doc.surname, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOC.SURNAME", ci, (errorIdx==string::npos?"":doc.surname.substr(errorIdx, 1)));

    if (!ci.CheckLetSpaceDash(doc.first_name, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOC.FIRST_NAME", ci, (errorIdx==string::npos?"":doc.first_name.substr(errorIdx, 1)));

    if (!ci.CheckLetSpaceDash(doc.second_name, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOC.SECOND_NAME", ci, (errorIdx==string::npos?"":doc.second_name.substr(errorIdx, 1)));

    //проверяем похожесть фамилий
    if (!pax_surname.empty() && !doc.surname.empty() && best_transliter_similarity(pax_surname, doc.surname)<70)  //не менее 70% схожести
      throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" ));

    //проверяем пустоту
    long int mask=doc.getNotEmptyFieldsMask();
    for(int pass=0; pass<10; pass++)
    {
      long int FIELD=NO_FIELDS;
      string CAP;
      switch(pass)
      {
        case 0: FIELD=DOC_TYPE_FIELD;          CAP="document/type";           break;
        case 1: FIELD=DOC_ISSUE_COUNTRY_FIELD; CAP="document/issue_country";  break;
        case 2: FIELD=DOC_NO_FIELD;            CAP="document/no";             break;
        case 3: FIELD=DOC_NATIONALITY_FIELD;   CAP="document/nationality";    break;
        case 4: FIELD=DOC_BIRTH_DATE_FIELD;    CAP="document/birth_date";     break;
        case 5: FIELD=DOC_GENDER_FIELD;        CAP="document/gender";         break;
        case 6: FIELD=DOC_EXPIRY_DATE_FIELD;   CAP="document/expiry_date";    break;
        case 7: FIELD=DOC_SURNAME_FIELD;       CAP="document/surname";        break;
        case 8: FIELD=DOC_FIRST_NAME_FIELD;    CAP="document/first_name";     break;
        case 9: FIELD=DOC_SECOND_NAME_FIELD;   CAP="document/second_name";    break;
      }
      if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
        throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", CAP));
    }
  }
  catch(UserException &e)
  {
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", "DOCS")
                        <<LParam("text", e.getLexemaData()));
  }
}

void CheckDocoHttp(const CheckIn::TPaxDocoItem &doc,
               TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo,
               TDateTime nowLocal,
               const std::string full_name)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);
  try
  {
    string::size_type errorIdx;
    modf(nowLocal, &nowLocal);

    if (doc.issue_date!=NoExists && (doc.issue_date>nowLocal || doc.issue_date<IncMonth(nowLocal, -70*12))) //до 70 лет назад
      throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_DATE", LParams()<<LParam("fieldname", "doco/issue_date" ));

    if (doc.expiry_date!=NoExists && (doc.expiry_date<nowLocal || doc.expiry_date>IncMonth(nowLocal, 70*12))) //до 70 лет вперед
      throw UserException("MSG.CHECK_DOCO.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "doco/expiry_date" ));

    if (!ci.CheckLetDigSpaceDash(doc.birth_place, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCO.BIRTH_PLACE", ci, (errorIdx==string::npos?"":doc.birth_place.substr(errorIdx, 1)));

    if (!ci.CheckLetDigSpace(doc.no, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCO.NO", ci, (errorIdx==string::npos?"":doc.no.substr(errorIdx, 1)));

    if (!ci.CheckLetDigSpaceDash(doc.issue_place, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCO.ISSUE_PLACE", ci, (errorIdx==string::npos?"":doc.issue_place.substr(errorIdx, 1)));

    //проверяем пустоту
    long int mask=doc.getNotEmptyFieldsMask();
    for(int pass=0; pass<7; pass++)
    {
      long int FIELD=NO_FIELDS;
      string CAP;
      switch(pass)
      {
        case 0: FIELD=DOCO_BIRTH_PLACE_FIELD;    CAP="doco/birth_place";     break;
        case 1: FIELD=DOCO_TYPE_FIELD;           CAP="doco/type";            break;
        case 2: FIELD=DOCO_NO_FIELD;             CAP="doco/no";              break;
        case 3: FIELD=DOCO_ISSUE_PLACE_FIELD;    CAP="doco/issue_place";     break;
        case 4: FIELD=DOCO_ISSUE_DATE_FIELD;     CAP="doco/issue_date";      break;
        case 5: FIELD=DOCO_APPLIC_COUNTRY_FIELD; CAP="doco/applic_country";  break;
        case 6: FIELD=DOCO_EXPIRY_DATE_FIELD;    CAP="doco/expiry_date";     break;
      }
      if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
        throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", CAP));
    }
  }
  catch(UserException &e)
  {
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", "DOCO")
                        <<LParam("text", e.getLexemaData()));
  }
}

void CheckDocaHttp(const CheckIn::TPaxDocaItem &doc,
               TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo,
               const std::string full_name)
{
  const TAPICheckInfo &ci=checkInfo.get(doc, pax_type_ext);
  try
  {
    string::size_type errorIdx;
    if (!ci.CheckLetDigSpaceDash(doc.address, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCA.ADDRESS", ci, (errorIdx==string::npos?"":doc.address.substr(errorIdx, 1)));

    if (!ci.CheckLetDigSpaceDash(doc.city, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCA.CITY", ci, (errorIdx==string::npos?"":doc.city.substr(errorIdx, 1)));

    if (!ci.CheckLetDigSpaceDash(doc.region, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCA.REGION", ci, (errorIdx==string::npos?"":doc.region.substr(errorIdx, 1)));

    if (!ci.CheckLetDigSpaceDash(doc.postal_code, errorIdx))
      throwInvalidSymbol("CAP.PAX_DOCA.POSTAL_CODE", ci, (errorIdx==string::npos?"":doc.postal_code.substr(errorIdx, 1)));

    //проверяем пустоту
    long int mask=doc.getNotEmptyFieldsMask();
    for(int pass=0; pass<5; pass++)
    {
      long int FIELD=NO_FIELDS;
      string CAP;
      switch(pass)
      {
        case 0: FIELD=DOCA_COUNTRY_FIELD;     CAP="doca/country";     break;
        case 1: FIELD=DOCA_ADDRESS_FIELD;     CAP="doca/address";     break;
        case 2: FIELD=DOCA_CITY_FIELD;        CAP="doca/city";        break;
        case 3: FIELD=DOCA_REGION_FIELD;      CAP="doca/region";      break;
        case 4: FIELD=DOCA_POSTAL_CODE_FIELD; CAP="doca/postal_code"; break;
      }
      if ((mask & FIELD) == 0 && (ci.required_fields & FIELD) != 0)
        throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams()<<LParam("fieldname", CAP));
    }
  }
  catch(UserException &e)
  {
    string doc_type;
    switch(doc.apiType())
    {
      case apiDocaB: doc_type = "DOCA_B"; break;
      case apiDocaR: doc_type = "DOCA_R"; break;
      case apiDocaD: doc_type = "DOCA_D"; break;
      default: break;
    }
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", doc_type)
                        <<LParam("text", e.getLexemaData()));
  }
}

/////

CheckIn::TPaxDocItem NormalizeDocHttp(const CheckIn::TPaxDocItem &doc, const std::string full_name)
{
  try
  {
    CheckIn::TPaxDocItem result(doc);
    TElemFmt fmt;

    result.type = doc.type;
    result.type = TrimString(result.type);
    if (!result.type.empty())
    {
      result.type=ElemToElemId(etPaxDocType, upperc(result.type), fmt);
      if (fmt==efmtUnknown || result.type=="V")
          throw UserException("MSG.CHECK_DOC.INVALID_TYPE", LParams()<<LParam("fieldname", "document/type" ));
    }

    result.subtype = doc.subtype;
    result.subtype = TrimString(result.subtype);
    if (!result.subtype.empty())
    {
      ElemToElemId(etPaxDocSubtype, TPaxDocSubtypes::ConstructCode(upperc(result.type), upperc(result.subtype)), fmt);
      if (fmt==efmtUnknown)
          throw UserException("MSG.CHECK_DOC.INVALID_SUBTYPE", LParams()<<LParam("fieldname", "document/subtype" ));
    }

    result.issue_country = doc.issue_country;
    result.issue_country = TrimString(result.issue_country);
    if (!result.issue_country.empty())
    {
      result.issue_country=ElemToPaxDocCountryId(upperc(result.issue_country), fmt);
      if (fmt==efmtUnknown)
          throw UserException("MSG.CHECK_DOC.INVALID_ISSUE_COUNTRY", LParams()<<LParam("fieldname", "document/issue_country" ));
    }

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
    }

    if (doc.birth_date!=NoExists)
      modf(doc.birth_date, &result.birth_date);

    result.gender = doc.gender;
    result.gender = TrimString(result.gender);
    if (!result.gender.empty())
    {
      result.gender=ElemToElemId(etGenderType, upperc(result.gender), fmt);
      if (fmt==efmtUnknown)
          throw UserException("MSG.CHECK_DOC.INVALID_GENDER", LParams()<<LParam("fieldname", "document/gender" ));
    }

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
  catch(UserException &e)
  {
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", "DOCS")
                        <<LParam("text", e.getLexemaData()));
  }
}

CheckIn::TPaxDocoItem NormalizeDocoHttp(const CheckIn::TPaxDocoItem &doc, const std::string full_name)
{
  try
  {
    CheckIn::TPaxDocoItem result(doc);
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
      if (fmt==efmtUnknown)
          throw UserException("MSG.CHECK_DOCO.INVALID_TYPE", LParams()<<LParam("fieldname", "doco/type" ));
    }

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
    }

    result.ReplacePunctSymbols();

    return result;
  }
  catch(UserException &e)
  {
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", "DOCO")
                        <<LParam("text", e.getLexemaData()));
  }
}

CheckIn::TPaxDocaItem NormalizeDocaHttp(const CheckIn::TPaxDocaItem &doc, const std::string full_name)
{
  try
  {
    CheckIn::TPaxDocaItem result(doc);
    TElemFmt fmt;

    result.type = upperc(doc.type);
    result.type = TrimString(result.type);

    if (result.apiType()==apiUnknown)
      throw Exception("NormalizeDoca: result.apiType()==apiUnknown!");

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
    if (result.address.size() > 35)
        throw UserException("MSG.CHECK_DOCA.INVALID_ADDRESS", LParams()<<LParam("fieldname", "doca/address"));

    result.city = upperc(doc.city);
    result.city = TrimString(result.city);
    if (result.city.size() > 35)
        throw UserException("MSG.CHECK_DOCA.INVALID_CITY", LParams()<<LParam("fieldname", "doca/city"));

    result.region = upperc(doc.region);
    result.region = TrimString(result.region);
    if (result.region.size() > 35)
        throw UserException("MSG.CHECK_DOCA.INVALID_REGION", LParams()<<LParam("fieldname", "doca/region"));

    result.postal_code = doc.postal_code;
    result.postal_code = TrimString(result.postal_code);
    if (result.postal_code.size() > 17)
        throw UserException("MSG.CHECK_DOCA.INVALID_POSTAL_CODE", LParams()<<LParam("fieldname", "doca/postal_code"));

    result.ReplacePunctSymbols();

    return result;
  }
  catch(UserException &e)
  {
    string doc_type;
    switch(doc.apiType())
    {
      case apiDocaB: doc_type = "DOCA_B"; break;
      case apiDocaR: doc_type = "DOCA_R"; break;
      case apiDocaD: doc_type = "DOCA_D"; break;
      default: break;
    }
    throw UserException("WRAP.PAX_ERROR.FULLNAME",
                        LParams()<<LParam("fullname", full_name)
                        <<LParam("doctype", doc_type)
                        <<LParam("text", e.getLexemaData()));
  }
}

std::string SubstrAfterLastSpace(const std::string& str)
{
  std::size_t found = str.rfind(' ');
  if (found != std::string::npos)
    return str.substr(found+1);
  else
    return str;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
