#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include "xml_unit.h"
#include "oralib.h"
#include "astra_consts.h"
#include "seats.h"
#include "seats_utils.h"
#include "salons.h"
#include "exceptions.h"
#include "base_tables.h"
#include "flt_settings.h"
#include "salons.h"
#include "astra_misc.h"
#include "SeatsPax.h"
#include "seat_number.h"


#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC::date_time;

namespace SEATSPAX {

int TSeatPax::getPointIdByPointTlg( const PointIdTlg_t& point_id )
{
  std::set<PointId_t> point_ids_spp;
  getPointIdsSppByPointIdTlg(point_id, point_ids_spp);
  return point_ids_spp.empty()?ASTRA::NoExists:point_ids_spp.begin()->get();
}

bool TSeatPax::getFlightSetsPrLatSeat( const PointId_t& point_id )
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  return getTripSetsPrLatSeat(point_id.get());
}

bool TSeatPax::getFlightSetsPrFreeSeating( const PointId_t& point_id )
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  return SALONS2::isFreeSeating(point_id.get());
}

BASIC_SALONS::TCompLayerTypes::Enum TSeatPax::getCompLayerPriorityFlag(const PointId_t& point_id)
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  TTripInfo fltInfo;
  fltInfo.getByPointId(point_id.get());
  return (GetTripSets( tsAirlineCompLayerPriority, fltInfo )?
                         BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                         BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
}

TFilterLayers TSeatPax::getCompLayerFilter(const PointId_t& point_id)
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  TFilterLayers filter;
  filter.getFilterLayers(point_id.get());
  return filter;
}

std::string TSeatPax::getFltAirline(const PointId_t& point_id)
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  TTripInfo fltInfo;
  fltInfo.getByPointId(point_id.get());
  return fltInfo.airline;
}

void TSeatPax::getTlgCompLayer( const PaxId_t& pax_id,
                                const PointId_t& point_id,
                                TTotalRanges& total_ranges )
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  //кэшировать для списков!
  BASIC_SALONS::TCompLayerTypes *layerInst = BASIC_SALONS::TCompLayerTypes::Instance();
  BASIC_SALONS::TCompLayerTypes::Enum flag = getCompLayerPriorityFlag(point_id);
  TFilterLayers filter = getCompLayerFilter(point_id);
  //конец кэшировать для списков!

  DB::TQuery CRSQry(PgOra::getROSession({"TLG_COMP_LAYERS"}), STDLOG);
  //здесь все как в salons, но не очень правильно. Хорошо бы сделать анализ на АП прилета и маршрута - возможно слой инвалидный,
  // ид. вылета рейса соответствует tlg_comp_layers.point_id ?
  //упрощаем )
  CRSQry.SQLText =
    "SELECT :point_id point_id, :point_id point_dep, "
    "       layer_type,first_xname,last_xname,first_yname,last_yname "
    " FROM tlg_comp_layers "
    "WHERE crs_pax_id=:pax_id "
    "ORDER BY first_yname, first_xname";
  CRSQry.CreateVariable( "point_id", otInteger, point_id.get() );
  CRSQry.CreateVariable( "pax_id", otInteger, pax_id.get() );
  CRSQry.Execute();
  if ( CRSQry.Eof ) return;
  int col_point_id = CRSQry.GetFieldIndex( "point_id" );
  int col_layer_type = CRSQry.FieldIndex( "layer_type" );
  int col_time_create = CRSQry.GetFieldIndex( "time_create" );
  int col_pax_id = CRSQry.GetFieldIndex( "pax_id" );
  int col_crs_pax_id = CRSQry.GetFieldIndex( "crs_pax_id" );
  int col_point_dep = CRSQry.GetFieldIndex( "point_dep" );
  int col_point_arv = CRSQry.GetFieldIndex( "point_arv" );
  int idx_first_xname = CRSQry.GetFieldIndex( "first_xname" );
  int idx_first_yname = CRSQry.GetFieldIndex( "first_yname" );
  int idx_last_xname = CRSQry.GetFieldIndex( "last_xname" );
  int idx_last_yname = CRSQry.GetFieldIndex( "last_yname" );

  for ( ; !CRSQry.Eof; CRSQry.Next() ) {
    TLayerSeat layer;
    if ( col_point_id < 0 || CRSQry.FieldIsNULL( col_point_id ) )
      layer.point_id = NoExists;
    else
      layer.point_id = CRSQry.FieldAsInteger( col_point_id );
    if ( col_point_dep < 0 )
      layer.point_dep = NoExists;
    else {
      if ( CRSQry.FieldIsNULL( col_point_dep ) ) // для алгоритма обязательно должен быть задан point_dep
        layer.point_dep = layer.point_id;
      else
        layer.point_dep = CRSQry.FieldAsInteger( col_point_dep );
    }
    if ( col_point_arv < 0 || CRSQry.FieldIsNULL( col_point_arv ) )
      layer.point_arv = NoExists;
    else
      layer.point_arv = CRSQry.FieldAsInteger( col_point_arv );
    if ( col_pax_id < 0 || CRSQry.FieldIsNULL( col_pax_id ) )
      layer.pax_id = NoExists;
    else
      layer.pax_id = CRSQry.FieldAsInteger( col_pax_id );
    if ( col_crs_pax_id < 0 || CRSQry.FieldIsNULL( col_crs_pax_id ) )
      layer.crs_pax_id = NoExists;
    else
      layer.crs_pax_id = CRSQry.FieldAsInteger( col_crs_pax_id );
    layer.layer_type = DecodeCompLayerType( CRSQry.FieldAsString( col_layer_type ).c_str() );
    if ( col_time_create < 0 || CRSQry.FieldIsNULL( col_time_create ) )
      layer.time_create = NoExists;
    else
      layer.time_create = CRSQry.FieldAsDateTime( col_time_create );
    if ( col_point_id >= 0 && //SOM, PRL телеграммы нас не интересуют
         !filter.CanUseLayer( layer.layer_type,
                              layer.point_dep,
                              layer.point_dep,
                              false /*filterRoutes.isTakeoff( layer.point_id )*/ ) )
      continue;
    // слой нужно добавить
    TLayerPrioritySeat layerPrioritySeat( layer,
                                          layerInst->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( getFltAirline(point_id),
                                                                                                        layer.layer_type ),
                                                                                                        flag ) );
    TSeatRange seatRange( TSeat(CRSQry.FieldAsString( idx_first_yname ),
                                CRSQry.FieldAsString( idx_first_xname ) ),
                          TSeat(CRSQry.FieldAsString( idx_last_yname ),
                                CRSQry.FieldAsString( idx_last_xname ) ) );
    auto iranges = total_ranges.find( layerPrioritySeat );
    if ( iranges != total_ranges.end() )
      iranges->second.emplace_back( seatRange ); //может быть много мест с одним слоем
    else {
      TSeatRanges ranges;
      ranges.emplace_back( seatRange );
      total_ranges.emplace( layerPrioritySeat, ranges );
    }
  } // CRSQry.Eof
}

