#ifndef _ASTRA_ELEM_UTILS_H_
#define _ASTRA_ELEM_UTILS_H_

#include <string>
#include "date_time.h"
#include "xml_unit.h"

using BASIC::date_time::TDateTime;

enum TCheckFieldFromXML {cfNothingIfEmpty, cfTraceIfEmpty, cfErrorIfEmpty};

std::string airl_fromXML(xmlNodePtr node, TCheckFieldFromXML check_type, const std::string &trace_info);
std::string airl_fromXML(const std::string &value, TCheckFieldFromXML check_type, const std::string &trace_info, const std::string &node_name);
std::string airp_fromXML(xmlNodePtr node, TCheckFieldFromXML check_type, const std::string &trace_info, const std::string& system_name = "");
std::string airp_fromXML(const std::string &value, TCheckFieldFromXML check_type, const std::string &trace_info, const std::string &node_name,
                         const std::string& system_name = "");
int flt_no_fromXML(std::string str);
std::string  suffix_fromXML(std::string str);
TDateTime scd_out_fromXML(std::string str, const char* fmt);
TDateTime date_fromXML(std::string str);

#endif /*_ASTRA_ELEM_UTILS_H_*/
