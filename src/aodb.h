#ifndef _AODB_H_
#define _AODB_H_

#include <string>
#include <map>

const std::string FILE_AODB_TYPE = "AODB";
	
bool createAODBCheckInInfoFile( int point_id, bool pr_unaccomp, 
                                std::map<std::string,std::string> &checkin_params, std::string &checkin_file_data/*,
                                std::map<std::string,std::string> &bag_params, std::string &bag_file_data*/ );
bool BuildAODBTimes( int point_id, std::map<std::string,std::string> &params, std::string &file_data);

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name, std::string &fd, const std::string &convert_aodb );

#endif /*_AODB_H_*/
