#ifndef APIS_CREATOR_H
#define APIS_CREATOR_H

#include <map>
#include <string>
//#include "apis.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_main.h"
#include "base_tables.h"
#include "apis_edi_file.h"
#include "misc.h"
#include "passenger.h"
#include "tlg/tlg.h"
#include "trip_tasks.h"
#include "file_queue.h"
#include "astra_service.h"
#include "apis_utils.h"
#include "date_time.h"
#include "apis_tools.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

const string ENDL = "\r\n";
const string TRANSPORT_TYPE_FILE = "FILE";
const string TRANSPORT_TYPE_RABBIT_MQ = "RABBIT_MQ";

const string apis_test_text =
"select 'ESAPIS:ZZ' AS edi_addr, 'AIR EUROPA:UX' AS edi_own_addr, code AS format "
+ TRANSPORT_TYPE_FILE + " AS transport_type, 'mvd_czech_edi' AS transport_params "
"FROM apis_formats "
"WHERE code<>'APPS_SITA' AND code<>'TEST' "
"ORDER BY format";

int apis_test_single(int argc, char **argv);

bool create_apis_file(int point_id, const string& task_name);

struct apis_test_key
{
  int route_point_id;
  string format;
  string ToString() const
  {
    ostringstream s;
    s << "route " << route_point_id << ", format \"" << format << "\"";
    return s.str();
  }
  void set(const int& a_route_point_id, const string& a_format)
  {
    route_point_id = a_route_point_id;
    format = a_format;
  }
  apis_test_key(const int& a_route_point_id = 0, const string& a_format = "")
  {
    set(a_route_point_id, a_format);
  }
};

inline bool operator== (const apis_test_key& k1, const apis_test_key& k2)
{
  return k1.route_point_id == k2.route_point_id && k1.format == k2.format;
}

struct apis_test_key_less
{
  bool operator() (const apis_test_key& key1, const apis_test_key& key2) const
  {
    return  key1.route_point_id < key2.route_point_id ||
            (key1.route_point_id == key2.route_point_id && key1.format < key2.format);
  }
};

struct apis_test_value
{
  vector< pair<string, string> > files;
  string text;
  string type;
  map<string, string> file_params;
  apis_test_value(  const vector< pair<string, string> >& a_files,
                    const string& a_text,
                    const string& a_type,
                    const map<string, string>& a_file_params)
  : files(a_files), text(a_text), type(a_type), file_params(a_file_params) {}
  bool operator== (const apis_test_value& v2) const
  {
    return  files == v2.files &&
            text == v2.text &&
            type == v2.type &&
            file_params == v2.file_params;
  }
  unsigned int PayloadSize() const
  {
    return files.size() + text.size() + type.size() + file_params.size();
  }
  string ToString() const
  {
    ostringstream f;
    f << "FILES:" << endl;
    for (const auto i : files)
    {
      f << i.first << endl;
      f << i.second << endl;
    }
    f << "TEXT:" << endl;
    f << text << endl;
    f << "TYPE:" << endl;
    f << type << endl;
    f << "FILE_PARAMS:" << endl;
    for (const auto i : file_params)
    {
      f << i.first << endl;
      f << i.second << endl;
    }
    return f.str();
  }
};

struct TApisTestMap : public map<apis_test_key, apis_test_value, apis_test_key_less>
{
  bool exception = false;
  string str_exception;
  apis_test_key try_key;
  string ToString()
  {
    ostringstream s;
    s << "TEST_MAP;"
      << " EXCEPTION: " << exception
      << " STRING: \"" << str_exception << "\""
      << " TRY_KEY: " << try_key.ToString() << endl;
    for (const auto i : *this)
      s << i.first.ToString() << endl << i.second.ToString() << endl;
    return s.str();
  }
};

//---------------------------------------------------------------------------------------
// from apis.cc TODO потом убрать все дубликаты

class TAirlineOfficeInfo
{
  public:
    string contact_name;
    string phone;
    string fax;
};

void GetAirlineOfficeInfo(const string &airline,
                          const string &country,
                          const string &airp,
                          list<TAirlineOfficeInfo> &offices);

//---------------------------------------------------------------------------------------

struct TApisPaxData : public CheckIn::TSimplePaxItem
{
  TPaxStatus status;

  string airp_final_lat;
  string airp_final_code;

  CheckIn::TPaxDocItem doc;
  boost::optional<CheckIn::TPaxDocoItem> doco;
  boost::optional<CheckIn::TPaxDocaItem> docaD;
  boost::optional<CheckIn::TPaxDocaItem> docaR;
  boost::optional<CheckIn::TPaxDocaItem> docaB;

  vector< pair<int, string> > seats;
  int amount;
  int weight;
  set<string> tags;

