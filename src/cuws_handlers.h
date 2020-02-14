#pragma once

#include <libxml/tree.h>
#include <string>

void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode);

void to_content(xmlNodePtr resNode, const std::string &resource, const std::string &tag = "", const std::string &tag_data = "");
