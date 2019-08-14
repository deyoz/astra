#ifndef _COMP_LAYERS_H_
#define _COMP_LAYERS_H_

#include <string>
#include <vector>
#include <set>
#include "astra_consts.h"
#include "seats_utils.h"

bool IsTlgCompLayer(ASTRA::TCompLayerType layer_type);

typedef std::set< std::pair<int, ASTRA::TCompLayerType> > TPointIdsForCheck;

class SeatRangeIds : public std::set<int> {};

class TlgSeatRanges : public std::map< ASTRA::TCompLayerType, std::set<TSeatRange> >
{
  public:
    void add(ASTRA::TCompLayerType layerType, const TSeatRange& seatRange);
    void add(ASTRA::TCompLayerType layerType, const TSeatRanges& seatRanges);
    void get(const std::initializer_list<ASTRA::TCompLayerType>& layerTypes,
             int crs_pax_id);
    void dump(const std::string& title) const;
};

class PaxIdsForDeleteTlgSeatRangesItem
{
  public:
    PaxIdsForDeleteTlgSeatRangesItem(const std::initializer_list<ASTRA::TCompLayerType>& _layerTypes) : layerTypes(_layerTypes) {}
    std::initializer_list<ASTRA::TCompLayerType> layerTypes;
};

class PaxIdsForDeleteTlgSeatRanges : public std::map< int, PaxIdsForDeleteTlgSeatRangesItem >
{
  public:
    void add(const std::initializer_list<ASTRA::TCompLayerType>& layerTypes,
             int crs_pax_id);
    void handle(int &curr_tid,
                TPointIdsForCheck &point_ids_spp) const;
    bool exists(int crs_pax_id) const
    {
      return find(crs_pax_id)!=end();
    }
};

class PaxIdsForInsertTlgSeatRangesItem
{
  public:
    PaxIdsForInsertTlgSeatRangesItem(const std::string& _airp_arv, const TlgSeatRanges& _tlgSeatRanges) :
      airp_arv(_airp_arv), tlgSeatRanges(_tlgSeatRanges) {}
    std::string airp_arv;
    TlgSeatRanges tlgSeatRanges;
};

class PaxIdsForInsertTlgSeatRanges : public std::map< int, PaxIdsForInsertTlgSeatRangesItem >
{
  public:
    void add(const std::string& airp_arv,
             const TlgSeatRanges& tlgSeatRanges,
             int crs_pax_id);
    void handle(int point_id_tlg,
                int tlg_id,
                int &curr_tid,
                TPointIdsForCheck &point_ids_spp) const;
    bool exists(int crs_pax_id) const
    {
      return find(crs_pax_id)!=end();
    }
};

//добавление слоя в tlg_comp_layers с синхронизацией trip_comp_layers и записью в журнал операций для clt...Pay, cltProtCkin
void InsertTlgSeatRanges(int point_id_tlg,
                         const std::string& airp_arv,
                         ASTRA::TCompLayerType layer_type,
                         const TSeatRanges &ranges,
                         int crs_pax_id,  //может быть NoExists
                         int tlg_id,      //может быть NoExists
                         int timeout,     //может быть NoExists
                         bool UsePriorContext,
                         int &curr_tid,                     //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения
//удаление слоя из tlg_comp_layers с синхронизацией trip_comp_layers и записью в журнал операций для clt...Pay, cltProtCkin
void DeleteTlgSeatRanges(const std::initializer_list<ASTRA::TCompLayerType>& layer_types,
                         int crs_pax_id,           //не может быть NoExists
                         int &curr_tid,            //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения

void DeleteTlgSeatRanges(const std::initializer_list<ASTRA::TCompLayerType>& layer_types,
                         int crs_pax_id,
                         int &curr_tid);

//вызывается из описанной выше DeleteTlgSeatRanges либо из astra_timer
void DeleteTlgSeatRanges(const SeatRangeIds &range_ids,
                         int crs_pax_id,           //может быть NoExists
                         int &curr_tid,            //если NoExists, то инициализируется в процедуре, служит для обновления crs_pax.tid
                         TPointIdsForCheck &point_ids_spp); //вектор point_id_spp по которым были изменения

void GetTlgSeatRanges(ASTRA::TCompLayerType layer_type,
                      int crs_pax_id,
                      TSeatRanges &ranges);

void GetTlgSeatRangeIds(const std::initializer_list<ASTRA::TCompLayerType>& layerTypes,
                        int crs_pax_id,
                        SeatRangeIds& range_ids);

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

void InsertTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TPointIdsForCheck &point_ids_spp);

void DeleteTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TPointIdsForCheck &point_ids_spp);

void SyncTripCompLayers(int point_id_tlg,
                        int point_id_spp,
                        TPointIdsForCheck &point_ids_spp);

//возвращает слой если ремарка места PNL для указанной компании может быть использована для платной регистрации
//иначе возвращает cltUnknown
ASTRA::TCompLayerType GetSeatRemLayer(const std::string &airline_mark, const std::string &seat_rem);

typedef std::vector< std::pair<std::string, int> > TSeatRemPriority;
//возвращает сортированный! по приоритетам список ремарок
void GetSeatRemPriority(const std::string &airline_mark, TSeatRemPriority &rems);

void check_layer_change(const TPointIdsForCheck &point_ids_spp,
                        const std::string& whence);

void check_layer_change(const TPointIdsForCheck &point_ids_spp,
                        const std::set<int> &paxs_external_logged,
                        const std::string& whence);
#endif

