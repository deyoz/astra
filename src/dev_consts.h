#ifndef _DEV_CONSTS_H_
#define _DEV_CONSTS_H_

#include "astra_consts.h"

namespace ASTRA {

class TDevOper
{
  public:
    enum Enum
    {
      PrnBP,
      PrnBT,
      PrnBR,
      PrnBI,
      PrnFlt,
      PrnArch,
      PrnDisp,
      PrnTlg,
      ScnBP1,
      ScnBP2,
      ScnDoc,
      ScnCard,
      Unknown
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l =
      {
        { PrnBP,   "PRINT_BP"   },
        { PrnBT,   "PRINT_BT"   },
        { PrnBR,   "PRINT_BR"   },
        { PrnBI,   "PRINT_BI"   },
        { PrnFlt,  "PRINT_FLT"  },
        { PrnArch, "PRINT_ARCH" },
        { PrnDisp, "PRINT_DISP" },
        { PrnTlg,  "PRINT_TLG"  },
        { ScnBP1,  "SCAN_BP"    },
        { ScnBP2,  "SCAN_BP2"   },
        { ScnDoc,  "SCAN_DOC"   },
        { ScnCard, "SCAN_CARD"  },
        { Unknown, "" }
      };
      return l;
    }
};

class TDevOperTypes : public ASTRA::PairList<TDevOper::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TDevOperTypes"; }
  public:
    TDevOperTypes() : ASTRA::PairList<TDevOper::Enum, std::string>(TDevOper::pairs(),
                                                                   TDevOper::Unknown,
                                                                   std::string("")) {}
};

const TDevOperTypes& DevOperTypes();


    enum TDevFmtType {dftATB, dftBTP, dftEPL2, dftZPL2, dftDPL, dftEPSON, dftGraphics2D, dftFRX, dftTEXT, dftSCAN1, dftSCAN2, dftSCAN3, dftBCR, dftUnknown};
    extern const char *TDevFmtTypeS[14];
    enum TDevClassType {dctATB, dctBTP, dctDCP, dctBGR, dctSCN, dctOCR, dctMSR, dctWGE, dctUnknown};
}

#endif
