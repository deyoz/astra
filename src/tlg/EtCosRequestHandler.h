#pragma once

#include "RequestHandler.h"
#include "astra_ticket.h"

#include <boost/optional.hpp>


namespace TlgHandling {

struct CosParams
{
    Ticketing::TicketNum_t m_tickNum;
    Ticketing::CouponNum_t m_cpnNum;

    CosParams(const Ticketing::TicketNum_t& tickNum,
              const Ticketing::CouponNum_t& cpnNum)
        : m_tickNum(tickNum),
          m_cpnNum(cpnNum)
    {}
};

//---------------------------------------------------------------------------------------

class CosRequestHandler: public AstraRequestHandler
{
    boost::optional<CosParams> m_cosParams;

public:
    CosRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                      const edilib::EdiSessRdData *edisess);

    virtual std::string mesFuncCode() const;
    virtual bool fullAnswer() const;
    virtual void parse();
    virtual void handle();
    virtual void makeAnAnswer();

    virtual void saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                               const std::string& errText);


    virtual ~CosRequestHandler() {}
};

}//namespace TlgHandling
