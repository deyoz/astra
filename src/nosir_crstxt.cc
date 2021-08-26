#include "nosir_crstxt.h"
#include <string>
#include "xml_unit.h"
#include "serverlib/xml_stuff.h" // для xml_decode_nodelist
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;

void crstxt_usage(string name, string what)
{
    cout 
        << "Error: " << what << endl
        << "Usage: " << name << " <point_id>" << endl;
    cout
        << "Example:" << endl
        << "  " << name << " 4917929" << endl;

}


int nosir_crstxt(int argc, char **argv)
{
    try {
        if(argc != 2)
            throw Exception("wrong params");

        int point_id = ToInt(argv[1]);

        ostringstream qry;
        qry <<
            "<?xml version='1.0' encoding='cp866'?> " // !!! должно быть cp866
            "<term> "
            "  <query handle='0' id='docs' ver='1' opr='DEN' screen='DOCS.EXE' mode='STAND' lang='RU' term_id='1226755661'> "
            "    <run_report2> "
            "      <dev_model>DIR PRINT</dev_model> "
            "      <fmt_type>TEXT</fmt_type> "
            "      <prnParams> "
            "        <pr_lat>0</pr_lat> "
            "        <encoding>CP866</encoding> "
            "        <offset>20</offset> "
            "        <top>0</top> "
            "      </prnParams> "
            "      <point_id>" << point_id << "</point_id> "
            "      <rpt_type>CRS</rpt_type> "
            "      <text>1</text> "
            "    </run_report2> "
            "    <response/>" // сам дописал, туда будет засовываться ответ
            "  </query> "
            "</term> ";
        xmlDocPtr doc = TextToXMLTree(qry.str()); // все данные в UTF
        xml_decode_nodelist(doc->children);
        xmlNodePtr rootNode=xmlDocGetRootElement(doc);
        TReqInfo *reqInfo = TReqInfo::Instance();
        reqInfo->Initialize("МОВ");
        XMLRequestCtxt *ctxt = getXmlCtxt();
        JxtInterfaceMng::Instance()->
            GetInterface("docs")->
            OnEvent("run_report2",  ctxt,
                    rootNode->children->children,
                    rootNode->children->children->next);
        LogTrace(TRACE5) << GetXMLDocText(doc);
    } catch(Exception &E) {
        crstxt_usage(argv[0], E.what());
        return 1;
    }
    return 1; // 0 - изменения коммитятся, 1 - rollback
}
