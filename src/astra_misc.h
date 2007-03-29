#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include "basic.h"
#include "tlg/tlg_parser.h"

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline=(std::string&)"");
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline=(std::string&)"");
BASIC::TDateTime DayToDate(int day, BASIC::TDateTime base_date);

#endif /*_ASTRA_MISC_H_*/


