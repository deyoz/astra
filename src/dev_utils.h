#ifndef _DEV_UTILS_H_
#define _DEV_UTILS_H_

#include "dev_consts.h"

ASTRA::TDevOperType DecodeDevOperType(const char *s);
ASTRA::TDevFmtType DecodeDevFmtType(const char *s);
char* EncodeDevOperType(ASTRA::TDevOperType s);
char* EncodeDevFmtType(ASTRA::TDevFmtType s);

#endif