void TSeatPax::getCheckInSeatRanges( const PaxId_t& pax_id,
                                     const PointId_t& point_id,
                                     bool from_waitlist,
                                     TSeatRanges &ranges )
{
  LogTrace(TRACE5) << "TSeatPax::"<<__func__ << " " << point_id << " read";
  ranges.clear();
  DB::TQuery SeatsQry(PgOra::getROSession("PAX_SEATS"), STDLOG);
  SeatsQry.SQLText=
    "SELECT yname, xname "
    "FROM pax_seats "
    "WHERE pax_id=:pax_id AND point_id=:point_id AND pr_wl=:pr_wl "
    "ORDER BY yname, xname";
  SeatsQry.CreateVariable("point_id",otInteger,point_id.get());
  SeatsQry.CreateVariable("pax_id",otInteger, pax_id.get());
  SeatsQry.CreateVariable("pr_wl",otInteger, from_waitlist);
  SeatsQry.Execute();
  for(;!SeatsQry.Eof;SeatsQry.Next())
    ranges.emplace_back(TSeatRange(TSeat(SeatsQry.FieldAsString("yname"),
                                         SeatsQry.FieldAsString("xname"))));
}

//one, _one, list, _list, seats, _seats
string TSeatPax::getCrsSeatNo(const TSeatRanges &ranges, const EnumFormatSeats &fmt, bool pr_lat, int seats)
{
  if (ranges.empty()) return "";
  string res;
  boost::optional<char> add_ch;
  if ( fmt == ef_List || fmt == ef_One || fmt == ef_Seats ) add_ch = ' ';
  res = SeatNumber::tryDenormalizeRow( ranges.begin()->first.row, add_ch ) +
        SeatNumber::tryDenormalizeLine( ranges.begin()->first.line, pr_lat );
  if ( fmt == ef_One || fmt == efOne ) return res;
  if ( !res.empty() && seats > 1 ) res += "+" + IntToString(seats-1);
  return res;
}

/*
   fmt - формат данных.
   Может принимать значения:
   'one':    одно левое верхнее место пассажира
   '_one':   одно левое верхнее место пассажира + пробелы слева
   'tlg':    перечисление псевдо-IATA мест пассажира. Сортировка по названию: 4А4Б4В
   'list':   перечисление всех мест пассажира через пробел. Сортировка по названию.
   '_list':  перечисление всех мест пассажира через пробел. Сортировка по названию + пробелы слева
   'voland': см. описание формата в процедуре. Используется для печати пос. талонов.
   '_seats': одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2 + пробелы слева
   иначе:    одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2
*/

