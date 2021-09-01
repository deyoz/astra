#pragma once
#include <string>
#include "astra_consts.h"
#include "salons.h"
#include "passenger.h"


namespace SEATSPAX
{
  using namespace SALONS2;

  struct TSeatsProps {
    ASTRA::TCompLayerType layer_type;
    bool from_waitlist;
    std::string seat_no;
    void clear() {
      layer_type = ASTRA::TCompLayerType::cltUnknown;
      from_waitlist = false;
    }
  };

  //класс для работы с одним пассажиром
  class TSeatPax {
      public:
        enum EnumFormatSeats { efOne, ef_One, efTlg, efList, ef_List, efVoland, ef_Seats, efSeats };
      private:
        //есть бага, когда пассажир зарегистрирован, а даных в таблице pax_seats нет, а слой есть
        // новый алгоритм "ЛО", а старый "(место)" не принципиально, но для сравнения сделаем
        std::string except_seat_trip_comp_layers( const PaxId_t& pax_id,
                                                  const PointId_t& point_id,
                                                  const ASTRA::TCompLayerType& checkin_layer,
                                                  const EnumFormatSeats& format,
                                                  bool pr_lat_seat,
                                                  int seats);
        std::string int_checkin_seat_no( const PaxId_t& pax_id,
                                         const PointId_t& point_id,
                                         int seats,
                                         bool from_waitlist,
                                         const EnumFormatSeats& format,
                                         bool pr_lat_seat);

        //one, _one, list, _list, seats, _seats
        std::string getCrsSeatNo(const TSeatRanges &ranges, const EnumFormatSeats &fmt, bool pr_lat, int seats);
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
        std::string getCheckinSeatNo(const TSeatRanges &ranges, const EnumFormatSeats &fmt, bool pr_lat, int seats);
    public:
      enum EnumCheckinStatus  { psCheckin, psCRS };
    protected:
      virtual int getPointIdByPointTlg( const PointIdTlg_t& point_id );
      virtual bool getFlightSetsPrLatSeat( const PointId_t& point_id );
      virtual bool getFlightSetsPrFreeSeating( const PointId_t& point_id );
      virtual BASIC_SALONS::TCompLayerTypes::Enum getCompLayerPriorityFlag(const PointId_t& point_id);
      virtual TFilterLayers getCompLayerFilter(const PointId_t& point_id);
      virtual std::string getFltAirline(const PointId_t& point_id);
      virtual void getTlgCompLayer( const PaxId_t& pax_id,
                                    const PointId_t& point_id,
                                    TTotalRanges& total_ranges );
      virtual void getCheckInSeatRanges( const PaxId_t& pax_id,
                                         const PointId_t& point_id,
                                         bool from_waitlist,
                                         TSeatRanges &ranges );
      std::string checkin_seat_no( const PaxId_t& pax_id,
                                   const PointId_t& point_id,
                                   const ASTRA::TPaxStatus& grp_status,
                                   bool is_jmp,
                                   int seats,
                                   const EnumFormatSeats& format,
                                   bool pr_lat_seat,
                                   TSeatsProps& seatProps);
      std::string crs_seat_no(const PaxId_t& pax_id,
                              const PointId_t& point_id,
                              int seats,
                              const EnumFormatSeats& format,
                              bool pr_lat_seat,
                              TSeatsProps& seatProps);
    public:
      virtual ~TSeatPax(){}
      //можно вызвать любую, то чем больше параметров передаем, тем меньше запросов
      //common - общая функция, которая вернет номер места регистрации, если пассажир зарегистрирован иначе брони
      std::string get_seat_no_common( const PaxId_t& pax_id,
                                      const EnumFormatSeats& format,
                                      const bool& only_lat=false );
      std::string get_crs_seat_no1( const PaxId_t& pax_id,
                                    const PointIdTlg_t& point_id,
                                    bool free_seating,
                                    const EnumFormatSeats& format,
                                    const bool& only_lat=false );
      std::string get_seat_no_pnl( const PaxId_t& pax_id,
                                   const PointId_t& point_id,
                                   const ASTRA::TPaxStatus& grp_status,
                                   const std::string& refuse,
                                   bool is_jmp,
                                   int seats,
                                   bool free_seating,
                                   const EnumFormatSeats& format,
                                   TSeatsProps& seatProps );
      std::string get_crs_seat_no_pnl( const PaxId_t& pax_id,
                                       const PointIdTlg_t& point_id,
                                       int seats,
                                       bool free_seating,
                                       const EnumFormatSeats& format,
                                       TSeatsProps& seatProps );
      std::string get_seat_no_int( const PaxId_t& pax_id,
                                   const PointId_t& point_id,
                                   const ASTRA::TPaxStatus& grp_status,
                                   const std::string& refuse,
                                   bool is_jmp,
                                   int seats,
                                   const EnumCheckinStatus& checkin_status,
                                   bool free_seating,
                                   const EnumFormatSeats& format,
                                   const bool& only_lat,
                                   TSeatsProps& seatProps );

