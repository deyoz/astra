#include "astra_ssim.h"

#define NICKNAME "GRISHA"
#define NICKTRACE GRISHA_TRACE
#include <serverlib/slogger.h>

#include <typeb/typeb_template.h>
#include <typeb/SSM_template.h>
#include <typeb/ASM_template.h>
using namespace typeb_parser;

#include <libssim/ssm_data_types.h> // parseSsm
#include <functional>
#include <base_tables.h>

#include "astra_main.h"
#include "astra_misc.h"

#include "date_time.h"
using namespace BASIC::date_time;
using namespace std;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace SEASON;

ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd );


int CodeToId(const std::string &code)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("CodeToId invalid sizeof");
  if (code.size() > 3) throw EXCEPTIONS::Exception("CodeToId invalid code size");
  const char *c = code.c_str();
  int id = 0;
  char* p = (char*)&id;
  for (unsigned int i = 0; i < code.size(); ++i) p[i] = c[i];
//  LogTrace(TRACE5) << __func__ << " code=\"" << code << "\" id=" << id;
  return id;
}

// ЗАМЕЧАНИЕ: в случае id = 10 получается code = символу перевода строки
std::string IdToCode(const int id)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("IdToCode invalid sizeof");
  const char* p = (const char*)&id;
  char code[4];
  for (unsigned int i = 0; i < 3; ++i) code[i] = p[i];
  code[3] = '\0';
  std::string code_str(code);
//  LogTrace(TRACE5) << __func__ << " id=" << id << " code=\"" << code_str << "\"";
  return code_str;
}

//------------------------------------------------------------------------------------------

// 1. Реализовать конвертацию системо-специфичного представления расписания
// в/из ssim::ScdPeriod (см. convertToSsim/makeScdPeriod)

//------------------------------------------------------------------------------------------

// 2. Реализовать ssim::SsimTlgCallbacks
// необходимо лишь SsimTlgCallbacks::getSchedules (все периоды
// расписания на рейс, пересекающиейся с указанным периодом)  и заглушки на
// все остальное.

// getSchedules надо реализовать
ssim::ScdPeriods AstraSsimCallbacks::getSchedules(const ct::Flight& flight, const Period& period) const
{
//  LogTrace(TRACE5) << __func__ << " flight=" << flight << " period=" << period;
  return ScdPeriodsFromDb( flight, period );
}

// остальное заглушки
ssim::ScdPeriods AstraSsimCallbacks::getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const
{
//  LogTrace(TRACE5) << __func__;
  return ssim::ScdPeriods();
}

Expected< boost::optional<ssim::CshSettings> > AstraSsimCallbacks::cshSettingsByTlg(nsi::CompanyId, ssim::ScdPeriod&) const
{
//  LogTrace(TRACE5) << __func__;
  return boost::optional<ssim::CshSettings>();
}

Expected< boost::optional<ssim::CshSettings> > AstraSsimCallbacks::cshSettingsByScd(const ssim::ScdPeriod&) const
{
//  LogTrace(TRACE5) << __func__;
  return boost::optional<ssim::CshSettings>();
}

ssim::DefValueSetter AstraSsimCallbacks::prepareDefaultValueSetter(ct::DeiCode, const nsi::DepArrPoints&, bool byLeg) const
{
//  LogTrace(TRACE5) << __func__;
  return ssim::DefValueSetter();
}

//------------------------------------------------------------------------------------------

// 3. Реализовать ф-ции вывода требуемых типов в Message::bind (см.
// sirena/src/message.cc)

// скопировано из Сирены
namespace message_details {

template<> std::string translate_impl<nsi::CompanyId>(const UserLanguage& lang, const nsi::CompanyId& v)
{
//  LogTrace(TRACE5) << __func__;
  return nsi::Company(v).code(ENGLISH).toUtf();
}

template<> std::string translate_impl<boost::gregorian::date>(const UserLanguage& lang, const boost::gregorian::date& d)
{
//  LogTrace(TRACE5) << __func__;
  return Dates::ddmmrr(d);
}

template<> std::string translate_impl<Period>(const UserLanguage& lang, const Period& v)
{
//  LogTrace(TRACE5) << __func__;
  return translate_impl(lang, v.start) + "-" + translate_impl(lang, v.end) + " (" + translate_impl(lang, v.freq) + ")";
}

template<> std::string translate_impl<Freq>(const UserLanguage&, const Freq& v)
{
//  LogTrace(TRACE5) << __func__;
  return v.str();
}

template<> std::string translate_impl<ct::Flight>(const UserLanguage&, const ct::Flight& v)
{
//  LogTrace(TRACE5) << __func__;
  return nsi::Company(v.airline).code(ENGLISH).toUtf()
       + '-' + StrUtils::LPad(std::to_string(v.number.get()), 3, '0')
       + ct::suffixToString(v.suffix, ENGLISH);
}

} // message_details