std::string TSeatPax::getCheckinSeatNo(const TSeatRanges &ranges, const EnumFormatSeats &fmt, bool pr_lat, int seats)
{
  if ( ranges.empty() ) return "";
  boost::optional<char> add_ch;
  if ( fmt == ef_List || fmt == ef_One || fmt == ef_Seats ) add_ch = ' ';
  bool all_seat_norm = true;
  string first_xname, first_yname;
  string last_xname, last_yname;
  string res_tlg, res_one, res_num, res_list, res_voland;
  int c_count = 0;
  for ( const auto &seat : ranges ) {
    std::string vyname = SeatNumber::tryDenormalizeRow(seat.first.row,add_ch);
    add_ch = ' ';
    std::string vxname = SeatNumber::tryDenormalizeLine(seat.first.line,pr_lat);
    if ( !SeatNumber::isIataLine(seat.first.line) || !SeatNumber::isIataRow(seat.first.row) )
      all_seat_norm = false;
    else
      res_tlg += vyname + vxname; /* простое перечисление нормальных номеров мест */
    if ( c_count == 0 ) {
      first_xname = vxname;
      first_yname = seat.first.row; /* важно, что спереди нули и длина = 3 цифрам */
    }
    else {
      last_xname = vxname;
      last_yname = seat.first.row; /* важно, что спереди нули и длина = 3 цифрам */
    }
    if ( res_one.empty() ) {
      /* составляем формат NUM (самое левое верхнее место салона) */
      res_one = vyname + vxname;
      if ( !res_one.empty() && seats > 1 )
        res_num = res_one + "+" + IntToString(seats-1);
      else
        res_num = res_one;
    }
    res_list += vyname + vxname + " ";
    c_count++;
  }
  if ( c_count > seats ) return "???";
  if ( c_count < seats ) return "";
  if ( fmt == efOne || fmt == ef_One ) return res_one;
  if ( fmt == efTlg ) return res_tlg;
  if ( fmt ==  efList || fmt == ef_List ) return StrUtils::trim( res_list );
  if ( fmt == efVoland && all_seat_norm ) {
      tst();
      /* Следующий формат используется при печати мест на пос. талоне
      -- максимальная длина строки - 6 символов
      -- Если рассадка горизонтальная, то формат однозначный 200А-Б.
      -- При вертикальной рассадке возможны 5 вариантов:
      -- 1. Переход номера ряда с 2-значного на 3-значный 99-01А
      -- 2. Не 3-начные номера ряда без перехода через десятку 45-7A
      -- 3. Не 3-начные номера ряда с переходом через десятку 49-51А
      -- 4. 3-начные номера ряда без перехода через десятку 150-2A
      -- 5. 3-начные номера ряда с переходом через десятку 19901А*/
    if ( first_yname == last_yname && first_xname == last_xname ) {
      /* одно место */
      res_voland = StrUtils::StringLTrimCh( first_yname,'0' ) + first_xname;
      return res_voland;
    }
    if ( first_yname == last_yname ) {
      /* рассадка горизонтальная */
      res_voland = StrUtils::StringLTrimCh( first_yname,'0' ) + first_xname + "-" + last_xname;
      return res_voland;
    }
    if ( first_xname == last_xname &&
         first_yname.size() == 3 &&
         last_yname.size() == 3 ) {
      tst();
      /* рассадка вертикальная */
      c_count = 1;
      while ( c_count <= 3 && first_yname[c_count] == last_yname[ c_count ] ) c_count++;
      if ( c_count == 1 ) c_count++;
      res_voland = StrUtils::StringLTrimCh( first_yname,'0' ) + "-" + last_yname.substr(c_count) + first_xname;
      if ( res_voland.size() >6 )
        res_voland = StrUtils::StringLTrimCh( first_yname,'0' ) + last_yname.substr(c_count) + first_xname;
      return res_voland;
    }
    tst();
  }
  return res_num;
}

std::string TSeatPax::get_seat_no( const PaxId_t& pax_id,
                                   const EnumFormatSeats& format )
{
  TSeatsProps seatProps;
  CheckIn::TSimplePaxItem pax;
  if (pax.getByPaxId(pax_id.get())) {
    CheckIn::TSimplePaxGrpItem grp;
    if (grp.getByPaxId(pax_id.get()))
      return get_seat_no( pax_id,
                          PointId_t(grp.point_dep),
                          grp.status,
                          pax.refuse,
                          pax.is_jmp,
                          pax.seats,
                          EnumCheckinStatus::psCheckin,
                          getFlightSetsPrFreeSeating(PointId_t(grp.point_dep)),
                          format,
                          getFlightSetsPrLatSeat(PointId_t(grp.point_dep)),
                          seatProps);
  }
  else {
    pax.getCrsByPaxId(pax_id);
    CheckIn::TSimplePnrItem pnr;
    if (!pnr.getByPaxId(pax_id.get())) return "";
    if (pnr.point_id_tlg == ASTRA::NoExists) return "";
    int id = getPointIdByPointTlg(PointIdTlg_t(pnr.point_id_tlg));
    if ( id == ASTRA::NoExists ) return "";
    return get_seat_no( pax_id,
                        PointId_t(id),
                        ASTRA::psCheckin,
                        pax.refuse,
                        pax.is_jmp,
                        pax.seats,
                        EnumCheckinStatus::psCRS,
                        getFlightSetsPrFreeSeating(PointId_t(id)),
                        format,
                        getFlightSetsPrLatSeat(PointId_t(id)),
                        seatProps);
  }
  return "";
}


