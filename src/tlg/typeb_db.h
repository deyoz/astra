#pragma once

#include <set>
#include <string>
#include "astra_types.h"

namespace TypeB
{

std::set<PaxId_t> loadPaxIdSet(const PointIdTlg_t& point_id, const std::string& system,
                               const std::optional<CrsSender_t>& sender);

bool deleteCrsSeatsBlocking(const PaxId_t& pax_id);
bool deleteCrsInf(const PaxId_t& pax_id);
bool deleteCrsInfDeleted(const PaxId_t& pax_id);
bool deleteCrsPaxRem(const PaxId_t& pax_id);
bool deleteCrsPaxDoc(const PaxId_t& pax_id);
bool deleteCrsPaxDoco(const PaxId_t& pax_id);
bool deleteCrsPaxDoca(const PaxId_t& pax_id);
bool deleteCrsPaxTkn(const PaxId_t& pax_id);
bool deleteCrsPaxFqt(const PaxId_t& pax_id);
bool deleteCrsPaxChkd(const PaxId_t& pax_id);
bool deleteCrsPaxAsvc(const PaxId_t& pax_id);
bool deleteCrsPaxRefuse(const PaxId_t& pax_id);
bool deleteCrsPaxAlarms(const PaxId_t& pax_id);
bool deleteCrsPaxContext(const PaxId_t& pax_id);
bool deleteCrsPaxContext(const PaxId_t& pax_id, const std::string& key);
bool deleteDcsBag(const PaxId_t& pax_id);
bool deleteDcsTags(const PaxId_t& pax_id);
bool deleteTripCompLayers(const PaxId_t& pax_id);
bool deleteTlgCompLayers(const PaxId_t& pax_id);
bool deleteTlgCompLayers(const PointIdTlg_t& point_id);
bool deleteTlgSource(const PointIdTlg_t& point_id);
bool deleteTlgTrips(const PointIdTlg_t& point_id);
bool deletePaxCalcData(const PaxId_t& pax_id);
bool deleteCrsDataStat(const PointIdTlg_t& point_id);
bool deleteTypeBDataStat(const PointIdTlg_t& point_id);

bool nullCrsDisplace2_point_id_tlg(const PointIdTlg_t& point_id);

bool deletePnrAddrs(const PnrId_t& pnr_id);
bool deleteCrsTransfer(const PnrId_t& pnr_id);
bool deleteCrsPax(const PnrId_t& pnr_id);
bool deletePnrMarketFlt(const PnrId_t& pnr_id);

bool deleteCrsPnr(const PointIdTlg_t& point_id,
                  const std::string& system, const std::optional<CrsSender_t>& sender);
bool deleteCrsData(const PointIdTlg_t& point_id,
                   const std::string& system, const std::optional<CrsSender_t>& sender);
bool deleteCrsRbd(const PointIdTlg_t& point_id,
                  const std::string& system, const std::optional<CrsSender_t>& sender);

bool deleteTypeBData(const PointIdTlg_t& point_id, const std::string& system = "",
                     const std::optional<CrsSender_t>& sender = {},
                     bool delete_trip_comp_layers = false);

std::string getPSPT(int pax_id, bool with_issue_country = false,
                    const std::string& language = "RU");

} //namespace TypeB
