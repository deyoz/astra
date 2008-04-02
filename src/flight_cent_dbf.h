//---------------------------------------------------------------------------
#ifndef flight_cent_dbfH
#define flight_cent_dbfH
#include <string>
#include <map>
#include "astra_service.h"

const std::string FILE_CENT_TYPE = "CENTR";

bool createCentringFile( int point_id, const std::string &point_addr, TFileDatas &fds );

#endif /* cent_dbfH */
