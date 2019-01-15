#ifndef _STAT_UTILS_H_
#define _STAT_UTILS_H_

#include <libxml/tree.h>
#include <string>

namespace STAT {
    bool bad_client_img_version();
    xmlNodePtr set_variables(xmlNodePtr resNode, std::string lang = "");
    xmlNodePtr getVariablesNode(xmlNodePtr resNode);
}

#endif
