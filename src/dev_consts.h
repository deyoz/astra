#ifndef _DEV_CONSTS_H_
#define _DEV_CONSTS_H_

namespace ASTRA {
    enum TDevOperType {dotPrnBP,dotPrnBT,dotScnDoc,dotPrnBR,dotScnBP1,dotScnBP2,dotPrnFlt,dotPrnArch,dotPrnDisp,dotPrnTlg,dotUnknown};
    extern const char *TDevOperTypeS[11];
    enum TDevFmtType {dftATB, dftBTP, dftEPL2, dftZPL2, dftDPL, dftEPSON, dftFRX, dftTEXT, dftSCAN1, dftSCAN2, dftBCR, dftUnknown};
    extern const char *TDevFmtTypeS[12];
};

#endif
