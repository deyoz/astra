#include "cuws_main.h"
#include "xml_unit.h"
#include "qrys.h"
#include "astra_locale_adv.h"
#include "http_consts.h"
#include "serverlib/str_utils.h"

#include "cuws_handlers.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;

namespace CUWS {

void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode);

typedef void (*TCUWSHandler)(xmlNodePtr, xmlNodePtr);
typedef map<string, TCUWSHandler> TCUWSHandlerList;
static const TCUWSHandlerList handler_list {
    {"Get_EligibleBagLegs_By_TagNum",                       Get_EligibleBagLegs_By_TagNum},
    {"Search_Bags_By_BCBP",                                 Search_Bags_By_BCBP},
    {"Search_Bags_By_PassengerDetails",                     Search_Bags_By_PassengerDetails},
    {"Search_FreeBagAllowanceOffer_By_BagType_PaxDetails",  Search_FreeBagAllowanceOffer_By_BagType_PaxDetails},
    {"Search_FreeBagAllowanceOffer_By_BagType_BCBP",        Search_FreeBagAllowanceOffer_By_BagType_BCBP},
    {"Set_Bag_as_Active",                                   Set_Bag_as_Active},
    {"Set_BagDetails_In_BagInfo",                           Set_BagDetails_In_BagInfo},
};

void dump_content(xmlNodePtr contentNode)
{
    if(not contentNode) return;
    LogTrace(TRACE5) << "content node: " << contentNode->name;
    xmlNodePtr curNode = contentNode->children;
    while(curNode) {
        dump_content(curNode);
        curNode = curNode->next;
    }
}

void CUWSInternalServerError(xmlNodePtr resNode)
{
    to_content(resNode, "/cuws_error.xml");
}

void params_from_xml(xmlNodePtr reqNode, const string &name, map<string, string> &params)
{
    xmlNodePtr paramsNode = NodeAsNode(name.c_str(), reqNode);
    if(not paramsNode) throw Exception(name + " node not found");
    xmlNodePtr currNode = paramsNode->children;
    while(currNode) {
        params[NodeAsString("name", currNode)] = NodeAsString("value", currNode);
        currNode = currNode->next;
    }
}

void CUWSwsdl(xmlNodePtr resNode, const string &uri, const string &host, const string &client_id)
{
    if(host.empty())
        throw Exception("host is empty");
    if(client_id.empty())
        throw Exception("client_id is empty");
    if(uri.empty())
        throw Exception("uri is empty");
    to_content(resNode, "/cuws.wsdl", "ENDPOINT", "http://" + host + uri + "?" + AstraHTTP::CLIENT_ID + "=" + client_id);
}

void CUWSDispatcher(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr curNode = reqNode->children;
    xmlNodePtr contentNode = NodeAsNodeFast("content", curNode);
    curNode = contentNode->children;
    xmlNodePtr envelopeNode = NodeAsNodeFast("Envelope", curNode);
    curNode = envelopeNode->children;
    xmlNodePtr bodyNode = NodeAsNodeFast("Body", curNode);
    xmlNodePtr actionNode = bodyNode->children;
    if(not actionNode) throw Exception("action not found");
    string action = (const char *)actionNode->name;
    auto i = handler_list.find(action);
    if(i == handler_list.end())
        throw Exception("Action not found: " + action);
    i->second(actionNode, resNode);
}

} //end namespace CUWS

void CUWSInterface::CUWS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    try {
        map<string, string> header;
        map<string, string> get_params;
        CUWS::params_from_xml(reqNode, "header", header);
        CUWS::params_from_xml(reqNode, "get_params", get_params);

        if(get_params.find("wsdl") != get_params.end()) {
            CUWS::CUWSwsdl(
                    resNode,
                    NodeAsString(AstraHTTP::URI_PATH.c_str(), reqNode),
                    header[AstraHTTP::HOST],
                    get_params[AstraHTTP::CLIENT_ID]);
        } else {
            CUWS::CUWSDispatcher(reqNode, resNode);
        }
    } catch(Exception &E) {
        ProgError(STDLOG, "%s: %s", __FUNCTION__, E.what());
        CUWS::CUWSInternalServerError(resNode);
    } catch(...) {
        ProgError(STDLOG, "%s: unknown error", __FUNCTION__);
        CUWS::CUWSInternalServerError(resNode);
    }
}

