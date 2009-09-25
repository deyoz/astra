#ifndef _DEV_UTILS_H_
#define _DEV_UTILS_H_

#include "dev_consts.h"
#include <string>

ASTRA::TDevOperType DecodeDevOperType(std::string s);
ASTRA::TDevFmtType DecodeDevFmtType(std::string s);
std::string EncodeDevOperType(ASTRA::TDevOperType s);
std::string EncodeDevFmtType(ASTRA::TDevFmtType s);

#endif