  string doc_type_lat() const
  {
    if (doc.type.empty())
      return "";

    const TPaxDocTypesRow &doc_type_row = (const TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",doc.type);
    if (doc_type_row.code_lat.empty())
      throw Exception("doc_type.code_lat empty (code=%s)",doc.type.c_str());

    return doc_type_row.code_lat;
  }

  string doco_type_lat() const
  {
    if (!doco)
      throw Exception("doco empty");

    const TPaxDocTypesRow &doco_type_row = (const TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",doco.get().type);
    if (doco_type_row.code_lat.empty())
      throw Exception("doco_type.code_lat empty (code=%s)",doco.get().type.c_str());

    return doco_type_row.code_lat;
  }
};

struct TApisSetsData
{
  string fmt;
  string edi_own_addr;
  string edi_addr;
  string transport_type;
  string transport_params;
};

struct FlightlegDataset
{
  string airp_code_lat;
  string airp_code;
  TDateTime scd_in_local;
  TDateTime scd_out_local;
  string country_code_iso;
  string country_code;
};

struct TApisRouteData
{
  int dataset_point_id;
  int route_point_id;
  string task_name;
  bool final_apis;
  string country_dep;
  // -------
  string country_arv_code;
  string country_arv_code_lat;
  string country_regul_dep;
  string country_regul_arv;
  bool use_us_customs_tasks;
  int flt_no;
  string suffix;
  TDateTime scd_out_local;
  TDateTime scd_in_local;
  string airline_name;
  list<TApisPaxData> lstPaxData;
  list<TApisSetsData> lstSetsData;
  list<FlightlegDataset> lstLegs;

  // getters

  string airline_code_qry;
  const TAirlinesRow& airline() const
  {
    return (const TAirlinesRow&)base_tables.get("airlines").get_row("code",airline_code_qry);
  }
  string airline_code() const
  {
    if (airline().code.empty())
      throw Exception("airline.code empty (qry=%s)",airline_code_qry.c_str());
    return airline().code;
  }
  string airline_code_lat() const
  {
    if (airline().code_lat.empty())
      throw Exception("airline.code_lat empty (code=%s)",airline_code().c_str());
    return airline().code_lat;
  }
  string airline_city() const
  {
    if (airline().city.empty())
      throw Exception("airline.city empty (code=%s)",airline_code().c_str());
    return airline().city;
  }

  string airp_dep_qry;
  const TAirpsRow& airp_dep() const
  {
    return (const TAirpsRow&)base_tables.get("airps").get_row("code",airp_dep_qry);
  }
  string airp_dep_code() const
  {
    if (airp_dep().code.empty())
      throw Exception("airp_dep.code empty (qry=%s)",airp_dep_qry.c_str());
    return airp_dep().code;
  }
  string airp_dep_code_lat() const
  {
    if (airp_dep().code_lat.empty())
      throw Exception("airp_dep.code_lat empty (code=%s)",airp_dep_code().c_str());
    return airp_dep().code_lat;
  }

  string airp_arv_qry;
  const TAirpsRow& airp_arv() const
  {
    return (const TAirpsRow&)base_tables.get("airps").get_row("code",airp_arv_qry);
  }
  string airp_arv_code() const
  {
    if (airp_arv().code.empty())
      throw Exception("airp_arv.code empty (qry=%s)",airp_arv_qry.c_str());
    return airp_arv().code;
  }
  string airp_arv_code_lat() const
  {
    if (airp_arv().code_lat.empty())
      throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv_code().c_str());
    return airp_arv().code_lat;
  }

  string airp_cbp_qry;
  const TAirpsRow& airp_cbp() const
  {
    return (const TAirpsRow&)base_tables.get("airps").get_row("code",airp_cbp_qry);
  }
  string airp_cbp_code() const
  {
    if (airp_cbp().code.empty())
      throw Exception("airp_cbp.code empty (qry=%s)",airp_cbp_qry.c_str());
    return airp_cbp().code;
  }
  string airp_cbp_code_lat() const
  {
    if (airp_cbp().code_lat.empty())
      throw Exception("airp_cbp.code_lat empty (code=%s)",airp_cbp_code().c_str());
    return airp_cbp().code_lat;
  }
};

struct TApisDataset
{
  bool FromDB(int point_id, const string& task_name, TApisTestMap* test_map = nullptr);
  list<TApisRouteData> lstRouteData;

  // getters

  string airline_code_qry;
  const TAirlinesRow& airline() const
  {
    return (const TAirlinesRow&)base_tables.get("airlines").get_row("code",airline_code_qry);
  }
  string airline_code() const
  {
    if (airline().code.empty())
      throw Exception("airline.code empty (qry=%s)",airline_code_qry.c_str());
    return airline().code;
  }
};

//---------------------------------------------------------------------------------------

string NormalizeDocNo(const string& str, bool try_keep_only_digits); // apis.cc

enum TApisRule
{
  // TODO убрать названия форматов из правил

  // PaxlstInfo
  r_notSetSenderRecipient, // edi // tr
  r_omitAirlineCode, // edi // tr // TODO переделать получше
  r_setCarrier, // edi // для всех edi?
  r_setFltLegs, // edi // tr

  // PassengerInfo
  r_notOmitCrew, // edi
  r_setPrBrd, // edi // tr
  r_setGoShow, // edi // tr
  r_setPersType, // edi // tr
  r_setTicketNumber, // edi // tr
  r_setFqts, // edi // tr
  r_addMarkFlt, // edi // tr
  r_setSeats, // edi
  r_setBagCount, // edi
  r_setBagWeight, // edi
  r_bagTagSerials,
  r_convertPaxNames, // edi
  r_setCBPPort, // edi // только для US? (LOC 22)
  r_processDocType, // edi
  r_processDocNumber, // edi
  r_docaD_US, // edi
  r_setResidCountry, // edi
  r_docaR_US, // edi
  r_setBirthCountry, // edi
  r_docaB_US, // edi

  // creation
  r_create_ON_CLOSE_CHECKIN,
  r_create_ON_CLOSE_BOARDING,
  r_skip_ON_TAKEOFF,

  // file
  r_fileExtTXT, // edi
  r_fileSimplePush, // edi
  r_lstTypeLetter, // edi
  r_file_XML_TR, // edi
  r_file_LT, // edi

  // txt & csv
  r_birth_date,
  r_expiry_date,
  r_birth_country, // TODO r_setBirthCountry
  r_trip_type,
  r_airp_arv_code_lat,
  r_airp_dep_code_lat,

