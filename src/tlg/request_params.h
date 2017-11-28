#pragma once

#include "CheckinBaseTypes.h"
#include "astra_ticket.h"
#include "remote_system_context.h"

namespace edifact {

struct KickInfo
{
    int reqCtxtId;
    std::string iface;
    std::string handle;
    int parentSessId;
    std::string msgId;
    std::string desk;

  public:
    KickInfo();
    KickInfo(const int v_reqCtxtId,
             const std::string& v_iface,
             const std::string& v_msgid,
             const std::string& v_desk);

    void clear();
    const KickInfo& toXML(xmlNodePtr node) const;
    KickInfo& fromXML(xmlNodePtr node);

    bool background_mode() const;
};

}//namespace edifact

/////////////////////////////////////////////////////////////////////////////////////////

class edi_common_data
{
    Ticketing::OrigOfRequest Org;
    std::string ediSessCtxt;
    edifact::KickInfo m_kickInfo;
public:
    edi_common_data(const Ticketing::OrigOfRequest &org,
                    const std::string &ctxt,
                    const edifact::KickInfo &kickInfo)
        : Org(org), ediSessCtxt(ctxt), m_kickInfo(kickInfo)
    {}
    const Ticketing::OrigOfRequest& org() const { return Org; }
    const std::string& context() const { return ediSessCtxt; }
    const edifact::KickInfo& kickInfo() const { return m_kickInfo; }
    virtual ~edi_common_data() {}
};

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact {

class RequestParams: public edi_common_data
{
    std::string m_airline;
    Ticketing::FlightNum_t m_flNum;

public:
    RequestParams(const Ticketing::OrigOfRequest& org,
                  const std::string& ctxt,
                  const edifact::KickInfo& kickInfo,
                  const std::string& airline,
                  const Ticketing::FlightNum_t& flNum)
        : edi_common_data(org, ctxt, kickInfo),
          m_airline(airline), m_flNum(flNum)
    {}

    virtual ~RequestParams() {}

    const std::string& airline() const { return m_airline; }
    const Ticketing::FlightNum_t& flightNum() const { return m_flNum; }

    virtual const Ticketing::RemoteSystemContext::SystemContext* readSysCont() const = 0;
};

}//namespace edifact
