#ifndef __READ_EDI_ELEMENTS_H_
#define __READ_EDI_ELEMENTS_H_

#include <boost/optional.hpp>
#include "edi_elements.h"

struct _EDI_REAL_MES_STRUCT_;
namespace Ticketing{
namespace TickReader{

boost::optional<ASTRA::edifact::TktElement> readEdiTkt(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<ASTRA::edifact::CpnElement> readEdiCpn(EDI_REAL_MES_STRUCT *pMes, int numCpn);

} // namespace Ticketing
} // namespace TickReader

#endif /* __READ_EDI_ELEMENTS_H_ */