  //  ВИЗА
  r_doco,
};

enum TApisFileRule
{
  r_file_rule_undef,
  r_file_rule_1,
  r_file_rule_2,
  r_file_rule_txt_AE_TH,
  r_file_rule_txt_common,
};

enum TApisFormatType
{
  t_format_undef,
  t_format_edi,
  t_format_txt,
  t_format_apps,
};

enum TIataCodeType
{
  iata_code_default,
  iata_code_UK,
  iata_code_TR,
  iata_code_DE,
};

struct TPaxDataFormatted
{
  TPaxStatus status;
  string doc_surname;
  string doc_first_name;
  string doc_second_name;
  string gender;
  string doc_type;
  string doc_no;
  string nationality;
  string issue_country;
  string birth_date;
  string expiry_date;
  string birth_country;
  string trip_type;
  string airp_final_lat;
  string airp_final_code;
  string airp_arv_code_lat;
  string airp_dep_code_lat;

  bool doco_exists = false;
  string doco_type;
  string doco_no;
  string doco_applic_country;

  int count_current;
};

struct TTxtDataFormatted
{
  int count_overall;
  list<TPaxDataFormatted> lstPaxData;
};

//---------------------------------------------------------------------------------------

struct TAPISFormat
{
  enum TPaxType { pass, crew };
  string fmt;
  string edi_own_addr;
  string edi_addr;
  string transport_type;
  string transport_params;
  set<TApisRule> rules;
  TApisFileRule file_rule;
  TApisFormatType format_type;
  void add_rule(TApisRule r) { rules.insert(r); }
  bool rule(TApisRule r) const { return rules.count(r); }
  virtual long int required_fields(TPaxType, TAPIType) const = 0;
  virtual bool CheckDocoIssueCountry(string issue_place) { return true; }

  TAPISFormat()
  {
    file_rule = r_file_rule_undef;
    format_type = t_format_undef;
  }
  virtual ~TAPISFormat() {}

  string dir() const
  {
    return transport_type == TRANSPORT_TYPE_FILE ? transport_params : fmt + string("_dir_error");
  }

  virtual void convert_pax_names(string& first_name, string& second_name) const {}
  virtual string unknown_gender() const { return ""; }

  virtual string Gender(ASTRA::TGender::Enum gender) const
  {
    switch(gender)
    {
      case TGender::Male:
        return "M"; break;
      case TGender::Female:
        return "F"; break;
      case TGender::Unknown:
      default:
        return unknown_gender(); break;
    }
  }

  virtual string lst_type_extra(bool final_apis) const { return ""; }
  virtual string process_doc_type(const string& doc_type) const { return doc_type; }
  virtual string process_doc_no(const string& no) const { return no; }
  virtual bool check_doc_type_no(const string& doc_type, const string& doc_no) const
  { return !doc_no.empty(); } // edi?

  // TODO только для EDI
  virtual string appRef() const { return "APIS"; }
  virtual string mesRelNum() const { return "02B"; }
  virtual string mesAssCode() const { return "IATA"; }
  virtual string respAgnCode() const { return "111"; }
  virtual bool viewUNGandUNE() const { return false; }

  virtual string ProcessPhoneFax(const string& s) const { return s; }
  virtual string apis_country() const { return ""; }
  string direction(string country_dep) const { return (country_dep == apis_country())?"O":"I"; }
  virtual string DateTimeFormat() const { return ""; }

  virtual void CreateTxtBodies( const TTxtDataFormatted& tdf,
                                ostringstream& body,
                                ostringstream& paxs_body,
                                ostringstream& crew_body) const {}

  virtual string CreateFilename(const TApisRouteData& route) const { return ""; }

  virtual void CreateHeaders( const TApisRouteData& route,
                              ostringstream& header,
                              ostringstream& pax_header,
                              ostringstream& crew_header,
                              ostringstream& crew_body,
                              int count_overall) const {}

  virtual void CreatePaxHeader_AE_TH(ostringstream& pax_header) const
  { throw Exception("CreatePaxHeader_AE_TH base class function called"); }
  // TODO сделать получше
  string HeaderStart_AE_TH() const { return "***START,,,,,,,,,,,,"; }
  string HeaderEnd_AE_TH() const { return "***END,,,,,,,,,,,,"; }
  string NameTrailPax_AE_TH() const { return "_P.CSV"; }
  string NameTrailCrew_AE_TH() const { return "_C.CSV"; }

  virtual TIataCodeType IataCodeType() const { return iata_code_default; }

  virtual bool NeedCBPPort(string country_regul_dep) const { return true; }

