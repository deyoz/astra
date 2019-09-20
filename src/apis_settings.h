#ifndef _APIS_SETTINGS_H_
#define _APIS_SETTINGS_H_

#include <string>
#include "oralib.h"

namespace APIS
{

class SettingsKey
{
  protected:
    std::string m_countryControl;
    std::string m_format;
  public:
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

    const std::string& countryControl() const { return m_countryControl; }
    const std::string& format() const { return m_format; }
};

class Settings : public SettingsKey
{
  protected:
    std::string m_ediAddrWithExt, m_ediOwnAddrWithExt;
    std::string m_transportType, m_transportParams;
  public:
    Settings() {}
    Settings& fromDB(TQuery &Qry);

    void clear()
    {
      SettingsKey::clear();
      m_ediAddrWithExt.clear();
      m_ediOwnAddrWithExt.clear();
      m_transportType.clear();
      m_transportParams.clear();
    }

    std::string ediAddr() const;
    std::string ediAddrExt() const;
    std::string ediOwnAddr() const;
    std::string ediOwnAddrExt() const;

    const std::string& transportType() const { return m_transportType; }
    const std::string& transportParams() const { return m_transportParams; }
};

class SettingsList : public std::map<SettingsKey, Settings>
{
  public:
    void add(const Settings& settings);

    void getByCountries(const std::string& airline,
                        const std::string& countryDep,
                        const std::string& countryArv);
    void getByAirps(const std::string& airline,
                    const std::string& airpDep,
                    const std::string& airpArv);
    void getByAddrs(const std::string& ediAddr,
                    const std::string& ediAddrExt,
                    const std::string& ediOwnAddr,
                    const std::string& ediOwnAddrExt);
    bool formatExists(const std::string& format) const;
};

}


#endif

