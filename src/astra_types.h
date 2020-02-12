#pragma once

#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>


DECL_RIP(PointId_t,    int);
DECL_RIP(PointIdTlg_t, int);
DECL_RIP(GrpId_t,      int);
DECL_RIP(PaxId_t,      int);
DECL_RIP(PnrId_t,      int);
DECL_RIP(ReqCtxtId_t,  int);
DECL_RIP(MoveId_t,     int);
DECL_RIP(RouteIdScd_t, int);

DECL_RIP_RANGED(RegNo_t, int, -999, 999);

DECL_RIP_LENGTH(AirlineCode_t, std::string, 2, 3);
DECL_RIP_LENGTH(AirportCode_t, std::string, 3, 3);
DECL_RIP_LENGTH(CityCode_t,    std::string, 3, 3);
DECL_RIP_LENGTH(CountryCode_t, std::string, 2, 2);

DECL_RIP_LENGTH(Surname_t,     std::string, 0, 64);
DECL_RIP_LENGTH(Name_t,        std::string, 0, 64);