//------------------------------------------------------------------------------------------

// 4. При необходимости локализации и/или дополнительного преобразования
// сообщений об ошибках реализовать message_details::Localization.

//------------------------------------------------------------------------------------------

// это относится к реализации nsi::Callbacks (города/порты и т.п.)
// См. sirena/src/nsi_callbacks.(h + cc)
// В принципе можно для целей SSIM ограничиться заглушками/ассертами для:
// nsi::DocTypeId
// nsi::SsrTypeId
// nsi::GeozoneId
// nsi::RouterId
// nsi::CurrencyId
// nsi::OrganizationId
// и соответсвующих им *Data.
// А остальные ф-ции придется реализовать.
// Инициализация опять же однократная на старте, см. nsi::init_callbacks (sirena/src/nsi_callbacks.cc + obrzap.cc)

// Тестовая реализация nsi::Callbacks уже есть в libnsi/src/callbacks.cc (nsi::TestCallbacks).

bool AstraCallbacks::needCheckVersion() const
{
//  LogTrace(TRACE5) << __func__;
  return false;
}

nsi::CityId AstraCallbacks::centerCity() const
{
//  LogTrace(TRACE5) << __func__;
  return nsi::CityId(255); // TODO получать реальный город
}

// /home/user/Sirena/sirenalibs/libnsi/src/nsi.h

// заглушки
boost::optional<nsi::DocTypeId> AstraCallbacks::findDocTypeId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::DocTypeId>();
}

boost::optional<DocTypeData> AstraCallbacks::findDocTypeData(const nsi::DocTypeId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit DocTypeData(const DocTypeId& id);
  return DocTypeData(id);
}

// заглушки
boost::optional<nsi::SsrTypeId> AstraCallbacks::findSsrTypeId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::SsrTypeId>();
}

boost::optional<SsrTypeData> AstraCallbacks::findSsrTypeData(const nsi::SsrTypeId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit SsrTypeData(const SsrTypeId&);
  return SsrTypeData(id);
}

// заглушки
boost::optional<nsi::GeozoneId> AstraCallbacks::findGeozoneId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::GeozoneId>();
}

boost::optional<GeozoneData> AstraCallbacks::findGeozoneData(const nsi::GeozoneId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit GeozoneData(const GeozoneId&);
  return GeozoneData(id);
}

// country !!! done
boost::optional<nsi::CountryId> AstraCallbacks::findCountryId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CountryId(CodeToId(code.to866()));
}

boost::optional<nsi::CountryId> AstraCallbacks::findCountryIdByIso(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CountryId(CodeToId(code.to866())); // ???
}

boost::optional<CountryData> AstraCallbacks::findCountryData(const nsi::CountryId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit CountryData(const CountryId&, const GeozoneId&, const CurrencyId&);
  CountryData data(id, GeozoneId(0), CurrencyId(0));
  const TCountriesRow &countryRow = (const TCountriesRow&)base_tables.get("countries").get_row("code/code_lat", IdToCode(id.get()));
  data.isoCode = countryRow.code_iso;
  data.codes[ENGLISH] = EncString::from866(countryRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(countryRow.code);
  data.names[ENGLISH] = EncString::from866(countryRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(countryRow.name);
  return data;
}

// Для обработки SSM не нужно.
// region !!! отсутствует в base_tables
boost::optional<nsi::RegionId> AstraCallbacks::findRegionId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code = " << code.to866();
  return nsi::RegionId(CodeToId(code.to866()));
}

boost::optional<RegionData> AstraCallbacks::findRegionData(const nsi::RegionId& id)
{
  LogTrace(TRACE5) << __func__ << " id = " << id.get();
  // explicit RegionData(const RegionId&, const CountryId&);
  RegionData data(id, CountryId(0));
  return data;
}

// city !!! done
boost::optional<nsi::CityId> AstraCallbacks::findCityId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CityId(CodeToId(code.to866()));
}

