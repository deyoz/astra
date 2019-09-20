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
    "SELECT code FROM apis_formats ORDER BY code";
  ApisSetsQry.Execute();
  for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
    add(settings.replaceFormat(ApisSetsQry));
}

bool SettingsList::formatExists(const std::string& format) const
{
  for(const auto& i : *this)
    if (i.second.format()==format) return true;
  return false;
}

}
