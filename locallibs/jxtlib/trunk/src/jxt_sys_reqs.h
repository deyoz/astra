#ifndef __JXT_SYS_REQS_H__
#define __JXT_SYS_REQS_H__

#ifdef __cplusplus
#include <string>
#include <libxml/tree.h>
#include "jxt_xml_cont.h"

void UpdateXmlData_inner(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                         xmlNodePtr resNode, const std::string &type,
                         const std::string &id);
xmlNodePtr closeWindow(xmlNodePtr resNode);

// event callers
void close_handle(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void quit_window(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void UpdateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

void UpdateXmlData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

#endif /* __cplusplus */

#endif /* __JXT_SYS_REQS_H__ */
