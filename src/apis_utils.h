#ifndef APIS_UTILS_H
#define APIS_UTILS_H

#include "passenger.h"
#include "astra_misc.h"
#include "term_version.h"
#include "file_queue.h"
#include "date_time.h"

using BASIC::date_time::TDateTime;

const long int DOC_TYPE_FIELD=0x0001;
const long int DOC_ISSUE_COUNTRY_FIELD=0x0002;
const long int DOC_NO_FIELD=0x0004;
const long int DOC_NATIONALITY_FIELD=0x0008;
const long int DOC_BIRTH_DATE_FIELD=0x0010;
const long int DOC_GENDER_FIELD=0x0020;
const long int DOC_EXPIRY_DATE_FIELD=0x0040;
const long int DOC_SURNAME_FIELD=0x0080;
const long int DOC_FIRST_NAME_FIELD=0x0100;
const long int DOC_SECOND_NAME_FIELD=0x0200;

const long int DOCO_BIRTH_PLACE_FIELD=0x0001;
const long int DOCO_TYPE_FIELD=0x0002;
const long int DOCO_NO_FIELD=0x0004;
const long int DOCO_ISSUE_PLACE_FIELD=0x0008;
const long int DOCO_ISSUE_DATE_FIELD=0x0010;
const long int DOCO_APPLIC_COUNTRY_FIELD=0x0020;
const long int DOCO_EXPIRY_DATE_FIELD=0x0040;

const long int DOCA_TYPE_FIELD=0x0001;
const long int DOCA_COUNTRY_FIELD=0x0002;
const long int DOCA_ADDRESS_FIELD=0x0004;
const long int DOCA_CITY_FIELD=0x0008;
const long int DOCA_REGION_FIELD=0x0010;
const long int DOCA_POSTAL_CODE_FIELD=0x0020;

const long int TKN_TICKET_NO_FIELD=0x0001;

//==============================================================================
const long int DOC_CSV_CZ_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD;

const long int DOC_EDI_CZ_FIELDS=DOC_CSV_CZ_FIELDS;

//==============================================================================
const long int DOC_EDI_CN_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD;
/*
const long int DOCO_EDI_CN_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD;
*/
//==============================================================================
const long int DOC_EDI_IN_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD;
/*
const long int DOCO_EDI_IN_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD;
*/
//==============================================================================
const long int DOC_EDI_US_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_EXPIRY_DATE_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;
/*
const long int DOCO_EDI_US_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD;
*/

const long int DOCA_B_CREW_EDI_US_FIELDS=DOCA_TYPE_FIELD|
                                         DOCA_COUNTRY_FIELD;

const long int DOCA_R_PASS_EDI_US_FIELDS=DOCA_TYPE_FIELD|
                                         DOCA_COUNTRY_FIELD;

const long int DOCA_R_CREW_EDI_US_FIELDS=DOCA_TYPE_FIELD|
                                         DOCA_ADDRESS_FIELD|
                                         DOCA_CITY_FIELD|
                                         DOCA_COUNTRY_FIELD;

const long int DOCA_D_PASS_EDI_US_FIELDS=DOCA_TYPE_FIELD|
                                         DOCA_ADDRESS_FIELD|
                                         DOCA_CITY_FIELD|
                                         DOCA_REGION_FIELD|
                                         DOCA_POSTAL_CODE_FIELD|
                                         DOCA_COUNTRY_FIELD;

//==============================================================================
const long int DOC_EDI_USBACK_FIELDS=DOC_TYPE_FIELD|
                                     DOC_ISSUE_COUNTRY_FIELD|
                                     DOC_NO_FIELD|
                                     DOC_NATIONALITY_FIELD|
                                     DOC_BIRTH_DATE_FIELD|
                                     DOC_GENDER_FIELD|
                                     DOC_EXPIRY_DATE_FIELD|
                                     DOC_SURNAME_FIELD|
                                     DOC_FIRST_NAME_FIELD;
/*
const long int DOCO_EDI_USBACK_FIELDS=DOCO_TYPE_FIELD|
                                      DOCO_NO_FIELD;
*/

const long int DOCA_B_CREW_EDI_USBACK_FIELDS=DOCA_TYPE_FIELD|
                                             DOCA_COUNTRY_FIELD;

const long int DOCA_R_PASS_EDI_USBACK_FIELDS=NO_FIELDS;

const long int DOCA_R_CREW_EDI_USBACK_FIELDS=DOCA_TYPE_FIELD|
                                             DOCA_ADDRESS_FIELD|
                                             DOCA_CITY_FIELD|
                                             DOCA_COUNTRY_FIELD;

const long int DOCA_D_PASS_EDI_USBACK_FIELDS=NO_FIELDS;

