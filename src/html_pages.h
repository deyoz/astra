#ifndef _HTML_PAGES_H_
#define _HTML_PAGES_H_

#include "jxtlib/JxtInterface.h"
#include "serverlib/http_parser.h"

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


// response HTTP params

struct TResHTTPParams {
    static const std::string NAME;
    ServerFramework::HTTP::reply::status_type status;
    std::map<std::string, std::string> hdrs;
    void toXML(xmlNodePtr hdrsNode);
    void fromXML(std::string &data);
    void Clear();
    TResHTTPParams() { Clear(); }
};

std::string html_get_param(const std::string &tag_name, xmlNodePtr reqNode);
std::string html_header_param(const std::string &tag_name, xmlNodePtr reqNode);

#endif
