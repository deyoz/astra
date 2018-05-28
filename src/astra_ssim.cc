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

int CodeToId(const std::string code);
std::string IdToCode(const int id);

ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd );

//------------------------------------------------------------------------------------------

// 1. Реализовать конвертацию системо-специфичного представления расписания
// в/из ssim::ScdPeriod (см. convertToSsim/makeScdPeriod)

SSIMScdPeriod MakeSSIMScdPeriod( const ssim::ScdPeriod& scd)
{
  SSIMFlight flight;
  flight.airline = IdToCode(scd.flight.airline.get());
  flight.flt_no = scd.flight.number.get();
  flight.suffix = IntToString(static_cast<int>(scd.flight.suffix.get()));
  SSIMScdPeriod result(flight);
  result.period.first = BoostToDateTime(scd.period.start);
  result.period.last = BoostToDateTime(scd.period.end);
  result.period.days = scd.period.freq.str();
  for (const ssim::Leg& leg : scd.route.legs)
  {
    SSIMSection section;
    section.from = IdToCode(leg.s.from.get());
    section.to = IdToCode(leg.s.to.get());
    section.dep = leg.s.dep;
    section.arr = leg.s.arr;
    std::string craft = IdToCode(leg.aircraftType.get());
    result.route.legs.push_back(SSIMLeg(section, craft));
  }
  return result;
}

ssim::ScdPeriod MakeScdPeriod( const SSIMScdPeriod& scd)
{
  int suffix;
  StrToInt(scd.flight.suffix.c_str(), suffix);
  ct::Flight flight(nsi::CompanyId(CodeToId(scd.flight.airline)),
                    ct::FlightNum(scd.flight.flt_no),
                    ct::Suffix(suffix));
  Period period(DateTimeToBoost(scd.period.first).date(),
                DateTimeToBoost(scd.period.last).date(),
                scd.period.days);
  ssim::ScdPeriod result(flight);
  result.period = period;
  for (const SSIMLeg& leg : scd.route.legs)
  {
    ssim::Section section(nsi::PointId(CodeToId(leg.section.from)),
                          nsi::PointId(CodeToId(leg.section.to)),
                          leg.section.dep,
                          leg.section.arr);
    boost::optional<ct::RbdLayout> rbd = ct::RbdLayout::fromString("CJZIDAYRSTEQGNBXWUOVHLK.C10Y100"); // HACK FIXME
    if (not rbd) throw EXCEPTIONS::Exception("NO RBD");
    ssim::Leg leg_r(section,
                    nsi::ServiceTypeId(0), // HACK FIXME
                    nsi::AircraftTypeId(CodeToId(leg.craft)),
                    rbd.get());
    result.route.legs.push_back(leg_r);
  }
  return result;
}

//------------------------------------------------------------------------------------------

// 2. Реализовать ssim::SsimTlgCallbacks
// необходимо лишь SsimTlgCallbacks::getSchedules (все периоды
// расписания на рейс, пересекающиейся с указанным периодом)  и заглушки на
// все остальное.

// getSchedules надо реализовать
ssim::ScdPeriods AstraSsimCallbacks::getSchedules(const ct::Flight& flight, const Period& period) const
{
  LogTrace(TRACE5) << __func__ << " flight=" << flight << " period=" << period;
//  return ssim::ScdPeriods();
  return ScdPeriodsFromDb( flight, period );
}

// остальное заглушки
ssim::ScdPeriods AstraSsimCallbacks::getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const
{
  LogTrace(TRACE5) << __func__;
  return ssim::ScdPeriods();
}

Expected< boost::optional<ssim::CshSettings> > AstraSsimCallbacks::cshSettingsByTlg(nsi::CompanyId, ssim::ScdPeriod&) const
{
  LogTrace(TRACE5) << __func__;
  return boost::optional<ssim::CshSettings>();
}

Expected< boost::optional<ssim::CshSettings> > AstraSsimCallbacks::cshSettingsByScd(const ssim::ScdPeriod&) const
{
  LogTrace(TRACE5) << __func__;
  return boost::optional<ssim::CshSettings>();
}

ssim::DefValueSetter AstraSsimCallbacks::prepareDefaultValueSetter(ct::DeiCode, const nsi::DepArrPoints&, bool byLeg) const
{
  LogTrace(TRACE5) << __func__;
  return ssim::DefValueSetter();
}

//------------------------------------------------------------------------------------------

// 3. Реализовать ф-ции вывода требуемых типов в Message::bind (см.
// sirena/src/message.cc)