  // используется только для Германии (коды D и DEU)
  virtual string ConvertCountry(string country) const
  {
    return PaxDocCountryIdToPrefferedElem(country, efmtCodeISOInter, AstraLocale::LANG_EN);
  }

protected:
  void ConvertPaxNamesTrunc(string& doc_first_name, string& doc_second_name) const
  {
    doc_first_name=TruncNameTitles(doc_first_name.c_str());
    doc_second_name=TruncNameTitles(doc_second_name.c_str());
  }
  void ConvertPaxNamesConcat(string& doc_first_name, string& doc_second_name) const
  {
    if (!doc_second_name.empty())
    {
      doc_first_name+=" "+doc_second_name;
      doc_second_name.clear();
    }
  }
  string HyphenToSpace(const string& s) const
  {
    string r(s);
    replace(r.begin(), r.end(), '-', ' ');
    return r;
  }
  string CreateCommonFilename( const TApisRouteData& route,
                                    string extension = ".CSV") const
  {
    ostringstream file_name;
    ostringstream f;
    f << route.airline_code_lat() << route.flt_no << route.suffix;
    file_name << dir()
              << "/"
              << route.airline_code_lat()
              << (f.str().size()<6?string(6-f.str().size(),'0'):"") << route.flt_no << route.suffix
                //по стандарту поле не должно превышать 6 символов
              << route.airp_dep_code_lat()
              << route.airp_arv_code_lat()
              << DateTimeToStr(route.scd_out_local,"yyyymmdd");
    file_name << extension;
    return file_name.str();
  }
  void CreateHeader_AE_TH(  const TApisRouteData& route,
                            ostringstream& header,
                            ostringstream& pax_header,
                            ostringstream& crew_header,
                            ostringstream& crew_body) const
  {
    string tb_date, tb_time, tb_airp;
    // virtual apis_country()
    getTBTripItem( route.dataset_point_id, route.route_point_id, apis_country(), tb_date, tb_time, tb_airp );
    header << "*DIRECTION," << direction(route.country_dep) << ",,,,,,,,,,," << ENDL
            << "*FLIGHT," << route.airline_code_lat() << setw(3) << setfill('0')
                          << route.flt_no << route.suffix << ",,,,,,,,,,," << ENDL
            << "*DEP PORT," << route.airp_dep_code_lat() << ",,,,,,,,,,," << ENDL
            << "*DEP DATE," << DateTimeToStr(route.scd_out_local,"dd-mmm-yyyy", true) << ",,,,,,,,,,," << ENDL
            << "*DEP TIME," << DateTimeToStr(route.scd_out_local,"hhnn") << ",,,,,,,,,,," << ENDL
            << "*ARR PORT," << route.airp_arv_code_lat() << ",,,,,,,,,,," << ENDL
            << "*ARR DATE," << DateTimeToStr(route.scd_in_local,"dd-mmm-yyyy", true) << ",,,,,,,,,,," << ENDL
            << "*ARR TIME," << DateTimeToStr(route.scd_in_local,"hhnn") << ",,,,,,,,,,," << ENDL;
    header << "*TB PORT," << tb_airp << ",,,,,,,,,,," << ENDL;
    header << "*TB DATE," << tb_date << ",,,,,,,,,,," << ENDL
            << "*TB TIME," << tb_time << ",,,,,,,,,,," << ENDL;

    // virtual CreatePaxHeader_AE_TH()
    CreatePaxHeader_AE_TH(pax_header);

    if (!crew_body.str().empty())
    {
      crew_header << pax_header.str()
                  << "*TYPE,C,,,,,,,,,,," << ENDL
                  << header.str();
    }

    pax_header << "*TYPE,P,,,,,,,,,,," << ENDL
                << header.str();
  }
};

struct TEdiAPISFormat : public TAPISFormat
{
  TEdiAPISFormat()
  {
    format_type = t_format_edi;
  }
  bool check_doc_type_no(const string& doc_type, const string& doc_no) const
  {
    return !doc_type.empty() && !doc_no.empty();
  }
  virtual bool viewUNGandUNE() const
  {
    return true;
  }
};

struct TTxtApisFormat : public TAPISFormat
{
  TTxtApisFormat()
  {
    format_type = t_format_txt;
  }
};

struct TAppsSitaFormat : public TAPISFormat
{
  TAppsSitaFormat()
  {
    format_type = t_format_apps;
  }
};

//---------------------------------------------------------------------------------------

struct TAPISFormat_CSV_CZ : public TTxtApisFormat
{
  TAPISFormat_CSV_CZ()
  {
    add_rule(r_birth_date);
    add_rule(r_expiry_date);
    file_rule = r_file_rule_txt_common;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_CZ_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "M"; }
  string DateTimeFormat() const { return "ddmmmyy"; }
  void CreateTxtBodies( const TTxtDataFormatted& tdf,
                        ostringstream& body,
                        ostringstream& paxs_body,
                        ostringstream& crew_body) const
  {
    for ( list<TPaxDataFormatted>::const_iterator ipdf = tdf.lstPaxData.begin();
          ipdf != tdf.lstPaxData.end();
          ++ipdf)
    {
      body << ipdf->doc_surname << ";"
            << ipdf->doc_first_name << ";"
            << ipdf->doc_second_name << ";"
            << ipdf->birth_date << ";"
            << ipdf->gender << ";"
            << ipdf->nationality << ";"
            << ipdf->doc_type << ";"
            << ipdf->doc_no << ";"
            << ipdf->expiry_date << ";"
            << ipdf->issue_country << ";";
      body << ENDL;
    }
  }
  string CreateFilename(const TApisRouteData& route) const
  {
    return CreateCommonFilename(route);
  }
  virtual void CreateHeaders( const TApisRouteData& route,
                              ostringstream& header,
                              ostringstream& pax_header,
                              ostringstream& crew_header,
                              ostringstream& crew_body,
                              int count_overall) const
  {
    header << "csv;"
      << route.airline_name << ";"
      << route.airline_code_lat() << setw(3) << setfill('0') << route.flt_no << route.suffix << ";"
      << route.airp_dep_code_lat() << ";"
      << DateTimeToStr(route.scd_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
      << route.airp_arv_code_lat() << ";"
      << DateTimeToStr(route.scd_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
      << count_overall << ";" << ENDL;
  }
};

struct TAPISFormat_EDI_CZ : public TEdiAPISFormat
{
  TAPISFormat_EDI_CZ()
  {
    add_rule(r_processDocNumber);
    add_rule(r_fileExtTXT);
    add_rule(r_fileSimplePush);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_CZ_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "M"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
};

struct TAPISFormat_EDI_CN : public TEdiAPISFormat
{
  TAPISFormat_EDI_CN()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocNumber);
    add_rule(r_notOmitCrew);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_CN_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_CN_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string respAgnCode() const { return "ZZZ"; }
  string ProcessPhoneFax(const string& s) const { return HyphenToSpace(s); }
};

// основано на TAPISFormat_EDI_CN
// -------------------------------------------------------------------------------------------------
struct TAPISFormat_EDI_IN : public TEdiAPISFormat // Индия
{
  TAPISFormat_EDI_IN()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocNumber);
    add_rule(r_notOmitCrew);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_IN_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_IN_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string ProcessPhoneFax(const string& s) const { return HyphenToSpace(s); }
};

/* старый формат для Индии
struct TAPISFormat_EDI_IN : public TEdiAPISFormat
{
  TAPISFormat_EDI_IN()
  {
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_IN_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_IN_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, true); }
  string appRef() const { return ""; }
  string mesRelNum() const { return "05B"; }
};
*/

struct TAPISFormat_EDI_US : public TEdiAPISFormat
{
  TAPISFormat_EDI_US()
  {
    add_rule(r_setCBPPort);
    add_rule(r_docaD_US);
    add_rule(r_docaR_US);
    add_rule(r_docaB_US);
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_US_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_US_FIELDS;
    if (pax == crew && api == apiDocaB) return DOCA_B_CREW_EDI_US_FIELDS;
    if (pax == pass && api == apiDocaR) return DOCA_R_PASS_EDI_US_FIELDS;
    if (pax == crew && api == apiDocaR) return DOCA_R_CREW_EDI_US_FIELDS;
    if (pax == pass && api == apiDocaD) return DOCA_D_PASS_EDI_US_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "M"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string mesAssCode() const { return "CBP"; }
  bool NeedCBPPort(string country_regul_dep) const override { return country_regul_dep!=US_CUSTOMS_CODE; }
};

struct TAPISFormat_EDI_USBACK : public TEdiAPISFormat
{
  TAPISFormat_EDI_USBACK()
  {
    add_rule(r_setCBPPort);
    add_rule(r_docaD_US);
    add_rule(r_docaR_US);
    add_rule(r_docaB_US);
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDocaB) return DOCA_B_CREW_EDI_USBACK_FIELDS;
    if (pax == pass && api == apiDocaR) return DOCA_R_PASS_EDI_USBACK_FIELDS;
    if (pax == crew && api == apiDocaR) return DOCA_R_CREW_EDI_USBACK_FIELDS;
    if (pax == pass && api == apiDocaD) return DOCA_D_PASS_EDI_USBACK_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "M"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string mesAssCode() const { return "CBP"; }
  bool NeedCBPPort(string country_regul_dep) const override { return country_regul_dep!=US_CUSTOMS_CODE; }
};

struct TAPISFormat_EDI_UK : public TEdiAPISFormat
{
  TAPISFormat_EDI_UK()
  {
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    add_rule(r_create_ON_CLOSE_CHECKIN);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_UK_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_UK_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  string lst_type_extra(bool final_apis) const { return final_apis?"DC:1.0":"CI:1.0"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string respAgnCode() const { return "109"; }
  string appRef() const { return "UKBAOP"; }
  string mesRelNum() const { return "05B"; }
  TIataCodeType IataCodeType() const { return iata_code_UK; }
};

struct TAPISFormat_EDI_ES : public TEdiAPISFormat
{
  TAPISFormat_EDI_ES()
  {
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_ES_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_ES_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
};

struct TAPISFormat_EDI_DE : public TEdiAPISFormat // Германия
{
  // уточнить viewUNGandUNE

  TAPISFormat_EDI_DE()
  {
    add_rule(r_setCarrier);
    add_rule(r_convertPaxNames);
    add_rule(r_setCBPPort); // УТОЧНИТЬ!
    add_rule(r_processDocType); // уточнить
    add_rule(r_processDocNumber); // уточнить
    add_rule(r_doco); // ВИЗА!!!
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_DE_FIELDS;
    if (pax == pass && api == apiDoco) return DOCO_EDI_DE_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  TIataCodeType IataCodeType() const { return iata_code_DE; }

  // уточнить
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesTrunc(first_name, second_name);
  }

  // уточнить
  string process_doc_type(const string& doc_type) const
  {
    if (doc_type!="P" && doc_type!="I") return "P";
    else return doc_type;
  }

  // уточнить
  string process_doc_no(const string& no) const
  {
    return NormalizeDocNo(no, false);
  }

  string ConvertCountry(string country) const override
  {
    return PaxDocCountryIdToPrefferedElem(country, efmtCodeNative, AstraLocale::LANG_EN);
  }
};

struct TAPISFormat_CSV_DE : public TTxtApisFormat // Германия
{
  TAPISFormat_CSV_DE()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocType);
    add_rule(r_birth_date);
    add_rule(r_airp_arv_code_lat);
    add_rule(r_airp_dep_code_lat);
    add_rule(r_doco);
    file_rule = r_file_rule_txt_common;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_DE_FIELDS;
    if (pax == pass && api == apiDoco) return DOCO_CSV_DE_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesTrunc(first_name, second_name);
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "U"; }
  string process_doc_type(const string& doc_type) const
  {
    if (doc_type!="P" && doc_type!="I") return "P";
    else return doc_type;
  }
  string DateTimeFormat() const { return "yymmdd"; }
  void CreateTxtBodies( const TTxtDataFormatted& tdf,
                        ostringstream& body,
                        ostringstream& paxs_body,
                        ostringstream& crew_body) const
  {
    for ( list<TPaxDataFormatted>::const_iterator ipdf = tdf.lstPaxData.begin();
          ipdf != tdf.lstPaxData.end();
          ++ipdf)
    {
      if (ipdf->airp_final_lat.empty())
        throw Exception("airp_final.code_lat empty (code=%s)",ipdf->airp_final_code.c_str());
      body << ipdf->doc_surname << ";"
            << ipdf->doc_first_name << ";"
            << ipdf->gender << ";"
            << ipdf->birth_date << ";"
            << ipdf->nationality << ";"
            << ipdf->airp_arv_code_lat << ";"
            << ipdf->airp_dep_code_lat << ";"
            << ipdf->airp_final_lat << ";"
            << ipdf->doc_type << ";"
            << convert_char_view(ipdf->doc_no,true) << ";"
            << ipdf->issue_country;
      if (ipdf->doco_exists)
      {
        body << ";"
              << ipdf->doco_type << ";"
              << convert_char_view(ipdf->doco_no,true) << ";"
              << ipdf->doco_applic_country;
      }
      body << ENDL;
    }
  }
  string CreateFilename(const TApisRouteData& route) const
  {
    return CreateCommonFilename(route);
  }
  virtual void CreateHeaders( const TApisRouteData& route,
                              ostringstream& header,
                              ostringstream& pax_header,
                              ostringstream& crew_header,
                              ostringstream& crew_body,
                              int count_overall) const
  {
    header << route.airline_code_lat() << ";"
      << route.airline_code_lat() << setw(3) << setfill('0') << route.flt_no << ";"
      << route.airp_dep_code_lat() << ";" << DateTimeToStr(route.scd_out_local,"yymmddhhnn") << ";"
      << route.airp_arv_code_lat() << ";" << DateTimeToStr(route.scd_in_local,"yymmddhhnn") << ";"
      << count_overall << ENDL;
  }

  string ConvertCountry(string country) const override
  {
    return PaxDocCountryIdToPrefferedElem(country, efmtCodeNative, AstraLocale::LANG_EN);
  }
};

struct TAPISFormat_TXT_EE : public TTxtApisFormat
{
  TAPISFormat_TXT_EE()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocType);
    add_rule(r_birth_date);
    add_rule(r_doco);
    file_rule = r_file_rule_txt_common;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_TXT_EE_FIELDS;
    if (pax == pass && api == apiDoco) return DOCO_TXT_EE_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesTrunc(first_name, second_name);
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "N"; }
  string process_doc_type(const string& doc_type) const
  {
    if (doc_type!="P") return ""; else return "2";
  }
  string DateTimeFormat() const { return "dd.mm.yyyy"; }
  void CreateTxtBodies( const TTxtDataFormatted& tdf,
                        ostringstream& body,
                        ostringstream& paxs_body,
                        ostringstream& crew_body) const
  {
    for ( list<TPaxDataFormatted>::const_iterator ipdf = tdf.lstPaxData.begin();
          ipdf != tdf.lstPaxData.end();
          ++ipdf)
    {
      body << "1# " << ipdf->count_current + 1 << ENDL
            << "2# " << ipdf->doc_surname << ENDL
            << "3# " << ipdf->doc_first_name << ENDL
            << "4# " << ipdf->birth_date << ENDL
            << "5# " << ipdf->nationality << ENDL
            << "6# " << ipdf->doc_type << ENDL
            << "7# " << convert_char_view(ipdf->doc_no,true) << ENDL
            << "8# " << ipdf->issue_country << ENDL
            << "9# " << ipdf->gender << ENDL;
      if (ipdf->doco_exists)
      {
        body << "10# " << ENDL
              << "11# " << (ipdf->doco_type=="V"?convert_char_view(ipdf->doco_no,true):"") << ENDL;
      }
      else
      {
        body << "10# " << ENDL
              << "11# " << ENDL;
      }
    }
  }
  string CreateFilename(const TApisRouteData& route) const
  {
    ostringstream file_name;
    file_name << dir()
              << "/"
              << "LL-" << route.airline_code_lat() << setw(3) << setfill('0') << route.flt_no << route.suffix
              << "-" << DateTimeToStr(route.scd_in_local,"ddmmyyyy-hhnn") << "-S.TXT";
    return file_name.str();
  }
  virtual void CreateHeaders( const TApisRouteData& route,
                              ostringstream& header,
                              ostringstream& pax_header,
                              ostringstream& crew_header,
                              ostringstream& crew_body,
                              int count_overall) const
  {
    const TCitiesRow& airlineCityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code",route.airline_city());
    const TCountriesRow& airlineCountryRow = (const TCountriesRow&)base_tables.get("countries").get_row("code",airlineCityRow.country);
    if (airlineCountryRow.code_iso.empty())
      throw Exception("airlineCountryRow.code_iso empty (code=%s)",airlineCityRow.country.c_str());
    string airline_country = airlineCountryRow.code_iso;
    header << "1$ " << route.airline_name << ENDL
              << "2$ " << ENDL
              << "3$ " << airline_country << ENDL
              << "4$ " << ENDL
              << "5$ " << ENDL
              << "6$ " << ENDL
              << "7$ " << ENDL
              << "8$ " << ENDL
              << "9$ " << DateTimeToStr(route.scd_in_local,"dd.mm.yy hh:nn") << ENDL
              << "10$ " << (route.airp_arv_code()=="TLL"?"Tallinna Lennujaama piiripunkt":
                          route.airp_arv_code()=="TAY"?"Tartu piiripunkt":
                          route.airp_arv_code()=="URE"?"Kuressaare-2 piiripunkt":
                          route.airp_arv_code()=="KDL"?"Kardla Lennujaama piiripunkt":"") << ENDL
              << "11$ " << ENDL
              << "1$ " << route.airline_code_lat() << setw(3) << setfill('0') << route.flt_no << route.suffix << ENDL
              << "2$ " << ENDL
              << "3$ " << count_overall << ENDL;
  }
};

struct TAPISFormat_XML_TR : public TEdiAPISFormat
{
  TAPISFormat_XML_TR()
  {
    add_rule(r_setPrBrd);
    add_rule(r_setGoShow);
    add_rule(r_setPersType);
    add_rule(r_setTicketNumber);
    add_rule(r_setFqts);
    add_rule(r_addMarkFlt);
    add_rule(r_setSeats);
    add_rule(r_convertPaxNames);
    add_rule(r_docaD_US);
    add_rule(r_setResidCountry);
    add_rule(r_setBirthCountry);
    add_rule(r_notSetSenderRecipient);
    add_rule(r_omitAirlineCode);
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_file_XML_TR);
    add_rule(r_setFltLegs);
    add_rule(r_create_ON_CLOSE_CHECKIN);
    file_rule = r_file_rule_2;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_XML_TR_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_XML_TR_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesTrunc(first_name, second_name);
  }
  string unknown_gender() const { return "U"; }
  TIataCodeType IataCodeType() const { return iata_code_TR; }
};

// основано на TAPISFormat_EDI_CN
// -------------------------------------------------------------------------------------------------
struct TAPISFormat_EDI_TR : public TEdiAPISFormat
{
  TAPISFormat_EDI_TR()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocNumber);
    add_rule(r_notOmitCrew);
    add_rule(r_setSeats);
    add_rule(r_setCBPPort);
    add_rule(r_setBagCount); // уточнить
    add_rule(r_bagTagSerials); // номера бирок: FTX+BAG
    file_rule = r_file_rule_1;
    add_rule(r_fileSimplePush); // формировать EDIFACT одним блоком
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_TR_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_TR_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string respAgnCode() const { return "ZZZ"; }
  string ProcessPhoneFax(const string& s) const { return HyphenToSpace(s); }
  string mesRelNum() const override { return "12B"; } // уточнить
};
// -------------------------------------------------------------------------------------------------

