#ifndef _COMP_LAYERS_H_
#define _COMP_LAYERS_H_

#include <string>
#include <vector>
#include <set>
#include "astra_consts.h"
#include "seats_utils.h"

bool IsTlgCompLayer(ASTRA::TCompLayerType layer_type);

typedef std::set< std::pair<int, ASTRA::TCompLayerType> > TPointIdsForCheck;

//добавление слоя в tlg_comp_layers с синхронизацией trip_comp_layers и записью в журнал операций для clt...Pay, cltProtCkin
void InsertTlgSeatRanges(int point_id_tlg,
                         std::string airp_arv,
                         ASTRA::TCompLayerType layer_type,
                         const std::vector<TSeatRange> &ranges,
                         int crs_pax_id,  //может быть NoExists
                         int tlg_id,      //может быть NoExists
                         int timeout,     //может быть NoExists
                         bool UsePriorContext,
                         int &curr_tid,                     //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения
//удаление слоя из tlg_comp_layers с синхронизацией trip_comp_layers и записью в журнал операций для clt...Pay, cltProtCkin
void DeleteTlgSeatRanges(ASTRA::TCompLayerType layer_type,
                         int crs_pax_id,           //не может быть NoExists
                         int &curr_tid,            //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения
//вызывается из описанной выше DeleteTlgSeatRanges либо из astra_timer
void DeleteTlgSeatRanges(std::vector<int> range_ids,
                         int crs_pax_id,           //может быть NoExists
                         int &curr_tid,            //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения
                         
void InsertTripCompLayers(int point_id_tlg, //point_id_tlg либо point_id_spp м.б. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения

void DeleteTripCompLayers(int point_id_tlg, //point_id_tlg либо point_id_spp м.б. NoExists
                          int point_id_spp,
                          ASTRA::TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения
                          
void SyncTripCompLayers(int point_id_tlg, //point_id_tlg либо point_id_spp м.б. NoExists
                        int point_id_spp,
                        ASTRA::TCompLayerType layer_type,
                        TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения

//возвращает слой если ремарка места PNL для указанной компании может быть использована для платной регистрации
//иначе возвращает cltUnknown
ASTRA::TCompLayerType GetSeatRemLayer(const std::string &airline_mark, const std::string &seat_rem);

typedef std::vector< std::pair<std::string, int> > TSeatRemPriority;
//возвращает сортированный! по приоритетам список ремарок
void GetSeatRemPriority(const std::string &airline_mark, TSeatRemPriority &rems);


#endif

