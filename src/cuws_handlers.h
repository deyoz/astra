#pragma once

#include <libxml/tree.h>
#include <string>

namespace CUWS {
    void Get_PassengerInfo_By_BCBP(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
    void Issue_TagNumber(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
    void Set_Bag_as_Active(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
    void Set_Bag_as_Inactive(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);

    void to_envelope(xmlNodePtr resNode, const std::string &data);
    void to_content(xmlNodePtr resNode, const std::string &resource, const std::string &tag = "", const std::string &tag_data = "");
}