struct TAPISFormat_CSV_AE : public TTxtApisFormat
{
  TAPISFormat_CSV_AE()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocType);
    add_rule(r_notOmitCrew);
    add_rule(r_birth_date);
    add_rule(r_expiry_date);
    add_rule(r_trip_type);
    file_rule = r_file_rule_txt_AE_TH;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_AE_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_CSV_AE_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "X"; }
  string process_doc_type(const string& doc_type) const
  {
    if (doc_type!="P") return "O";
    else return doc_type;
  }
  string apis_country() const { return "AE"; }
  string DateTimeFormat() const { return "dd-mmm-yyyy"; }
  void CreateTxtBodies( const TTxtDataFormatted& tdf,
                        ostringstream& body,
                        ostringstream& paxs_body,
                        ostringstream& crew_body) const
  {
    for ( list<TPaxDataFormatted>::const_iterator ipdf = tdf.lstPaxData.begin();
          ipdf != tdf.lstPaxData.end();
          ++ipdf)
    {
      ostringstream data;
      data << ipdf->doc_no << ","
            << ipdf->nationality << ","
            << ipdf->doc_type << ","
            << ipdf->issue_country << ","
            << ipdf->doc_surname << ","
            << ipdf->doc_first_name << ","
            << ipdf->birth_date << ","
            << ipdf->gender << ","
            << ipdf->birth_country << ","
            << ipdf->expiry_date << ","
            << ipdf->trip_type << ",,";
      if (ipdf->status == psCrew)
        crew_body << data.str() << ENDL;
      else
        paxs_body << data.str() << ENDL;
    }
  }
  string CreateFilename(const TApisRouteData& route) const
  {
    return CreateCommonFilename(route, "");
  }
  void CreateHeaders( const TApisRouteData& route,
                      ostringstream& header,
                      ostringstream& pax_header,
                      ostringstream& crew_header,
                      ostringstream& crew_body,
                      int count_overall) const
  {
    CreateHeader_AE_TH(route, header, pax_header, crew_header, crew_body);
  }
  void CreatePaxHeader_AE_TH(ostringstream& pax_header) const
  {
    pax_header << "***VERSION 2,,,,,,,,,,,," << ENDL
               << "***HEADER,,,,,,,,,,,," << ENDL;
  }
};

