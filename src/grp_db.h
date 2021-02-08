#pragma once

#include "astra_types.h"

bool deleteTransfers(GrpId_t grp_id);
bool deleteTCkinSegments(GrpId_t grp_id);
bool deleteAnnulTags(GrpId_t grp_id);
bool deleteAnnulBag(GrpId_t grp_id);
bool deleteBagPrepay(GrpId_t grp_id);
bool deleteBagTags(GrpId_t grp_id);
bool deleteBagTags(GrpId_t grp_id, int bag_num);
bool deleteBagTagsGenerated(GrpId_t grp_id);
bool deleteUnaccompBagInfo(GrpId_t grp_id);
bool deleteUnaccompBagInfo(GrpId_t grp_id, int num);
bool deleteBag2(GrpId_t grp_id);
bool deleteBag2(GrpId_t grp_id, int num);
bool deleteGrpNorms(GrpId_t grp_id);
bool deletePaidBag(GrpId_t grp_id);
bool deletePaidBagEmdProps(GrpId_t grp_id);
bool deleteServicePayment(GrpId_t grp_id);
bool deleteTckinPaxGrp(GrpId_t grp_id);
bool deleteValueBag(GrpId_t grp_id);
bool deleteValueBag(GrpId_t grp_id, int num);
bool deletePnrAddrsPC(GrpId_t grp_id);
bool deleteGrpServiceLists(GrpId_t grp_id);
bool deleteSvcPrices(GrpId_t grp_id);
bool deletePaxGrp(GrpId_t grp_id);
bool clearBagReceiptsGrpId(GrpId_t grp_id);

bool deleteGroupData(GrpId_t grp_id);
