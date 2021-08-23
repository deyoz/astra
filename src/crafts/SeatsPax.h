#pragma once
#include <string>
#include "astra_consts.h"
#include "salons.h"

namespace SEATSPAX
{
  using namespace SALONS2;

  class PaxListSeatNo { // для работы с номерапми мест при начитке списка пассажиров, использовать один экземпляр класса для всего списка!!!
    private: //point_dep, pair<free_seating,TSalonList>
      std::map<int,std::pair<bool,TSalonList>> salonLists;
      bool free_seating;
      static std::string int_checkin_seat_no( int point_id, PaxId_t pax_id, bool pr_wl,
                                              const std::string& format, bool pr_lat_seat );
    public:
      enum EnumWaitList  { spNone,
                           spWL //Лист ожидания "(12D)" или "ЛО"
                            };
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
      enum EnumFormat { one, _one, tlg, list, _list, voland, _seats, def };
      //для работы с группой пассажиров, инициализация экземпляра класса для каждого пассажира очень затратно
      //это вызывать, когда надо получить get_seat_no || get_crs_seat_no

      std::string get( const PointId_t& point_id, PaxId_t pax_id,
                       const std::string& format,
                       ASTRA::TCompLayerType& layer_type );
      std::string get( const PointId_t& point_id, PaxId_t pax_id,
                       const std::string& format ) {
         ASTRA::TCompLayerType layer_type;
         return get( point_id, pax_id, format, layer_type );
      }
      std::string get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                       const std::string& format,
                       ASTRA::TCompLayerType& layer_type );
      std::string get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                       const std::string& format ) {
         ASTRA::TCompLayerType layer_type;
         return get( point_id_tlg, pax_id, format, layer_type );
      }

      static std::string get( const  TSalonList& salonList,
                              PaxId_t pax_id,
                              const std::string& format,
                              bool free_seating,
                              ASTRA::TCompLayerType& layer_type );
      //получение места зарегистрированного пассажира или предыдущее место "(..)" - загрузка карты мест не происходит
      static std::string checkin_seat_no( int point_id, PaxId_t pax_id, const std::string& format, bool pr_lat_seat );
  };
} //end namespace SEATSPAX
