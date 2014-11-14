#ifndef _EMD_DISP_REQUEST_H_
#define _EMD_DISP_REQUEST_H_

#include "emd_request.h"

enum EmdDispType_e{
    emdDispByNum,
};

namespace edifact
{

class EmdDispParams : public EmdRequestParams
{
    EmdDispType_e m_dispType;
public:
    EmdDispParams(const Ticketing::OrigOfRequest& org,
                  const std::string& ctxt,
                  const edifact::KickInfo &kickInfo,
                  const std::string& airline,
                  const Ticketing::FlightNum_t& flNum,
                  EmdDispType_e dt)
    : EmdRequestParams(org, ctxt, kickInfo, airline, flNum),
      m_dispType(dt)
    {}

    EmdDispType_e dispType() { return m_dispType; }
};

//-----------------------------------------------------------------------------

class EmdDispByNum : public EmdDispParams
{
    Ticketing::TicketNum_t m_tickNum;
public:
    EmdDispByNum(const Ticketing::OrigOfRequest &org,
                 const std::string &ctxt,
                 const edifact::KickInfo &kickInfo,
                 const std::string& airline,
                 const Ticketing::FlightNum_t& flNum,
                 const Ticketing::TicketNum_t &ticknum)
    : EmdDispParams(org, ctxt, kickInfo, airline, flNum, emdDispByNum),
      m_tickNum(ticknum)
    {}

    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
};

//-----------------------------------------------------------------------------

class EmdDispRequestByNum: public EmdRequest
{
    EmdDispByNum m_dispParams;
public:
    EmdDispRequestByNum(const EmdDispByNum& dispParams);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~EmdDispRequestByNum() {}
};

}//namespace edifact

#endif /* _EMD_DISP_REQUEST_H_ */
