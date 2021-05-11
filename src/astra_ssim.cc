#include "astra_ssim.h"

#define NICKNAME "GRISHA"
#define NICKTRACE GRISHA_TRACE
#include <serverlib/slogger.h>
const bool TRACE_ALL_FUNCTIONS = true;

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
#include "flt_binding.h"
#include "PgOraConfig.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>

using namespace BASIC::date_time;
using namespace std;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace SEASON;

#include "tlg/tlg_parser.h"
using namespace TypeB;

#include <serverlib/lngv_user.h>
#include <serverlib/lwriter.h>
#include <boost/scoped_ptr.hpp>

#define NICKNAME "GRISHA"
#define NICKTRACE GRISHA_TRACE
#include <serverlib/slogger.h>

using namespace nsi;

//------------------------------------------------------------------------------------------
ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd );
//------------------------------------------------------------------------------------------

int CodeToId(const std::string &code)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("CodeToId invalid sizeof");
  if (code.size() > 3) throw EXCEPTIONS::Exception("CodeToId invalid size=%d code='%s'", code.size(), code.c_str());
  const char *c = code.c_str();
  int id = 0;
  char* p = (char*)&id;
  for (unsigned int i = 0; i < code.size(); ++i) p[i] = c[i];
  return id;
}

// ���������: � ��砥 id = 10 ����砥��� code = ᨬ���� ��ॢ��� ��ப�
std::string IdToCode(const int id)
{
  // http://ru.cppreference.com/w/cpp/language/types
  if (sizeof(int) < sizeof(char)*3) throw EXCEPTIONS::Exception("IdToCode invalid sizeof");
  const char* p = (const char*)&id;
  char code[4];
  for (unsigned int i = 0; i < 3; ++i) code[i] = p[i];
  code[3] = '\0';
  std::string code_str(code);
  return code_str;
}

//------------------------------------------------------------------------------------------

// 1. ����������� ��������� ��⥬�-ᯥ��筮�� �।�⠢����� �ᯨᠭ��
// �/�� ssim::ScdPeriod (�. convertToSsim/makeScdPeriod)

//------------------------------------------------------------------------------------------

// 2. ����������� ssim::SsimTlgCallbacks
// ����室��� ���� SsimTlgCallbacks::getSchedules (�� ��ਮ��
// �ᯨᠭ�� �� ३�, ���ᥪ��騥��� � 㪠����� ��ਮ���)  � �����誨 ��
// �� ��⠫쭮�.

// getSchedules ���� ॠ��������
ssim::ScdPeriods AstraSsimCallbacks::getSchedules(const ct::Flight& flight, const Period& period) const
{
//  LogTrace(TRACE1) << __func__ << " flight=" << flight << " period=" << period;
  return ScdPeriodsFromDb( flight, period );
}

// ��⠫쭮� �����誨
ssim::ScdPeriods AstraSsimCallbacks::getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const
{
  return ssim::ScdPeriods();
}

Expected< ssim::PeriodicCshs > AstraSsimCallbacks::cshSettingsByTlg(nsi::CompanyId, const ssim::ScdPeriod&) const
{
  return ssim::PeriodicCshs();
}

Expected< ssim::PeriodicCshs > AstraSsimCallbacks::cshSettingsByScd(const ssim::ScdPeriod&) const
{
  return ssim::PeriodicCshs();
}

ssim::DefValueSetter AstraSsimCallbacks::prepareDefaultValueSetter(ct::DeiCode, const nsi::DepArrPoints&, bool byLeg) const
{
  return ssim::DefValueSetter();
}

//------------------------------------------------------------------------------------------

// 3. ����������� �-樨 �뢮�� �ॡ㥬�� ⨯�� � Message::bind (�.
// sirena/src/message.cc)

// ᪮��஢��� �� ��७�
namespace message_details {

template<> std::string translate_impl<nsi::CompanyId>(const UserLanguage& lang, const nsi::CompanyId& v)
{
  return nsi::Company(v).code(ENGLISH).toUtf();
}

template<> std::string translate_impl<boost::gregorian::date>(const UserLanguage& lang, const boost::gregorian::date& d)
{
  return Dates::ddmmrr(d);
}

template<> std::string translate_impl<Period>(const UserLanguage& lang, const Period& v)
{
  return translate_impl(lang, v.start) + "-" + translate_impl(lang, v.end) + " (" + translate_impl(lang, v.freq) + ")";
}

template<> std::string translate_impl<Freq>(const UserLanguage&, const Freq& v)
{
  return v.str();
}

template<> std::string translate_impl<ct::Flight>(const UserLanguage&, const ct::Flight& v)
{
  return nsi::Company(v.airline).code(ENGLISH).toUtf()
       + '-' + StrUtils::LPad(std::to_string(v.number.get()), 3, '0')
       + ct::suffixToString(v.suffix, ENGLISH);
}

} // message_details

//------------------------------------------------------------------------------------------

// 4. �� ����室����� ��������樨 �/��� �������⥫쭮�� �८�ࠧ������
// ᮮ�饭�� �� �訡��� ॠ�������� message_details::Localization.

