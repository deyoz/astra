#ifndef _SOFI_H_
#define _SOFI_H_

#include <string>
#include <map>
#include "astra_service.h"

const std::string FILE_SOFI_TYPE = "SOFI";
const std::string SOFI_AGENCY_PARAMS = "DEP_AGENCY";
	
bool createSofiFile( int receipt_id, std::map<std::string,std::string> &inparams, const std::string &point_addr, TFileDatas &fds );


#endif /*_SOFI_H_*/
