#ifndef _AODB_H_
#define _AODB_H_

#include <string>
#include <map>

const std::string FILE_AODB_TYPE = "AODB";
	
bool createAODBCheckInInfoFile( int point_id, std::map<std::string,std::string> &params, std::string &file_data );



#endif /*_AODB_H_*/