std::string TSeatPax::get_seat_no( const PaxId_t& pax_id,
                                   const PointIdTlg_t& point_id,
                                   bool free_seating,
                                   const EnumFormatSeats& format,
                                   bool pr_lat_seat )
{
  if (free_seating) return "";
  TSeatsProps seatProps;
  CheckIn::TSimplePaxItem pax;
  if (!pax.getCrsByPaxId(pax_id)) return "";
  int id = getPointIdByPointTlg(point_id);
  if ( id == ASTRA::NoExists ) return "";
  return get_seat_no( pax_id,
                      PointId_t(id),
                      ASTRA::psCheckin,
                      pax.refuse,
                      pax.is_jmp,
                      pax.seats,
                      EnumCheckinStatus::psCRS,
                      free_seating,
                      format, pr_lat_seat,
                      seatProps );
}

std::string TSeatPax::get_seat_no( const PaxId_t& pax_id,
                                   const PointIdTlg_t& point_id,
                                   int seats,
                                   bool free_seating,
                                   const EnumFormatSeats& format,
                                   TSeatsProps& seatProps )
{
  PointId_t id(getPointIdByPointTlg(point_id));
  return get_seat_no( pax_id,
                      id,
                      ASTRA::TPaxStatus::psCheckin,
                      "",
                      false,
                      seats,
                      psCRS,
                      free_seating,
                      format,
                      getFlightSetsPrLatSeat(id),
                      seatProps);
}

std::string TSeatPax::get_seat_no( const PaxId_t& pax_id,
                                   const PointId_t& point_id,
                                   const ASTRA::TPaxStatus& grp_status,
                                   const std::string& refuse,
                                   bool is_jmp,
                                   int seats,
                                   const EnumCheckinStatus& checkin_status,
                                   bool free_seating,
                                   const EnumFormatSeats& format,
                                   TSeatsProps& seatProps )
{
  return get_seat_no( pax_id,
                      point_id,
                      grp_status,
                      refuse,
                      is_jmp,
                      seats,
                      checkin_status,
                      free_seating,
                      format,
                      getFlightSetsPrLatSeat(point_id),
                      seatProps);
}

std::string TSeatPax::get_seat_no( const PaxId_t& pax_id,
                                   const PointId_t& point_id,
                                   const ASTRA::TPaxStatus& grp_status,
                                   const std::string& refuse,
                                   bool is_jmp,
                                   int seats,
                                   const EnumCheckinStatus& checkin_status,
                                   bool free_seating,
                                   const EnumFormatSeats& format, bool pr_lat_seat,
                                   TSeatsProps& seatProps )
{
  seatProps.clear();

  if (free_seating) return "";
  if (seats == 0) return "";
  if (!refuse.empty()) return "";
  if (grp_status == psCrew) return "";

  switch (checkin_status) {
    case psCheckin:
      return checkin_seat_no(pax_id,point_id,grp_status,is_jmp,seats,format,pr_lat_seat,seatProps);
      break;
    case psCRS:
      return crs_seat_no(pax_id,point_id,seats,format,pr_lat_seat,seatProps);
      break;
    default:
      return "";
  }
}

std::string TSeatPax::int_checkin_seat_no( const PaxId_t& pax_id,
                                           const PointId_t& point_id,
                                           int seats,
                                           bool from_waitlist,
                                           const EnumFormatSeats& format, bool pr_lat_seat)
{
  TSeatRanges ranges;
  getCheckInSeatRanges( pax_id,
                        point_id,
                        from_waitlist,
                        ranges );
  if ( ranges.empty() ) return "";
  return getCheckinSeatNo(ranges, format, pr_lat_seat, seats);
}

std::string TSeatPax::except_seat_trip_comp_layers( const PaxId_t& pax_id,
                                                    const PointId_t& point_id,
                                                    const ASTRA::TCompLayerType& checkin_layer,
                                                    const EnumFormatSeats& format, bool pr_lat_seat,
                                                    int seats)
{
  DB::TQuery SeatsQry(PgOra::getROSession({"TRIP_COMP_LAYERS"}), STDLOG);
  SeatsQry.SQLText =
    "SELECT first_xname, last_xname, first_yname, last_yname FROM trip_comp_layers "
    " WHERE point_id=:point_id AND pax_id=:pax_id AND layer_type=:layer_type "
    "ORDER BY first_yname, first_xname";
  SeatsQry.CreateVariable("point_id",otInteger,point_id.get());
  SeatsQry.CreateVariable("pax_id",otInteger,pax_id.get());
  SeatsQry.CreateVariable("layer_type",otString,EncodeCompLayerType(checkin_layer));
  SeatsQry.Execute();
  TSeatRanges ranges;
  for(;!SeatsQry.Eof;SeatsQry.Next())
    ranges.emplace_back(TSeatRange(TSeat(SeatsQry.FieldAsString("first_yname"),
                                         SeatsQry.FieldAsString("first_xname")),
                                   TSeat(SeatsQry.FieldAsString("last_yname"),
                                         SeatsQry.FieldAsString("last_xname"))));
  if ( ranges.empty() ) return "";
  return getCheckinSeatNo(ranges, format, pr_lat_seat, seats );
}

