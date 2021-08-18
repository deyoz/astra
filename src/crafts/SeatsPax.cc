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
std::string PaxListSeatNo::get( PaxId_t pax_id, const std::string& format,
                                ASTRA::TCompLayerType& layer_type )
{
  return PaxListSeatNo::get( salonList, pax_id, format, free_seating, layer_type );
}

std::string PaxListSeatNo::get( const  TSalonList& salonList,
                                PaxId_t pax_id,
                                const std::string& format,
                                bool free_seating,
                                ASTRA::TCompLayerType& layer_type )
{
  if (free_seating)
    return "";
  std::set<TCompLayerType> checkinLayers { ASTRA::cltGoShow, ASTRA::cltTranzit, ASTRA::cltCheckin, ASTRA::cltTCheckin };
  SALONS2::TSalonPax salonPax;
  layer_type = ASTRA::TCompLayerType::cltUnknown;
  std::string seat_no;
  if ( !salonList.getPax(salonList.getDepartureId(), pax_id.get(), salonPax ) ) {
    //LogError(STDLOG) << __func__ << " point_id " << salonList.getDepartureId() << " pax_id " << pax_id.get();
    return "";
  }
  if (salonPax.seats == 0)
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
    seat_no = salonPax.seat_no( format, salonList.isCraftLat(), waitListReason );
    layer_type = waitListReason.layerType();
    if ( waitListReason.status == layerValid )
      return seat_no;
    seat_no = salonPax.prior_seat_no( format, salonList.isCraftLat() );
    if ( seat_no.empty() )
      seat_no = AstraLocale::getLocaleText("ЛО");
    else
      seat_no = "(" + seat_no + ")";
    return seat_no;
  }
/*  seat_no = salonPax.crs_seat_no( format, salonList.isCraftLat(), waitListReason );
  LogTrace(TRACE5) << seat_no;
  if ( waitListReason.status == layerValid )
    return seat_no;*/
  //самый приоритетный, пусть даже не валидный
  seat_no = salonPax.prior_crs_seat_no( format, salonList.isCraftLat(), layer_type );
  LogTrace(TRACE5) << seat_no;
  return seat_no;
}


std::string PaxListSeatNo::int_checkin_seat_no( int point_id, PaxId_t pax_id, bool pr_wl,
                                                const std::string& format, bool pr_lat_seat )
{
  DB::TQuery SeatsQry(PgOra::getROSession("PAX_SEATS"), STDLOG);
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

