#pragma once

#include "iatci_types.h"

#include <edilib/EdiSessionId_t.h>
#include <libtlg/tlgnum.h>


namespace iatci
{

Result checkinPax(const CkiParams& ckiParams);
Result cancelCheckin(const CkxParams& ckxParams);
Result updateCheckin(const CkuParams& ckuParams);
Result fillPasslist(const PlfParams& plfParams);
boost::optional<FlightDetails> findCascadeFlight(const FlightDetails& flight);

//---------------------------------------------------------------------------------------

void saveDeferredCkiData(tlgnum_t msgId, const std::list<Result>& lRes);
std::list<Result> loadDeferredCkiData(tlgnum_t msgId);

void saveCkiData(edilib::EdiSessionId_t sessId, const std::list<Result>& lRes);
std::list<Result> loadCkiData(edilib::EdiSessionId_t sessId);

}//namespace iatci
