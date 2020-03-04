#pragma once

#include <libxml/tree.h>
#include <string>

namespace CUWS {
    void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Search_Bags_By_PassengerDetails(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Get_EligibleBagLegs_By_TagNum(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Search_FreeBagAllowanceOffer_By_BagType_PaxDetails(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Search_FreeBagAllowanceOffer_By_BagType_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Set_Bag_as_Active(xmlNodePtr actionNode, xmlNodePtr resNode);
    void Set_BagDetails_In_BagInfo(xmlNodePtr actionNode, xmlNodePtr resNode);

    void to_content(xmlNodePtr resNode, const std::string &resource, const std::string &tag = "", const std::string &tag_data = "");
    void CUWSSuccess(xmlNodePtr resNode);
}
