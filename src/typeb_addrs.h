#ifndef _TYPEB_ADDRS_H_
#define _TYPEB_ADDRS_H_

#include "hist.h"

namespace TypeB {

void deleteCreatePoints(const RowId_t &typebAddrsId);

void deleteTypebOptions(const RowId_t &typebAddrsId);

void syncTypebOptions(const RowId_t &typebAddrsId, const std::string &basicType);

void syncCOMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version);

void syncETLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &rbd);

void syncMVTOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string noend);

void syncLDMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string cfg,
                    const std::string cabinBaggage, const std::string gender,
                    const std::string exb, const std::string noend);

void syncLCIOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string equipment,
                    const std::string weightAvail, const std::string seating,
                    const std::string weightMode, const std::string seatRestrict,
                    const std::string pasTotals, const std::string bagTotals,
                    const std::string pasDistrib, const std::string seatPlan);

void syncPRLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string createPoint,
                    const std::string paxState, const std::string rbd,
                    const std::string xbag);

void syncBSMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string classOfTravel, const std::string tagPrinterId,
                    const std::string pasNameRp1745, const std::string actualDepDate,
                    const std::string brd, const std::string trferIn,
                    const std::string longFltNo);

void syncPNLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &forwarding);

} // namespace TypeB

#endif
