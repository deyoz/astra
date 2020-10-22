#include "cuws_handlers.h"
#include "xml_unit.h"
#include "web_search.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>
#include "html_pages.h"
#include "baggage_calc.h"
#include "zamar_dsm.h"
#include "serverlib/xml_stuff.h" // для xml_decode_nodelist

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

namespace CUWS {

void to_content(xmlNodePtr resNode, const string &resource, const string &tag, const string &tag_data)
{
    string data = getResource(resource);
    if(not tag.empty())
        data = StrUtils::b64_encode(
                boost::regex_replace(
                    StrUtils::b64_decode(data),
                    boost::regex(tag), ConvertCodepage(tag_data, "CP866", "UTF8")));
    SetProp( NewTextChild(resNode, "content",  data), "b64", true);
    TResHTTPParams rhp;
    rhp.hdrs[HTTP_HDR::CONTENT_TYPE] = "text/xml";
    rhp.toXML(resNode);
}

void to_envelope(xmlNodePtr resNode, const string &data)
{
    to_content(resNode, "/cuws_envelope.xml", "BODY", data);
}

void Get_PassengerInfo_By_BCBP(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
    PassengerSearchResult result;
    ZamarXML::ProcessXML(result, actionNode, externalSysResNode, resNode, ZamarType::CUWS);
}

void Set_Bag_as_Inactive(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
    ZamarBaggageTagRevoke tag_revoke;
    ZamarXML::ProcessXML(tag_revoke, actionNode, externalSysResNode, resNode, ZamarType::CUWS);
}

void Set_Bag_as_Active(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
    ZamarBaggageTagConfirm tag_confirm;
    ZamarXML::ProcessXML(tag_confirm, actionNode, externalSysResNode, resNode, ZamarType::CUWS);
}

void Issue_TagNumber(xmlNodePtr actionNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  ZamarBaggageTagAdd tag_add;
  ZamarXML::ProcessXML(tag_add, actionNode, externalSysResNode, resNode, ZamarType::CUWS);
}

} //end namespace CUWS

