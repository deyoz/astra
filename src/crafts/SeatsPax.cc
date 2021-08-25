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
#include "SeatsPax.h"

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC::date_time;

namespace SEATSPAX {
  /*
   * Новая концепция одной функции, которая выдает актуальное место пассажира + слой
   * раньше в начитке списка пассажиров определялся статус пассажира, если зарегистрирован, то get_seat_no, иначе get_crs_seat_no
   * Внутри get_crs_seat_no слой cltPNLCkin обрабатывался отдельно. (почему?)
   */

std::string PaxListSeatNo::get( const PointId_t& point_id, PaxId_t pax_id, const std::string& format,
                                ASTRA::TCompLayerType& layer_type )
{
  auto ret = salonLists.insert(std::make_pair(point_id.get(),std::make_pair(false,SALONS2::TSalonList(true))));
  if ( ret.second ) {
    TSalonListReadParams params;
    params.for_calc_waitlist = true; //не ругаемся если isFreeSeating || или нет салона
    params.for_get_seat_no = true; //начитка tlg_comp_layers
    LogTrace(TRACE5) << point_id;
    ret.first->second.first = isFreeSeating(point_id.get());
    ret.first->second.second.ReadFlight(TFilterRoutesSets(point_id.get()),params);
  }

  return PaxListSeatNo::get( ret.first->second.second, pax_id, format, ret.first->second.first, layer_type );
}

std::string PaxListSeatNo::get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                                const std::string& format,
                                ASTRA::TCompLayerType& layer_type)
{
  auto pids = point_tlgs.find(point_id_tlg.get());
  if ( pids == point_tlgs.end() ) {
    std::set<PointId_t> point_ids_spp;
    getPointIdsSppByPointIdTlg(point_id_tlg, point_ids_spp);
    pids = point_tlgs.emplace(point_id_tlg.get(),point_ids_spp).first;
  }
  if ( pids->second.empty() )
    return "";
  return get(*pids->second.begin(),pax_id,format,layer_type); //??? или пробег по point_id_spp?
}

std::string PaxListSeatNo::get( const  TSalonList& salonList,
                                PaxId_t pax_id,
                                const std::string& format,
                                bool free_seating,
                                ASTRA::TCompLayerType& layer_type )
{
  LogTrace(TRACE5) << __func__ << " point_id " << salonList.getDepartureId() << " pax_id " << pax_id.get();
  if (free_seating)
    return "";
  std::set<TCompLayerType> checkinLayers { ASTRA::cltGoShow, ASTRA::cltTranzit, ASTRA::cltCheckin, ASTRA::cltTCheckin };
  SALONS2::TSalonPax salonPax;
  layer_type = ASTRA::TCompLayerType::cltUnknown;
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
  //  нет - ищем валидный слой не зарегистрированного пассажира
  //    нашли?
  //      да - отображаем место
  //      нет - отображаем самый приоритетный слой пассажира инвалидный
  TWaitListReason waitListReason;
  if ( pr_checkin ) {
    tst();
    seat_no = salonPax.seat_no( format, salonList.isCraftLat(),
                                waitListReason, layer_type );
    LogTrace(TRACE5) << EncodeCompLayerType(layer_type) << " " << pax_id;
    if ( !seat_no.empty() )
      return seat_no;
    tst();
    seat_no = salonPax.prior_seat_no( format, salonList.isCraftLat() );
    if ( seat_no.empty() )
      seat_no = AstraLocale::getLocaleText("ЛО");
    else
      seat_no = "(" + seat_no + ")";
    return seat_no;
  }
  //самый приоритетный, пусть даже не валидный
  seat_no = salonPax.prior_crs_seat_no( format, salonList.isCraftLat(),layer_type);
  LogTrace(TRACE5) << seat_no;
  return seat_no;
}


std::string PaxListSeatNo::int_checkin_seat_no( int point_id, PaxId_t pax_id, bool pr_wl,
                                                const std::string& format, bool pr_lat_seat )
{
  TQuery SeatsQry(&OraSession);
  SeatsQry.SQLText=
    "SELECT yname AS seat_row, xname AS seat_column "
    "FROM pax_seats "
    "WHERE pax_id=:pax_id AND point_id=:point_id AND pr_wl=:pr_wl";
  SeatsQry.CreateVariable("point_id",otInteger,point_id);
  SeatsQry.CreateVariable("pax_id",otInteger, pax_id.get());
  SeatsQry.CreateVariable("pr_wl",otInteger, pr_wl);
  TSeatRanges ranges;
  for(;!SeatsQry.Eof;SeatsQry.Next())
    ranges.emplace_back(TSeatRange(TSeat(SeatsQry.FieldAsString("seat_row"),
                                         SeatsQry.FieldAsString("seat_line"))));
  return GetSeatRangeView(ranges, format, pr_lat_seat);
}

//еще один быстрый способ получения номера места для зарегистрированного пассажира
std::string PaxListSeatNo::checkin_seat_no( int point_id, PaxId_t pax_id, const std::string& format, bool pr_lat_seat )
{
  std::string seat_no = PaxListSeatNo::int_checkin_seat_no( point_id, pax_id, false, format, pr_lat_seat );
  if ( !seat_no.empty() )
    return seat_no;
  seat_no = PaxListSeatNo::int_checkin_seat_no( point_id, pax_id, true, format, pr_lat_seat );
  if ( !seat_no.empty() )
    seat_no = "(" + seat_no + ")";
  else
    seat_no = AstraLocale::getLocaleText("ЛО");
  return seat_no;
}

} //end namespace SEATSPAX

