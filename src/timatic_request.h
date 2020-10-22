#pragma once

#include <libxml/tree.h>
#include "libtimatic/timatic_request.h"
#include "libtimatic/timatic_http.h"
#include "oralib.h"

static const std::string TIMATIC_XML_RESOURCE = "/timatic_layout.xml";

namespace Timatic {

    struct TTimaticAccess {
        std::string airline, airp;
        std::string host;
        int port;
        std::string username;
        std::string sub_username;
        std::string pwd;
        bool pr_denial;

        void clear();

        const TTimaticAccess &fromDB(TQuery &Qry);

        void fromXML(xmlNodePtr node);
        void toXML(xmlNodePtr node);
        void trace() const;

        TTimaticAccess() { clear(); }
    };

    void processReq(xmlNodePtr reqNode, xmlNodePtr resNode, const HttpData &httpData);
    std::shared_ptr<Timatic::Request> parseRequest(xmlNodePtr reqNode);
    std::shared_ptr<Timatic::Request> parseDocRequest(xmlNodePtr reqNode);

    void layout_add_data(xmlNodePtr node, xmlNodePtr dataNode);

    boost::optional<TTimaticAccess> GetTimaticUserAccess();
    boost::optional<TTimaticAccess> GetTimaticAccess(const std::string &airline, const std::string &airp);
}
