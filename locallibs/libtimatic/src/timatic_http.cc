#include "timatic_http.h"
#include "timatic_xml.h"

#include <serverlib/httpsrv.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>
#include <serverlib/xml_cpp.h>

namespace Timatic {

static void splitText(const std::string &text, std::string &header, std::string &content)
{
    static const std::string rnrn = "\r\n\r\n";

    size_t pos = text.find(rnrn);
    if (pos != text.npos) {
        header = std::string(text.begin(), text.begin() + pos);
        content = std::string(text.begin() + pos + rnrn.size(), text.end());
    } else {
        header = text;
    }
}

static int getHttpCode(const std::string &header)
{
    enum class State {
        Start, H, HT, HTT, HTTP, Slash,
        Major, Dot, Minor, SpaceOrTab,
        Code100, Code10, Code1, Done, Error
    };

    int httpCode = 0;
    State state = State::Start;
    for (auto it = header.begin(); it != header.end(); it++) {
        if (*it == '\r' or *it == '\n') {
            state = State::Error;
            break;
        }
        switch (state) {
        case State::Start:
            if (*it == 'H') state = State::H;
            else state = State::Error;
            break;
        case State::H:
            if (*it == 'T') state = State::HT;
            else state = State::Error;
            break;
        case State::HT:
            if (*it == 'T') state = State::HTT;
            else state = State::Error;
            break;
        case State::HTT:
            if (*it == 'P') state = State::HTTP;
            else state = State::Error;
            break;
        case State::HTTP:
            if (*it == '/') state = State::Slash;
            else state = State::Error;
            break;
        case State::Slash:
            if (std::isdigit(*it)) state = State::Major;
            else state = State::Error;
            break;
        case State::Major:
            if (*it == '.') state = State::Dot;
            else if (*it == ' ' or *it == '\t') state = State::SpaceOrTab;
            else state = State::Error;
            break;
        case State::Dot:
            if (std::isdigit(*it)) state = State::Minor;
            else state = State::Error;
            break;
        case State::Minor:
            if (*it == ' ' or *it == '\t') state = State::SpaceOrTab;
            else state = State::Error;
            break;
        case State::SpaceOrTab:
            if (std::isdigit(*it)) { state = State::Code100; httpCode = 100 * (*it - '0'); }
            else if (*it == ' ' or *it == '\t') state = State::SpaceOrTab;
            else state = State::Error;
            break;
        case State::Code100:
            if (std::isdigit(*it)) { state = State::Code10; httpCode += 10 * (*it - '0'); }
            else state = State::Error;
            break;
        case State::Code10:
            if (std::isdigit(*it)) { state = State::Code1; httpCode += 1 * (*it - '0'); }
            else state = State::Error;
            break;
        case State::Code1:
            if (not std::isdigit(*it)) state = State::Done;
            else state = State::Error;
            break;
        default:
            break;
        }
        if (state == State::Done or state == State::Error)
            break;
    }

    if (state == State::Error)
        return 500;
    else
        return httpCode;
}

static std::string getJsessionID(const std::string &header)
{
    static const std::string scookie = "Set-Cookie:";
    static const std::string jcookie = "JSESSIONID=";

    size_t pos = header.find(scookie);
    if (pos != header.npos) {
        size_t beg = header.find(jcookie, pos);
        if (beg != header.npos) {
            size_t end = header.find(";", beg);
            if (end != header.npos)
                return std::string(header.begin() + beg + jcookie.size(), header.begin() + end);
        }
    }

    return {};
}

static std::shared_ptr<Response> getResponse(const std::string &content)
{
    size_t pos = content.find("<?xml");
    if (pos == content.npos)
        return nullptr;

    XmlDoc doc = xMakeDoc(std::string(content.begin() + pos, content.end()));
    xmlNodePtr bodyNode = xFindNode(xmlDocGetRootElement(doc.get()), "Body");

    if (bodyNode && bodyNode->children && bodyNode->children->children) {
        xmlNodePtr child = bodyNode->children;
        xmlNodePtr child2 = bodyNode->children->children;

        if (!xCmpNames(child, "processRequestResponse") && !xCmpNames(child, "processLoginResponse"))
            return nullptr;

        if (xCmpNames(child2, ErrorResp::cname()))
            return std::make_shared<ErrorResp>(child2);

        if (xCmpNames(child2, CheckNameResp::cname()))
            return std::make_shared<CheckNameResp>(child2);

        if (xCmpNames(child2, LoginResp::cname()))
            return std::make_shared<LoginResp>(child2);

        if (xCmpNames(child2, DocumentResp::cname()))
            return std::make_shared<DocumentResp>(child2);

        if (xCmpNames(child2, ParamResp::cname()))
            return std::make_shared<ParamResp>(child2);

        if (xCmpNames(child2, ParamValuesResp::cname()))
            return std::make_shared<ParamValuesResp>(child2);

        if (xCmpNames(child2, CountryResp::cname()))
            return std::make_shared<CountryResp>(child2);

        if (xCmpNames(child2, VisaResp::cname()))
            return std::make_shared<VisaResp>(child2);
    }

    return nullptr;
}

//-----------------------------------------------

void HttpData::Init(const std::string &content)
{
    splitText(content, header_, content_);
    httpCode_ = getHttpCode(header_);
    jsessionID_ = getJsessionID(header_);
    response_ = Timatic::getResponse(content_);
}

HttpData::HttpData(const std::string &content)
    : httpCode_(500)
{
    Init(content);
}

HttpData::HttpData(const httpsrv::HttpResp &httpResp)
    : httpCode_(500)
{
    Init(httpResp.text);
}

} // Timatic