      bool is_waitlist( const PointId_t& point_id );
      /////////////////////////////////////////////////////////
      bool is_waitlist( const PaxId_t& pax_id,
                        const int seats,
                        const bool is_jmp,
                        const ASTRA::TPaxStatus& grp_status,
                        const PointId_t& point_id );
      std::string get_crs_seat_no( const PaxId_t& pax_id,
                                   const int seats,
                                   const PointIdTlg_t& point_id,
                                   const EnumFormatSeats& format,
                                   const bool& only_lat=false );
      std::string get_crs_seat_no( const PaxId_t& pax_id,
                                   const int seats,
                                   const PointIdTlg_t& point_id,
                                   TCompLayerType& layer_type,
                                   const EnumFormatSeats& format,
                                   const bool& only_lat=false );
      std::string get_seat_no( const PaxId_t& pax_id,
                               const int seats,
                               const bool is_jmp,
                               const ASTRA::TPaxStatus& grp_status,
                               const PointId_t& point_id,
                               const EnumFormatSeats& format,
                               const bool& only_lat=false);
      std::string get_seat_no(const PaxId_t& pax_id,
                              const EnumFormatSeats& format,
                              const bool& only_lat=false);
  };

  struct FlightSetsCache {
    std::map<int,bool> pr_lat_seats;
    std::map<int,bool> pr_free_seatings;
    std::map<int,int> point_tlgs;
    std::map<int,BASIC_SALONS::TCompLayerTypes::Enum> flags;
    std::map<int,std::string> fltAirlines;
    std::map<int,TFilterLayers> filters;
  };

  //имеет смысл использовать, когда есть несколько запросов по номеру места пассажира. например с разными pr_lat_seat или форматами
  //например в посадке
  class TSeatPaxCached: public TSeatPax {
    protected:
      FlightSetsCache data;
      virtual int getPointIdByPointTlg( const PointIdTlg_t& point_id ) override;
      virtual bool getFlightSetsPrLatSeat( const PointId_t& point_id ) override;
      virtual bool getFlightSetsPrFreeSeating( const PointId_t& point_id );
      virtual BASIC_SALONS::TCompLayerTypes::Enum getCompLayerPriorityFlag(const PointId_t& point_id) override;
      virtual TFilterLayers getCompLayerFilter(const PointId_t& point_id) override;
      virtual std::string getFltAirline(const PointId_t& point_id) override;
    public:
      virtual ~TSeatPaxCached(){}
  };

  //этот класс для работы со списком пассажиров
  class TSeatPaxListCached: public TSeatPaxCached {
    private:
      //tlg_comp_layers
      std::map<int,std::map<int,TTotalRanges>> paxlist_total_ranges; // key=point_id,pax_id
      //pax_seats  checkin and waitlist
      std::map<int,std::map<int,TSeatRanges>> seat_ranges;
      std::map<int,std::map<int,TSeatRanges>> waitlist_ranges;
      static void int_getCheckInSeatRanges( const PointId_t& point_id,
                                            bool from_waitlist,
                                            std::map<int,std::map<int,TSeatRanges>> &seats );
    protected:
      virtual void getTlgCompLayer( const PaxId_t& pax_id,
                                    const PointId_t& point_id,
                                    TTotalRanges& total_ranges ) override;
      virtual void getCheckInSeatRanges( const PaxId_t& pax_id,
                                         const PointId_t& point_id,
                                         bool from_waitlist,
                                         TSeatRanges &ranges ) override;
    public:
      virtual ~TSeatPaxListCached(){}
  };

  //это скоро умрет
  class PaxListSeatNo { // для работы с номерапми мест при начитке списка пассажиров, использовать один экземпляр класса для всего списка!!!
    private: //point_dep, pair<free_seating,TSalonList>
      std::map<int,std::pair<bool,TSalonList>> salonLists;
      std::map<int,std::set<PointId_t>> point_tlgs;
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
                       ASTRA::TCompLayerType& layer_type,
                       bool &isseat);
      std::string get( const PointId_t& point_id, PaxId_t pax_id,
                       const std::string& format,
                       bool &isseat) {
         ASTRA::TCompLayerType layer_type;
         return get( point_id, pax_id, format, layer_type, isseat );
      }
      std::string get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                       const std::string& format,
                       ASTRA::TCompLayerType& layer_type,
                       bool &isseat );
      std::string get( const PointIdTlg_t& point_id_tlg, PaxId_t pax_id,
                       const std::string& format,
                       bool &isseat) {
         ASTRA::TCompLayerType layer_type;
         return get( point_id_tlg, pax_id, format, layer_type, isseat );
      }

      static std::string get( const  TSalonList& salonList,
                              PaxId_t pax_id,
                              const std::string& format,
                              bool free_seating,
                              ASTRA::TCompLayerType& layer_type,
                              bool &isseat);
      //получение места зарегистрированного пассажира или предыдущее место "(..)" - загрузка карты мест не происходит
  /*    static std::string checkin_seat_no( int point_id, PaxId_t pax_id,
                                          const std::string& format, bool pr_lat_seat,
                                          bool &isseat);*/
  };
} //end namespace SEATSPAX
