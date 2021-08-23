#pragma once
#include <string>
#include "astra_consts.h"
#include "salons.h"

namespace SEATSPAX
{
  using namespace SALONS2;

  class PaxListSeatNo { // ��� ࠡ��� � ����࠯�� ���� �� ���⪥ ᯨ᪠ ���ᠦ�஢, �ᯮ�짮���� ���� ��������� ����� ��� �ᥣ� ᯨ᪠!!!
    private:
      TSalonList salonList;
      bool free_seating;
      static std::string int_checkin_seat_no( int point_id, PaxId_t pax_id, bool pr_wl,
                                              const std::string& format, bool pr_lat_seat );
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
      //��� ࠡ��� � ��㯯�� ���ᠦ�஢, ���樠������ ��������� ����� ��� ������� ���ᠦ�� �祭� ����⭮
      //�� ��뢠��, ����� ���� ������� get_seat_no || get_crs_seat_no
      std::string get( PaxId_t pax_id,
                       const std::string& format,
                       ASTRA::TCompLayerType& layer_type );
      std::string get( PaxId_t pax_id,
                       const std::string& format ) {
         ASTRA::TCompLayerType layer_type;
         return get( pax_id, format, layer_type );
      }
      static std::string get( const  TSalonList& salonList,
                              PaxId_t pax_id,
                              const std::string& format,
                              bool free_seating,
                              ASTRA::TCompLayerType& layer_type );
      //����祭�� ���� ��ॣ����஢������ ���ᠦ�� ��� �।��饥 ���� "(..)" - ����㧪� ����� ���� �� �ந�室��
      static std::string checkin_seat_no( int point_id, PaxId_t pax_id, const std::string& format, bool pr_lat_seat );
    public:
      PaxListSeatNo( int point_id ):salonList(true) {
        TSalonListReadParams params;
        params.for_calc_waitlist = true; //�� �㣠���� �᫨ isFreeSeating || ��� ��� ᠫ���
        salonList.ReadFlight(TFilterRoutesSets(point_id),params);
        free_seating = isFreeSeating(point_id);
      }
  };
} //end namespace SEATSPAX