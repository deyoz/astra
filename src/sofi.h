#ifndef _SOFI_H_
#define _SOFI_H_

#include <string>
#include <map>

const std::string FILE_SOFI_TYPE = "SOFI";
	
bool createSofiFile( int receipt_id, std::map<std::string,std::string> &params, std::string &file_data );


#endif /*_SOFI_H_*/
