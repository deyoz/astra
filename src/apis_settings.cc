#include "apis_settings.h"
#include "astra_utils.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"


using namespace ASTRA;
using namespace std;

namespace APIS
{

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

Settings& Settings::replaceFormat(TQuery &Qry)
{
  m_format=Qry.FieldAsString("format");
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

void SettingsList::getByCountries(const std::string& airline,
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
    "      (country_dep IS NULL OR country_dep=:country_dep) AND "
    "      (country_arv IS NULL OR country_arv=:country_arv) AND "
    "      country_control IN (country_dep, country_arv) AND "
    "      pr_denial=0 "
    "ORDER BY priority DESC";
  ApisSetsQry.CreateVariable("airline", otString, airline);
  ApisSetsQry.CreateVariable("country_dep", otString, countryDep);
  ApisSetsQry.CreateVariable("country_arv", otString, countryArv);
  ApisSetsQry.Execute();
  for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
    add(Settings().fromDB(ApisSetsQry));
}

void SettingsList::getByAirps(const std::string& airline,
                              const std::string& airpDep,
                              const std::string& airpArv)
{
  getByCountries(airline,
                 getCountryByAirp(airpDep).code,
                 getCountryByAirp(airpArv).code);
}

void SettingsList::getByAddrs(const std::string& ediAddr,
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
}

void SettingsList::getForTesting(const Settings& settingsPattern)
{
  clear();

  Settings settings(settingsPattern);
  TQuery ApisSetsQry(&OraSession);
  ApisSetsQry.SQLText=
    "SELECT code AS format FROM apis_formats ORDER BY code";
  ApisSetsQry.Execute();
  for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
    add(settings.replaceFormat(ApisSetsQry));
}

void SettingsList::filterFormatsFromList(const std::set<std::string>& formats)
{
  for(SettingsList::iterator i=begin(); i!=end();)
  {
    const Settings& settings=i->second;
    if (formats.find(settings.format())==formats.end())
      i=erase(i);
    else
      ++i;
  }
}

bool SettingsList::formatExists(const std::string& format) const
{
  for(const auto& i : *this)
    if (i.second.format()==format) return true;
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

const set<string> &customsUS()
{
  static bool init=false;
  static set<string> depend;
  if (!init)
  {
    TQuery Qry(&OraSession);
    GetCustomsDependCountries("ž‘", depend, Qry);
    init=true;
  };
  return depend;
}

void GetCustomsDependCountries(const string &regul,
                               set<string> &depend,
                               TQuery &Qry)
{
  depend.clear();
  depend.insert(regul);
  const char *sql =
    "SELECT country_depend FROM apis_customs WHERE country_regul=:country_regul";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_regul",otString);
  };
  Qry.SetVariable("country_regul", regul);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    depend.insert(Qry.FieldAsString("country_depend"));
}

string GetCustomsRegulCountry(const string &depend,
                              TQuery &Qry)
{
  const char *sql =
    "SELECT country_regul FROM apis_customs WHERE country_depend=:country_depend";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_depend",otString);
  };
  Qry.SetVariable("country_depend", depend);
  Qry.Execute();
  if (!Qry.Eof)
    return Qry.FieldAsString("country_regul");
  else
    return depend;
}

} //namespace APIS
