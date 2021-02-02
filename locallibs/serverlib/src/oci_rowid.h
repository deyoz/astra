#pragma once

#include "oci_types.h"
#ifdef __cplusplus

#define ROWID_LEN 23
typedef OciCpp::OciVcs<ROWID_LEN> ROWIDOCI;

#endif /*__cplusplus*/
