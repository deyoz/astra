#pragma once

#include <list>
#include <string>
#include <set>
#include "astra_consts.h"
#include "astra_types.h"
#include "brands.h"
#include "remarks.h"
#include <serverlib/cursctl.h>

namespace PaxConfirmations
{

class SettingsFilter
{
  private:
    mutable boost::optional<std::list<TBrand::Key>> brandKeyList;
    mutable boost::optional<std::set<CheckIn::TPaxFQTItem>> fqts;
    mutable boost::optional<std::multiset<CheckIn::TPaxRemItem>> rems;
    mutable boost::optional<std::set<std::string>> rfiscs;
  public:
    PaxId_t paxId;
    AirlineCode_t airline;
    AirportCode_t airpDep;
    Class_t cl;
    SubClass_t subcl;
    DCSAction::Enum dcsAction;

    SettingsFilter(const PaxId_t& paxId_,
                   const AirlineCode_t& airline_,
                   const AirportCode_t& airpDep_,
                   const Class_t& cl_,
                   const SubClass_t& subcl_,
                   const DCSAction::Enum dcsAction_) :
      paxId(paxId_),
      airline(airline_),
      airpDep(airpDep_),
      cl(cl_),
      subcl(subcl_),
      dcsAction(dcsAction_) {}

    bool suitable(const boost::optional<TBrand::Key>& brand, TBrands& brands) const;
    bool suitable(const boost::optional<TierLevelKey>& tierLevel,
                  const bool fqtShouldNotExists) const;
    bool suitableRfisc(const std::string& rfisc) const;
    bool suitableRemCode(const std::string& rem_code) const;
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
    std::string rfisc;
    std::string rem_code;
    boost::optional<TBrand::Key> brand;
    boost::optional<TierLevelKey> tierLevel;
    bool fqtShouldNotExists=false;
    std::string text;
    std::string text_lat;

    bool operator < (const Setting &setting) const
    {
      return id<setting.id;
    }

    static const std::string& selectedFields()
    {
      static const std::string result=" id, rfisc, rem_code, brand_airline, brand_code, fqt_airline, fqt_tier_level, text, text_lat ";
      return result;
    }

    static void curDef(OciCpp::CursCtl& cur, Setting& setting)
    {
      cur.def(setting.id)
         .defNull(setting.rfisc, "")
         .defNull(setting.rem_code, "")
         .defNull(setting.brand_airline, "")
         .defNull(setting.brand_code, "")
         .defNull(setting.fqt_airline, "")
         .defNull(setting.fqt_tier_level, "")
         .def(setting.text)
         .def(setting.text_lat);
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
typedef std::map<PaxId_t, Settings> SettingsByPaxId;

class Segment
{
  public:
    TTripInfo flt;
    CheckIn::TSimplePaxGrpItem grp;
    CheckIn::TSimplePaxList paxs;
};

typedef std::list<Segment> Segments;

class AppliedMessage
{
  public:
    int id;
    std::string text;

    AppliedMessage(const int id_, const std::string& text_) :
      id(id_), text(text_) {}

    bool operator < (const AppliedMessage &message) const
    {
      return id<message.id;
    }
};

class AppliedMessages
{
  private:
    std::map<PaxId_t, std::set<AppliedMessage>> messages;
    void get(int id, bool isGrpId);
    void get(xmlNodePtr node);
  public:
    AppliedMessages(const PaxId_t& paxId);
    AppliedMessages(const GrpId_t& grpId);
    AppliedMessages(xmlNodePtr node);
    bool exists(const PaxId_t& paxId, const int id) const;
    void toDB() const;
    void toLog(const Segments& segs) const;

    static bool exists(xmlNodePtr node);
};

class Messages
{
  private:
    TBrands brands;
    std::list<std::pair<SettingsFilter, Settings>> settingsCache;
    Segments segments;
    SettingsByPaxId settingsByPaxId;
    const Settings& filterRoughly(const SettingsFilter& filter);
  public:
    void add(const SettingsFilter& filter, const boost::optional<AppliedMessages>& messages);
    bool toXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;

    Messages(const DCSAction::Enum dcsAction, const Segments& segs);

    static void replyInfoToXML(xmlNodePtr messageNode,
                               const PaxId_t& paxId,
                               const Settings& settings,
                               const AstraLocale::OutputLang &lang);

    static std::string getText(const CheckIn::TSimplePaxItem& pax,
                               const TTripInfo& flt,
                               const bool showFlight,
                               const Settings& settings,
                               const AstraLocale::OutputLang &lang);
};

} //namespace PaxConfirmations