//------------------------------------------------------------------------------------------

string GetElemId(TElemType type, const string& elem)
{
  string type_encode = EncodeElemType(type);
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " type = '" << type_encode << "' elem = '" << elem << "'";
  switch (type)
  {
    case etCountry: type_encode = "Country"; break;
    case etCity: type_encode = "City"; break;
    case etAirp: type_encode = "Airport"; break;
    case etAirline: type_encode = "Airline"; break;
    case etCraft: type_encode = "Craft"; break;
    case etSuffix: type_encode = "Suffix"; break;
    default: break;
  }
  TElemFmt fmt;
  string result = ElemToElemId(type, elem, fmt);
  if (fmt == efmtUnknown)
    throw EXCEPTIONS::Exception("%s not found: '%s'", type_encode.c_str(), elem.c_str());
  return result;
}

// �� �⭮���� � ॠ����樨 nsi::Callbacks (��த�/����� � �.�.)
// ��. sirena/src/nsi_callbacks.(h + cc)
// � �ਭ樯� ����� ��� 楫�� SSIM ��࠭������ �����誠��/����⠬� ���:
// nsi::DocTypeId
// nsi::SsrTypeId
// nsi::GeozoneId
// nsi::RouterId
// nsi::CurrencyId
// nsi::OrganizationId
// � ᮮ⢥������ �� *Data.
// � ��⠫�� �-樨 �ਤ���� ॠ��������.
// ���樠������ ����� �� ������⭠� �� ����, �. nsi::init_callbacks (sirena/src/nsi_callbacks.cc + obrzap.cc)

// ���⮢�� ॠ������ nsi::Callbacks 㦥 ���� � libnsi/src/callbacks.cc (nsi::TestCallbacks).

bool AstraCallbacks::needCheckVersion() const
{
//  ��뢠���� �� 5 ࠧ � ०��� ��������
  return false;
}

// ��� SSM ॠ�����뢠�� �� ��易⥫쭮
nsi::CityId AstraCallbacks::centerCity() const
{
  return nsi::CityId(0);
}

// /home/user/Sirena/sirenalibs/libnsi/src/nsi.h

// �����誨
boost::optional<nsi::DocTypeId> AstraCallbacks::findDocTypeId(const EncString& code)
{
  return boost::optional<nsi::DocTypeId>();
}

boost::optional<DocTypeData> AstraCallbacks::findDocTypeData(const nsi::DocTypeId& id)
{
  // explicit DocTypeData(const DocTypeId& id);
  return DocTypeData(id);
}

// �����誨
boost::optional<nsi::SsrTypeId> AstraCallbacks::findSsrTypeId(const EncString& code)
{
  return boost::optional<nsi::SsrTypeId>();
}

boost::optional<SsrTypeData> AstraCallbacks::findSsrTypeData(const nsi::SsrTypeId& id)
{
  // explicit SsrTypeData(const SsrTypeId&);
  return SsrTypeData(id);
}

// �����誨
boost::optional<nsi::GeozoneId> AstraCallbacks::findGeozoneId(const EncString& code)
{
  return boost::optional<nsi::GeozoneId>();
}

boost::optional<GeozoneData> AstraCallbacks::findGeozoneData(const nsi::GeozoneId& id)
{
  // explicit GeozoneData(const GeozoneId&);
  return GeozoneData(id);
}

// country !!! done
boost::optional<nsi::CountryId> AstraCallbacks::findCountryId(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::CountryId(CodeToId(code.to866()));
}

boost::optional<nsi::CountryId> AstraCallbacks::findCountryIdByIso(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::CountryId(CodeToId(code.to866())); // ???
}