std::string TSeatPax::checkin_seat_no( const PaxId_t& pax_id,
                                       const PointId_t& point_id,
                                       const ASTRA::TPaxStatus& grp_status,
                                       bool is_jmp,
                                       int seats,
                                       const EnumFormatSeats& format,
                                       bool pr_lat_seat,
                                       TSeatsProps& seatProps)
{
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", EncodePaxStatus(grp_status) );
  seatProps.layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
  if (is_jmp ) {
    seatProps.seat_no = "JMP";
    return seatProps.seat_no;
  }
  seatProps.seat_no = int_checkin_seat_no( pax_id, point_id, seats,
                                           false, format, pr_lat_seat );
  seatProps.from_waitlist = (seatProps.seat_no.empty());
  if (!seatProps.from_waitlist) return seatProps.seat_no;
  seatProps.seat_no = int_checkin_seat_no( pax_id, point_id, seats,
                                                     seatProps.from_waitlist,
                                                     format, pr_lat_seat );
  if ( !seatProps.seat_no.empty() )
    seatProps.seat_no = "(" + StrUtils::trim(seatProps.seat_no) + ")";
  else {
    //!!! во время тестирования на рабочем сервере найдены несколько случаев на 10000 рейсов,
    //! когда пассажир регистрируется на ЛО - данных по пассажиру в pax_seats нет, а слой CHECKIN есть
    //! это исключение из правил - выводим ЛО !!!разобраться
    seatProps.seat_no = except_seat_trip_comp_layers(pax_id,
                                                     point_id,
                                                     seatProps.layer_type,
                                                     format,
                                                     pr_lat_seat,
                                                     seats);
    if ( !seatProps.seat_no.empty() )
      seatProps.seat_no = "(" + StrUtils::trim(seatProps.seat_no) + ")";
    else
      seatProps.seat_no = AstraLocale::getLocaleText("ЛО");
  }
  return seatProps.seat_no;
}

std::string TSeatPax::crs_seat_no(const PaxId_t& pax_id,
                                  const PointId_t& point_id,
                                  int seats,
                                  const EnumFormatSeats& format, bool pr_lat_seat,
                                  TSeatsProps& seatProps)
{
  TTotalRanges total_ranges;

  getTlgCompLayer( pax_id,
                   point_id,
                   total_ranges );
  if ( total_ranges.empty() ) return "";
  TSeatRanges ranges;
  std::map<TLayerPrioritySeat,TSeatRanges,LayerPrioritySeatCompare>::const_iterator iranges=total_ranges.begin();
  seatProps.layer_type = iranges->first.layerType();
  for ( TSeatRanges::const_iterator irange=iranges->second.begin();
        irange!=iranges->second.end(); irange++ ) {
    ranges.push_back( *irange );
  }
  int vseats = seats;
  seatProps.seat_no = getCrsSeatNo(ranges, format, pr_lat_seat, vseats);
  return seatProps.seat_no;
}

bool TSeatPax::is_waitlist( const PointId_t& point_id )
{
  if ( SALONS2::isFreeSeating( point_id.get() ) ) return false;
  DB::TQuery WLTripQry(PgOra::getROSession({"PAX_SEATS","PAX","PAX_GRP"}), STDLOG);
  //если в таблице pax_seats нет даных по зарегистрированному пассажиру или только pr_wl=0 то есть ЛО
  WLTripQry.SQLText =
    " SELECT MIN(COALESCE(pr_wl,0)) AS pr_wl "
    "  FROM pax "
    "  JOIN pax_grp ON pax_grp.grp_id=pax.grp_id AND "
    "       pax_grp.point_dep=:point_id AND "
    "       pax.refuse IS NULL AND "
    "       pax_grp.status NOT IN ('E') AND "
    "       pax.pr_brd IS NOT NULL AND "
    "       seats>0 AND "
    "       is_jmp=0 "
    "  LEFT JOIN pax_seats ON pax_seats.pax_id=pax.pax_id AND "
    "            pax_seats.point_id=:point_id ";

  WLTripQry.CreateVariable("point_id",otInteger,point_id.get());
  WLTripQry.Execute();
  return (!WLTripQry.Eof && WLTripQry.FieldAsInteger("pr_wl") == 0);
}

//===============================================
int TSeatPaxCached::getPointIdByPointTlg( const PointIdTlg_t& point_id )
{
  auto id = data.point_tlgs.find(point_id.get());
  if ( id == data.point_tlgs.end() )
    id = data.point_tlgs.insert(make_pair(point_id.get(),TSeatPax::getPointIdByPointTlg(point_id))).first;
  return id->second;
}

bool TSeatPaxCached::getFlightSetsPrLatSeat( const PointId_t& point_id )
{
  if (data.pr_lat_seats.find(point_id.get()) == data.pr_lat_seats.end())
    data.pr_lat_seats.emplace(point_id.get(),TSeatPax::getFlightSetsPrLatSeat(point_id));
  return data.pr_lat_seats[point_id.get()];
}