//==============================================================================
const long int DOC_EDI_UK_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;
//==============================================================================
const long int DOC_EDI_ES_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;
//==============================================================================
const long int DOC_CSV_DE_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD;

const long int DOCO_CSV_DE_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD|
                                  DOCO_APPLIC_COUNTRY_FIELD;

//==============================================================================
const long int DOC_TXT_EE_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_GENDER_FIELD;

const long int DOCO_TXT_EE_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD;

//==============================================================================
const long int DOC_MINTRANS_FIELDS=DOC_TYPE_FIELD|
                                   DOC_NO_FIELD|
                                   DOC_BIRTH_DATE_FIELD;

const long int TKN_MINTRANS_FIELDS=NO_FIELDS;

//==============================================================================
const long int DOC_XML_TR_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_APPS_SITA_FIELDS=DOC_TYPE_FIELD|
                                    DOC_ISSUE_COUNTRY_FIELD|
                                    DOC_NO_FIELD|
                                    DOC_EXPIRY_DATE_FIELD|
                                    DOC_NATIONALITY_FIELD|
                                    DOC_BIRTH_DATE_FIELD|
                                    DOC_GENDER_FIELD|
                                    DOC_SURNAME_FIELD|
                                    DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_CSV_AE_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_EDI_LT_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_EXPIRY_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_CSV_TH_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_EDI_KR_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_EXPIRY_DATE_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

const long int DOC_EDI_AZ_FIELDS=DOC_TYPE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_EXPIRY_DATE_FIELD|
                                 DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD;

//==============================================================================

static TAPIType const APITypeArray[] = {apiDoc, apiDoco, apiDocaB, apiDocaR, apiDocaD, apiTkn};

class TAPICheckInfo
{
  public:
    bool is_inter;
    long int required_fields; //битовая маска
    long int readonly_fields; //битовая маска временно не используется
    bool not_apis;
    TAPICheckInfo()
    {
      clear();
    }
    void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      NewTextChild(node, "is_inter", (int)is_inter, (int)false);
      NewTextChild(node, "required_fields", required_fields, (int)NO_FIELDS);
      NewTextChild(node, "readonly_fields", readonly_fields, (int)NO_FIELDS);
    }
    void clear()
    {
      is_inter=false;
      not_apis=true;
      required_fields=NO_FIELDS;
      readonly_fields=NO_FIELDS;
    }
    bool CheckLetDigSpace(const std::string &str, std::string::size_type &errorIdx) const;
    bool CheckLetSpaceDash(const std::string &str, std::string::size_type &errorIdx) const;
    bool CheckLetDigSpaceDash(const std::string &str, std::string::size_type &errorIdx) const;
};

class TAPICheckInfoList : private std::map<TAPIType, TAPICheckInfo>
{
  public:
    TAPICheckInfoList()
    {
      for(unsigned i=0; i<sizeof(APITypeArray)/sizeof(APITypeArray[0]); i++)
        insert(std::make_pair(APITypeArray[i], TAPICheckInfo()));
    }
    void clear()
    {
      for(TAPICheckInfoList::iterator i=begin(); i!=end(); ++i)
        i->second.clear();
    }
    void toXML(xmlNodePtr node) const;
    const TAPICheckInfo& get(TAPIType apiType) const;
    TAPICheckInfo& get(TAPIType apiType);
    void set_is_inter(bool _is_inter)
    {
      for(TAPICheckInfoList::iterator i=begin(); i!=end(); ++i)
        i->second.is_inter=_is_inter;
    }
    void set_not_apis(bool _not_apis)
    {
      for(TAPICheckInfoList::iterator i=begin(); i!=end(); ++i)
        i->second.not_apis=_not_apis;
    }
};

