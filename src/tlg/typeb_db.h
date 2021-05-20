#pragma once

#include <set>
#include <string>
#include "astra_types.h"

namespace TypeB
{

std::set<PaxId_t> loadPaxIdSet(PointIdTlg_t point_id, const std::string& system,
                           const std::string& sender);
bool deleteCrsSeatsBlocking(PaxId_t pax_id);
bool deleteCrsInf(PaxId_t pax_id);
bool deleteCrsInfDeleted(PaxId_t pax_id);
bool deleteCrsPaxRem(PaxId_t pax_id);
bool deleteCrsPaxDoc(PaxId_t pax_id);
bool deleteCrsPaxDoco(PaxId_t pax_id);
bool deleteCrsPaxDoca(PaxId_t pax_id);
bool deleteCrsPaxTkn(PaxId_t pax_id);
bool deleteCrsPaxFqt(PaxId_t pax_id);
bool deleteCrsPaxChkd(PaxId_t pax_id);
bool deleteCrsPaxAsvc(PaxId_t pax_id);
bool deleteCrsPaxRefuse(PaxId_t pax_id);
bool deleteCrsPaxAlarms(PaxId_t pax_id);
bool deleteCrsPaxContext(PaxId_t pax_id);
bool deleteCrsPaxContext(PaxId_t pax_id, const std::string& key);
bool deleteDcsBag(PaxId_t pax_id);
bool deleteDcsTags(PaxId_t pax_id);
bool deleteTripCompLayers(PaxId_t pax_id);
bool deleteTlgCompLayers(PaxId_t pax_id);
bool deleteTlgCompLayers(const PointIdTlg_t& point_id);
bool deleteTlgSource(const PointIdTlg_t& point_id);
bool deleteTlgTrips(const PointIdTlg_t& point_id);
bool deletePaxCalcData(PaxId_t pax_id);
bool deleteCrsDataStat(const PointIdTlg_t& point_id);
bool deleteTypeBDataStat(const PointIdTlg_t& point_id);

bool nullCrsDisplace2_point_id_tlg(const PointIdTlg_t& point_id);


bool deleteTypeBData(const PointIdTlg_t& point_id, const std::string& system = "",
                     const std::optional<CrsSender_t>& sender = {},
                     bool delete_trip_comp_layers = false);

std::string getPSPT(int pax_id, bool with_issue_country = false,
                    const std::string& language = "RU");

} //namespace TypeB
