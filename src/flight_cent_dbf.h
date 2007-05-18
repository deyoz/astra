//---------------------------------------------------------------------------
#ifndef flight_cent_dbfH
#define flight_cent_dbfH
#include <string>
#include <map>

const std::string PARAM_WORK_DIR = "WORKDIR";
const std::string PARAM_FILE_NAME = "FileName";
const std::string FILE_CENT_TYPE = "CENTR";

bool createCentringFile( int point_id, std::map<std::string,std::string> &params, std::string &file_data  );

#endif /* cent_dbfH */
