#ifndef _DEV_CONSTS_H_
#define _DEV_CONSTS_H_

namespace ASTRA {
    enum TDevOperType {dotPrnBP,dotPrnBT,dotPrnBR,dotPrnFlt,dotPrnArch,dotPrnDisp,dotPrnTlg,dotScnBP1,dotScnBP2,dotScnDoc,dotScnCard,dotUnknown};
    extern const char *TDevOperTypeS[12];
    enum TDevFmtType {dftATB, dftBTP, dftEPL2, dftZPL2, dftDPL, dftEPSON, dftFRX, dftTEXT, dftSCAN1, dftSCAN2, dftSCAN3, dftBCR, dftUnknown};
    extern const char *TDevFmtTypeS[13];
    enum TDevClassType {dctATB, dctBTP, dctDCP, dctBGR, dctSCN, dctOCR, dctMSR, dctWGE, dctUnknown};
};

#endif