// скопировано из Сирены
namespace message_details {

template<> std::string translate_impl<nsi::CompanyId>(const UserLanguage& lang, const nsi::CompanyId& v)
{
  LogTrace(TRACE5) << __func__;
  return nsi::Company(v).code(ENGLISH).toUtf();
}

template<> std::string translate_impl<boost::gregorian::date>(const UserLanguage& lang, const boost::gregorian::date& d)
{
  LogTrace(TRACE5) << __func__;
  return Dates::ddmmrr(d);
}

template<> std::string translate_impl<Period>(const UserLanguage& lang, const Period& v)
{
  LogTrace(TRACE5) << __func__;
  return translate_impl(lang, v.start) + "-" + translate_impl(lang, v.end) + " (" + translate_impl(lang, v.freq) + ")";
}

template<> std::string translate_impl<Freq>(const UserLanguage&, const Freq& v)
{
  LogTrace(TRACE5) << __func__;
  return v.str();
}

template<> std::string translate_impl<ct::Flight>(const UserLanguage&, const ct::Flight& v)
{
  LogTrace(TRACE5) << __func__;
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

// ЗАМЕЧАНИЕ: в случае id = 10 получается code = символу перевода строки

int CodeToId(const std::string code)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("CodeToId invalid sizeof");
  if (code.size() > 3) throw EXCEPTIONS::Exception("CodeToId invalid code size");
  const char *c = code.c_str();
  int id = 0;
  char* p = (char*)&id;
  for (unsigned int i = 0; i < code.size(); ++i) p[i] = c[i];
  LogTrace(TRACE5) << __func__ << " code=\"" << code << "\" id=" << id;
  return id;
}

std::string IdToCode(const int id)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("IdToCode invalid sizeof");
  const char* p = (char*)&id;
  char code[4];
  for (unsigned int i = 0; i < 3; ++i) code[i] = p[i];
  code[3] = '\0';
  std::string code_str(code);
  LogTrace(TRACE5) << __func__ << " id=" << id << " code=\"" << code_str << "\"";
  return code_str;
}

bool AstraCallbacks::needCheckVersion() const
{
  LogTrace(TRACE5) << __func__;
  return false;
}

nsi::CityId AstraCallbacks::centerCity() const
{
  LogTrace(TRACE5) << __func__;
  return nsi::CityId(255); // TODO получать реальный город
}

// /home/user/Sirena/sirenalibs/libnsi/src/nsi.h

// заглушки
boost::optional<nsi::DocTypeId> AstraCallbacks::findDocTypeId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::DocTypeId>();
}

boost::optional<DocTypeData> AstraCallbacks::findDocTypeData(const nsi::DocTypeId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit DocTypeData(const DocTypeId& id);
  return DocTypeData(id);
}

// заглушки
boost::optional<nsi::SsrTypeId> AstraCallbacks::findSsrTypeId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::SsrTypeId>();
}

boost::optional<SsrTypeData> AstraCallbacks::findSsrTypeData(const nsi::SsrTypeId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit SsrTypeData(const SsrTypeId&);
  return SsrTypeData(id);
}

// заглушки
boost::optional<nsi::GeozoneId> AstraCallbacks::findGeozoneId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<nsi::GeozoneId>();
}

boost::optional<GeozoneData> AstraCallbacks::findGeozoneData(const nsi::GeozoneId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit GeozoneData(const GeozoneId&);
  return GeozoneData(id);
}

// country !!! done
boost::optional<nsi::CountryId> AstraCallbacks::findCountryId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CountryId(CodeToId(code.to866()));
}

boost::optional<nsi::CountryId> AstraCallbacks::findCountryIdByIso(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CountryId(CodeToId(code.to866())); // ???
}

boost::optional<CountryData> AstraCallbacks::findCountryData(const nsi::CountryId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
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

// TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// region !!! отсутствует в base_tables
boost::optional<nsi::RegionId> AstraCallbacks::findRegionId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::RegionId(CodeToId(code.to866()));
}

boost::optional<RegionData> AstraCallbacks::findRegionData(const nsi::RegionId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit RegionData(const RegionId&, const CountryId&);
  RegionData data(id, CountryId(0));
  return data;
}

// city !!! done
boost::optional<nsi::CityId> AstraCallbacks::findCityId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CityId(CodeToId(code.to866()));
}

boost::optional<CityData> AstraCallbacks::findCityData(const nsi::CityId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
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
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::PointId(CodeToId(code.to866()));
}