boost::optional<CityData> AstraCallbacks::findCityData(const nsi::CityId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // CityData(const CityId&, const CountryId&, const boost::optional<RegionId>&);
  const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code/code_lat", IdToCode(id.get()));
  CityData data(id, CountryId(CodeToId(cityRow.country)), RegionId(CodeToId(cityRow.tz_region))); // уточнить tz_region
  data.codes[ENGLISH] = EncString::from866(cityRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(cityRow.code);
  data.names[ENGLISH] = EncString::from866(cityRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(cityRow.name);
  return data;
}

// point !!! отсутствует в base_tables !!! done
// TODO уточнить про аэропорт и город
boost::optional<nsi::PointId> AstraCallbacks::findPointId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::PointId(CodeToId(code.to866()));
}

boost::optional<PointData> AstraCallbacks::findPointData(const nsi::PointId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  string code_en, code_ru, name_en, name_ru;
  int city_id;
  const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code/code_lat", IdToCode(id.get()));
  if (airpsRow.code != "")
  {
    // аэропорт
    code_en = airpsRow.code_lat;
    code_ru = airpsRow.code;
    name_en = airpsRow.name_lat;
    name_ru = airpsRow.name;
    city_id = ((const TCitiesRow&)base_tables.get("cities").get_row("code/code_lat", airpsRow.city)).id;
  }
  else
  {
    // город
    const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code/code_lat", IdToCode(id.get()));
    code_en = cityRow.code_lat;
    code_ru = cityRow.code;
    name_en = cityRow.name_lat;
    name_ru = cityRow.name;
    city_id = cityRow.id;
  }
  // explicit PointData(const PointId&, const CityId&);
  PointData data(id, CityId(city_id));
  data.codes[ENGLISH] = EncString::from866(code_en);
  data.codes[RUSSIAN] = EncString::from866(code_ru);
  data.names[ENGLISH] = EncString::from866(name_en);
  data.names[RUSSIAN] = EncString::from866(name_ru);
  return data;
}

// company !!! done
boost::optional<nsi::CompanyId> AstraCallbacks::findCompanyId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<nsi::CompanyId> AstraCallbacks::findCompanyIdByAccountCode(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866(); // уточнить AccountCode
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<CompanyData> AstraCallbacks::findCompanyData(const nsi::CompanyId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit CompanyData(const CompanyId&);
  CompanyData data(id);
  const TAirlinesRow &airlineRow = (const TAirlinesRow&)base_tables.get("airlines").get_row("code/code_lat", IdToCode(id.get()));
  data.codes[ENGLISH] = EncString::from866(airlineRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(airlineRow.code);
  data.names[ENGLISH] = EncString::from866(airlineRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(airlineRow.name);
  return data;
}

// aircraft !!! done
boost::optional<nsi::AircraftTypeId> AstraCallbacks::findAircraftTypeId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::AircraftTypeId(CodeToId(code.to866()));
}

boost::optional<AircraftTypeData> AstraCallbacks::findAircraftTypeData(const nsi::AircraftTypeId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit AircraftTypeData(const AircraftTypeId&);
  AircraftTypeData data(id);
  const TCraftsRow &craftRow = (const TCraftsRow&)base_tables.get("crafts").get_row("code/code_lat", IdToCode(id.get()));
  data.codes[ENGLISH] = EncString::from866(craftRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(craftRow.code);
  data.names[ENGLISH] = EncString::from866(craftRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(craftRow.name);
  return data;
}

// заглушки
boost::optional<RouterId> AstraCallbacks::findRouterId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<RouterId>();
}

boost::optional<RouterData> AstraCallbacks::findRouterData(const nsi::RouterId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // RouterData(const RouterId& );
  return RouterData(id);
}

// заглушки
boost::optional<CurrencyId> AstraCallbacks::findCurrencyId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<CurrencyId>();
}

boost::optional<CurrencyData> AstraCallbacks::findCurrencyData(const nsi::CurrencyId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit CurrencyData(const CurrencyId&);
  return CurrencyData(id);
}

// заглушки
boost::optional<OrganizationId> AstraCallbacks::findOrganizationId(const EncString& code)
{
//  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<OrganizationId>();
}

boost::optional<OrganizationData> AstraCallbacks::findOrganizationData(const OrganizationId& id)
{
//  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit OrganizationData(const OrganizationId&);
  return OrganizationData(id);
}

//------------------------------------------------------------------------------------------

string FlightToString(ct::Flight flight)
{
  string line = IdToCode(flight.airline.get());
  string num = IntToString(flight.number.get());
  string suffix = suffixToString(flight.suffix);
  return line + num + suffix;
}

TDateTime DatePlusTime(TDateTime aDate, TDateTime aTime)
{
  double res_date, res_time;
  res_time = modf(aTime, &res_date);
  res_time = fabs(res_time);
  return aDate + res_date + res_time;
}

// УДАЛИТЬ ИЗ БД РАСПИСАНИЕ

void DeleteScdPeriodsFromDb( const std::set<ssim::ScdPeriod> &scds )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeLocalAirp; // останется на рабочем
  reqInfo->desk.code = "MOVGRG"; // удалить
  for (auto &scd : scds)
  {
    LogTrace(TRACE5) << __func__ << " scd = " << scd;
    TDateTime first = BoostToDateTime(scd.period.start);
    TDateTime last = BoostToDateTime(scd.period.end);
    LogTrace(TRACE5) << "INITIAL first = " << DateTimeToStr(first) << " last = " << DateTimeToStr(last);
    TDateTime period_scd_out = 0;
    if (!scd.route.legs.empty())
    {
      const ssim::Leg &leg = scd.route.legs.front();
      EncodeTime(leg.s.dep.hours(), leg.s.dep.minutes(), 0, period_scd_out);
      TElemFmt airp_fmt;
      period_scd_out = ConvertFlightDate(period_scd_out, first,
                                         ElemToElemId(etAirp, IdToCode(leg.s.from.get()), airp_fmt),
                                         false, mtoUTC);
    }
    first = DatePlusTime(first, period_scd_out);
    last = DatePlusTime(last, period_scd_out);
    LogTrace(TRACE5) << "period_scd_out = " << DateTimeToStr(period_scd_out) << " (" << period_scd_out << ")";
    LogTrace(TRACE5) << "FINAL first = " << DateTimeToStr(first) << " last = " << DateTimeToStr(last);
    TQuery Qry( &OraSession );
    // можно TRUNC а не время вылета
    Qry.SQLText =
        "BEGIN "
        "DELETE FROM routes WHERE move_id IN ( "
        " SELECT move_id FROM sched_days "
        "WHERE flight=:flight AND TRUNC(first_day)=TRUNC(:first) AND TRUNC(last_day)=TRUNC(:last) ); "
        "DELETE FROM sched_days "
        "WHERE flight=:flight AND TRUNC(first_day)=TRUNC(:first) AND TRUNC(last_day)=TRUNC(:last) ; "
        "END;";
    Qry.CreateVariable("flight", otString, FlightToString(scd.flight));
    Qry.CreateVariable("first", otDate, first);
    Qry.CreateVariable("last", otDate, last);
    Qry.Execute();
  }
}

// ПОЛОЖИТЬ В БД РАСПИСАНИЯ

void ScdPeriodToDb( const ssim::ScdPeriod &scd )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  /*???if ( utc ) {
    reqInfo->user.sets.time = ustTimeUTC;
  }
  else {*/
    reqInfo->user.sets.time = ustTimeLocalAirp; // останется на рабочем
  /*}*/
  reqInfo->desk.code = "MOVGRG"; // удалить
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT trip_id FROM sched_days WHERE flight=:flight ";
  Qry.CreateVariable("flight", otString, FlightToString(scd.flight));
  Qry.Execute();
  int trip_id;
  if (!Qry.Eof)
    trip_id = Qry.FieldAsInteger("trip_id");
  else
    trip_id = ASTRA::NoExists;
  vector<TPeriod> speriods; //- периоды выполнения
  map<int,TDestList> mapds; //move_id,маршрут
  TFilter filter;
  bool first_airp = true;
  LogTrace(TRACE5) << __func__ << " scd = " << scd;
  TPeriod p;
  p.move_id = ASTRA::NoExists;
  p.first = BoostToDateTime(scd.period.start);
  p.last = BoostToDateTime(scd.period.end);
  LogTrace(TRACE5) << "INITIAL p.first = " << DateTimeToStr(p.first) << " p.last = " << DateTimeToStr(p.last);
  p.days = scd.period.freq.str();
  p.modify = ASTRA::finsert;
  TDestList dests;
  TDest curr, next;
 // TDateTime period_scd_out = ASTRA::NoExists;
  for (ssim::Legs::const_iterator ileg = scd.route.legs.begin(); ileg != scd.route.legs.end(); ++ileg)
  {
    const ssim::Leg &leg = *ileg;
    curr = next;
    curr.airline = ElemToElemId( etAirline, IdToCode(scd.flight.airline.get()), curr.airline_fmt );
    curr.trip = scd.flight.number.get();
    curr.suffix = ElemToElemId( etSuffix, suffixToString(scd.flight.suffix), curr.suffix_fmt );
    curr.airp = ElemToElemId( etAirp, IdToCode(leg.s.from.get()), curr.airp_fmt );
    curr.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), curr.craft_fmt );
    curr.triptype = DefaultTripType(false);
    LogTrace(TRACE5) << "leg.s.dep.hours() = " << leg.s.dep.hours() << " leg.s.dep.minutes() = " << leg.s.dep.minutes();
    int shift_day = leg.s.dep.hours() / 24; // сдвиг даты (время вида KJA1400/1)
    EncodeTime( leg.s.dep.hours() % 24, leg.s.dep.minutes(), 0, curr.scd_out );
    if (first_airp)
    {
      filter.filter_tz_region = AirpTZRegion(curr.airp);
      first_airp = false;
      SEASON::ConvertPeriod( p, curr.scd_out, filter.filter_tz_region ); //конывертация периода
    }
    curr.scd_out = ConvertFlightDate(curr.scd_out, p.first, curr.airp, false, mtoUTC) + shift_day;
    LogTrace(TRACE5) << "curr.scd_out = " << DateTimeToStr(curr.scd_out) << " (" << curr.scd_out << ")";
    next.airp = ElemToElemId( etAirp, IdToCode(leg.s.to.get()), next.airp_fmt );
    next.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), next.craft_fmt );
    next.triptype = DefaultTripType(false);
    LogTrace(TRACE5) << "leg.s.arr.hours() = " << leg.s.arr.hours() << " leg.s.arr.minutes() = " << leg.s.arr.minutes();
    shift_day = leg.s.arr.hours() / 24; // сдвиг даты
    EncodeTime( leg.s.arr.hours() % 24, leg.s.arr.minutes(), 0, next.scd_in );
    next.scd_in = ConvertFlightDate(next.scd_in, p.first, next.airp, true, mtoUTC) + shift_day;
    LogTrace(TRACE5) << "next.scd_in = " << DateTimeToStr(next.scd_in) << " (" << next.scd_in << ")";
    // struct Leg -> ct::RbdLayout subclOrder;
    LogTrace(TRACE5) << " leg.subclOrder = " << leg.subclOrder.toString();
    curr.rbd_order = leg.subclOrder.rbdOrder().toString();
    for (auto &cfg : leg.subclOrder.config())
      if (cfg.second)
      {
        if (cabinCode(cfg.first)=="F") curr.f = cfg.second.get();
        if (cabinCode(cfg.first)=="C") curr.c = cfg.second.get();
        if (cabinCode(cfg.first)=="Y") curr.y = cfg.second.get();
      }
