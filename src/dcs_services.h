#pragma once

#include <list>
#include <string>
#include <set>
#include "astra_consts.h"
#include "astra_types.h"
#include "brands.h"
#include <serverlib/cursctl.h>

namespace DCSServiceApplying
{

class SettingsFilter
{
  private:
    mutable boost::optional<std::list<TBrand::Key>> brandKeyList;
    mutable boost::optional<std::set<CheckIn::TPaxFQTItem>> fqts;
  public:
    PaxId_t paxId;
    AirlineCode_t airline;
    DCSAction::Enum dcsAction;
    Class_t cl;

    SettingsFilter(const PaxId_t& paxId_,
                   const AirlineCode_t& airline_,
                   const DCSAction::Enum dcsAction_,
                   const Class_t& cl_) :
      paxId(paxId_),
      airline(airline_),
      dcsAction(dcsAction_),
      cl(cl_) {}

    bool suitable(const boost::optional<TBrand::Key>& brand, TBrands& brands) const;
    bool suitable(const boost::optional<TierLevelKey>& tierLevel,
                  const bool fqtShouldNotExists) const;
};

class Setting
{
  private:
    AirlineCode_t::base_type brand_airline;
    std::string brand_code;
    AirlineCode_t::base_type fqt_airline;
    std::string fqt_tier_level;
  public:
    int id;
    boost::optional<TBrand::Key> brand;
    boost::optional<TierLevelKey> tierLevel;
    bool fqtShouldNotExists=false;
    std::string rfisc;

    bool operator < (const Setting &setting) const
    {
      return id<setting.id;
    }

    static const std::string& selectedFields()
    {
      static const std::string result=" id, brand_airline, brand_code, fqt_airline, fqt_tier_level, rfisc ";
      return result;
    }

    static void curDef(OciCpp::CursCtl& cur, Setting& setting)
    {
      cur.def(setting.id)
         .defNull(setting.brand_airline, "")
         .defNull(setting.brand_code, "")
         .defNull(setting.fqt_airline, "")
         .defNull(setting.fqt_tier_level, "")
         .def(setting.rfisc);
    }

    Setting& afterFetchProcessing()
    {
      if (!brand_airline.empty() && !brand_code.empty())
        brand=boost::in_place(AirlineCode_t(brand_airline), brand_code);
      else
        brand=boost::none;

      if (!fqt_airline.empty() && !fqt_tier_level.empty())
        tierLevel=boost::in_place(AirlineCode_t(fqt_airline), fqt_tier_level);
      else
        tierLevel=boost::none;

      if (fqt_airline.empty() && !fqt_tier_level.empty())
        fqtShouldNotExists=true;
      else
        fqtShouldNotExists=false;

      return *this;
    }

};

typedef std::set<Setting> Settings;
typedef std::pair<PaxId_t, Settings> SettingsByPaxId;
typedef std::set<std::string> RfiscsSet;

class RequiredRfiscs
{
  private:
    TBrands brands;
    std::list<std::pair<SettingsFilter, Settings>> settingsCache;
    SettingsByPaxId settingsByPaxId;
    DCSAction::Enum dcsAction;
    bool notRequiredAtAll;
    const Settings& filterRoughly(const SettingsFilter& filter);
    void add(const SettingsFilter& filter);
    RfiscsSet get() const;
  public:
    RequiredRfiscs(const DCSAction::Enum dcsAction_,
                   const PaxId_t& paxId);
    bool exists() const;
    void throwIfNotExists() const;
};

} //namespace DCSServiceApplying