boost::optional<PointData> AstraCallbacks::findPointData(const nsi::PointId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
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
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<nsi::CompanyId> AstraCallbacks::findCompanyIdByAccountCode(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866(); // уточнить AccountCode
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<CompanyData> AstraCallbacks::findCompanyData(const nsi::CompanyId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
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
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return nsi::AircraftTypeId(CodeToId(code.to866()));
}

boost::optional<AircraftTypeData> AstraCallbacks::findAircraftTypeData(const nsi::AircraftTypeId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
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
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<RouterId>();
}

boost::optional<RouterData> AstraCallbacks::findRouterData(const nsi::RouterId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // RouterData(const RouterId& );
  return RouterData(id);
}

// заглушки
boost::optional<CurrencyId> AstraCallbacks::findCurrencyId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<CurrencyId>();
}

boost::optional<CurrencyData> AstraCallbacks::findCurrencyData(const nsi::CurrencyId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit CurrencyData(const CurrencyId&);
  return CurrencyData(id);
}

// заглушки
boost::optional<OrganizationId> AstraCallbacks::findOrganizationId(const EncString& code)
{
  LogTrace(TRACE5) << __func__ << " code=" << code.to866();
  return boost::optional<OrganizationId>();
}

boost::optional<OrganizationData> AstraCallbacks::findOrganizationData(const OrganizationId& id)
{
  LogTrace(TRACE5) << __func__ << " id=" << id.get();
  // explicit OrganizationData(const OrganizationId&);
  return OrganizationData(id);
}

//------------------------------------------------------------------------------------------

ssim::ScdPeriods SplitScdPeriodByDays( const ssim::ScdPeriod &src )
{
  ssim::ScdPeriods res;
  for (boost::gregorian::date date = src.period.start; date <= src.period.end; date += boost::gregorian::days(1))
  {
    ssim::ScdPeriod scd(src);
    scd.period.start = date;
    scd.period.end = date;
    short wday = date.day_of_week();
    if ( wday == 0 )
      wday = 7;
    if (scd.period.freq.hasDayOfWeek(wday))
      res.push_back(scd);
  }
  return res;
}

//------------------------------------------------------------------------------------------

// удалить из БД расписания, содержащиеся внутри периода
// TODO уточнить правила пересечения!!!
// TODO уточнить насчёт freq
void DeleteSchedulesFromDb( const ssim::ScdPeriod &scd )
{
  LogTrace(TRACE5) << __func__ << " beg=" << scd.period.start << " end=" << scd.period.end;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
      "DELETE FROM SCHEDULE_TEST "
      " WHERE F_LINE = :line AND F_NUMBER = :num AND F_SUFFIX = :suffix "
      " AND :begDt <= P_DATE1 AND P_DATE2 <= :endDt ";

  Qry.CreateVariable("line", otString, IdToCode(scd.flight.airline.get()));
  Qry.CreateVariable("num", otInteger, scd.flight.number.get());
  Qry.CreateVariable("suffix", otInteger, static_cast<int>(scd.flight.suffix.get()));
  Qry.CreateVariable("begDt", otDate, BoostToDateTime(scd.period.start));
  Qry.CreateVariable("endDt", otDate, BoostToDateTime(scd.period.end));
  Qry.Execute();
}

// положить в БД расписание на основе периода
// TODO уточнить насчёт суффикса
void ScdPeriodToDb( const ssim::ScdPeriod &scd )
{
  LogTrace(TRACE5) << __func__ << " beg=" << scd.period.start << " end=" << scd.period.end;
  TQuery Qry(&OraSession);
  for (const ssim::Leg& leg : scd.route.legs)
  {
    Qry.Clear();
    Qry.SQLText =
        "INSERT INTO SCHEDULE_TEST "
        " (F_LINE, F_NUMBER, F_SUFFIX, "
        " P_DATE1, P_DATE2, P_FREQ, "
        " AIRP_OUT, SCD_OUT, AIRP_IN, SCD_IN, "
        " SERV_TYPE, CRAFT_TYPE, RBD_LAYOUT) "
        " VALUES (:line, :num, :suffix, :begDt, :endDt, :freq, :a_out, :s_out, :a_in, :s_in, :serv, :craft, :rbd) ";

    Qry.CreateVariable("line", otString, IdToCode(scd.flight.airline.get()));
    Qry.CreateVariable("num", otInteger, scd.flight.number.get());
    Qry.CreateVariable("suffix", otInteger, static_cast<int>(scd.flight.suffix.get()));
    Qry.CreateVariable("begDt", otDate, BoostToDateTime(scd.period.start));
    Qry.CreateVariable("endDt", otDate, BoostToDateTime(scd.period.end));
    Qry.CreateVariable("freq", otString, scd.period.freq.str());

    Qry.CreateVariable("a_out", otString, IdToCode(leg.s.from.get()));
    Qry.CreateVariable("a_in", otString, IdToCode(leg.s.to.get()));

    Qry.CreateVariable("s_out", otDate, BoostToDateTime(boost::posix_time::ptime(scd.period.start, leg.s.dep)));
    Qry.CreateVariable("s_in", otDate, BoostToDateTime(boost::posix_time::ptime(scd.period.start, leg.s.arr)));

    Qry.CreateVariable("serv", otInteger, leg.serviceType.get());
    Qry.CreateVariable("craft", otString, IdToCode(leg.aircraftType.get()));
    Qry.CreateVariable("rbd", otString, leg.subclOrder.toString());

    Qry.Execute();
  }
}

// получить из БД расписания, пересекающиеся с периодом
// TODO уточнить насчёт суффикса!!!
// TODO уточнить насчёт freq
// TODO и насчёт пересечения тоже уточнить
// /home/user/leonardo/src/res/scd_locate.cc:226
ssim::ScdPeriods ScdPeriodsFromDbTest( const ct::Flight& flt, const Period& prd )
{
  LogTrace(TRACE5) << __func__ << " flt=" << flt << " prd=" << prd;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
      "SELECT F_LINE, F_NUMBER, F_SUFFIX, "
      " P_DATE1, P_DATE2, P_FREQ, "
      " AIRP_OUT, AIRP_IN, SCD_OUT, SCD_IN, "
      " SERV_TYPE, CRAFT_TYPE, RBD_LAYOUT "
      " FROM SCHEDULE_TEST "
      " WHERE F_LINE = :line AND F_NUMBER = :num AND F_SUFFIX = :suffix "
      " AND P_DATE1 <= :endDt AND P_DATE2 >= :begDt ";

  Qry.CreateVariable("line", otString, IdToCode(flt.airline.get()));
  Qry.CreateVariable("num", otInteger, flt.number.get());
  Qry.CreateVariable("suffix", otInteger, static_cast<int>(flt.suffix.get()));
  Qry.CreateVariable("begDt", otDate, BoostToDateTime(prd.start));
  Qry.CreateVariable("endDt", otDate, BoostToDateTime(prd.end));
  Qry.Execute();

  ssim::ScdPeriods scds;
  while (!Qry.Eof)
  {
    ct::Flight flight(nsi::CompanyId(CodeToId(Qry.FieldAsString("F_LINE"))),
                      ct::FlightNum(Qry.FieldAsInteger("F_NUMBER")),
                      ct::Suffix(Qry.FieldAsInteger("F_SUFFIX")));
    Period period(DateTimeToBoost(Qry.FieldAsDateTime("P_DATE1")).date(),
                  DateTimeToBoost(Qry.FieldAsDateTime("P_DATE2")).date(),
                  Qry.FieldAsString("P_FREQ"));
    ssim::ScdPeriod scd(flight);
    scd.period = period;
    // сформировать Leg
    ssim::Section section(nsi::PointId(CodeToId(Qry.FieldAsString("AIRP_OUT"))),
                          nsi::PointId(CodeToId(Qry.FieldAsString("AIRP_IN"))),
                          DateTimeToBoost(Qry.FieldAsDateTime("SCD_OUT")).time_of_day(),
                          DateTimeToBoost(Qry.FieldAsDateTime("SCD_IN")).time_of_day());
    boost::optional<ct::RbdLayout> rbd = ct::RbdLayout::fromString(Qry.FieldAsString("RBD_LAYOUT"));
    if (not rbd) throw EXCEPTIONS::Exception("NO RBD");
    ssim::Leg leg(section,
                  nsi::ServiceTypeId(Qry.FieldAsInteger("SERV_TYPE")),
                  nsi::AircraftTypeId(CodeToId(Qry.FieldAsString("CRAFT_TYPE"))),
                  rbd.get());
    // новый период или добавить к существующему
    auto iter = find_if(scds.begin(), scds.end(), [&scd] (const ssim::ScdPeriod& x) {return x.flight == scd.flight && x.period == scd.period;});
    if (iter == scds.end())
    {
      scd.route.legs.push_back(leg);
      scds.push_back(scd);
    }
    else
    {
      iter->route.legs.push_back(leg);
    }
    Qry.Next();
  }
  return scds;
}

ssim::Route RouteFromDb(int move_id)
{
  ssim::Route route;
  return route;
}

ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd )
{
  LogTrace(TRACE5) << __func__ << " flt=" << flt << " prd=" << prd;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT trip_id,move_id,first_day,last_day,days,pr_del,tlg.reference,region"
    " FROM sched_days "
    "WHERE flight=:flight AND first_day<=:last AND last_day>=:first "
    " ORDER BY num ";
  string line = IdToCode(flt.airline.get());
  string num = IntToString(flt.number.get());
  string suffix = IntToString(static_cast<int>(flt.suffix.get()));
  Qry.CreateVariable( "flight", otString, line + num + suffix );
  Qry.CreateVariable( "first", otDate, BoostToDateTime(prd.start) );
  Qry.CreateVariable( "last", otDate, BoostToDateTime(prd.end) );
  Qry.Execute();
  ssim::ScdPeriods scds;
  while (!Qry.Eof)
  {
    ssim::ScdPeriod scd(flt);
    scd.period = Period(DateTimeToBoost(Qry.FieldAsDateTime("first_day")).date(),
                        DateTimeToBoost(Qry.FieldAsDateTime("last_day")).date(),
                        Qry.FieldAsString("days"));
  }
  return scds;
}

