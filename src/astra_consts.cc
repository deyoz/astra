#include "astra_consts.h"

namespace ASTRA
{

const char * ClientTypeS[] =
{"TERM", "WEB", "KIOSK", "PNL", "HTTP", "MOBIL"};

const char * OperModeS[] =
    {"CUSE", "CUTE", "MUSE", "RESA", "STAND"};

const char * EventTypeS[] =
    {"ëÖá","Ñàë","êÖâ","Éêî","áÑó","èÄë","éèã","äåè","íãÉ","Ñëí","ëàë","äéÑ","èêÑ","èóí","!","?"};

const char* TClassS[] = {"è","Å","ù",""};

const char* TPersonS[] = {"Çá","êÅ","êå",""};

const int TQueueS[] = {1,2,4,6,8,0};

const char* TPaxStatusS[] = {"K","C","T","P","E"};

const char* CompLayerTypeS[] = {"BLOCK_CENT","TRANZIT","CHECKIN","TCHECKIN","GOSHOW","BLOCK_TRZT","SOM_TRZT","PRL_TRZT",
                                "PROT_BPAY", "PROT_APAY", "PNL_BPAY", "PNL_APAY",
                                  "PROT_TRZT","PNL_CKIN","PROT_CKIN", "PROTECT","UNCOMFORT","SMOKE","DISABLE",""};

const char* BagNormTypeS[] = {"Åè", "éÅ", "èã", "Åè/ëç", "Åè/éÅ", "Åè/èã", "éÅ/èã", ""};

const char *RptTypeS[] = {
    "PTM",
    "PTMTXT",
    "BTM",
    "BTMTXT",
    "WEB",
    "WEBTXT",
    "REFUSE",
    "REFUSETXT",
    "NOTPRES",
    "NOTPRESTXT",
    "REM",
    "REMTXT",
    "CRS",
    "CRSTXT",
    "CRSUNREG",
    "CRSUNREGTX",
    "EXAM",
    "EXAMTXT",
    "NOREC",
    "NORECTXT",
    "GOSHO",
    "GOSHOTXT",
    "BDOCS",
    "SPEC",
    "SPECTXT",
    "EMD",
    "EMDTXT",
    "LOADSHEET",
    "NOTOC",
    "LIR",
    "ANNUL_TAGS",
    "VOUCHERS",
    "SERVICES",
    "SERVICESTXT",
    "?"
};

}

const ASTRA::TCrewTypes& CrewTypes()
{
  static ASTRA::TCrewTypes crewTypes;
  return crewTypes;
}

const TAlignments& Alignments()
{
  static TAlignments alignments;
  return alignments;
}

std::string ASTRA::TPaxTypeExt::ToString() const
{
    std::ostringstream oss;
    oss << "TPaxTypeExt: _pax_status=" << ASTRA::TPaxStatusS[_pax_status] << "; _crew_type=" << CrewTypes().encode(_crew_type) << ";";
    return oss.str();
}
