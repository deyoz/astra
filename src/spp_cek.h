#ifndef _SPP_CEK_H_
#define _SPP_CEK_H_

#include "develop_dbf.h"
#include "astra_service.h"
#include <string>

const std::string FILE_SPPCEK_TYPE = "SPCEK";
	
bool createSPPCEKFile( int point_id, const std::string &point_addr, TFileDatas &fds );

#endif /*_SPP_CEK_H_*/
