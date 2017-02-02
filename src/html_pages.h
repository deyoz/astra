#ifndef _HTML_PAGES_H_
#define _HTML_PAGES_H_

#include "jxtlib/JxtInterface.h"

int html_to_db(int argc,char **argv);
int html_from_db(int argc,char **argv);

class HtmlInterface: public JxtInterface
{
    public:
        HtmlInterface(): JxtInterface("", "html")
    {
        Handler *evHandle;
        evHandle=JxtHandler<HtmlInterface>::CreateHandler(&HtmlInterface::get_resource);
        AddEvent("get_resource",evHandle);
    }

  void get_resource(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
