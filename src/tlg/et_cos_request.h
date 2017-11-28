#pragma once

#include "et_request.h"


namespace edifact {

class EtCosParams: public EtRequestParams
{
    std::list<Ticketing::Ticket> lTick;
    Ticketing::Itin::SharedPtr Itin;

public:
    EtCosParams(const Ticketing::OrigOfRequest& org,
                const std::string& ctxt,
                const edifact::KickInfo& kickInfo,
                const std::string& airline,
                const Ticketing::FlightNum_t& flNum,
                const std::list<Ticketing::Ticket> &lt,
                const Ticketing::Itin *itin = NULL)
        : EtRequestParams(org, ctxt, kickInfo, airline, flNum), lTick(lt)
    {
        if(itin) {
            Itin.reset(new Ticketing::Itin(*itin));
        }
    }
    const std::list<Ticketing::Ticket>& ltick() const { return lTick; }
    bool isGlobItin() const { return Itin.get(); }
    const Ticketing::Itin& itin() const { return *Itin.get(); }

    virtual ~EtCosParams () {}
};

//---------------------------------------------------------------------------------------

class EtCosRequest: public EtRequest
{
    EtCosParams m_chngStatData;

public:
    EtCosRequest(const EtCosParams& chngStatData);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~EtCosRequest() {}
};

edilib::EdiSessionId_t SendEtCosRequest(const EtCosParams& cosParams);

}//namespace edifact
