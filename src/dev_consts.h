#ifndef _DEV_CONSTS_H_
#define _DEV_CONSTS_H_

namespace ASTRA {
    enum TDevOperType {dotPrnBP,dotPrnBT,dotPrnBR,dotScnBP1,dotScnBP2,dotPrnFlt,dotPrnArch,dotPrnDisp,dotPrnTlg,dotUnknown};
    extern const char *TDevOperTypeS[10];
    enum TDevFmtType {dftATB, dftBTP, dftEPL2, dftZPL2, dftDPL, dftEPSON, dftFRX, dftTEXT, dftZEBRA, dftUnknown};
    extern const char *TDevFmtTypeS[10];
};

#endif
