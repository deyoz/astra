#include "apis_settings.h"
#include "astra_utils.h"
#include "PgOraConfig.h"
#include "cache_access.h"
#include <boost/utility/in_place_factory.hpp>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

namespace APIS
{

const std::set<std::string>& allFormats()
{
  static boost::optional<std::set<std::string>> formats;

  if (!formats)
  {
    formats=boost::in_place();

    DB::TQuery ApisSetsQry(PgOra::getROSession("APIS_FORMATS"), STDLOG);
    ApisSetsQry.SQLText=
        "SELECT code AS format FROM apis_formats ORDER BY code";
    ApisSetsQry.Execute();
    for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
      formats.get().insert(ApisSetsQry.FieldAsString("format"));
  }

  return formats.get();
}

Settings& Settings::fromDB(DB::TQuery &Qry)
{
  clear();
  m_countryControl=Qry.FieldAsString("country_control");
  m_format=Qry.FieldAsString("format");
  m_ediAddrWithExt=Qry.FieldAsString("edi_addr");
  m_ediOwnAddrWithExt=Qry.FieldAsString("edi_own_addr");
  m_transportType=Qry.FieldAsString("transport_type");
  m_transportParams=Qry.FieldAsString("transport_params");
  return *this;
}

Settings& Settings::replaceFormat(const std::string& format)
{
  m_format=format;
  return *this;
}

std::string Settings::ediAddr() const
{
  size_t pos=m_ediAddrWithExt.find(':');
  return pos==string::npos?m_ediAddrWithExt:m_ediAddrWithExt.substr(0,pos);
}

std::string Settings::ediAddrExt() const
{
  size_t pos=m_ediAddrWithExt.find(':');
  return pos==string::npos?"":m_ediAddrWithExt.substr(pos+1);
}

std::string Settings::ediOwnAddr() const
{
  size_t pos=m_ediOwnAddrWithExt.find(':');
  return pos==string::npos?m_ediOwnAddrWithExt:m_ediOwnAddrWithExt.substr(0,pos);
}

std::string Settings::ediOwnAddrExt() const
{
  size_t pos=m_ediOwnAddrWithExt.find(':');
  return pos==string::npos?"":m_ediOwnAddrWithExt.substr(pos+1);
}

void SettingsList::add(const Settings& settings)
{
  emplace(settings, settings);
}

SettingsList& SettingsList::getByCountries(const std::string& airline,
                                           const std::string& countryDep,
                                           const std::string& countryArv)
{
  clear();

  DB::TQuery ApisSetsQry(PgOra::getROSession("APIS_SETS"), STDLOG);
  ApisSetsQry.SQLText=
    "SELECT apis_sets.*,"
    "       (CASE WHEN country_arv IS NOT NULL THEN 2 ELSE 0 END + "
    "        CASE WHEN country_dep IS NOT NULL THEN 1 ELSE 0 END) AS priority "
    "FROM apis_sets "
    "WHERE airline=:airline AND "
    "      (country_dep IS NULL AND :country_dep<>:country_arv OR country_dep=:country_dep) AND "
    "      (country_arv IS NULL AND :country_dep<>:country_arv OR country_arv=:country_arv) AND "
    "      country_control IN (country_dep, country_arv) AND "
    "      pr_denial=0 "
    "ORDER BY priority DESC";
  ApisSetsQry.CreateVariable("airline", otString, airline);
  ApisSetsQry.CreateVariable("country_dep", otString, countryDep);
  ApisSetsQry.CreateVariable("country_arv", otString, countryArv);
  ApisSetsQry.Execute();
  for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
    add(Settings().fromDB(ApisSetsQry));

  return *this;
}

SettingsList& SettingsList::getByAirps(const std::string& airline,
                                       const std::string& airpDep,
                                       const std::string& airpArv)
{
  getByCountries(airline,
                 getCountryByAirp(airpDep).code,
                 getCountryByAirp(airpArv).code);

  return *this;
}

SettingsList& SettingsList::getByAddrs(const std::string& ediAddr,
                                       const std::string& ediAddrExt,
                                       const std::string& ediOwnAddr,
                                       const std::string& ediOwnAddrExt)
{
  clear();

  DB::TQuery ApisSetsQry(PgOra::getROSession("APIS_SETS"), STDLOG);
  ApisSetsQry.SQLText=
    "SELECT apis_sets.* "
    "FROM apis_sets "
    "WHERE edi_addr LIKE :edi_addr||'%' AND "
    "      edi_own_addr LIKE :edi_own_addr||'%' AND "
    "      country_control IN (country_dep, country_arv) AND "
    "      pr_denial=0 "
    "ORDER BY id";
  ApisSetsQry.CreateVariable("edi_addr", otString, ediAddr);
  ApisSetsQry.CreateVariable("edi_own_addr", otString, ediOwnAddr);
  ApisSetsQry.Execute();
  for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
  {
    Settings settings;
    settings.fromDB(ApisSetsQry);
    if (settings.ediAddr()==ediAddr &&
        settings.ediOwnAddr()==ediOwnAddr &&
        (ediAddrExt.empty() || settings.ediAddrExt()==ediAddrExt) &&
        (ediOwnAddrExt.empty() || settings.ediOwnAddrExt()==ediOwnAddrExt))
      add(settings);
  }

  return *this;
}

SettingsList& SettingsList::getForTesting(const Settings& settingsPattern)
{
  clear();

  Settings settings(settingsPattern);
  for(const auto& f : allFormats())
    add(settings.replaceFormat(f));

  return *this;
}

SettingsList& SettingsList::filterFormatsFromList(const std::set<std::string>& formats)
{
  for(SettingsList::iterator i=begin(); i!=end();)
  {
    const Settings& settings=i->second;
    if (formats.find(settings.format())==formats.end())
      i=erase(i);
    else
      ++i;
  }

  return *this;
}

bool SettingsList::formatExists(const std::string& format) const
{
  for(const auto& i : *this)
    if (i.second.format()==format) return true;
  return false;
}

bool SettingsList::settingsExists(const Settings& settings) const
{
  APIS::SettingsList::const_iterator i=find(settings);
  if (i!=end() && i->second==settings) return true;
  return false;
}

const char* APIS_PARTY_INFO()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("APIS_PARTY_INFO","");
  return VAR.c_str();
}

