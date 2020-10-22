#include "libra.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_calls.h"

#include <serverlib/oci8.h>
#include <serverlib/oci8cursor.h>
#include <serverlib/str_utils.h>

#include <optional>

#define NICKNAME "LIBRA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace OciCpp;

/////////////////////////////////////////////////////////////////////////////////////////

namespace  {

std::string getRequestName(xmlNodePtr rootNode)
{
    std::string requestName = "";
    auto name = getprop(rootNode, "name");
    if(!name) {
        LogWarning(STDLOG) << "Xml from client hasn't name property!";
    } else {
        requestName = std::string(name);
    }

    return requestName;
}

void addOprPult2Req(xmlNodePtr rootNode)
{
    NewTextChild(rootNode, "login", TReqInfo::Instance()->user.login);
    NewTextChild(rootNode, "descr", TReqInfo::Instance()->user.descr);
    NewTextChild(rootNode, "pult",  TReqInfo::Instance()->desk.code);
}

void addRights2Req(xmlNodePtr rootNode)
{
    TReqInfo::Instance()->user.access.toXML(NewTextChild(rootNode, "access"));
}

std::string callLibra(xmlNodePtr reqRootNode)
{
    // Доклейка пульта, оператора и прав доступа
    //addOprPult2Req(rootNode);  пока отключено
    //addRights2Req(rootNode);   пока отключено

    std::string xml_in = XMLTreeToText(reqRootNode->doc);
    LogTrace(TRACE5) << __func__ << " xml_in=" << xml_in;

    std::string xml_out;
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"declare\n"
"   CLOB_OUT clob;\n"
"begin\n"
"   delete from WB_REF_ASTRA_TMP;\n"
"   SP_WB_ASTRA_CALLS(:clob_in, CLOB_OUT);\n"
"   insert into WB_REF_ASTRA_TMP(CLOB_OUT) values (CLOB_OUT);\n"
"end;", &os);
    cur.bindClob(":clob_in", xml_in)
       .exec();

    Curs8Ctl selCur(STDLOG,
"select CLOB_OUT from WB_REF_ASTRA_TMP", &os);
    selCur.defClob(xml_out)
          .EXfet();

    LogTrace(TRACE5) << "xml_out=" << xml_out;
    return xml_out;
}

std::string callAstra(xmlNodePtr reqRootNode)
{
    std::string xml_in = XMLTreeToText(reqRootNode->doc);
    LogTrace(TRACE5) << __func__ << " xml_in=" << xml_in;
    auto resultXmlDoc = ASTRA::createXmlDoc("<root/>");
    xmlNodePtr resRootNode = findNodeR(resultXmlDoc.docPtr()->children, "root");
    ASSERT(resRootNode);
    bool result = AstraCalls::callByLibra(reqRootNode, resRootNode);
    xmlSetProp(resRootNode, "name", "astra_call");
    xmlSetProp(resRootNode, "result", result ? "ok" : "err");
    std::string xml_out = XMLTreeToText(resRootNode->doc);
    LogTrace(TRACE5) << "xml_out=" << xml_out;
    return xml_out;
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

void LibraInterface::RequestHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    std::string dataFromClient = NodeAsString("data", reqNode);
    auto clientXmlDoc = ASTRA::createXmlDoc(dataFromClient);
    xmlNodePtr reqRootNode = findNodeR(clientXmlDoc.docPtr()->children, "root");
    ASSERT(reqRootNode);
    auto data = getRequestName(reqRootNode) == "astra_call" ? callAstra(reqRootNode)
                                                            : callLibra(reqRootNode);

    NewTextChild(resNode, "data", data);
}