/*    if (ileg == scd.route.legs.begin())
    {
      period_scd_out = curr.scd_out;
      LogTrace(TRACE5) << "period_scd_out = " << DateTimeToStr(period_scd_out) << " (" << period_scd_out << ")";
    }*/
    dests.dests.push_back( curr );
  } // ileg
/*  if ( period_scd_out != ASTRA::NoExists )
  {
    p.first = DatePlusTime(p.first, period_scd_out);
    p.last = DatePlusTime(p.last, period_scd_out);
    LogTrace(TRACE5) << "FINAL p.first = " << DateTimeToStr(p.first) << " p.last = " << DateTimeToStr(p.last);
  }*/
  next.airline.clear();
  next.trip = ASTRA::NoExists;
  next.suffix.clear();
  dests.dests.push_back( next );
  mapds.insert( make_pair(ASTRA::NoExists, dests) );
  speriods.push_back( p );
  //!!!UTC
  int_write( filter, FlightToString(scd.flight), speriods, trip_id, mapds );
}

// ПОЛУЧИТЬ ИЗ БД РАСПИСАНИЯ, ПЕРЕСЕКАЮЩИЕСЯ С ПЕРИОДОМ

//
ssim::Route RouteFromDb(int move_id, TDateTime first)
{
  // уточнить использование delta_in, delta_out
  LogTrace(TRACE5) << __func__ << " move_id = " << move_id;
  TReqInfo *reqInfo = TReqInfo::Instance(); // ??? уже есть в вызывающей функции
  reqInfo->user.sets.time = ustTimeLocalAirp; // останется на рабочем
  reqInfo->desk.code = "MOVGRG"; // удалить
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,airp,airp_fmt,flt_no,suffix,scd_in,craft,craft_fmt,scd_out,delta_in,delta_out,f,c,y,rbd_order "
    " FROM routes WHERE move_id=:move_id "
    " ORDER BY num";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  ssim::Route route;
  TDest dest1, dest2;
  string rbd_string;
  for (; !Qry.Eof; Qry.Next())
  {
    dest1 = dest2;
    dest2.num = Qry.FieldAsInteger( "num" );
    dest2.airp = Qry.FieldAsString( "airp" );
    dest2.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );

    if ( Qry.FieldIsNULL( "scd_in" ) )
      dest2.scd_in = ASTRA::NoExists;
    else
    {
      dest2.scd_in = ConvertFlightDate(Qry.FieldAsDateTime("scd_in"), first, dest2.airp, true, mtoLocal);
      LogTrace(TRACE5) << "dest2.airp = " << dest2.airp << " AirpTZRegion = " << AirpTZRegion(dest2.airp);
      LogTrace(TRACE5) << "qry scd_in = " << DateTimeToStr(Qry.FieldAsDateTime("scd_in"));
      LogTrace(TRACE5) << "dest2.scd_in = " << DateTimeToStr(dest2.scd_in);
    }

    dest2.craft = Qry.FieldAsString( "craft" );
    dest2.craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );

    if ( Qry.FieldIsNULL( "scd_out" ) )
      dest2.scd_out = ASTRA::NoExists;
    else
    {
      dest2.scd_out = ConvertFlightDate(Qry.FieldAsDateTime("scd_out"), first, dest2.airp, false, mtoLocal);
      LogTrace(TRACE5) << "dest2.airp = " << dest2.airp << " AirpTZRegion = " << AirpTZRegion(dest2.airp);
      LogTrace(TRACE5) << "qry scd_out = " << DateTimeToStr(Qry.FieldAsDateTime("scd_out"));
      LogTrace(TRACE5) << "dest2.scd_out = " << DateTimeToStr(dest2.scd_out);
    }

    if ( dest1.num == ASTRA::NoExists )
    {
      rbd_string = Qry.FieldAsString("rbd_order") + string(".") +
          string("F") + IntToString(Qry.FieldAsInteger("f")) +
          string("C") + IntToString(Qry.FieldAsInteger("c")) +
          string("Y") + IntToString(Qry.FieldAsInteger("y"));
      continue;
    }
    //leg
    int hours_o, mins_o, secs_o;
    int hours_i, mins_i, secs_i;
    DecodeTime( dest1.scd_out, hours_o, mins_o, secs_o );
    DecodeTime( dest2.scd_in, hours_i, mins_i, secs_i );
    if (!Qry.FieldIsNULL("delta_out")) hours_o += Qry.FieldAsInteger("delta_out")*24;
    if (!Qry.FieldIsNULL("delta_in")) hours_i += Qry.FieldAsInteger("delta_in")*24;
    ssim::Section section(nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest1.airp, dest1.airp_fmt ))), //out
                          nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest2.airp, dest2.airp_fmt ))), //in
                          time_duration( hours_o, mins_o, secs_o ),
                          time_duration( hours_i, mins_i, secs_i ));
    LogTrace(TRACE5) << "rbd_string = " << rbd_string;
    boost::optional<ct::RbdLayout> rbd = ct::RbdLayout::fromString(rbd_string);
    if (not rbd) throw EXCEPTIONS::Exception("NO RBD");
    LogTrace(TRACE5) << " RBD = " << rbd.get().toString();
    ssim::Leg leg(section,
                  nsi::ServiceTypeId(0), // HACK
                  nsi::AircraftTypeId(CodeToId(ElemIdToClientElem( etCraft, dest1.craft, dest1.craft_fmt ))),
                  rbd.get());
    route.legs.push_back(leg);
  }
  return route;
}