bool TSeatPaxCached::getFlightSetsPrFreeSeating( const PointId_t& point_id )
{
  if (data.pr_free_seatings.find(point_id.get()) == data.pr_free_seatings.end())
    data.pr_free_seatings.emplace(point_id.get(),TSeatPax::getFlightSetsPrFreeSeating(point_id));
  return data.pr_free_seatings[point_id.get()];
}

BASIC_SALONS::TCompLayerTypes::Enum TSeatPaxCached::getCompLayerPriorityFlag(const PointId_t& point_id)
{
  if (data.flags.find(point_id.get())==data.flags.end()) {
    LogTrace(TRACE5) << "TSeatPaxCached::"<<__func__ << " " << point_id << " read";
    TTripInfo fltInfo;
    fltInfo.getByPointId(point_id.get());
    data.fltAirlines.emplace(point_id.get(),fltInfo.airline);
    data.flags.emplace(point_id.get(),(GetTripSets( tsAirlineCompLayerPriority, fltInfo )?
                                       BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                       BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline));
  }
  return data.flags[point_id.get()];
}

TFilterLayers TSeatPaxCached::getCompLayerFilter(const PointId_t& point_id)
{
  //!!! копирование продумать!!!
  if (data.filters.find(point_id.get())==data.filters.end())
    data.filters.emplace(point_id.get(),TSeatPax::getCompLayerFilter(point_id));
  return data.filters[point_id.get()];
}

std::string TSeatPaxCached::getFltAirline(const PointId_t& point_id)
{
  if ( data.fltAirlines.find(point_id.get()) == data.fltAirlines.end() )
    data.fltAirlines.emplace(point_id.get(),TSeatPax::getFltAirline(point_id));
  return data.fltAirlines[point_id.get()];
}

void TSeatPaxListCached::getTlgCompLayer( const PaxId_t& pax_id,
                                          const PointId_t& point_id,
                                          TTotalRanges& total_ranges )
{
  total_ranges.clear();
  if ( paxlist_total_ranges.find(point_id.get()) == paxlist_total_ranges.end() ) {
    LogTrace(TRACE5) << "TSeatPaxListCached::" << __func__ << " " << point_id << " read";
    paxlist_total_ranges.emplace(point_id.get(),std::map<int,TTotalRanges>());

    //кэшировать для списков!
    BASIC_SALONS::TCompLayerTypes *layerInst = BASIC_SALONS::TCompLayerTypes::Instance();
    BASIC_SALONS::TCompLayerTypes::Enum flag = getCompLayerPriorityFlag(point_id);
    TFilterLayers filter = getCompLayerFilter(point_id);
    //конец кэшировать для списков!

    DB::TQuery CRSQry(PgOra::getROSession({"TLG_COMP_LAYERS","TLG_BINDING"}), STDLOG);
    //здесь все как в salons, но не очень правильно. Хорошо бы сделать анализ на АП прилета и маршрута - возможно слой инвалидный,
    // ид. вылета рейса соответствует tlg_comp_layers.point_id ?
    //упрощаем )
    CRSQry.SQLText =
      "SELECT tlg_binding.point_id_spp as point_id,"
      " layer_type,first_xname,last_xname,first_yname,last_yname,crs_pax_id "
      " FROM tlg_binding,tlg_comp_layers "
      "WHERE tlg_comp_layers.point_id=tlg_binding.point_id_tlg AND "
      "      tlg_binding.point_id_spp=:point_id "
      "ORDER BY first_yname, first_xname, layer_type";
    CRSQry.CreateVariable( "point_id", otInteger, point_id.get() );
    CRSQry.Execute();
    if ( CRSQry.Eof ) return;
    int col_point_id = CRSQry.GetFieldIndex( "point_id" );
    int col_layer_type = CRSQry.FieldIndex( "layer_type" );
    int col_time_create = CRSQry.GetFieldIndex( "time_create" );
    int col_pax_id = CRSQry.GetFieldIndex( "pax_id" );
    int col_crs_pax_id = CRSQry.GetFieldIndex( "crs_pax_id" );
    int col_point_dep = CRSQry.GetFieldIndex( "point_dep" );
    int col_point_arv = CRSQry.GetFieldIndex( "point_arv" );
    int idx_first_xname = CRSQry.GetFieldIndex( "first_xname" );
    int idx_first_yname = CRSQry.GetFieldIndex( "first_yname" );
    int idx_last_xname = CRSQry.GetFieldIndex( "last_xname" );
    int idx_last_yname = CRSQry.GetFieldIndex( "last_yname" );

    for ( ; !CRSQry.Eof; CRSQry.Next() ) {
      TLayerSeat layer;
      if ( col_point_id < 0 || CRSQry.FieldIsNULL( col_point_id ) )
        layer.point_id = NoExists;
      else
        layer.point_id = CRSQry.FieldAsInteger( col_point_id );
      if ( col_point_dep < 0 )
        layer.point_dep = NoExists;
      else {
        if ( CRSQry.FieldIsNULL( col_point_dep ) ) // для алгоритма обязательно должен быть задан point_dep
          layer.point_dep = layer.point_id;
        else
          layer.point_dep = CRSQry.FieldAsInteger( col_point_dep );
      }
      if ( col_point_arv < 0 || CRSQry.FieldIsNULL( col_point_arv ) )
        layer.point_arv = NoExists;
      else
        layer.point_arv = CRSQry.FieldAsInteger( col_point_arv );
      if ( col_pax_id < 0 || CRSQry.FieldIsNULL( col_pax_id ) )
        layer.pax_id = NoExists;
      else
        layer.pax_id = CRSQry.FieldAsInteger( col_pax_id );
      if ( col_crs_pax_id < 0 || CRSQry.FieldIsNULL( col_crs_pax_id ) )
        layer.crs_pax_id = NoExists;
      else
        layer.crs_pax_id = CRSQry.FieldAsInteger( col_crs_pax_id );
      layer.layer_type = DecodeCompLayerType( CRSQry.FieldAsString( col_layer_type ).c_str() );
      if ( col_time_create < 0 || CRSQry.FieldIsNULL( col_time_create ) )
        layer.time_create = NoExists;
      else
        layer.time_create = CRSQry.FieldAsDateTime( col_time_create );
      if ( col_point_id >= 0 && //SOM, PRL телеграммы нас не интересуют
           !filter.CanUseLayer( layer.layer_type,
                                layer.point_dep,
                                layer.point_dep,
                                false /*filterRoutes.isTakeoff( layer.point_id )*/ ) )
        continue;
      // слой нужно добавить
      TLayerPrioritySeat layerPrioritySeat( layer,
                                            layerInst->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( getFltAirline(point_id),
                                                                                                          layer.layer_type ),
                                                                                                          flag ) );
      TSeatRange seatRange( TSeat(CRSQry.FieldAsString( idx_first_yname ),
                                  CRSQry.FieldAsString( idx_first_xname ) ),
                            TSeat(CRSQry.FieldAsString( idx_last_yname ),
                                  CRSQry.FieldAsString( idx_last_xname ) ) );
      auto iranges = paxlist_total_ranges[point_id.get()][layer.getPaxId()].find( layerPrioritySeat );
      if ( iranges != paxlist_total_ranges[point_id.get()][layer.getPaxId()].end() )
        iranges->second.emplace_back( seatRange ); //может быть много мест с одним слоем
      else {
        TSeatRanges ranges;
        ranges.emplace_back( seatRange );
        paxlist_total_ranges[point_id.get()][layer.getPaxId()].emplace( layerPrioritySeat, ranges );
      }
    } // CRSQry.Eof
  }
  total_ranges = paxlist_total_ranges[point_id.get()][ pax_id.get() ];
}