struct TAPISFormat_EDI_LT : public TEdiAPISFormat
{
  TAPISFormat_EDI_LT()
  {
    add_rule(r_file_LT);
    add_rule(r_processDocNumber);
    add_rule(r_create_ON_CLOSE_BOARDING);
    add_rule(r_skip_ON_TAKEOFF);
    file_rule = r_file_rule_2;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_LT_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "M"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
};

struct TAPISFormat_CSV_TH : public TTxtApisFormat
{
  TAPISFormat_CSV_TH()
  {
    add_rule(r_convertPaxNames);
    add_rule(r_processDocType);
    add_rule(r_notOmitCrew);
    add_rule(r_birth_date);
    add_rule(r_expiry_date);
    add_rule(r_birth_country);
    add_rule(r_trip_type);
    file_rule = r_file_rule_txt_AE_TH;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_CSV_TH_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_CSV_TH_FIELDS;
    return NO_FIELDS;
  }
  void convert_pax_names(string& first_name, string& second_name) const
  {
    ConvertPaxNamesConcat(first_name, second_name);
  }
  string unknown_gender() const { return "X"; }
  string process_doc_type(const string& doc_type) const
  {
    if (doc_type!="P") return "O";
    else return doc_type;
  }
  string apis_country() const { return "TH"; }
  string DateTimeFormat() const { return "dd-mmm-yyyy"; }
  void CreateTxtBodies( const TTxtDataFormatted& tdf,
                        ostringstream& body,
                        ostringstream& paxs_body,
                        ostringstream& crew_body) const
  {
    for ( list<TPaxDataFormatted>::const_iterator ipdf = tdf.lstPaxData.begin();
          ipdf != tdf.lstPaxData.end();
          ++ipdf)
    {
      ostringstream data;
      data << ipdf->doc_type << ","
            << ipdf->nationality << ","
            << ipdf->doc_no << ","
            << ipdf->expiry_date << ","
            << ipdf->issue_country << ","
            << ipdf->doc_surname << ","
            << ipdf->doc_first_name << ","
            << ipdf->birth_date << ","
            << ipdf->gender << ","
            << ipdf->birth_country << ","
            << ipdf->trip_type << ",,";
      if (ipdf->status == psCrew)
        crew_body << data.str() << ENDL;
      else
        paxs_body << data.str() << ENDL;
    }
  }
  string CreateFilename(const TApisRouteData& route) const
  {
    return CreateCommonFilename(route, "");
  }
  void CreateHeaders( const TApisRouteData& route,
                      ostringstream& header,
                      ostringstream& pax_header,
                      ostringstream& crew_header,
                      ostringstream& crew_body,
                      int count_overall) const
  {
    CreateHeader_AE_TH(route, header, pax_header, crew_header, crew_body);
  }
  void CreatePaxHeader_AE_TH(ostringstream& pax_header) const
  {
    pax_header << "***VERSION 4,,,,,,,,,,,," << ENDL
               << "***HEADER,,,,,,,,,,,," << ENDL
               << "***BATCH,APP,,,,,,,,,,," << ENDL;
  }
};

struct TAPISFormat_EDI_KR : public TEdiAPISFormat
{
  TAPISFormat_EDI_KR()
  {
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    add_rule(r_fileExtTXT);
    add_rule(r_lstTypeLetter);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_KR_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_KR_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string appRef() const { return ""; }
  string mesRelNum() const { return "05B"; }
};

// TODO разобраться с этим форматом
struct TAPISFormat_APPS_SITA : public TAPISFormat
{
  TAPISFormat_APPS_SITA()
  {
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_APPS_21_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_APPS_21_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return ""; }
};

struct TAPISFormat_EDI_AZ : public TEdiAPISFormat
{
  TAPISFormat_EDI_AZ()
  {
    add_rule(r_setSeats);
    add_rule(r_setBagCount);
    add_rule(r_setBagWeight);
    add_rule(r_setCarrier);
    add_rule(r_notOmitCrew);
    add_rule(r_processDocNumber);
    add_rule(r_create_ON_CLOSE_CHECKIN);
    add_rule(r_skip_ON_TAKEOFF);
    file_rule = r_file_rule_1;
  }
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (pax == pass && api == apiDoc) return DOC_EDI_AZ_FIELDS;
    if (pax == crew && api == apiDoc) return DOC_EDI_AZ_FIELDS;
    return NO_FIELDS;
  }
  string unknown_gender() const { return "U"; }
  string process_doc_no(const string& no) const { return NormalizeDocNo(no, false); }
  string appRef() const { return ""; }
  string mesRelNum() const { return "05B"; }
};

//---------------------------------------------------------------------------------------

struct TAPPSVersion21 : public TAppsSitaFormat
{
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (api == apiDoc) return DOC_APPS_21_FIELDS;
    return NO_FIELDS;
  }
  bool CheckDocoIssueCountry(string issue_place)
  {
    return true; // для 21 версии не требуется
  }
};

