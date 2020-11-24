#include "dcs_services.h"
#include "qrys.h"
#include "passenger.h"
#include "brands.h"
#include "rfisc.h"
#include "astra_locale_adv.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;

namespace DCSServiceApplying
{

std::ostream& operator<<(std::ostream& os, const SettingsFilter& filter)
{
  os << endl << "  airline:        " << filter.airline
     << endl << "  dcsAction:      " << dcsActions().encode(filter.dcsAction)
     << endl << "  cl:             " << filter.cl;

  return os;
}

std::ostream& operator<<(std::ostream& os, const Setting& setting)
{
  os << "  id: " << setting.id;
  os << ", brand: ";
  if (setting.brand)
    os << setting.brand.get();
  os << ", tierLevel: ";
  if (setting.tierLevel)
    os << setting.tierLevel.get();
  os << ", fqtShouldNotExists: " << boolalpha << setting.fqtShouldNotExists
     << ", rfisc: " << setting.rfisc;

  return os;
}

std::ostream& operator<<(std::ostream& os, const Settings& settings)
{
  for(const auto& i : settings)
    os << endl << i;

  return os;
}

bool SettingsFilter::suitable(const boost::optional<TBrand::Key>& brand, TBrands& brands) const //!!!в PaxConfirmations::SettingsFilter метод один в один
{
  if (!brand) return true;

  if (!brandKeyList)
  {
    brands.get(paxId.get());

    brandKeyList=boost::in_place();
    brandKeyList.get()=algo::transform<std::list<TBrand::Key>>(brands, [](const auto& i) { return i.key(); } );
  }

  return algo::any_of(brandKeyList.get(), [&brand](const auto& i) { return i==brand.get(); } );
}

bool SettingsFilter::suitable(const boost::optional<TierLevelKey>& tierLevel,                   //!!!в PaxConfirmations::SettingsFilter метод один в один
                              const bool fqtShouldNotExists) const
{
  if (!tierLevel && !fqtShouldNotExists) return true;

  if (!fqts)
  {
    fqts=boost::in_place();
    LoadPaxFQT(paxId.get(), fqts.get());
  }

  if (fqtShouldNotExists) return fqts.get().empty();

  return algo::any_of(fqts.get(), [&tierLevel](const auto& i) { return i.tierLevelKey()==tierLevel; } );
}

const Settings& RequiredRfiscs::filterRoughly(const SettingsFilter& filter)
{
  auto iSettings=std::find_if(settingsCache.begin(), settingsCache.end(),
                              [&filter](const auto& i) {  return filter.airline==i.first.airline &&
                                                                 filter.dcsAction==i.first.dcsAction &&
                                                                 filter.cl==i.first.cl; } );

  if (iSettings!=settingsCache.end()) return iSettings->second;


  iSettings=settingsCache.emplace(settingsCache.end(), filter, Settings());

  Settings& settings=iSettings->second;

  LogTrace(TRACE5) << __func__ << ": " << filter;

  auto cur = make_curs(
    "SELECT " + Setting::selectedFields() +
    "FROM dcs_service_applying "
    "WHERE airline=:airline AND "
    "      dcs_service=:dcs_service AND "
    "      (class IS NULL OR class=:class) AND "
    "      pr_denial=0 ");



  Setting setting;

  Setting::curDef(cur, setting);

  cur.bind(":airline", filter.airline.get())
     .bind(":dcs_service", dcsActions().encode(filter.dcsAction))
     .bind(":class", filter.cl.get())
     .exec();

  while(!cur.fen())
  {
    settings.insert(setting.afterFetchProcessing());
  }

  if (!settings.empty())
    LogTrace(TRACE5) << __func__ << ": " << settings;

  return settings;
}

void RequiredRfiscs::add(const SettingsFilter& filter)
{
  const Settings& settingsRoughly=filterRoughly(filter);

  for(const Setting& setting : settingsRoughly)
  {
    if (filter.suitable(setting.brand, brands) &&
        filter.suitable(setting.tierLevel, setting.fqtShouldNotExists))
    {
      settingsByPaxId.second.insert(setting);
    }
  }
}

RequiredRfiscs::RequiredRfiscs(const DCSAction::Enum dcsAction_,
                               const PaxId_t& paxId) :
  settingsByPaxId(paxId, Settings()),
  dcsAction(dcsAction_),
  notRequiredAtAll(false)
{
  LogTrace(TRACE5) << __func__ << ": dcsAction=" << dcsActions().encode(dcsAction)
                               << ", paxId=" << paxId;

  if ( TReqInfo::Instance()->client_type != ASTRA::ctTerm )
  {
    notRequiredAtAll=true;
    return;// только с терминала
  }

  CheckIn::TSimplePaxItem pax;
  if (!pax.getByPaxId(paxId.get())) return;

  CheckIn::TSimplePaxGrpItem grp;
  if (!grp.getByGrpId(pax.grp_id)) return;

  if (grp.grpCategory()!=CheckIn::TPaxGrpCategory::Passenges)
  {
    notRequiredAtAll=true;
    return;
  }

  TTripInfo flt;
  if (!flt.getByPointId(grp.point_dep)) return;

  add(SettingsFilter(paxId,
                     AirlineCode_t(flt.airline),
                     dcsAction,
                     Class_t(grp.cl)));
}

RfiscsSet RequiredRfiscs::get() const
{
  RfiscsSet result;
  for(const Setting& setting : settingsByPaxId.second)
    result.insert(setting.rfisc);
  return result;
}

bool RequiredRfiscs::exists() const
{
  if (notRequiredAtAll) return true;

  if (settingsCache.empty()) return false;

  RfiscsSet reqRFISCs=get();

  if (reqRFISCs.empty()) return true;  //не нашли по компании/операции ни одного требуемого RFISCа

  RfiscsSet paxRFISCs, intersectRFISCs;

  TPaidRFISCListWithAuto paid;
  paid.fromDB(settingsByPaxId.first.get(), false);
  paid.getUniqRFISCSs(settingsByPaxId.first.get(), paxRFISCs);

  set_intersection(reqRFISCs.begin(), reqRFISCs.end(),
                   paxRFISCs.begin(), paxRFISCs.end(),
                   inserter(intersectRFISCs, intersectRFISCs.end()));

  LogTrace(TRACE5) << __func__ << ": reqRFISCs: " << LogCont(" ", reqRFISCs);
  LogTrace(TRACE5) << __func__ << ": paxRFISCs: " << LogCont(" ", paxRFISCs);
  LogTrace(TRACE5) << __func__ << ": intersectRFISCs: " << LogCont(" ", intersectRFISCs);

  return !intersectRFISCs.empty();
}

void RequiredRfiscs::throwIfNotExists() const
{
  if (exists()) return;

  RfiscsSet requiredRFISCs=get();

  LexemaData lexemeData;

  lexemeData.lparams << LParam("service", ElemIdToNameLong(etDCSAction, dcsActions().encode(dcsAction)));

  if (!requiredRFISCs.empty())
  {
    ostringstream s;
    for(RfiscsSet::const_iterator i=requiredRFISCs.begin(); i!=requiredRFISCs.end(); ++i)
    {
      if (i!=requiredRFISCs.begin()) s << ", ";
      s << *i;
    }
    lexemeData.lparams << LParam("rfiscs", s.str());

    if (requiredRFISCs.size()==1)
      lexemeData.lexema_id="MSG.DCS_SERVICE.NOT_AVAIL_RFISC_REQUIRED";
    else
      lexemeData.lexema_id="MSG.DCS_SERVICE.NOT_AVAIL_RFISCS_REQUIRED";
  }
  else lexemeData.lexema_id="MSG.DCS_SERVICE.NOT_AVAIL";

  throw UserException(lexemeData.lexema_id, lexemeData.lparams);
}

} //namespace DCSServiceApplying

