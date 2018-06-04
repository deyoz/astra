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

int CodeToId(const std::string code);
std::string IdToCode(const int id);

ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd );

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
  LogTrace(TRACE5) << __func__ << " flight=" << flight << " period=" << period;
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

string SchedDaysFlight(ct::Flight flight)
{
  string line = IdToCode(flight.airline.get());
  string num = IntToString(flight.number.get());
  string suffix = IntToString(flight.suffix.get());
  return line + num + suffix;
}

// УДАЛИТЬ ИЗ БД РАСПИСАНИЕ

void DeleteScdPeriodsFromDb( const std::set<ssim::ScdPeriod> &scds )
{
  for (auto &scd : scds)
  {
    LogTrace(TRACE5) << __func__ << " beg=" << scd.period.start << " end=" << scd.period.end;
    TQuery Qry( &OraSession );
    Qry.SQLText =
        "BEGIN "
        "DELETE FROM routes WHERE move_id IN ( "
        " SELECT move_id FROM shed_days "
        "WHERE flight=:flight AND first_day=:first AND last_day=:last ); "
        "DELETE FROM sched_days "
        "WHERE flight=:flight AND first_day=:first AND last_day=:last ; "
        "END;";
    Qry.CreateVariable("flight", otString, SchedDaysFlight(scd.flight));
    Qry.CreateVariable("first", otDate, BoostToDateTime(scd.period.start));
    Qry.CreateVariable("last", otDate, BoostToDateTime(scd.period.end));
    Qry.Execute();
  }
}

// ПОЛОЖИТЬ В БД РАСПИСАНИЯ

void ScdPeriodsToDb( const ssim::ScdPeriods &scds )
{
  if (scds.empty()) return;
  TReqInfo *reqInfo = TReqInfo::Instance();
  /*???if ( utc ) {
    reqInfo->user.sets.time = ustTimeUTC;
  }
  else {*/
    reqInfo->user.sets.time = ustTimeLocalAirp;
  /*}*/
  reqInfo->desk.code = "MOVGRG";
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT trip_id FROM sched_days WHERE flight=:flight ";
  // обоснованно предполагаем, что у всех элементов scds одинаковый flight
  Qry.CreateVariable("flight", otString, SchedDaysFlight(scds.front().flight));
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
  for ( auto &scd : scds )
  {
    LogTrace(TRACE5) << __func__ << " scd=" << scd;
    TPeriod p;
    p.move_id = ASTRA::NoExists;
    p.first = BoostToDateTime(scd.period.start);
    p.last = BoostToDateTime(scd.period.end);
    p.days = scd.period.freq.str();
    p.modify = ASTRA::finsert;
    speriods.push_back( p );
    TDestList dests;
    TDest curr, next;
    for (auto &leg : scd.route.legs)
    {
      curr = next;
      curr.airline = ElemToElemId( etAirline, IdToCode(scd.flight.airline.get()), curr.airline_fmt );
      curr.trip = scd.flight.number.get();
      curr.suffix = ElemToElemId( etSuffix, IntToString(scd.flight.suffix.get()), curr.suffix_fmt );
      curr.airp = ElemToElemId( etAirp, IdToCode(leg.s.from.get()), curr.airp_fmt );
      if (first_airp)
      {
        filter.filter_tz_region = AirpTZRegion(curr.airp);
        first_airp = false;
      }
      EncodeTime( leg.s.dep.hours(), leg.s.dep.minutes(), 0, curr.scd_out );
      curr.scd_out = ConvertFlightDate( curr.scd_out, p.first, curr.airp, false, mtoUTC );
      curr.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), curr.craft_fmt );

      next.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), next.craft_fmt );
      next.airp = ElemToElemId( etAirp, IdToCode(leg.s.to.get()), next.airp_fmt );
      EncodeTime( leg.s.arr.hours(), leg.s.arr.minutes(), 0, next.scd_in );
      next.scd_in = ConvertFlightDate( next.scd_in, p.first, next.airp, true, mtoUTC );
      dests.dests.push_back( curr );
    }
    next.airline.clear();
    next.trip = ASTRA::NoExists;
    next.suffix.clear();
    dests.dests.push_back( next );
    mapds.insert( make_pair(ASTRA::NoExists, dests) );
  }
  //!!!UTC
  int_write( filter, SchedDaysFlight(scds.front().flight), speriods, trip_id, mapds );
}

// ПОЛУЧИТЬ ИЗ БД РАСПИСАНИЯ, ПЕРЕСЕКАЮЩИЕСЯ С ПЕРИОДОМ