void TSeatPaxListCached::int_getCheckInSeatRanges( const PointId_t& point_id,
                                                   bool from_waitlist,
                                                   std::map<int,std::map<int,TSeatRanges>> &seats )

{
  if (seats.find(point_id.get())!=seats.end()) return;
  LogTrace(TRACE5) << "TSeatPaxListCached::" << __func__ << " " << point_id;
  seats.emplace(point_id.get(),std::map<int,TSeatRanges>());
  DB::TQuery SeatsQry(PgOra::getROSession({"PAX_SEATS"}), STDLOG);
  SeatsQry.SQLText=
    "SELECT pax_id, yname, xname "
    "FROM pax_seats "
    "WHERE point_id=:point_id AND pr_wl=:pr_wl "
    " ORDER BY yname, xname";
  SeatsQry.CreateVariable("point_id",otInteger,point_id.get());
  SeatsQry.CreateVariable("pr_wl",otInteger, from_waitlist);
  SeatsQry.Execute();
  for(;!SeatsQry.Eof;SeatsQry.Next())
    seats[point_id.get()][SeatsQry.FieldAsInteger("pax_id")].emplace_back(
                                         TSeatRange(TSeat(SeatsQry.FieldAsString("yname"),
                                                          SeatsQry.FieldAsString("xname"))));
}

void TSeatPaxListCached::getCheckInSeatRanges( const PaxId_t& pax_id,
                                               const PointId_t& point_id,
                                               bool from_waitlist,
                                               TSeatRanges &ranges )
{
  ranges.clear();
  TSeatPaxListCached::int_getCheckInSeatRanges(point_id,from_waitlist,(from_waitlist?waitlist_ranges:seat_ranges));
  from_waitlist?(ranges=waitlist_ranges[point_id.get()][pax_id.get()]):(ranges=seat_ranges[point_id.get()][pax_id.get()]);
}

  /*
   * Новая концепция одной функции, которая выдает актуальное место пассажира + слой
   * раньше в начитке списка пассажиров определялся статус пассажира, если зарегистрирован, то get_seat_no, иначе get_crs_seat_no
   * Внутри get_crs_seat_no слой cltPNLCkin обрабатывался отдельно - устарело
   */

