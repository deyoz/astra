#ifndef _AODB_H_
#define _AODB_H_

#include <string>
#include <map>
#include "astra_service.h"

const std::string FILE_AODB_TYPE = "AODB";

	
bool createAODBFiles( int point_id, const std::string &point_addr, TFileDatas &fds );
bool createAODBCheckInInfoFile( int point_id, bool pr_unaccomp, const std::string &point_addr, TFileDatas &fds );
bool BuildAODBTimes( int point_id, const std::string &point_addr, TFileDatas &fds );

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name, const std::string airline, 
	                    std::string &fd, const std::string &convert_aodb );
	                    	
	                    	
void VerifyParseFlight( );	                    	

#endif /*_AODB_H_*/
