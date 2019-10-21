#include "apis_settings.h"
#include "astra_utils.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"


using namespace ASTRA;
using namespace std;

namespace APIS
{

const std::set<std::string>& allFormats()
{
  static boost::optional<std::set<std::string>> formats;

  if (!formats)
  {
    formats=boost::in_place();

    TQuery ApisSetsQry(&OraSession);
    ApisSetsQry.SQLText=
        "SELECT code AS format FROM apis_formats ORDER BY code";
    ApisSetsQry.Execute();
    for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
      formats.get().insert(ApisSetsQry.FieldAsString("format"));
  }

  return formats.get();
}

Settings& Settings::fromDB(TQuery &Qry)
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

  TQuery ApisSetsQry(&OraSession);
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

  TQuery ApisSetsQry(&OraSession);
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

  TQuery Qry(&OraSession);
  Qry.Clear();
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

    TQuery Qry(&OraSession);
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
    getCustomsDependCountries("ž‘", countries.get());
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

} //namespace APIS