boost::optional<CountryData> AstraCallbacks::findCountryData(const nsi::CountryId& id)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << IdToCode(id.get()) << "'";
  // explicit CountryData(const CountryId&, const GeozoneId&, const CurrencyId&);
  CountryData data(id, CurrencyId(0));
  string country_code = GetElemId(etCountry, IdToCode(id.get()));
  const TCountriesRow &countryRow = (const TCountriesRow&)base_tables.get("countries").get_row("code", country_code);
  data.isoCode = countryRow.code_iso;
  data.codes[ENGLISH] = EncString::from866(countryRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(countryRow.code);
  data.names[ENGLISH] = EncString::from866(countryRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(countryRow.name);
  return data;
}

// region - ��� ��ࠡ�⪨ SSM �� �㦭�.
boost::optional<nsi::RegionId> AstraCallbacks::findRegionId(const EncString& code)
{
  return nsi::RegionId(0);
}

boost::optional<RegionData> AstraCallbacks::findRegionData(const nsi::RegionId& id)
{
  // explicit RegionData(const RegionId&, const CountryId&);
  RegionData data(id, CountryId(0));
  return data;
}

// city !!! done
boost::optional<nsi::CityId> AstraCallbacks::findCityId(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::CityId(CodeToId(code.to866()));
}

boost::optional<CityData> AstraCallbacks::findCityData(const nsi::CityId& id)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " id = '" << id.get() << "' code = '" << IdToCode(id.get()) << "'";
  // CityData(const CityId&, const CountryId&, const boost::optional<RegionId>&);
  string city_code = GetElemId(etCity, IdToCode(id.get()));
  const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code", city_code);
  CityData data(id, CountryId(CodeToId(cityRow.country)), boost::optional<RegionId>());
  data.codes[ENGLISH] = EncString::from866(cityRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(cityRow.code);
  data.names[ENGLISH] = EncString::from866(cityRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(cityRow.name);
  data.timezone = cityRow.tz_region;
  return data;
}

// point !!! done
boost::optional<nsi::PointId> AstraCallbacks::findPointId(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::PointId(CodeToId(code.to866()));
}

boost::optional<PointData> AstraCallbacks::findPointData(const nsi::PointId& id)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << IdToCode(id.get()) << "'";
  string airp_code = GetElemId(etAirp, IdToCode(id.get()));
  const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code", airp_code);
  // explicit PointData(const PointId&, const CityId&);
  PointData data(id, CityId(CodeToId(airpsRow.city)));
  data.codes[ENGLISH] = EncString::from866(airpsRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(airpsRow.code);
  data.names[ENGLISH] = EncString::from866(airpsRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(airpsRow.name);
  return data;
}

// company !!! done
boost::optional<nsi::CompanyId> AstraCallbacks::findCompanyId(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<nsi::CompanyId> AstraCallbacks::findCompanyIdByAccountCode(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::CompanyId(CodeToId(code.to866()));
}

boost::optional<CompanyData> AstraCallbacks::findCompanyData(const nsi::CompanyId& id)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << IdToCode(id.get()) << "'";
  // explicit CompanyData(const CompanyId&);
  CompanyData data(id);
  string airline_code = GetElemId(etAirline, IdToCode(id.get()));
  const TAirlinesRow &airlineRow = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", airline_code);
  data.codes[ENGLISH] = EncString::from866(airlineRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(airlineRow.code);
  data.names[ENGLISH] = EncString::from866(airlineRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(airlineRow.name);
  return data;
}

// aircraft !!! done
boost::optional<nsi::AircraftTypeId> AstraCallbacks::findAircraftTypeId(const EncString& code)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << code.to866() << "'";
  return nsi::AircraftTypeId(CodeToId(code.to866()));
}

boost::optional<AircraftTypeData> AstraCallbacks::findAircraftTypeData(const nsi::AircraftTypeId& id)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " code = '" << IdToCode(id.get()) << "'";
  // explicit AircraftTypeData(const AircraftTypeId&);
  AircraftTypeData data(id);
  string craft_code = GetElemId(etCraft, IdToCode(id.get()));
  const TCraftsRow &craftRow = (const TCraftsRow&)base_tables.get("crafts").get_row("code", craft_code);
  data.codes[ENGLISH] = EncString::from866(craftRow.code_lat);
  data.codes[RUSSIAN] = EncString::from866(craftRow.code);
  data.names[ENGLISH] = EncString::from866(craftRow.name_lat);
  data.names[RUSSIAN] = EncString::from866(craftRow.name);
  return data;
}

// �����誨
boost::optional<RouterId> AstraCallbacks::findRouterId(const EncString& code)
{
  return boost::optional<RouterId>();
}

boost::optional<RouterData> AstraCallbacks::findRouterData(const nsi::RouterId& id)
{
  // RouterData(const RouterId& );
  return RouterData(id);
}

// �����誨
boost::optional<CurrencyId> AstraCallbacks::findCurrencyId(const EncString& code)
{
  return boost::optional<CurrencyId>();
}

boost::optional<CurrencyData> AstraCallbacks::findCurrencyData(const nsi::CurrencyId& id)
{
  // explicit CurrencyData(const CurrencyId&);
  return CurrencyData(id);
}

// �����誨
std::set<GeozoneId> AstraCallbacks::getGeozones(const CountryId&)
{
    return {};
}

std::set<GeozoneId> AstraCallbacks::getGeozones(const RegionId&)
{
    return {};
}

std::set<GeozoneId> AstraCallbacks::getGeozones(const CityId&)
{
    return {};
}


//------------------------------------------------------------------------------------------

void AstraSsimParseCollector::appendFlight(const ct::Flight& f)
{
    flt = f;
}

void AstraSsimParseCollector::appendPeriod(const ct::Flight& f, const Period& p)
{
    flt = f;
}
//------------------------------------------------------------------------------------------

string GetAirline(ct::Flight flight)
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " airline = '" << IdToCode(flight.airline.get()) << "'";
  return GetElemId(etAirline, IdToCode(flight.airline.get()));
}

int GetFltNo(ct::Flight flight)
{
  return flight.number.get();
}

string GetSuffix(ct::Flight flight)
{
  string s = ct::suffixToString(flight.suffix); // lang = ENGLISH
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " suffix = '" << s << "'";
  if (s.empty())
    return s;
  else
    return GetElemId(etSuffix, s);
}

string FlightToString(ct::Flight f)
{
  return GetAirline(f) + IntToString(GetFltNo(f)) + GetSuffix(f);
}

TDateTime BoostToDateTimeCorrectInfinity(boost::gregorian::date date)
{
  // TODO ��筨�� �����⨬���� ������ neg_infin/pos_infin �� min_date_time/max_date_time
  // https://www.boost.org/doc/libs/1_55_0/doc/html/date_time/gregorian.html#date_time.gregorian.date_class
  boost::gregorian::date result = date;
  if (date.is_pos_infinity())
    result = boost::gregorian::date(boost::gregorian::max_date_time);
  else if (date.is_neg_infinity())
    result = boost::gregorian::date(boost::gregorian::min_date_time);
  return BoostToDateTime(result);
}

// ������� �� �� ����������

void DeleteScdPeriodsFromDb( const std::set<ssim::ScdPeriod> &scds )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeLocalAirp; // ��⠭���� �� ࠡ�祬
//  reqInfo->desk.code = "MOVGRG"; // 㤠����
  const bool pg_enabled = PgOra::supportsPg("SCHED_DAYS");
  for (auto &scd : scds)
  {
    if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " scd = " << scd;
    const TDateTime first = BoostToDateTime(scd.period.start);
    const TDateTime last = BoostToDateTime(scd.period.end);
    const auto scd_flight = FlightToString(scd.flight);

    if (pg_enabled) {
      DbCpp::CursCtl del1 = make_db_curs("DELETE FROM routes WHERE move_id IN ( "
                          " SELECT move_id FROM sched_days WHERE ssm_id IN ( "
                          " SELECT ssm_id FROM ssm_schedule "
                          " WHERE flight=:flight AND DATE_TRUNC('day', first)=DATE_TRUNC('day', :first) "
                          " AND DATE_TRUNC('day', last) = DATE_TRUNC('day', :last) ) )",
                          PgOra::getRWSession("ROUTES"));
      del1.
        bind(":flight", scd_flight).
        bind(":first", scd.period.start).
        bind(":last", scd.period.end).
        exec();

      DbCpp::CursCtl del2 = make_db_curs("DELETE FROM sched_days WHERE ssm_id IN ( "
                              " SELECT ssm_id FROM ssm_schedule "
                              " WHERE flight=:flight AND DATE_TRUNC('day', first) = DATE_TRUNC('day', :first) "
                              " AND DATE_TRUNC('day', last) = DATE_TRUNC('day', :last) )",
                              PgOra::getRWSession("ROUTES"));
      del2.
        bind(":flight", scd_flight).
        bind(":first", scd.period.start).
        bind(":last", scd.period.end).
        exec();

      DbCpp::CursCtl del3 = make_db_curs("DELETE FROM ssm_schedule "
                          " WHERE flight=:flight AND DATE_TRUNC('day', first) = DATE_TRUNC('day', :first) "
                          " AND DATE_TRUNC('day', last) = DATE_TRUNC('day', :last)",
                          PgOra::getRWSession("ROUTES"));
      del3.
        bind(":flight", scd_flight).
        bind(":first", scd.period.start).
        bind(":last", scd.period.end).
        exec();
    } else {
      TQuery Qry(&OraSession);
      Qry.SQLText =
          "BEGIN "
          "DELETE FROM routes WHERE move_id IN ( "
          " SELECT move_id FROM sched_days WHERE ssm_id IN ( "
          " SELECT ssm_id FROM ssm_schedule "
          " WHERE flight=:flight AND TRUNC(first)=TRUNC(:first) AND TRUNC(last)=TRUNC(:last) ) ); "
          "DELETE FROM sched_days WHERE ssm_id IN ( "
          " SELECT ssm_id FROM ssm_schedule "
          " WHERE flight=:flight AND TRUNC(first)=TRUNC(:first) AND TRUNC(last)=TRUNC(:last) ); "
          "DELETE FROM ssm_schedule "
          " WHERE flight=:flight AND TRUNC(first)=TRUNC(:first) AND TRUNC(last)=TRUNC(:last); "
          "END;";
      Qry.CreateVariable("flight", otString, scd_flight);
      Qry.CreateVariable("first", otDate, first);
      Qry.CreateVariable("last", otDate, last);
      Qry.Execute();
    }
  } // scd
}

// �஢�ઠ �� ࠧ���� ����� �६���� �ਫ�� � �뫥�
/*
void CheckDepArrTimeEqual( const ssim::ScdPeriod &scd )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeLocalAirp;
  TFilter filter;
  filter.GetSeason();
  bool first_airp = true;
  TPeriod p;
  p.move_id = ASTRA::NoExists;
  p.first = BoostToDateTime(scd.period.start);
  p.last = BoostToDateTime(scd.period.end);
  p.days = scd.period.freq.str();
  p.modify = ASTRA::finsert;
  TDest curr, next;
  for (const auto leg : scd.route.legs)
  {
    curr = next;
    curr.airp = ElemToElemId( etAirp, IdToCode(leg.s.from.get()), curr.airp_fmt );
    int shift_day = leg.s.dep.hours() / 24;
    EncodeTime( leg.s.dep.hours() % 24, leg.s.dep.minutes(), 0, curr.scd_out );
    if (first_airp)
    {
      filter.filter_tz_region = AirpTZRegion(curr.airp);
      first_airp = false;
      SEASON::ConvertPeriod( p, curr.scd_out, filter.filter_tz_region );
    }
    curr.scd_out = ConvertFlightDate(curr.scd_out, p.first, curr.airp, false, mtoUTC) + shift_day;
    next.airp = ElemToElemId( etAirp, IdToCode(leg.s.to.get()), next.airp_fmt );
    shift_day = leg.s.arr.hours() / 24;
    EncodeTime( leg.s.arr.hours() % 24, leg.s.arr.minutes(), 0, next.scd_in );
    next.scd_in = ConvertFlightDate(next.scd_in, p.first, next.airp, true, mtoUTC) + shift_day;
    // TODO �஢����� �� ⮫쪮 ��� ������ ����, � ��� ��� �।������� �६� �ਫ�� � �뫥� !!!
    if (next.scd_in <= curr.scd_out)
    {
      ostringstream s;
      s << "Arrival time <= Departure time on leg '"
        << IdToCode(leg.s.from.get()) << "-" << IdToCode(leg.s.to.get()) << "'";
      throw EXCEPTIONS::Exception("%s", s.str().c_str());
    }
  }
}
*/

// �������� � �� ����������

void ScdPeriodToDb( const ssim::ScdPeriod &scd )
{
  int ssm_id = PgOra::getSeqNextVal("SSM_ID");
  if (TRACE_ALL_FUNCTIONS)
    LogTrace(TRACE1) << __func__ << " ssm_id = " << ssm_id << " scd = " << scd;
  TReqInfo *reqInfo = TReqInfo::Instance();
  /*???if ( utc ) {
    reqInfo->user.sets.time = ustTimeUTC;
  }
  else {*/
    reqInfo->user.sets.time = ustTimeLocalAirp; // ��⠭���� �� ࠡ�祬
  /*}*/
//  reqInfo->desk.code = "MOVGRG"; // 㤠����
  string flight = FlightToString(scd.flight);
  TDateTime first = BoostToDateTime(scd.period.start);
  TDateTime last = BoostToDateTime(scd.period.end);
  string days = scd.period.freq.str();
  // ������ � ssm_schedule
  DB::TQuery QryIns(PgOra::getRWSession("SSM_SCHEDULE"), STDLOG);
  QryIns.SQLText =
      "INSERT INTO ssm_schedule(ssm_id, flight, first, last, days) "
      " VALUES(:ssm_id, :flight, :first, :last, :days) ";
  QryIns.CreateVariable("ssm_id", otInteger, ssm_id);
  QryIns.CreateVariable("flight", otString, flight);
  QryIns.CreateVariable("first", otDate, first);
  QryIns.CreateVariable("last", otDate, last);
  QryIns.CreateVariable("days", otString, days);
  QryIns.Execute();

  // ������ � sched_days
  auto SchdQry = DB::TQuery(PgOra::getROSession("SCHED_DAYS"), STDLOG);
  SchdQry.SQLText = "SELECT trip_id FROM sched_days s, ssm_schedule m WHERE m.flight=:flight and s.ssm_id=m.ssm_id FETCH FIRST 1 ROWS ONLY";
  SchdQry.CreateVariable("flight", otString, flight);
  SchdQry.Execute();
  int trip_id;
  if (!SchdQry.Eof)
    trip_id = SchdQry.FieldAsInteger("trip_id");
  else
    trip_id = ASTRA::NoExists;
  vector<TPeriod> speriods; //- ��ਮ�� �믮������
  map<int,TDestList> mapds; //move_id,�������
  TFilter filter;
  filter.GetSeason();
  bool first_airp = true;
  TPeriod p;
  p.move_id = ASTRA::NoExists;
  p.first = first;
  p.last = last;
  p.days = days;
  p.modify = ASTRA::finsert;
  TDestList dests;
  TDest curr, next;
  for (ssim::Legs::const_iterator ileg = scd.route.legs.begin(); ileg != scd.route.legs.end(); ++ileg)
  {
    const ssim::Leg &leg = *ileg;
//    LogTrace(TRACE1) << "serviceType = " << leg.serviceType.get();
    curr = next;
    curr.airline = ElemToElemId( etAirline, IdToCode(scd.flight.airline.get()), curr.airline_fmt );
    curr.trip = scd.flight.number.get();
    curr.suffix = ElemToElemId( etSuffix, ct::suffixToString(scd.flight.suffix), curr.suffix_fmt );
    curr.airp = ElemToElemId( etAirp, IdToCode(leg.s.from.get()), curr.airp_fmt );
    curr.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), curr.craft_fmt );
    curr.triptype = DefaultTripType(false);

    time_duration td_dep = leg.s.dep; // departure
    // ��� ��ࢮ�� �㭪� ������� ᤢ�� ���� �� ࠧ���, �� ����� �������� ����⥫�� ᤢ�� - ����ࠡ�⪠ libssim
    if (first_airp && td_dep < time_duration(0,0,0,0))
      throw EXCEPTIONS::Exception("negative date shift in first point of route is not allowed");
    // ��� SSM �।�ᬮ�७ ���ᨬ���� ����⥫�� ᤢ�� ���� �� 1 ��⪨ ����� (/M1), �� ����� �������� � ����� - ����ࠡ�⪠ libssim
    if (td_dep < time_duration(-24,0,0,0)) throw EXCEPTIONS::Exception("departure: maximum negative shift is 1 day");
    bool dep_neg = td_dep < time_duration(0,0,0,0);
    int shift_day = dep_neg ? -1 : td_dep.hours() / 24; // ᤢ�� ���� (�६� ���� KJA1400/1)
    LogTrace(TRACE5) << "EncodeTime dep: hours=" << td_dep.hours() << " minutes=" << td_dep.minutes() << " shift_day=" << shift_day;
    if (dep_neg)
    {
      td_dep = td_dep + time_duration(24,0,0,0);
      LogTrace(TRACE5) << "EncodeTime dep_neg: hours=" << td_dep.hours() << " minutes=" << td_dep.minutes() << " shift_day=" << shift_day;
    }
    EncodeTime( td_dep.hours() % 24, td_dep.minutes(), 0, curr.scd_out );
    if (first_airp)
    {
      filter.filter_tz_region = AirpTZRegion(curr.airp);
      first_airp = false;
      p.delta = SEASON::ConvertPeriod( p, curr.scd_out, filter.filter_tz_region ); //���뢥���� ��ਮ��
      if ( p.delta  ) {
        LogTrace(TRACE5) << "shift to prior day";
      }
    }
    curr.scd_out = ConvertFlightDate(curr.scd_out, p.first, curr.airp, false, mtoUTC);
    if (dep_neg) curr.scd_out = -curr.scd_out;
    curr.scd_out += shift_day;

    next.airp = ElemToElemId( etAirp, IdToCode(leg.s.to.get()), next.airp_fmt );
    next.craft = ElemToElemId( etCraft, IdToCode(leg.aircraftType.get()), next.craft_fmt );
    next.triptype = DefaultTripType(false);

    time_duration td_arr = leg.s.arr; //arrival
    if (td_arr < time_duration(-24,0,0,0)) throw EXCEPTIONS::Exception("arrival: maximum negative shift is 1 day");
    bool arr_neg = td_arr < time_duration(0,0,0,0);
    shift_day = arr_neg ? -1 : td_arr.hours() / 24;
    LogTrace(TRACE5) << "EncodeTime arr: hours=" << td_arr.hours() << " minutes=" << td_arr.minutes() << " shift_day=" << shift_day;
    if (arr_neg)
    {
      td_arr = td_arr + time_duration(24,0,0,0);
      LogTrace(TRACE5) << "EncodeTime arr_neg: hours=" << td_arr.hours() << " minutes=" << td_arr.minutes() << " shift_day=" << shift_day;
    }
    EncodeTime( td_arr.hours() % 24, td_arr.minutes(), 0, next.scd_in );
    next.scd_in = ConvertFlightDate(next.scd_in, p.first, next.airp, true, mtoUTC);
    if (arr_neg) next.scd_in = -next.scd_in;
    next.scd_in += shift_day;

    curr.rbd_order = leg.subclOrder.rbdOrder().toString();
    for (auto &cfg : leg.subclOrder.config())
      if (cfg.second)
      {
        if (ct::cabinCode(cfg.first)=="F") curr.f = cfg.second.get();
        if (ct::cabinCode(cfg.first)=="C") curr.c = cfg.second.get();
        if (ct::cabinCode(cfg.first)=="Y") curr.y = cfg.second.get();
      }
    dests.dests.push_back( curr );
  } // ileg
  next.airline.clear();
  next.trip = ASTRA::NoExists;
  next.suffix.clear();
  next.craft.clear();
  next.triptype.clear();
  dests.dests.push_back( next );
  mapds.insert( make_pair(ASTRA::NoExists, dests) );
  speriods.push_back( p );
  //!!!UTC
  int_write( filter, ssm_id, speriods, trip_id, mapds );
}

