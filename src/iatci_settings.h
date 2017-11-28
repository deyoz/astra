#pragma once

#include "tlg/CheckinBaseTypes.h"


namespace iatci {

struct IatciSettings
{
    Ticketing::SystemAddrs_t DcsId;
    // Поддерживаемые типы сообщений
    bool Cki;
    bool Ckx;
    bool Cku;
    bool Bpr;
    bool Plf;
    bool Smf;

    // Поддержка IFM
    bool Ifm;


    IatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                  bool cki, bool ckx, bool cku,
                  bool bpr, bool plf, bool smf,
                  bool ifm);

    static IatciSettings defaultSettings(const Ticketing::SystemAddrs_t& dcsId);

public: 
    bool ifmSupported() const { return Ifm; }
};

IatciSettings readIatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                                       bool createDefault = false);
void writeIatciSettings(const IatciSettings& settings);

bool checkExistance(const Ticketing::SystemAddrs_t& dcsId);

}//namespace iatci