struct TAPPSVersion26 : public TAppsSitaFormat
{
  long int required_fields(TPaxType pax, TAPIType api) const
  {
    if (api == apiDoc) return DOC_APPS_26_FIELDS;
    if (api == apiDoco) return DOCO_APPS_26_FIELDS;
    return NO_FIELDS;
  }
  bool CheckDocoIssueCountry(string issue_place);
};

//---------------------------------------------------------------------------------------

inline TAPISFormat* SpawnAPISFormat(const string& fmt)
{
  TAPISFormat* p = nullptr;
  if (fmt=="CSV_CZ")      p = new TAPISFormat_CSV_CZ; else
  if (fmt=="EDI_CZ")      p = new TAPISFormat_EDI_CZ; else
  if (fmt=="EDI_CN")      p = new TAPISFormat_EDI_CN; else
  if (fmt=="EDI_IN")      p = new TAPISFormat_EDI_IN; else
  if (fmt=="EDI_US")      p = new TAPISFormat_EDI_US; else
  if (fmt=="EDI_USBACK")  p = new TAPISFormat_EDI_USBACK; else
  if (fmt=="EDI_UK")      p = new TAPISFormat_EDI_UK; else
  if (fmt=="EDI_ES")      p = new TAPISFormat_EDI_ES; else
  if (fmt=="CSV_DE")      p = new TAPISFormat_CSV_DE; else
  if (fmt=="TXT_EE")      p = new TAPISFormat_TXT_EE; else
  if (fmt=="XML_TR")      p = new TAPISFormat_XML_TR; else
  if (fmt=="CSV_AE")      p = new TAPISFormat_CSV_AE; else
  if (fmt=="EDI_LT")      p = new TAPISFormat_EDI_LT; else
  if (fmt=="CSV_TH")      p = new TAPISFormat_CSV_TH; else
  if (fmt=="EDI_KR")      p = new TAPISFormat_EDI_KR; else
  if (fmt=="APPS_SITA")   p = new TAPISFormat_APPS_SITA; else // TODO remove
  if (fmt=="EDI_AZ")      p = new TAPISFormat_EDI_AZ; else
  if (fmt=="EDI_DE")      p = new TAPISFormat_EDI_DE; else
  if (fmt=="EDI_TR")      p = new TAPISFormat_EDI_TR; else

  if (fmt=="APPS_21")     p = new TAPPSVersion21; else
  if (fmt=="APPS_26")     p = new TAPPSVersion26;

  if (p == nullptr) throw Exception("SpawnAPISFormat: unhandled format %s", fmt.c_str());
  p->fmt = fmt;
  return p;
}

inline TAPISFormat* SpawnAPISFormat(const TApisSetsData& sd)
{
  TAPISFormat* p = SpawnAPISFormat(sd.fmt);
  p->edi_own_addr = sd.edi_own_addr;
  p->edi_addr = sd.edi_addr;
  p->transport_type = sd.transport_type;
  p->transport_params = sd.transport_params;
  return p;
}

#endif // APIS_CREATOR_H