void AirlineOfficeList::get(const std::string& airline,
                            const std::string& countryControl)
{
  clear();

  DB::TQuery Qry(PgOra::getROSession("AIRLINE_OFFICES"), STDLOG);
  Qry.SQLText=
    "SELECT contact_name, phone, fax "
    "FROM airline_offices "
    "WHERE airline=:airline AND country_control=:country_control AND to_apis<>0";
  Qry.CreateVariable("airline", otString, airline);
  Qry.CreateVariable("country_control", otString, countryControl);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    emplace_back(Qry.FieldAsString("contact_name"),
                 Qry.FieldAsString("phone"),
                 Qry.FieldAsString("fax"));

  vector<string> strs;
  SeparateString(string(APIS_PARTY_INFO()), ':', strs);
  if (!strs.empty())
  {
    string s1, s2, s3;
    vector<string>::const_iterator i;
    i=strs.begin();
    if (i!=strs.end()) s1=(*i++);
    if (i!=strs.end()) s2=(*i++);
    if (i!=strs.end()) s3=(*i++);
    emplace_back(s1, s2, s3);
  };
}

const map<string/*country_depend*/, string/*country_regul*/>& customs()
{
  static boost::optional<map<string, string>> customsMapByDepend;

  if (!customsMapByDepend)
  {
    customsMapByDepend=boost::in_place();

    DB::TQuery Qry(PgOra::getROSession("APIS_CUSTOMS"), STDLOG);
    Qry.SQLText="SELECT country_depend, country_regul FROM apis_customs";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
      customsMapByDepend.get().emplace(Qry.FieldAsString("country_depend"), Qry.FieldAsString("country_regul"));
  }

  return customsMapByDepend.get();
}

const set<string>& customsUS()
{
  static boost::optional<set<string>> countries;
  if (!countries)
  {
    countries=boost::in_place();
    getCustomsDependCountries("ûë", countries.get());
  }
  return countries.get();
}

void getCustomsDependCountries(const string &regul, set<string> &depend)
{
  depend.clear();
  depend.insert(regul);

  static boost::optional<multimap<string, string>> customsMapByRegul;

  if (!customsMapByRegul)
  {
    customsMapByRegul=boost::in_place();

    for(const auto& i : customs())
      customsMapByRegul.get().emplace(i.second, i.first);
  }

  auto range=customsMapByRegul.get().equal_range(regul);
  for(auto i=range.first; i!=range.second; ++i)
    depend.insert(i->second);
}