class TCompleteAPICheckInfo
{
  private:
    TAPICheckInfoList _pass;
    TAPICheckInfoList _crew;
    TAPICheckInfoList _extra_crew;
    std::set<std::string> _apis_formats;
  public:
    TCompleteAPICheckInfo() { clear(); }
    TCompleteAPICheckInfo(const int point_dep, const std::string& airp_arv);
    void clear()
    {
      _pass.clear();
      _crew.clear();
      _extra_crew.clear();
      _apis_formats.clear();
    }
    void set(const int point_dep, const std::string& airp_arv);
    // get
    inline const TAPICheckInfoList& get(ASTRA::TPaxTypeExt pax_ext) const
    {
      if (pax_ext._pax_status == ASTRA::psCrew)
      return _crew;
      if (pax_ext._crew_type == ASTRA::TCrewType::ExtraCrew or
          pax_ext._crew_type == ASTRA::TCrewType::DeadHeadCrew or
          pax_ext._crew_type == ASTRA::TCrewType::MiscOperStaff)
      return _extra_crew;
      return _pass;
    }
    const TAPICheckInfo& get(const CheckIn::TPaxAPIItem &item, ASTRA::TPaxTypeExt pax_ext) const
    {
      return get(pax_ext).get(item.apiType());
    }
    const TAPICheckInfo& get(const CheckIn::TPaxAPIItem &item,
                             ASTRA::TPaxStatus status,
                             ASTRA::TCrewType::Enum crew_type) const
    {
      return get(ASTRA::TPaxTypeExt(status, crew_type)).get(item.apiType());
    }
    // incomplete
    bool incomplete(const CheckIn::TPaxAPIItem &item, ASTRA::TPaxTypeExt pax_ext) const
    {
      long int required_fields=get(item, pax_ext).required_fields;
      return ((item.getNotEmptyFieldsMask()&required_fields)!=required_fields);
    }
    bool incomplete(const CheckIn::TPaxAPIItem &item,
                    ASTRA::TPaxStatus status,
                    ASTRA::TCrewType::Enum crew_type) const
    {
      return incomplete(item, ASTRA::TPaxTypeExt(status, crew_type));
    }
    //для пассажиров
    bool incomplete(const CheckIn::TPaxAPIItem &item) const
    {
      long int required_fields = get(item,ASTRA::psCheckin, ASTRA::TCrewType::Unknown).required_fields;
      return ((item.getNotEmptyFieldsMask()&required_fields)!=required_fields);
    }

    void set_is_inter(bool _is_inter)
    {
      _pass.set_is_inter(_is_inter);
      _crew.set_is_inter(_is_inter);
      _extra_crew.set_is_inter(_is_inter);
    }
    void set_not_apis(bool _not_apis)
    {
      _pass.set_not_apis(_not_apis);
      _crew.set_not_apis(_not_apis);
      _extra_crew.set_not_apis(_not_apis);
    }
    const TAPICheckInfoList& pass() const { return _pass; }
    const TAPICheckInfoList& crew() const { return _crew; }
    const TAPICheckInfoList& extra_crew() const { return _extra_crew; }
    const std::set<std::string>& apis_formats() const { return _apis_formats; }

    static const std::set<TAPIType>& get_apis_doc_set()
    {
      static std::set<TAPIType> _apis_doc_set;
      if (_apis_doc_set.empty())
      {
        _apis_doc_set.insert(apiDoc);
        _apis_doc_set.insert(apiDoco);
        _apis_doc_set.insert(apiDocaB);
        _apis_doc_set.insert(apiDocaR);
        _apis_doc_set.insert(apiDocaD);
      }
      return _apis_doc_set;
    }

    void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      TAPICheckInfoList pass_client(_pass);
      for (std::set<TAPIType>::const_iterator api = get_apis_doc_set().begin(); api != get_apis_doc_set().end(); ++api)
        pass_client.get(*api).required_fields &= _extra_crew.get(*api).required_fields;
      pass_client.toXML(NewTextChild(node, "pass"));
      _crew.toXML(NewTextChild(node, "crew"));
    }
};

class TRouteAPICheckInfo : private std::map<std::string/*airp_arv*/, TCompleteAPICheckInfo>
{
  public:
    TRouteAPICheckInfo(const int point_id);
    bool apis_generation() const;

    boost::optional<const TCompleteAPICheckInfo&> get(const std::string &airp_arv) const;
};

std::string ElemToPaxDocCountryId(const std::string &elem, TElemFmt &fmt);

CheckIn::TPaxDocItem NormalizeDoc(const CheckIn::TPaxDocItem &doc);
CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc);
CheckIn::TPaxDocaItem NormalizeDoca(const CheckIn::TPaxDocaItem &doc);

void CheckDoc(const CheckIn::TPaxDocItem &doc,
              ASTRA::TPaxTypeExt pax_type_ext,
              const std::string &pax_surname,
              const TCompleteAPICheckInfo &checkInfo,
              TDateTime nowLocal);
void CheckDoco(const CheckIn::TPaxDocoItem &doc,
               ASTRA::TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo,
               TDateTime nowLocal);

void CheckDoca(const CheckIn::TPaxDocaItem &doc,
               ASTRA::TPaxTypeExt pax_type_ext,
               const TCompleteAPICheckInfo &checkInfo);

namespace APIS
{
enum TAlarmType { atDiffersFromBooking,
                  atIncomplete,
                  atManualInput,
                  atLength };

std::string EncodeAlarmType(const TAlarmType alarm );

bool isValidGender(const std::string &fmt, const std::string &pax_doc_gender, const std::string &pax_name);
bool isValidDocType(const std::string &fmt, const ASTRA::TPaxStatus &status, const std::string &doc_type);

}; //namespace APIS

void HandleDoc(const CheckIn::TPaxGrpItem &grp,
               const CheckIn::TSimplePaxItem &pax,
               const TCompleteAPICheckInfo &checkInfo,
               const TDateTime &checkDate,
               CheckIn::TPaxDocItem &doc);