ssim::Route RouteFromDb(int move_id, TDateTime first)
{
  // ��筨�� �ᯮ�짮����� delta_in, delta_out
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " move_id = " << move_id;
  TReqInfo *reqInfo = TReqInfo::Instance(); // ??? 㦥 ���� � ��뢠�饩 �㭪樨
  reqInfo->user.sets.time = ustTimeLocalAirp; // ��⠭���� �� ࠡ�祬
//  reqInfo->desk.code = "MOVGRG"; // 㤠����
  DB::TQuery Qry(PgOra::getROSession("ROUTES"), STDLOG);
  Qry.SQLText =
    "SELECT num,airp,airp_fmt,flt_no,suffix,scd_in,craft,craft_fmt,scd_out,delta_in,delta_out,f,c,y,rbd_order "
    " FROM routes WHERE move_id=:move_id "
    " ORDER BY num";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  ssim::Route route;
  TDest dest1, dest2;
  int dest1_delta_out = 0;
  int dest2_delta_in = 0, dest2_delta_out = 0;
  string rbd_string;
  for (; !Qry.Eof; Qry.Next())
  {
    dest1 = dest2;
    dest1_delta_out = dest2_delta_out;

    dest2.num = Qry.FieldAsInteger( "num" );
    dest2.airp = Qry.FieldAsString( "airp" );
    dest2.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );

    if ( Qry.FieldIsNULL( "scd_in" ) )
      dest2.scd_in = ASTRA::NoExists;
    else
      dest2.scd_in = ConvertFlightDate(Qry.FieldAsDateTime("scd_in"), first, dest2.airp, true, mtoLocal);

    if (Qry.FieldIsNULL("delta_in"))
      dest2_delta_in = 0;
    else
      dest2_delta_in = Qry.FieldAsInteger("delta_in");

    dest2.craft = Qry.FieldAsString( "craft" );
    dest2.craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );

    if ( Qry.FieldIsNULL( "scd_out" ) )
      dest2.scd_out = ASTRA::NoExists;
    else
      dest2.scd_out = ConvertFlightDate(Qry.FieldAsDateTime("scd_out"), first, dest2.airp, false, mtoLocal);

    if (Qry.FieldIsNULL("delta_out"))
      dest2_delta_out = 0;
    else
      dest2_delta_out = Qry.FieldAsInteger("delta_out");

    if ( dest1.num == ASTRA::NoExists )
    {
      rbd_string = Qry.FieldAsString("rbd_order") + string(".") +
          string("F") + IntToString(Qry.FieldAsInteger("f")) +
          string("C") + IntToString(Qry.FieldAsInteger("c")) +
          string("Y") + IntToString(Qry.FieldAsInteger("y"));
      continue;
    }
    //leg
    if (dest1.scd_out == ASTRA::NoExists) throw EXCEPTIONS::Exception("dest1.scd_out == ASTRA::NoExists");
    if (dest2.scd_in == ASTRA::NoExists) throw EXCEPTIONS::Exception("dest2.scd_in == ASTRA::NoExists");
    double days_o, days_i;
    modf(dest1.scd_out, &days_o);
    modf(dest2.scd_in, &days_i);
    int hours_o, mins_o, secs_o;
    int hours_i, mins_i, secs_i;
    DecodeTime( dest1.scd_out, hours_o, mins_o, secs_o );
    DecodeTime( dest2.scd_in, hours_i, mins_i, secs_i );
    time_duration td_out( hours_o, mins_o, secs_o );
    time_duration td_in( hours_i, mins_i, secs_i );
    td_out = td_out + hours(24 * (static_cast<int>(days_o) + dest1_delta_out));
    td_in = td_in + hours(24 * (static_cast<int>(days_i) + dest2_delta_in));
    ssim::Section section(nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest1.airp, dest1.airp_fmt ))), //out
                          nsi::PointId(CodeToId(ElemIdToClientElem( etAirp, dest2.airp, dest2.airp_fmt ))), //in
                          td_out,
                          td_in);
    boost::optional<ct::RbdLayout> rbd = ct::RbdLayout::fromString(rbd_string);
    if (not rbd) throw EXCEPTIONS::Exception("Cannot make RBD from string '%s'", rbd_string.c_str());
    ssim::Leg leg(section,
                  nsi::ServiceTypeId(0), // HACK
                  nsi::AircraftTypeId(CodeToId(ElemIdToClientElem( etCraft, dest1.craft, dest1.craft_fmt ))),
                  rbd.get());
    route.legs.push_back(leg);
  }
  return route;
}