string getCustomsRegulCountry(const string &depend)
{
  auto i=customs().find(depend);
  if (i!=customs().end())
    return i->second;
  else
    return depend;
}

std::string checkAndNormalizeEdiAddr(const std::string& addr)
{
  if (addr.empty()) return "";

  std::string addrPart1(addr), addrPart2;

  string::size_type pos=addr.find(':');
  if (pos!=string::npos)
  {
    addrPart1=addr.substr(0,pos);
    addrPart2=addr.substr(pos+1);
  };

  TrimString(addrPart1);
  TrimString(addrPart2);

  if (addrPart1.empty())
    throw UserException("MSG.TLG.INVALID_ADDR",
                        LParams() << LParam("addr", addr));
  if (addrPart1.size()>35)
    throw UserException("MSG.TLG.INVALID_ADDR_LENGTH",
                        LParams() << LParam("addr", addrPart1));
  if (!isValidName(addrPart1, true, " "))
    throw UserException("MSG.TLG.INVALID_ADDR_CHARS",
                        LParams() << LParam("addr", addrPart1));

  if (!addrPart2.empty())
  {
    if (addrPart2.size()>4)
      throw UserException("MSG.TLG.INVALID_ADDR_LENGTH",
                          LParams() << LParam("addr", addrPart2));
    if (!IsLatinUpperLettersOrDigits(addrPart2))
      throw UserException("MSG.TLG.INVALID_ADDR_CHARS",
                          LParams() << LParam("addr", addrPart2));

    return addrPart1+":"+addrPart2;
  }

  return addrPart1;
}

void checkSetting(const AirlineCode_t& airline,
                  const std::optional<CountryCode_t>& countryDep,
                  const std::optional<CountryCode_t>& countryArv,
                  const CountryCode_t& countryControl,
                  const std::string& format,
                  std::string& ediAddr,
                  std::string& ediOwnAddr)
{
  if (!((countryDep && countryControl==countryDep.value()) ||
        (countryArv && countryControl==countryArv.value())))
    throw UserException("MSG.COUNTRY_DEP_OR_ARV_MUST_MATCH_COUNTRY_CONTROL");

  enum class FormatType { Edi, Iapi, Other };

  FormatType fmtType(FormatType::Other);
  if (format.substr(0,4)=="EDI_")  fmtType=FormatType::Edi;
  if (format.substr(0,5)=="IAPI_") fmtType=FormatType::Iapi;

  if (fmtType==FormatType::Other)
  {
    ediAddr.clear();
    ediOwnAddr.clear();
    return;
  }

  CountryCode_t countryRegulControl(getCustomsRegulCountry(countryControl.get()));

  if (countryRegulControl==CountryCode_t("ñç") && fmtType==FormatType::Edi) ediAddr="CNADAPIS:ZZ";
  if (countryRegulControl==CountryCode_t("ñç") && fmtType==FormatType::Iapi) ediAddr="NIAC";
  if (countryRegulControl==CountryCode_t("àç")) ediAddr="NZCS";
  if (countryRegulControl==CountryCode_t("ûë")) ediAddr="USCSAPIS:ZZ";
  if (countryRegulControl==CountryCode_t("ÉÅ")) ediAddr="UKBAOP:ZZ";

  ediAddr=checkAndNormalizeEdiAddr(ediAddr);
  ediOwnAddr=checkAndNormalizeEdiAddr(ediOwnAddr);

  if (ediAddr.empty())
  {
    try
    {
      const TCountriesRow& row=dynamic_cast<const TCountriesRow&>(base_tables.get("countries").get_row("code",countryControl.get()));

      std::string addrPart1(row.code_lat);
      if (addrPart1.empty()) addrPart1=row.code_iso;
      if (addrPart1.empty()) addrPart1=row.code;
      ediAddr=checkAndNormalizeEdiAddr(addrPart1+"APIS:ZZ");
    }
    catch(const std::exception&)
    {
      std::string fieldName=getLocaleText(getCacheInfo("APIS_SETS").fieldTitle.at("EDI_ADDR"));
      throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE",
                          LParams() << LParam("fieldname", fieldName));
    }
  }

  if (ediOwnAddr.empty())
  {
    try
    {
      const TAirlinesRow& row=dynamic_cast<const TAirlinesRow&>(base_tables.get("airlines").get_row("code",airline.get()));

      std::string addrPart1(row.short_name_lat);
      if (addrPart1.empty()) addrPart1=row.name_lat;
      if (addrPart1.empty()) addrPart1=row.short_name;
      if (addrPart1.empty()) addrPart1=row.name;
      addrPart1.erase(35);

      std::string addrPart2(row.code_lat);
      if (addrPart2.empty()) addrPart2=row.code_icao_lat;
      if (addrPart2.empty()) addrPart2=row.code;
      if (addrPart2.empty()) addrPart2=row.code_icao;

      ediOwnAddr=checkAndNormalizeEdiAddr(addrPart1+":"+addrPart2);

      if (countryRegulControl==CountryCode_t("ñç") && fmtType==FormatType::Iapi) ediOwnAddr=addrPart2;
    }
    catch(const std::exception&)
    {
      std::string fieldName=getLocaleText(getCacheInfo("APIS_SETS").fieldTitle.at("EDI_OWN_ADDR"));
      throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE",
                          LParams() << LParam("fieldname", fieldName));
    }
  }
}

} //namespace APIS