//
ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& a_prd )
{
  Period prd(a_prd);
  LogTrace(TRACE5) << __func__ << " flt = " << flt << " prd = " << prd;
//  LogTrace(TRACE5) << "prd.start (boost) = " << prd.start << " prd.end (boost) = " << prd.end;
  // HACK
  // TODO уточнить допустимость замены neg_infin/pos_infin на min_date_time/max_date_time
  // https://www.boost.org/doc/libs/1_55_0/doc/html/date_time/gregorian.html#date_time.gregorian.date_class
  if (prd.start.is_neg_infinity())
  {
    prd.start = boost::gregorian::date(boost::gregorian::min_date_time);
    LogTrace(TRACE5) << "prd.start CHANGED to " << prd.start;
  }
  if (prd.end.is_pos_infinity())
  {
    prd.end = boost::gregorian::date(boost::gregorian::max_date_time);
    LogTrace(TRACE5) << "prd.end CHANGED to " << prd.end;
  }
  LogTrace(TRACE5) << "prd.start = " << DateTimeToStr(BoostToDateTime(prd.start)) << " (" << BoostToDateTime(prd.start) << ")" <<
                      " prd.end = " << DateTimeToStr(BoostToDateTime(prd.end)) << " (" << BoostToDateTime(prd.end) << ")";
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeLocalAirp; // останется на рабочем
  reqInfo->desk.code = "MOVGRG"; // удалить
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT move_id, first_day, last_day, days, region"
    " FROM sched_days "
    "WHERE flight=:flight AND first_day<=:last AND last_day>=:first "
    " ORDER BY num ";
  Qry.CreateVariable("flight", otString, FlightToString(flt));
  Qry.CreateVariable("first", otDate, BoostToDateTime(prd.start));
  Qry.CreateVariable("last", otDate, BoostToDateTime(prd.end));
  Qry.Execute();
  ssim::ScdPeriods scds;
  for (; !Qry.Eof; Qry.Next())
  {
    TDateTime first = Qry.FieldAsDateTime("first_day");
    TDateTime last = Qry.FieldAsDateTime("last_day");
    string days = Qry.FieldAsString("days");
    string err_tz_region;
    double date_for_route;
    modf(first, &date_for_route);
    if (!ConvertPeriodToLocal(first, last, days, Qry.FieldAsString("region"), err_tz_region ))
      throw EXCEPTIONS::Exception("ConvertPeriodToLocal err_tz_region = %s", err_tz_region.c_str());
    ssim::ScdPeriod scd(flt);
    scd.route = RouteFromDb(Qry.FieldAsInteger("move_id"), date_for_route);
    scd.period = Period(DateTimeToBoost(first).date(),
                        DateTimeToBoost(last).date(),
                        days);
    LogTrace(TRACE5) << __func__ << " scd = " << scd;
    scds.push_back(scd);
  }
  return scds;
}