std::string PaxListSeatNo::get( const PointId_t& point_id,
                                PaxId_t pax_id,
                                const std::string& format,
                                ASTRA::TCompLayerType& layer_type,
                                bool &isseat)
{
  auto ret = salonLists.insert(std::make_pair(point_id.get(),std::make_pair(false,SALONS2::TSalonList(true))));
  if ( ret.second ) {
    TSalonListReadParams params;
    params.for_calc_waitlist = true; //не ругаемся если isFreeSeating || или нет салона
    params.for_get_seat_no = true; //начитка tlg_comp_layers
    ret.first->second.first = isFreeSeating(point_id.get());
    ret.first->second.second.ReadFlight(TFilterRoutesSets(point_id.get()),params);
  }

  return PaxListSeatNo::get( ret.first->second.second, pax_id, format, ret.first->second.first,
                             layer_type, isseat );
}

std::string PaxListSeatNo::get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                                const std::string& format,
                                ASTRA::TCompLayerType& layer_type,
                                bool &isseat)
{
  auto pids = point_tlgs.find(point_id_tlg.get());
  if ( pids == point_tlgs.end() ) {
    std::set<PointId_t> point_ids_spp;
    getPointIdsSppByPointIdTlg(point_id_tlg, point_ids_spp);
    pids = point_tlgs.emplace(point_id_tlg.get(),point_ids_spp).first;
  }
  if ( pids->second.empty() )
    return "";
  return get(*pids->second.begin(),pax_id,format,
             layer_type,isseat); //??? или пробег по point_id_spp?
}

std::string PaxListSeatNo::get( const  TSalonList& salonList,
                                PaxId_t pax_id,
                                const std::string& format,
                                bool free_seating,
                                ASTRA::TCompLayerType& layer_type,
                                bool &isseat)
{
  LogTrace(TRACE5) << __func__ << " point_id " << salonList.getDepartureId() << " pax_id " << pax_id.get();
  layer_type = cltUnknown;
  isseat = false;
  if (free_seating)
    return "";
  std::set<TCompLayerType> checkinLayers { ASTRA::cltGoShow, ASTRA::cltTranzit, ASTRA::cltCheckin, ASTRA::cltTCheckin };
  SALONS2::TSalonPax salonPax;
  std::string seat_no;
  if ( !salonList.getPax(salonList.getDepartureId(), pax_id.get(), salonPax ) ) {
    //LogError(STDLOG) << __func__ << " point_id " << salonList.getDepartureId() << " pax_id " << pax_id.get();
    tst();
    return "";
  }
  if (salonPax.seats == 0)
    return "";
  if (salonList.isRefused(salonList.getDepartureId(),pax_id.get()))
    return "";
  ASTRA::TPaxStatus grp_status = DecodePaxStatus(salonPax.grp_status.c_str());
  if ( grp_status == psCrew )
    return "";
  ASTRA::TCompLayerType grp_layer_type = cltUnknown;
  if ( !salonPax.grp_status.empty() ) {
    TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
    const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", salonPax.grp_status );
    grp_layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
  }
  bool pr_checkin = (checkinLayers.find( grp_layer_type ) != checkinLayers.end());

  //пассажир зарегистрирован?
  //   да -ищем валидный слой зарегистрированного пассажира
  //     нашли?
  //       да - отображаем место
  //       нет - отображаем предыдущие места пассажира "(...)"
  //  нет - отображаем самый приоритетный слой пассажира
  TWaitListReason waitListReason;
  if ( pr_checkin ) {
    tst();
    seat_no = salonPax.seat_no( format, salonList.isCraftLat(),
                                waitListReason, layer_type );
    LogTrace(TRACE5) << EncodeCompLayerType(layer_type) << " " << pax_id;
    isseat = ( !seat_no.empty() && (layer_type != cltUnknown || seat_no == "JMP") );
    if ( !seat_no.empty() ) return seat_no;
    tst();
    seat_no = salonPax.prior_seat_no( format, salonList.isCraftLat() );
    if ( seat_no.empty() )
      seat_no = AstraLocale::getLocaleText("ЛО");
    else
      seat_no = "(" + StrUtils::trim(seat_no) + ")";
    return seat_no;
  }
  //самый приоритетный, пусть даже не валидный
  seat_no = salonPax.prior_crs_seat_no( format, salonList.isCraftLat(),layer_type);
  isseat = ( !seat_no.empty() && layer_type != cltUnknown );
  LogTrace(TRACE5) << seat_no;
  return seat_no;
}



//еще один быстрый способ получения номера места для зарегистрированного пассажира
/*std::string PaxListSeatNo::checkin_seat_no( int point_id, PaxId_t pax_id,
                                            const EnumFormatSeats& format, bool pr_lat_seat,
                                            bool &isseat)
{
  int seats = 0;
  std::string seat_no = int_checkin_seat_no( PaxId_t(pax_id), PointId_t(point_id), seats, false, format, pr_lat_seat );
  isseat = !seat_no.empty();
  if (isseat)
    return seat_no;
  seat_no = int_checkin_seat_no( PaxId_t(pax_id), PointId_t(point_id), seats, false, format, pr_lat_seat );
  if ( !seat_no.empty() )
    seat_no = "(" + StrUtils::trim(seat_no) + ")";
  else
    seat_no = AstraLocale::getLocaleText("ЛО");
  return seat_no;
}*/

} //end namespace SEATSPAX
