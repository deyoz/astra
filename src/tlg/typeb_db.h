#pragma once

#include <set>
#include <string>

namespace TypeB
{

std::set<int> LoadPaxIdSet(int point_id, const std::string& system,
                           const std::string& sender);
bool DeleteCrsSeatsBlocking(int pax_id);
bool DeleteCrsInf(int pax_id);
bool DeleteCrsInfDeleted(int pax_id);
bool DeleteCrsPaxRem(int pax_id);
bool DeleteCrsPaxDoco(int pax_id);
bool DeleteCrsPaxDoca(int pax_id);
bool DeleteCrsPaxTkn(int pax_id);
bool DeleteCrsPaxFqt(int pax_id);
bool DeleteCrsPaxChkd(int pax_id);
bool DeleteCrsPaxAsvc(int pax_id);
bool DeleteCrsPaxRefuse(int pax_id);
bool DeleteCrsPaxAlarms(int pax_id);
bool DeleteCrsPaxContext(int pax_id);
bool DeleteDcsBag(int pax_id);
bool DeleteDcsTags(int pax_id);
bool DeleteTripCompLayers(int pax_id);
bool DeleteTlgCompLayers(int pax_id);
bool DeletePaxCalcData(int pax_id);

bool DeleteTypeBData(int point_id, const std::string& system, const std::string& sender,
                     bool delete_trip_comp_layers);

std::string getPSPT(int pax_id, bool with_issue_country = false,
                    const std::string& language = "RU");

} //namespace TypeB