// �������� �� �� ����������, �������������� � ��������

ssim::ScdPeriods ScdPeriodsFromDb( const ct::Flight& flt, const Period& prd )
{
  if (TRACE_ALL_FUNCTIONS) LogTrace(TRACE1) << __func__ << " flt = " << flt << " prd = " << prd;
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeLocalAirp; // ��⠭���� �� ࠡ�祬
//  reqInfo->desk.code = "MOVGRG"; // 㤠����
  // �᫨ � ��ਮ�� ��� ��ன ����, � end = pos_infin
  TDateTime start = BoostToDateTimeCorrectInfinity(prd.start);
  TDateTime end = BoostToDateTimeCorrectInfinity(prd.end);
  DB::TQuery Qry(PgOra::getROSession("SCHED_DAYS"), STDLOG);
  Qry.SQLText =
      "SELECT s.move_id move_id, m.first as first, m.last as last, m.days days "
      " FROM sched_days s, ssm_schedule m "
      " WHERE m.flight = :flight AND m.first <= :last AND m.last >= :first AND s.ssm_id = m.ssm_id ";
  Qry.CreateVariable("flight", otString, FlightToString(flt));
  Qry.CreateVariable("first", otDate, start);
  Qry.CreateVariable("last", otDate, end);
  Qry.Execute();
  ssim::ScdPeriods scds;
  for (; !Qry.Eof; Qry.Next())
  {
    TDateTime first = Qry.FieldAsDateTime("first");
    TDateTime last = Qry.FieldAsDateTime("last");
    string days = Qry.FieldAsString("days");
    ssim::ScdPeriod scd(flt);
    scd.period = Period(DateTimeToBoost(first).date(),
                        DateTimeToBoost(last).date(),
                        days);
    double date_for_route;
    modf(first, &date_for_route);
    scd.route = RouteFromDb(Qry.FieldAsInteger("move_id"), date_for_route);
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

void HandleSSMTlg(string body, int tlg_id, TypeB::TFlightsForBind& flightsForBind)
{
  flightsForBind.clear();
  // RSD /home/user/sirena/src/ssim/proc_ssm.cc:503
  // ��४��⨥ ��ਮ��� /home/user/sirena/src/ssim/proc_ssm.cc:555
  // ��筨�� arc.appendFlight(ssmStr.submsgs.begin()->first); /home/user/sirena/src/ssim/proc_ssm.cc:440
  try
  {
    InitSSIM();
    bool need_write = true;
    AstraSsimParseCollector collector;
    LogTrace(TRACE5) << __func__ << " Parsing SSM begin";
    boost::scoped_ptr<CutLogLevel> pLLHolder(new CutLogLevel(1));
    const auto ssm = ssim::parseSsm(body, &collector);
    pLLHolder.reset();
    LogTrace(TRACE5) << __func__ << " Parsing SSM complete";
    if (collector.flt)
    {
      LogTrace(TRACE5) << "SSM TLG FLIGHT = " << FlightToString(*collector.flt);
      string airline = GetAirline(*collector.flt);
      int flt_no = GetFltNo(*collector.flt);
      string suffix = GetSuffix(*collector.flt);
      TFltInfo flt;
      strcpy(flt.airline, airline.substr(0,3).c_str());
      flt.flt_no = flt_no;
      strcpy(flt.suffix, suffix.substr(0,1).c_str());
      flightsForBind.emplace_back(flt, btNone);
    }

    if (!ssm)
    {
      string m = string("parse: ") + ssm.err().toString(UserLanguage::en_US());
      throw ETlgError(tlgeYesMonitorNotAlarm, m);
    }

    const ssim::SsmStruct& ssmStr = *ssm;

    if (ssmStr.specials.size() > 1)
      LogTrace(TRACE5) << "SSM: Several ACK/NAC inside SSM";

    for (const ssim::SsmSubmsgPtr& spec : ssmStr.specials)
    {
      if (spec->type() == ssim::SSM_ACK)
      {
        LogTrace(TRACE5) << "SSM: ACK received";
        need_write = false;
      }
      if (spec->type() == ssim::SSM_NAC)
      {
        // ������������� ����� �� ���� ����������
        LogTrace(TRACE5) << "SSM: NAC received";
        need_write = false;
      }
    }

    // ��������� ������ � ��� �� ��������������
    if (ssmStr.submsgs.size() > 1)
    {
      throw ETlgError(tlgeYesMonitorNotAlarm, "Multiple flights is not supported");
    }

    for (const auto& v : ssmStr.submsgs)
      for (const ssim::SsmSubmsgPtr& s : v.second)
        if (s->type() == ssim::SSM_RSD)
        {
          LogTrace(TRACE5) << "SSM: RSD received";
          need_write = false;
        }

    if (need_write)
    {
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

      for (const ssim::SsmSubmsgPtr& subMsg : submsgs)
        if (const Message msg = subMsg->modify(cache, attr))
        {
          string m = string("modify: ") + msg.toString(UserLanguage::en_US());
          throw ETlgError(tlgeYesMonitorNotAlarm, m);
        }

      // std::map<ct::Flight, std::set<ssim::ScdPeriod> > forDeletion;
      for (auto &rm : cache.forDeletion)
        DeleteScdPeriodsFromDb(rm.second);

      // std::map<ct::Flight, ssim::ScdPeriods> forSaving() const;
      // using ScdPeriods = std::vector<ScdPeriod>;
      for (auto &sv : cache.forSaving())
        for (auto &scd : sv.second)
          ScdPeriodToDb(scd);
    }
  }
  catch (...)
  {
    LogTrace(TRACE5) << "SSM TLG EXCEPTION";
    throw;
  }
  for(const TypeB::TFltForBind& i : flightsForBind)
  {
    int point_id = i.flt_info.getPointId(i.bind_type).first;
    LogTrace(TRACE5) << "SSM TLG OK, point_id = " << point_id;
    TlgSource(point_id, tlg_id, false, false).toDB();
  }
}

//------------------------------------------------------------------------------------------
#if 0
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
#endif