//------------------------------------------------------------------------------------------

void InitSSIM()
{
  static bool init = false;
  if (!init)
  {
    typeb_template::addTemplate(new SsmTemplate());
    typeb_template::addTemplate(new AsmTemplate());
    setCallbacks(new AstraCallbacks);
    init = true;
    LogTrace(TRACE5) << "SSIM parsers init complete";
  }
}

int HandleSSMTlg(string body)
{
//  LogTrace(TRACE5) << __func__ << endl << body;
  // TODO RSD /home/user/sirena/src/ssim/proc_ssm.cc:503
  // TODO перекрытие периодов /home/user/sirena/src/ssim/proc_ssm.cc:555

  InitSSIM();

  const auto ssm = ssim::parseSsm(body, nullptr);
  if (!ssm)
  {
    throw EXCEPTIONS::Exception("SSM: parse: %s", ssm.err().toString(UserLanguage::en_US()).c_str());
    return -1;
  }

  const ssim::SsmStruct& ssmStr = *ssm;

  if (ssmStr.specials.size() > 1)
  {
    LogTrace(TRACE5) << "SSM: Several ACK/NAC inside SSM";
  }

  for (const ssim::SsmSubmsgPtr& spec : ssmStr.specials)
  {
    if (spec->type() == ssim::SSM_ACK)
    {
      LogTrace(TRACE5) << "SSM: ACK received";
      return 0;
    }
    if (spec->type() == ssim::SSM_NAC)
    {
      // ОТРИЦАТЕЛЬНЫЙ ОТВЕТ НА НАШУ ТЕЛЕГРАММУ
      LogTrace(TRACE5) << "SSM: NAC received";
      return 0;
    }
  }

  if (ssmStr.submsgs.size() > 1)
  {
    // НЕСКОЛЬКО РЕЙСОВ В ССМ НЕ ПОДДЕРЖИВАЕТСЯ
    throw EXCEPTIONS::Exception("SSM: ssmStr.submsgs.size() > 1");
    return -1;
  }

  for (const auto& v : ssmStr.submsgs)
    for (const ssim::SsmSubmsgPtr& s : v.second)
      if (s->type() == ssim::SSM_RSD)
      {
        LogTrace(TRACE5) << "SSM: RSD received";
        return 0;
      }

  const ssim::ProcContext attr
  {
    nsi::CompanyId(0),
    false,
    ssmStr.head.timeIsLocal,
    false,
    std::make_unique<AstraSsimCallbacks>()
  };

  ssim::CachedScdPeriods cache;
  const std::vector<ssim::SsmSubmsgPtr>& submsgs = ssmStr.submsgs.begin()->second;
  LogTrace(TRACE5) << "START MODIFY --------------------------------";
  for (const ssim::SsmSubmsgPtr& subMsg : submsgs)
  {
    if (const Message msg = subMsg->modify(cache, attr))
    {
      throw EXCEPTIONS::Exception("SSM: modify: %s", msg.toString(UserLanguage::en_US()).c_str());
      return -1;
    }
  }
  LogTrace(TRACE5) << "END MODIFY --------------------------------";
  LogTrace(TRACE5) << "CACHE: " << cache;

  // std::map<ct::Flight, std::set<ssim::ScdPeriod> > forDeletion;
  for (auto &rm : cache.forDeletion)
  {
    DeleteScdPeriodsFromDb(rm.second);
  }

  // std::map<ct::Flight, ssim::ScdPeriods> forSaving() const;
  for (auto &sv : cache.forSaving())
  {
    for (auto &scd : sv.second)
    {
      ScdPeriodToDb(scd);
    }
  }

  return 0;
}

