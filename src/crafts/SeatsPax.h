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

  //����� ��� ࠡ��� � ����� ���ᠦ�஬
  class TSeatPax {
      public:
        enum EnumFormatSeats { efOne, ef_One, efTlg, efList, ef_List, efVoland, ef_Seats, efSeats };
      private:
        //���� ����, ����� ���ᠦ�� ��ॣ����஢��, � ����� � ⠡��� pax_seats ���, � ᫮� ����
        // ���� ������ "��", � ���� "(����)" �� �ਭ樯���쭮, �� ��� �ࠢ����� ᤥ����
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
           fmt - �ଠ� ������.
           ����� �ਭ����� ���祭��:
           'one':    ���� ����� ���孥� ���� ���ᠦ��
           '_one':   ���� ����� ���孥� ���� ���ᠦ�� + �஡��� ᫥��
           'tlg':    ����᫥��� �ᥢ��-IATA ���� ���ᠦ��. ����஢�� �� ��������: 4�4�4�
           'list':   ����᫥��� ��� ���� ���ᠦ�� �१ �஡��. ����஢�� �� ��������.
           '_list':  ����᫥��� ��� ���� ���ᠦ�� �१ �஡��. ����஢�� �� �������� + �஡��� ᫥��
           'voland': �. ���ᠭ�� �ଠ� � ��楤��. �ᯮ������ ��� ���� ���. ⠫����.
           '_seats': ���� ����� ���孥� ���� ���ᠦ�� ���� ��⠢襥�� ���-�� ����: 4�+2 + �஡��� ᫥��
           ����:    ���� ����� ���孥� ���� ���ᠦ�� ���� ��⠢襥�� ���-�� ����: 4�+2
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
      //����� �맢��� ����, � 祬 ����� ��ࠬ��஢ ��।���, ⥬ ����� ����ᮢ
      //common - ���� �㭪��, ����� ��୥� ����� ���� ॣ����樨, �᫨ ���ᠦ�� ��ॣ����஢�� ���� �஭�
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

  //����� ��� �ᯮ�짮����, ����� ���� ��᪮�쪮 ����ᮢ �� ������ ���� ���ᠦ��. ���ਬ�� � ࠧ�묨 pr_lat_seat ��� �ଠ⠬�
  //���ਬ�� � ��ᠤ��
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

  //��� ����� ��� ࠡ��� � ᯨ᪮� ���ᠦ�஢
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

  //�� ᪮� ���
  class PaxListSeatNo { // ��� ࠡ��� � ����࠯�� ���� �� ���⪥ ᯨ᪠ ���ᠦ�஢, �ᯮ�짮���� ���� ������� ����� ��� �ᥣ� ᯨ᪠!!!
    private: //point_dep, pair<free_seating,TSalonList>
      std::map<int,std::pair<bool,TSalonList>> salonLists;
      std::map<int,std::set<PointId_t>> point_tlgs;
    public:
      enum EnumWaitList  { spNone,
                           spWL //���� �������� "(12D)" ��� "��"
                            };
      /*
         fmt - �ଠ� ������.
         ����� �ਭ����� ���祭��:
         'one':    ���� ����� ���孥� ���� ���ᠦ��
         '_one':   ���� ����� ���孥� ���� ���ᠦ�� + �஡��� ᫥��
         'tlg':    ����᫥��� �ᥢ��-IATA ���� ���ᠦ��. ����஢�� �� ��������: 4�4�4�
         'list':   ����᫥��� ��� ���� ���ᠦ�� �१ �஡��. ����஢�� �� ��������.
         '_list':  ����᫥��� ��� ���� ���ᠦ�� �१ �஡��. ����஢�� �� �������� + �஡��� ᫥��
         'voland': �. ���ᠭ�� �ଠ� � ��楤��. �ᯮ������ ��� ���� ���. ⠫����.
         '_seats': ���� ����� ���孥� ���� ���ᠦ�� ���� ��⠢襥�� ���-�� ����: 4�+2 + �஡��� ᫥��
         ����:    ���� ����� ���孥� ���� ���ᠦ�� ���� ��⠢襥�� ���-�� ����: 4�+2
      */
      enum EnumFormat { one, _one, tlg, list, _list, voland, _seats, def };
      //��� ࠡ��� � ��㯯�� ���ᠦ�஢, ���樠������ ������� ����� ��� ������� ���ᠦ�� �祭� ����⭮
      //�� ��뢠��, ����� ���� ������� get_seat_no || get_crs_seat_no

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
      //����祭�� ���� ��ॣ����஢������ ���ᠦ�� ��� �।��饥 ���� "(..)" - ����㧪� ����� ���� �� �ந�室��
  /*    static std::string checkin_seat_no( int point_id, PaxId_t pax_id,
                                          const std::string& format, bool pr_lat_seat,
                                          bool &isseat);*/
  };
} //end namespace SEATSPAX
