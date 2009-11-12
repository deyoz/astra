#ifndef _DEV_CONSTS_H_
#define _DEV_CONSTS_H_

namespace ASTRA {
    enum TDevOperType {dotPrnBP,dotPrnBT,dotPrnBR,dotScnBP,dotPrnFlt,dotPrnArch,dotPrnDisp,dotPrnTlg,dotUnknown};
    extern const char *TDevOperTypeS[9];
    enum TDevFmtType {dftATB, dftBTP, dftEPL2, dftZPL2, dftDPL, dftEPSON, dftFRX, dftUnknown};
    extern const char *TDevFmtTypeS[8];
};

#endif