//
ssim::Route RouteFromDb(int move_id)
{
  LogTrace(TRACE5) << __func__ << " move_id=" << move_id;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT num,airp,airp_fmt,scd_in,craft,craft_fmt,scd_out,delta_in,delta_out "
    " FROM routes WHERE move_id=:move_id "
    " ORDER BY num";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  ssim::Route route;
  TDest dest1, dest2;
  for (; !Qry.Eof; Qry.Next())
  {
    dest1 = dest2;
    dest2.num = Qry.FieldAsInteger( "num" );
    dest2.airp = Qry.FieldAsString( "airp" );
    dest2.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );
    if ( Qry.FieldIsNULL( "scd_in" ) )
      dest2.scd_in = ASTRA::NoExists;
    else
      dest2.scd_in = UTCToLocal( Qry.FieldAsDateTime( "scd_in" ), AirpTZRegion( dest2.airp ) );
    dest2.craft = Qry.FieldAsString( "craft" );
    dest2.craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );
    if ( Qry.FieldIsNULL( "scd_out" ) )
      dest2.scd_out = ASTRA::NoExists;
    else
      dest2.scd_out = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), AirpTZRegion( dest2.airp ) );
    if ( dest1.num == ASTRA::NoExists )
      continue;
    //leg
    int hours_o, mins_o, secs_o;
    int hours_i, mins_i, secs_i;
    DecodeTime( dest1.scd_out, hours_o, mins_o, secs_o );
    DecodeTime( dest2.scd_in, hours_i, mins_i, secs_i );
    ssim::Section section(nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest1.airp, dest1.airp_fmt ))), //out
                          nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest2.airp, dest2.airp_fmt ))), //in
                          time_duration( hours_o + Qry.FieldAsInteger( "delta_out" ), mins_o, secs_o ), //out
                          time_duration( hours_i + Qry.FieldAsInteger( "delta_in" ), mins_i, secs_i )); //in
    boost::optional<ct::RbdLayout> rbd = ct::RbdLayout::fromString("CJZIDAYRSTEQGNBXWUOVHLK.C10Y100"); // HACK
    if (not rbd) throw EXCEPTIONS::Exception("NO RBD");
    ssim::Leg leg(section,
                  nsi::ServiceTypeId(0), // HACK
                  nsi::AircraftTypeId(CodeToId(ElemIdToClientElem( etCraft, dest1.craft, dest1.craft_fmt ))),
                  rbd.get());
    route.legs.push_back(leg);
  }
  return route;
}

//
ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd )
{
  LogTrace(TRACE5) << __func__ << " flt=" << flt << " prd=" << prd;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT move_id, first_day, last_day, days"
    " FROM sched_days "
    "WHERE flight=:flight AND first_day<=:last AND last_day>=:first "
    " ORDER BY num ";
  Qry.CreateVariable("flight", otString, SchedDaysFlight(flt));
  Qry.CreateVariable("first", otDate, BoostToDateTime(prd.start));
  Qry.CreateVariable("last", otDate, BoostToDateTime(prd.end));
  Qry.Execute();
  ssim::ScdPeriods scds;
  for (; !Qry.Eof; Qry.Next())
  {
    ssim::ScdPeriod scd(flt);
    scd.period = Period(DateTimeToBoost(Qry.FieldAsDateTime("first_day")).date(),
                        DateTimeToBoost(Qry.FieldAsDateTime("last_day")).date(),
                        Qry.FieldAsString("days"));
    scd.route = RouteFromDb(Qry.FieldAsInteger("move_id"));
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
  }
}

int HandleSSMTlg(string body)
{
  LogTrace(TRACE5) << __func__ << endl << body;
  InitSSIM();
  const auto ssm = ssim::parseSsm(body, nullptr);
  if (!ssm)
  {
    LogTrace(TRACE5) << "SSM NOT PARSED!";
    return -1;
  }
  const ssim::SsmStruct& ssmStr = *ssm;
  const ssim::ProcContext attr
  {
    nsi::CompanyId(0),
    false,
    ssmStr.head.timeIsLocal,
    false,
    std::make_unique<AstraSsimCallbacks>()
  };
  const std::vector<ssim::SsmSubmsgPtr>& submsgs = ssmStr.submsgs.begin()->second;
  ssim::CachedScdPeriods cache;
  for (const ssim::SsmSubmsgPtr& subMsg : submsgs)
  {
    if(const Message msg = subMsg->modify(cache, attr))
    {
      LogTrace(TRACE5) << msg;
      return -1;
    }
  }
  for (auto &rm : cache.forDeletion)
    DeleteScdPeriodsFromDb(rm.second);
  for (auto &sv : cache.forSaving())
    ScdPeriodsToDb(sv.second);
  return 0;
}

int ssim_test(int argc, char **argv)
{
//  Необходимо "зарегистрировать" парсеры (однократно где-нибудь в районе инициализации других глобальных сущностей):
//  typeb_template::addTemplate(new SsmTemplate());
//  typeb_template::addTemplate(new AsmTemplate());
//  см. sirena/src/airimp/typeb_init.cc (использование в sirena/src/obrzap.cc)
//  nsi::setupTestNsi();

  InitSSIM();

  std::ifstream t("file.txt");
  std::string str((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());

  cout << "Parsing SSM ..." << endl;
  cout << "---------------------------------------" << endl;

  const auto ssm = ssim::parseSsm(str, nullptr); // !!!

  if (!ssm)
  {
    cout << "SSM NOT PARSED!" << endl;
    return -1;
  }

  cout << ssm->toString() << endl;
  cout << "---------------------------------------" << endl;

  const ssim::SsmStruct& ssmStr = *ssm;
  // /home/user/sirena/src/ssim/proc_ssm.cc:438
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

  for (auto &rm : cache.forDeletion)
    DeleteScdPeriodsFromDb(rm.second);

  for (auto &sv : cache.forSaving())
    ScdPeriodsToDb(sv.second);

  return 0;
}

