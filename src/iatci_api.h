#pragma once

#include "iatci_types.h"

#include <libtlg/tlgnum.h>


namespace iatci {

dcrcka::Result checkin(const CkiParams& ckiParams);
dcrcka::Result cancelCheckin(const CkxParams& ckxParams);
dcrcka::Result updateCheckin(const CkuParams& ckuParams);
dcrcka::Result reprintBoardingPass(const BprParams& bprParams);
dcrcka::Result fillPasslist(const PlfParams& plfParams);
dcrcka::Result fillSeatmap(const SmfParams& smfParams);

std::list<dcrcka::Result> checkin(tlgnum_t postponeTlgNum);
std::list<dcrcka::Result> cancelCheckin(tlgnum_t postponeTlgNum);
std::list<dcrcka::Result> updateCheckin(tlgnum_t postponeTlgNum);

}//namespace iatci
