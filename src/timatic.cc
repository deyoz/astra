#include "timatic.h"
#include "serverlib/trace_signature.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "timatic_request.h"
#include "libtimatic/timatic_service.h"
#include "serverlib/query_runner.h"
#include "serverlib/internal_msgid.h"
#include "timatic_response.h"
#include "serverlib/xml_stuff.h"
#include "serverlib/str_utils.h"
#include "html_pages.h"
#include "passenger.h"
#include "timatic_exchange.h"
#include "flt_settings.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace Timatic;
using namespace ASTRA;
using namespace AstraLocale;

// коммент

void trace_xml(TRACE_SIGNATURE, const string &descr, const string &content)
{
    string converted = ConvertCodepage(content, "CP866", "UTF8");
    XMLDoc doc(converted);
    LogTrace(TRACE_PARAMS) << descr << ": " << GetXMLDocText(doc.docPtr());
}

void processReq(const TTimaticAccess &access, xmlNodePtr reqNode)
{
    if(TimaticSession(access).fromDB().empty())
        Timatic::CheckName(access, reqNode).request();
    else
        Timatic::Data(access, reqNode).request();
}

void TimaticInterface::run(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    auto access = Timatic::GetTimaticUserAccess();
    if(not access) throw UserException("MSG.TIMATIC.USER_ACCESS_DENIED");
    processReq(access.get(), reqNode);
}

void TimaticInterface::TimaticDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int pax_id = NodeAsInteger("pax_id", reqNode);
    TTripInfo flt;
    flt.getByPaxId(pax_id) or flt.getByCRSPaxId(pax_id);
    if(not GetTripSets(tsTimaticManual, flt))
        throw UserException("MSG.TIMATIC.MANUAL_MODE_NOT_ALLOWED");
    auto access = GetTimaticAccess(flt.airline, flt.airp);
    if(not access) throw UserException("MSG.TIMATIC.ACCESS_DENIED");
    processReq(access.get(), reqNode);
}

void TimaticInterface::layout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    XMLDoc layout = XMLDoc(StrUtils::b64_decode(getResource(TIMATIC_XML_RESOURCE)));
    xml_decode_nodelist(layout.docPtr()->children);

    xmlNodePtr curNode = layout.docPtr()->children;
    xmlNodePtr layoutNode = NodeAsNodeFast("layout", curNode);
    curNode = layoutNode->children;
    xmlNodePtr dataNode = GetNodeFast("data", curNode);
    if(not dataNode) {
        dataNode = NewTextChild(layoutNode, "data");
    }
    dataNode = NewTextChild(dataNode, "source_list");
    curNode = layoutNode->children;
    curNode = NodeAsNodeFast("ctrl_list", curNode);

    layout_add_data(curNode, dataNode);

    CopyNode(resNode, layout.docPtr()->children);
}