//------------------------------------------------------------------------------------------

int ssim_test(int argc, char **argv)
{
//  Необходимо "зарегистрировать" парсеры (однократно где-нибудь в районе инициализации других глобальных сущностей):
//  typeb_template::addTemplate(new SsmTemplate());
//  typeb_template::addTemplate(new AsmTemplate());
//  см. sirena/src/airimp/typeb_init.cc (использование в sirena/src/obrzap.cc)
  typeb_template::addTemplate(new SsmTemplate());
  typeb_template::addTemplate(new AsmTemplate());

//  nsi::setupTestNsi();

  setCallbacks(new AstraCallbacks);

  std::ifstream t("file.txt");
  std::string str((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());

  cout << "Parsing SSM ..." << endl;
  cout << "---------------------------------------" << endl;

  const auto ssm = ssim::parseSsm(str, nullptr); // !!!

  if (!ssm)
  {
    cout << "NO SSM" << endl;
    return -1;
  }

  cout << ssm->toString() << endl;
  cout << "---------------------------------------" << endl;

  const ssim::SsmStruct& ssmStr = *ssm;
  const std::vector<ssim::SsmSubmsgPtr>& submsgs = ssmStr.submsgs.begin()->second;

//  struct ProcContext
//  {
//      nsi::CompanyId ourComp;
//      bool sloader;
//      bool timeIsLocal;
//      bool inventoryHost;
//      std::unique_ptr<SsimTlgCallbacks> callbacks;
//  };

  const ssim::ProcContext attr {
    nsi::CompanyId(0),
    false,
    ssmStr.head.timeIsLocal,
    false,
    std::make_unique<AstraSsimCallbacks>()
  };

  cout << "Filling ssim::CachedScdPeriods ..." << endl;
  cout << "---------------------------------------" << endl;

  ssim::CachedScdPeriods cache;
  for (const ssim::SsmSubmsgPtr& subMsg : submsgs)
  {
    // CALL_MSG_RET(subMsg->modify(cache, attr)); // /home/user/Sirena/src/ssim/proc_ssm.cc:516
    if( Message const msg = subMsg->modify(cache, attr) )
    {
      LogTrace( TRACE5 ) << msg;
      cout << msg << endl;
    }
  }

  cout << cache << endl;
  cout << "---------------------------------------" << endl;

  cout << "forDeletion" << endl;
  for (auto rm : cache.forDeletion)
    for (auto scd : rm.second)
    {
      cout << scd << endl;
      DeleteSchedulesFromDb(scd);
    }
  cout << "---------------------------------------" << endl;

  const bool SPLIT = 0;

  cout << "forSaving" << endl;
  for (auto sv : cache.forSaving())
    for (auto scd : sv.second)
    {
      cout << scd << endl;

      if (SPLIT)
      {
        ssim::ScdPeriods splitted = SplitScdPeriodByDays(scd);
        for (auto s : splitted) ScdPeriodToDb(s);
      }
      else
      {
        ScdPeriodToDb(scd);
      }

    }
  cout << "---------------------------------------" << endl;

  return 0;
}

