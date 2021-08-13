#pragma once

#include "astra_types.h"

bool deleteAnnulTagsById(int id);

bool deleteAnnulTags(PaxId_t pax_id);
bool deleteAnnulBag(PaxId_t pax_id);
bool deletePaxEvents(PaxId_t pax_id);
bool deleteStatAd(PaxId_t pax_id);
bool deleteStatServices(PaxId_t pax_id);
bool deleteConfirmPrint(PaxId_t pax_id);
bool deletePaxTranslit(PaxId_t pax_id);
bool deletePaxDOC(PaxId_t pax_id);
bool deletePaxDOCO(PaxId_t pax_id);
bool deletePaxDOCA(PaxId_t pax_id);
bool deletePaxFQT(PaxId_t pax_id);
bool deletePaxASVC(PaxId_t pax_id);
bool deletePaxEmd(PaxId_t pax_id);
bool deletePaxNorms(PaxId_t pax_id);
bool deletePaxBrands(PaxId_t pax_id);
bool deletePaxRem(PaxId_t pax_id);
bool deletePaxRemOrigin(PaxId_t pax_id);
bool deletePaxSeats(PaxId_t pax_id);
bool deleteRozysk(PaxId_t pax_id);
bool deleteTransferSubcls(PaxId_t pax_id);
bool deleteTripCompLayers(PaxId_t pax_id);
bool deletePaxAlarms(PaxId_t pax_id);
bool deletePaxCustomAlarms(PaxId_t pax_id);
bool deletePaxServiceLists(PaxId_t pax_id);
bool deletePaxServices(PaxId_t pax_id);
bool deletePaxServicesAuto(PaxId_t pax_id);
bool deletePaidRfisc(PaxId_t pax_id);
bool deletePaxNormsText(PaxId_t pax_id);
bool deleteTrferPaxStat(PaxId_t pax_id);
bool deleteBiStat(PaxId_t pax_id);
bool deleteSBDOTagsGenerated(PaxId_t pax_id);
bool deletePaxCalcData(PaxId_t pax_id);
bool deletePaxConfirmations(PaxId_t pax_id);
bool deletePax(PaxId_t pax_id);
bool clearServicePaymentPaxId(PaxId_t pax_id);

bool deletePaxData(PaxId_t pax_id);
