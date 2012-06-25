#ifndef _COMP_LAYERS_H_
#define _COMP_LAYERS_H_

#include <string>
#include <vector>
#include "astra_consts.h"
#include "seats_utils.h"

bool IsTlgCompLayer(ASTRA::TCompLayerType layer_type);

//���������� ᫮� � tlg_comp_layers � ᨭ�஭���樥� trip_comp_layers � ������� � ��ୠ� ����権 ��� clt...Pay, cltProtCkin
void InsertTlgSeatRanges(int point_id_tlg,
                         std::string airp_arv,
                         ASTRA::TCompLayerType layer_type,
                         const std::vector<TSeatRange> &ranges,
                         int crs_pax_id,  //����� ���� NoExists
                         int tlg_id,      //����� ���� NoExists
                         int timeout,     //����� ���� NoExists
                         bool UsePriorContext,
                         int &curr_tid);  //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
//㤠����� ᫮� �� tlg_comp_layers � ᨭ�஭���樥� trip_comp_layers � ������� � ��ୠ� ����権 ��� clt...Pay, cltProtCkin
void DeleteTlgSeatRanges(ASTRA::TCompLayerType layer_type,
                         int crs_pax_id,           //�� ����� ���� NoExists
                         int &curr_tid);           //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
//��뢠���� �� ���ᠭ��� ��� DeleteTlgSeatRanges ���� �� astra_timer
void DeleteTlgSeatRanges(std::vector<int> range_ids,
                         int crs_pax_id,           //����� ���� NoExists
                         int &curr_tid);           //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
                         
/*void InsertTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type); �� �ᯮ������. ��� �ᯮ�짮����� �஢���� �맮� check_waitlist_alarm( point_id );*/

//��� �ᯮ�짮����� �஢���� �맮� check_waitlist_alarm( point_id );
void DeleteTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type);
                          
//��� �ᯮ�짮����� �஢���� �맮� check_waitlist_alarm( point_id );*/
void SyncTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                        int point_id_spp,
                        ASTRA::TCompLayerType layer_type);

//�����頥� ᫮� �᫨ ६�ઠ ���� PNL ��� 㪠������ �������� ����� ���� �ᯮ�짮���� ��� ���⭮� ॣ����樨
//���� �����頥� cltUnknown
ASTRA::TCompLayerType GetSeatRemLayer(const std::string &airline_mark, const std::string &seat_rem);

typedef std::vector< std::pair<std::string, int> > TSeatRemPriority;
//�����頥� ���஢����! �� �ਮ��⠬ ᯨ᮪ ६�ப
void GetSeatRemPriority(const std::string &airline_mark, TSeatRemPriority &rems);


#endif

