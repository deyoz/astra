#pragma once

#include "CheckinBaseTypes.h"
#include "astra_ticket.h"
#include "remote_system_context.h"

namespace edifact {

struct JxtHandlerForKick
{
    std::string iface;
    std::string handle;

  public:
    JxtHandlerForKick(const std::string &_iface, const std::string &_handle) :
      iface(_iface), handle(_handle) {}
    JxtHandlerForKick(xmlNodePtr node) { fromXML(node); }
    void clear();
    const JxtHandlerForKick& toXML(xmlNodePtr node) const;
    JxtHandlerForKick& fromXML(xmlNodePtr node);
};

struct TripTaskForPostpone
{
    int point_id;
    std::string name;

  public:
    TripTaskForPostpone(const int &_point_id, const std::string &_name) :
      point_id(_point_id), name(_name) {}
    TripTaskForPostpone(xmlNodePtr node) { fromXML(node); }
    void clear();
    const TripTaskForPostpone& toXML(xmlNodePtr node) const;
    TripTaskForPostpone& fromXML(xmlNodePtr node);
};

struct KickInfo
{
    int reqCtxtId;
    int parentSessId;
    std::string msgId;
    std::string desk;
    boost::optional<JxtHandlerForKick> jxt;
    boost::optional<TripTaskForPostpone> task;

  public:
    KickInfo() { clear(); }
    KickInfo(const int v_reqCtxtId,
             const std::string& v_iface,
             const std::string& v_msgid,
             const std::string& v_desk);
    KickInfo(const int v_reqCtxtId,
             const int v_point_id,
             const std::string& v_task_name,
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
