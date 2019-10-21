#ifndef _APIS_SETTINGS_H_
#define _APIS_SETTINGS_H_

#include <set>
#include <string>
#include "oralib.h"

namespace APIS
{

const std::set<std::string>& allFormats();

class SettingsKey
{
  protected:
    std::string m_countryControl;
    std::string m_format;
  public:
    SettingsKey() {}
    SettingsKey(const std::string& countryControl,
                const std::string& format) :
      m_countryControl(countryControl),
      m_format(format) {}

    void clear()
    {
      m_countryControl.clear();
      m_format.clear();
    }

    bool operator < (const SettingsKey &key) const
    {
      if (m_countryControl!=key.m_countryControl)
        return m_countryControl<key.m_countryControl;
      return m_format<key.m_format;
    }

    bool operator == (const SettingsKey &key) const
    {
      return m_countryControl==key.m_countryControl &&
             m_format==key.m_format;
    }

    const std::string& countryControl() const { return m_countryControl; }
    const std::string& format() const { return m_format; }

    std::string traceStr() const
    {
      return m_countryControl + "+" + m_format;
    }
};

class Settings : public SettingsKey
{
  protected:
    std::string m_ediAddrWithExt, m_ediOwnAddrWithExt;
    std::string m_transportType, m_transportParams;
  public:
    Settings() {}
    Settings(const std::string& countryControl,
             const std::string& format,
             const std::string& ediAddrWithExt,
             const std::string& ediOwnAddrWithExt,
             const std::string& transportType,
             const std::string& transportParams) :
      SettingsKey(countryControl, format),
      m_ediAddrWithExt(ediAddrWithExt),
      m_ediOwnAddrWithExt(ediOwnAddrWithExt),
      m_transportType(transportType),
      m_transportParams(transportParams) {}

    Settings& fromDB(TQuery &Qry);
    Settings& replaceFormat(const std::string& format);

    void clear()
    {
      SettingsKey::clear();
      m_ediAddrWithExt.clear();
      m_ediOwnAddrWithExt.clear();
      m_transportType.clear();
      m_transportParams.clear();
    }

    bool operator == (const Settings &item) const
    {
      return SettingsKey::operator ==(item) &&
             m_ediAddrWithExt == m_ediAddrWithExt &&
             m_ediOwnAddrWithExt == m_ediOwnAddrWithExt &&
             m_transportType == m_transportType &&
             m_transportParams == m_transportParams;
    }

    std::string ediAddr() const;
    std::string ediAddrExt() const;
    std::string ediOwnAddr() const;
    std::string ediOwnAddrExt() const;

    const std::string& ediAddrWithExt() const { return m_ediAddrWithExt; }
    const std::string& ediOwnAddrWithExt() const { return m_ediOwnAddrWithExt; }
    const std::string& transportType() const { return m_transportType; }
    const std::string& transportParams() const { return m_transportParams; }
};

class SettingsList : public std::map<SettingsKey, Settings>
{
  public:
    void add(const Settings& settings);

    SettingsList& getByCountries(const std::string& airline,
                                 const std::string& countryDep,
                                 const std::string& countryArv);
    SettingsList& getByAirps(const std::string& airline,
                             const std::string& airpDep,
                             const std::string& airpArv);
    SettingsList& getByAddrs(const std::string& ediAddr,
                             const std::string& ediAddrExt,
                             const std::string& ediOwnAddr,
                             const std::string& ediOwnAddrExt);
    SettingsList& getForTesting(const Settings& settingsPattern);

    SettingsList& filterFormatsFromList(const std::set<std::string>& formats);

    bool formatExists(const std::string& format) const;
    bool settingsExists(const Settings& settings) const;
};

class AirlineOfficeInfo
{
  protected:
    std::string m_contactName;
    std::string m_phone;
    std::string m_fax;
  public:
    AirlineOfficeInfo(const std::string& contactName,
                      const std::string& phone,
                      const std::string& fax) :
      m_contactName(contactName),
      m_phone(phone),
      m_fax(fax) {}

    const std::string& contactName() const { return m_contactName; }
    const std::string& phone() const { return m_phone; }
    const std::string& fax() const { return m_fax; }
};

class AirlineOfficeList : public std::list<AirlineOfficeInfo>
{
  public:
    void get(const std::string& airline,
             const std::string& countryControl);
};

const std::set<std::string> &customsUS();
void getCustomsDependCountries(const std::string &regul, std::set<std::string> &depend);
std::string getCustomsRegulCountry(const std::string &depend);

} //namespace APIS

#endif

