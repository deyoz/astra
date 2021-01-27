#include "nosir_prepare_flt.h"
#include "serverlib/xml_stuff.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;

int nosir_prepare_flt(int argc, char **argv)
{
    LogTrace(TRACE5) << "DEN: " << __func__ << " started";
    string qry =
        "<?xml version='1.0' encoding='CP866'?> "
        " <term> "
        "   <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'> "
        "     <UserLogon> "
        "       <term_version>201509-0173355</term_version> "
        "       <userr>PIKE</userr> "
        "       <passwd>PIKE</passwd> "
        "       <airlines/> "
        "       <devices/> "
        "       <command_line_params> "
        "         <param>RESTART</param> "
        "         <param>NOCUTE</param> "
        "         <param>LANGRU</param> "
        "       </command_line_params> "
        "     </UserLogon> "
        "    <response/>" // сам дописал, туда будет засовываться ответ
        "   </query> "
        "</term> ";

    xmlDocPtr doc = TextToXMLTree(qry); // все данные в UTF
    xml_decode_nodelist(doc->children);
    xmlNodePtr rootNode=xmlDocGetRootElement(doc);
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");
    XMLRequestCtxt *ctxt = getXmlCtxt();
    JxtInterfaceMng::Instance()->
        GetInterface("MainDCS")->
        OnEvent("UserLogon",  ctxt,
                rootNode->children->children,
                rootNode->children->children->next);
    LogTrace(TRACE5) << "result: '" << GetXMLDocText(doc) << "'";
    return 1; // 0 - изменения коммитятся, 1 - rollback
}