void HandleDoco(const CheckIn::TPaxGrpItem &grp,
                const CheckIn::TSimplePaxItem &pax,
                const TCompleteAPICheckInfo &checkInfo,
                const TDateTime &checkDate,
                CheckIn::TPaxDocoItem &doco);

void HandleDoca(const CheckIn::TPaxGrpItem &grp,
                const CheckIn::TSimplePaxItem &pax,
                const TCompleteAPICheckInfo &checkInfo,
                CheckIn::TDocaMap &doca_map);

const std::string APIS_TR = "APIS_TR";
const std::string APIS_LT = "APIS_LT";

const TFilterQueue& getApisFilter( const std::string& type );
std::string toSoapReq( const std::string& text, const std::string& login, const std::string& pswd, const std::string& type );
void send_apis( const std::string type );
void send_apis_tr();
void send_apis_lt();
void process_reply( const std::string& result, const std::string& type );

struct TAPISFormat
{
  enum TPaxType { pass, crew };
  virtual long int required_fields(TPaxType, TAPIType) = 0;
  virtual ~TAPISFormat() {}
};

struct TAPISFormat_CSV_CZ : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_CZ_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_CZ : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_CZ_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_CN : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_CN_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_CN_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_IN : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_IN_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_IN_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_US : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_US_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_US_FIELDS;
    if (pax == crew && api == apiDocaB) return DOCA_B_CREW_EDI_US_FIELDS;
    if (pax == pass && api == apiDocaR) return DOCA_R_PASS_EDI_US_FIELDS;
    if (pax == crew && api == apiDocaR) return DOCA_R_CREW_EDI_US_FIELDS;
    if (pax == pass && api == apiDocaD) return DOCA_D_PASS_EDI_US_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_USBACK : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDocaB) return DOCA_B_CREW_EDI_USBACK_FIELDS;
    if (pax == pass && api == apiDocaR) return DOCA_R_PASS_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDocaR) return DOCA_R_CREW_EDI_USBACK_FIELDS;
    if (pax == pass && api == apiDocaD) return DOCA_D_PASS_EDI_USBACK_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_UK : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_UK_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_UK_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_ES : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_ES_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_ES_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_CSV_DE : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_DE_FIELDS;
    if (pax == pass && api == apiDoco) return DOCO_CSV_DE_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_TXT_EE : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_TXT_EE_FIELDS;
    if (pax == pass && api == apiDoco) return DOCO_TXT_EE_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_XML_TR : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_XML_TR_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_XML_TR_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_CSV_AE : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_AE_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_CSV_AE_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_LT : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_LT_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_CSV_TH : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_TH_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_CSV_TH_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_KR : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_KR_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_KR_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_APPS_SITA : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_APPS_SITA_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_APPS_SITA_FIELDS;
    return NO_FIELDS;
  }
};

struct TAPISFormat_EDI_AZ : public TAPISFormat
{
  long int required_fields(TPaxType pax, TAPIType api)
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_AZ_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_AZ_FIELDS;
    return NO_FIELDS;
  }
};

inline TAPISFormat* SpawnAPISFormat(const std::string& fmt)
{
  if (fmt=="CSV_CZ") return new TAPISFormat_CSV_CZ;
  if (fmt=="EDI_CZ") return new TAPISFormat_EDI_CZ;
  if (fmt=="EDI_CN") return new TAPISFormat_EDI_CN;
  if (fmt=="EDI_IN") return new TAPISFormat_EDI_IN;
  if (fmt=="EDI_US") return new TAPISFormat_EDI_US;
  if (fmt=="EDI_USBACK") return new TAPISFormat_EDI_USBACK;
  if (fmt=="EDI_UK") return new TAPISFormat_EDI_UK;
  if (fmt=="EDI_ES") return new TAPISFormat_EDI_ES;
  if (fmt=="CSV_DE") return new TAPISFormat_CSV_DE;
  if (fmt=="TXT_EE") return new TAPISFormat_TXT_EE;
  if (fmt=="XML_TR") return new TAPISFormat_XML_TR;
  if (fmt=="CSV_AE") return new TAPISFormat_CSV_AE;
  if (fmt=="EDI_LT") return new TAPISFormat_EDI_LT;
  if (fmt=="CSV_TH") return new TAPISFormat_CSV_TH;
  if (fmt=="EDI_KR") return new TAPISFormat_EDI_KR;
  if (fmt=="APPS_SITA") return new TAPISFormat_APPS_SITA;
  if (fmt=="EDI_AZ") return new TAPISFormat_EDI_AZ;
  throw EXCEPTIONS::Exception("SpawnAPISFormat: unhandled format %s", fmt.c_str());
}

#endif // APIS_UTILS_H
