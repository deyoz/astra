#pragma once

#include <serverlib/xmllibcpp.h>
#include <xml_unit.h>

#include <string>
#include <vector>


namespace AstraCalls {

const std::string BALANCE_RUN_PARAM = "BALANCE";

bool callByLibra(xmlNodePtr reqNode, xmlNodePtr resNode);

bool isUserLogonWithBalance( const std::vector<std::string>& run_params);
void SetAstraSpecMarkLibraRequest( xmlNodePtr resNode );

}//namespace AstraCalls
