#pragma once

#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>


DECL_RIP(PointId_t,    int);
DECL_RIP(PointIdTlg_t, int);
DECL_RIP(GrpId_t,      int);
DECL_RIP(PnrId_t,      int);
DECL_RIP(ReqCtxtId_t,  int);
DECL_RIP(MoveId_t,     int);
DECL_RIP(RouteIdScd_t, int);

DECL_RIP(TrferId_t,    int);
DECL_RIP(TrferGrpId_t, int);
DECL_RIP(TlgId_t,      int);

DECL_RIP_RANGED(PaxId_t, int,    1, 2000000000);
DECL_RIP_RANGED(RegNo_t, int, -999, 999);
DECL_RIP_RANGED(SegNo_t, int,    1, 9);

DECL_RIP_LENGTH(AirlineCode_t, std::string, 2, 3);
DECL_RIP_LENGTH(AirportCode_t, std::string, 3, 3);
DECL_RIP_LENGTH(CityCode_t,    std::string, 3, 3);
DECL_RIP_LENGTH(CountryCode_t, std::string, 2, 2);

DECL_RIP_LENGTH(Class_t,       std::string, 1, 1);
DECL_RIP_LENGTH(SubClass_t,    std::string, 1, 1);

DECL_RIP_LENGTH(Surname_t,     std::string, 0, 64);
DECL_RIP_LENGTH(Name_t,        std::string, 0, 64);

DECL_RIP_LENGTH(CrsSender_t,   std::string, 7, 7);
DECL_RIP_RANGED(CrsPriority_t, int,    0, 9);