//------------------------------------------------------------------------------------------

int ssim_test(int argc, char **argv)
{
  InitSSIM();
  std::ifstream t("file.txt");
  std::string str((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());
  cout << "Parsing SSM ..." << endl;
  const auto ssm = ssim::parseSsm(str, nullptr); // !!!
  if (!ssm)
  {
    cout << "SSM NOT PARSED!" << endl;
    return -1;
  }
  cout << ssm->toString() << endl;
  cout << "---------------------------------------" << endl;
  const ssim::SsmStruct& ssmStr = *ssm;
  const std::vector<ssim::SsmSubmsgPtr>& submsgs = ssmStr.submsgs.begin()->second;
  const ssim::ProcContext attr {
    nsi::CompanyId(0),
    false,
    ssmStr.head.timeIsLocal,
    false,
    std::make_unique<AstraSsimCallbacks>()
  };
  ssim::CachedScdPeriods cache;
  for (const ssim::SsmSubmsgPtr& subMsg : submsgs)
  {
    if( Message const msg = subMsg->modify(cache, attr) )
    {
      cout << msg << endl;
      return -1;
    }
  }

  cout << "cache.forSaving().size()=" << cache.forSaving().size() << endl;
  for (auto &sv : cache.forSaving())
  {
    cout << "sv.second.size()=" << sv.second.size() << endl;
    for (auto &scd : sv.second)
    {
      cout << "scd.route.legs.size()=" << scd.route.legs.size() << endl;
      for (auto leg : scd.route.legs)
      {
        cout << "leg.subclOrder.config().size()=" << leg.subclOrder.config().size() << endl;
        cout << "CFG: " << toString(leg.subclOrder.config()) << endl;
        for (auto &c : leg.subclOrder.config())
        {
          if (c.second)
          {
            cout << "cabinCode=" << cabinCode(c.first) << " i=" << c.second.get() << endl;
          }
        }
      }
    }
  }
  cout << "---------------------------------------" << endl;
  return 0;
}

