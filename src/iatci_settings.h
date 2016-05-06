#pragma once

#include "tlg/CheckinBaseTypes.h"


namespace iatci {

struct IatciSettings
{
    Ticketing::SystemAddrs_t DcsId;
    bool Cki;
    bool Ckx;
    bool Cku;
    bool Bpr;
    bool Plf;
    bool Smf;

    IatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                  bool cki, bool ckx, bool cku,
                  bool bpr, bool plf, bool smf);

    static IatciSettings defaultSettings(const Ticketing::SystemAddrs_t& dcsId);
};

IatciSettings readIatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                                       bool createDefault = false);
void writeIatciSettings(const IatciSettings& settings);

bool checkExistance(const Ticketing::SystemAddrs_t& dcsId);

}//namespace iatci
