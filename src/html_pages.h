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

namespace HTTP_HDR {
    const std::string IF_NONE_MATCH                 = "If-None-Match";
    const std::string IF_MODIFIED_SINCE             = "If-Modified-Since";
    const std::string LAST_MODIFIED                 = "Last-Modified";
    const std::string ETAG                          = "ETag";
    const std::string DATE                          = "Date";
    const std::string CONTENT_LENGTH                = "Content-Length";
    const std::string CONTENT_TYPE                  = "Content-Type";
    const std::string ACCESS_CONTROL_ALLOW_ORIGIN   = "Access-Control-Allow-Origin";
    const std::string ACCESS_CONTROL_ALLOW_HEADERS  = "Access-Control-Allow-Headers";
    const std::string CACHE_CONTROL                 = "Cache-Control";
};

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

std::string getResource(const std::string &file_path);

#endif
