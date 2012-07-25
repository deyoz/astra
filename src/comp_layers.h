#ifndef _COMP_LAYERS_H_
#define _COMP_LAYERS_H_

#include <string>
#include <vector>
#include <set>
#include "astra_consts.h"
#include "seats_utils.h"

bool IsTlgCompLayer(ASTRA::TCompLayerType layer_type);

typedef std::set< std::pair<int, ASTRA::TCompLayerType> > TPointIdsForCheck;

//���������� ᫮� � tlg_comp_layers � ᨭ�஭���樥� trip_comp_layers � ������� � ��ୠ� ����権 ��� clt...Pay, cltProtCkin
void InsertTlgSeatRanges(int point_id_tlg,
                         std::string airp_arv,
                         ASTRA::TCompLayerType layer_type,
                         const std::vector<TSeatRange> &ranges,
                         int crs_pax_id,  //����� ���� NoExists
                         int tlg_id,      //����� ���� NoExists
                         int timeout,     //����� ���� NoExists
                         bool UsePriorContext,
                         int &curr_tid,                     //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������
//㤠����� ᫮� �� tlg_comp_layers � ᨭ�஭���樥� trip_comp_layers � ������� � ��ୠ� ����権 ��� clt...Pay, cltProtCkin
void DeleteTlgSeatRanges(ASTRA::TCompLayerType layer_type,
                         int crs_pax_id,           //�� ����� ���� NoExists
                         int &curr_tid,            //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������
//��뢠���� �� ���ᠭ��� ��� DeleteTlgSeatRanges ���� �� astra_timer
void DeleteTlgSeatRanges(std::vector<int> range_ids,
                         int crs_pax_id,           //����� ���� NoExists
                         int &curr_tid,            //�᫨ NoExists, � ���樠��������� � ��楤��, �㦨� ��� ���������� crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������
                         
void InsertTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������

void DeleteTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������
                          
void SyncTripCompLayers(int point_id_tlg, //point_id_tlg ���� point_id_spp �.�. NoExists
                        int point_id_spp,
                        ASTRA::TCompLayerType layer_type,
                        TPointIdsForCheck &point_ids_spp); //����� point_id_spp �� ����� �뫨 ���������

//�����頥� ᫮� �᫨ ६�ઠ ���� PNL ��� 㪠������ �������� ����� ���� �ᯮ�짮���� ��� ���⭮� ॣ����樨
//���� �����頥� cltUnknown
ASTRA::TCompLayerType GetSeatRemLayer(const std::string &airline_mark, const std::string &seat_rem);

typedef std::vector< std::pair<std::string, int> > TSeatRemPriority;
//�����頥� ���஢����! �� �ਮ��⠬ ᯨ᮪ ६�ப
void GetSeatRemPriority(const std::string &airline_mark, TSeatRemPriority &rems);

void check_layer_change(const TPointIdsForCheck &point_ids_spp);


#endif