namespace CacheTable
{

//ApisFormats

bool ApisFormats::userDependence() const {
  return false;
}
std::string ApisFormats::selectSql() const {
  return "SELECT code, name, name_lat FROM apis_formats WHERE code NOT IN ('APPS_SITA') ORDER BY code";
}
std::list<std::string> ApisFormats::dbSessionObjectNames() const {
  return {"APIS_FORMATS"};
}

//ApisTransports

bool ApisTransports::userDependence() const {
  return false;
}
std::string ApisTransports::selectSql() const {
  return "SELECT code, name, name_lat FROM msg_transports WHERE code IN ('FILE', 'RABBIT_MQ') ORDER BY code";
}
std::list<std::string> ApisTransports::dbSessionObjectNames() const {
  return {"MSG_TRANSPORTS"};
}

//ApisSets

bool ApisSets::userDependence() const {
  return true;
}
std::string ApisSets::selectSql() const {
  return "SELECT id, airline, country_dep, country_arv, country_control, format, "
                "transport_type, transport_params, edi_addr, edi_own_addr, pr_denial "
         "FROM apis_sets "
         "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) +
         "ORDER BY airline, country_control, format, country_dep, country_arv";
}
std::string ApisSets::insertSql() const {
  return "INSERT INTO apis_sets(id, airline, country_dep, country_arv, country_control, format, "
           "transport_type, transport_params, edi_addr, edi_own_addr, pr_denial) "
         "VALUES(:id, :airline, :country_dep, :country_arv, :country_control, :format, "
           ":transport_type, :transport_params, :edi_addr, :edi_own_addr, :pr_denial)";
}
std::string ApisSets::updateSql() const {
  return "UPDATE apis_sets "
         "SET airline=:airline, country_dep=:country_dep, country_arv=:country_arv, "
             "country_control=:country_control, format=:format, "
             "transport_type=:transport_type, transport_params=:transport_params, "
             "edi_addr=:edi_addr, edi_own_addr=:edi_own_addr, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string ApisSets::deleteSql() const {
  return "DELETE FROM apis_sets WHERE id=:OLD_id";
}
std::list<std::string> ApisSets::dbSessionObjectNames() const {
  return {"APIS_SETS"};
}

void ApisSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  if (newRow)
  {
    CacheTable::Row &row=newRow.value();

    std::string countryDep=row.getAsString("country_dep");
    std::string countryArv=row.getAsString("country_arv");
    std::string ediAddr   =row.getAsString("edi_addr");
    std::string ediOwnAddr=row.getAsString("edi_own_addr");

    APIS::checkSetting(AirlineCode_t(row.getAsString_ThrowOnEmpty("airline")),
                       countryDep.empty()?std::nullopt:std::optional(CountryCode_t(countryDep)),
                       countryArv.empty()?std::nullopt:std::optional(CountryCode_t(countryArv)),
                       CountryCode_t(row.getAsString_ThrowOnEmpty("country_control")),
                       row.getAsString_ThrowOnEmpty("format"),
                       ediAddr,
                       ediOwnAddr);
    row.setFromString("edi_addr", ediAddr);
    row.setFromString("edi_own_addr", ediOwnAddr);
  }

  setRowId("id", status, newRow);
}

void ApisSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                       const std::optional<CacheTable::Row>& oldRow,
                                       const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("apis_sets").synchronize(getRowId("id", oldRow, newRow));
}

} //CacheTable