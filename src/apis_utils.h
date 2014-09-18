#ifndef APIS_UTILS_H
#define APIS_UTILS_H

#include "passenger.h"
#include "astra_misc.h"

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

const long int TKN_MINTRANS_FIELDS=TKN_TICKET_NO_FIELD;

//==============================================================================

class TCheckDocTknInfo
{
  public:
    bool is_inter;
    long int required_fields; //битовая маска
    long int readonly_fields; //битовая маска временно не используется
    bool not_apis;
    TCheckDocTknInfo()
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
      not_apis=false;
      required_fields=NO_FIELDS;
      readonly_fields=NO_FIELDS;
    }
};

class TCheckTknInfo
{
  public:
    TCheckDocTknInfo tkn;
    void clear()
    {
      tkn.clear();
    }
    void toXML(xmlNodePtr node) const
    {
      tkn.toXML(NewTextChild(node, "tkn"));
    }
};

class TCheckDocInfo
{
  public:
    TCheckDocTknInfo doc, doco, docaB, docaR, docaD;
    void clear()
    {
      doc.clear();
      doco.clear();
      docaB.clear();
      docaR.clear();
      docaD.clear();
    }
    void toXML(xmlNodePtr node) const
    {
      doc.toXML(NewTextChild(node, "doc"));
      doco.toXML(NewTextChild(node, "doco"));
      docaB.toXML(NewTextChild(node, "doca_b"));
      docaR.toXML(NewTextChild(node, "doca_r"));
      docaD.toXML(NewTextChild(node, "doca_d"));
    }
};

class TCompleteCheckTknInfo
{
  public:
    TCheckTknInfo pass;
    TCheckTknInfo crew;
    void clear()
    {
      pass.clear();
      crew.clear();
    }
};

class TCompleteCheckDocInfo
{
  public:
    TCheckDocInfo pass;
    TCheckDocInfo crew;
    void clear()
    {
      pass.clear();
      crew.clear();
    }
};

enum TCheckInfoType { ciDoc, ciDoco, ciDocaB, ciDocaR, ciDocaD, ciTkn };

TCompleteCheckDocInfo GetCheckDocInfo(const int point_dep, const std::string& airp_arv);
TCompleteCheckDocInfo GetCheckDocInfo(const int point_dep, const std::string& airp_arv,
                                      std::set<std::string> &apis_formats);
TCompleteCheckTknInfo GetCheckTknInfo(const int point_dep);
std::string ElemToPaxDocCountryId(const std::string &elem, TElemFmt &fmt);

typedef std::map<std::string/*airp_arv*/, std::pair<TCompleteCheckDocInfo, std::set<std::string> > > TAPISMap;
void GetAPISSets( const int point_id, TAPISMap &apis_map, std::set<std::string> &apis_formats);

bool CheckLetDigSpace(const std::string &str, const TCheckDocTknInfo &checkDocInfo, std::string::size_type &errorIdx);
bool CheckLetSpaceDash(const std::string &str, const TCheckDocTknInfo &checkDocInfo, std::string::size_type &errorIdx);
bool CheckLetDigSpaceDash(const std::string &str, const TCheckDocTknInfo &checkDocInfo, std::string::size_type &errorIdx);

void CheckDoc(const CheckIn::TPaxDocItem &doc,
              const TCheckDocTknInfo &checkDocInfo,
              BASIC::TDateTime nowLocal);
CheckIn::TPaxDocItem NormalizeDoc(const CheckIn::TPaxDocItem &doc);
void CheckDoco(const CheckIn::TPaxDocoItem &doc,
               const TCheckDocTknInfo &checkDocInfo,
               BASIC::TDateTime nowLocal);
CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc);
void CheckDoca(const CheckIn::TPaxDocaItem &doc,
               const TCheckDocTknInfo &checkDocInfo);
CheckIn::TPaxDocaItem NormalizeDoca(const CheckIn::TPaxDocaItem &doc);

namespace APIS
{
enum TAlarmType { atDiffersFromBooking,
                  atIncomplete,
                  atManualInput,
                  atLength };

std::string EncodeAlarmType(const TAlarmType alarm );
}; //namespace APIS

#endif // APIS_UTILS_H
