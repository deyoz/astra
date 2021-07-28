#ifndef _AODB_H_
#define _AODB_H_

#include <string>
#include <map>
#include <tcl.h>
#include "astra_service.h"
#include "date_time.h"
#include "astra_misc.h"

using BASIC::date_time::TDateTime;

const std::string FILE_AODB_TYPE = "AODB";
const std::string FILE_AODB_IN_TYPE = "AODBI";
const std::string FILE_AODB_OUT_TYPE = "AODBO";

bool createAODBFiles( int point_id, const std::string &point_addr, TFileDatas &fds );

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name,
                      const std::string &airline, const std::string &airp,
                      std::string &fd, const std::string &convert_aodb );
bool Get_AODB_overload_alarm( int point_id, int max_commerce );
void Set_AODB_overload_alarm( int point_id, bool overload_alarm );

void VerifyParseFlight( );

int main_aodb_handler_tcl(int supervisorSocket, int argc, char *argv[]);

namespace AODB_POINTS {
  // �ਢ離� � ������ ३��
  void bindingAODBFlt( const std::string &point_addr, int point_id, double aodb_point_id );
  void bindingAODBFlt( const std::string &airline, const int flt_no, const std::string suffix,
                       const TDateTime locale_scd_out, const std::string airp );
  void recNoFltNext( int point_id, std::string point_addr );
  void setDelete( double aodb_point_id );
  void setSCD_OUT( double aodb_point_id, TDateTime aodb_scd_out );
  bool isDelete( int point_id );
  TDateTime getSCD_OUT( int point_id );
}

std::string getAODBFranchisFlight( int point_id, std::string &airline, const std::string &point_addr );

#endif /*_AODB_H_*/
