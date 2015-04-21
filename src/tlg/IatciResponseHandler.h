#pragma once

#include "ResponseHandler.h"
#include "iatci_types.h"
#include "edi_elements.h"


namespace TlgHandling
{

class IatciResponseHandler: public AstraEdiResponseHandler
{
protected:
    std::list<iatci::Result> m_lRes;

public:
    IatciResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                         const edilib::EdiSessRdData *edisess);

    virtual void fillFuncCodeRespStatus();
    virtual void fillErrorDetails();
    virtual void handle();
    virtual void onTimeOut();
    virtual void onCONTRL() {}

    virtual ~IatciResponseHandler() {}
};

//---------------------------------------------------------------------------------------

class IatciResultMaker
{
private:
    edifact::FdrElem m_fdr;
    edifact::RadElem m_rad;
    boost::optional<edifact::PpdElem> m_ppd;
    boost::optional<edifact::ChdElem> m_chd;
    boost::optional<edifact::PfdElem> m_pfd;
    boost::optional<edifact::FsdElem> m_fsd;
    boost::optional<edifact::ErdElem> m_erd;

public:
    void setFdr(const boost::optional<edifact::FdrElem>& fdr);
    void setRad(const boost::optional<edifact::RadElem>& rad);
    void setPpd(const boost::optional<edifact::PpdElem>& ppd, bool required = false);
    void setPfd(const boost::optional<edifact::PfdElem>& pfd, bool required = false);
    void setFsd(const boost::optional<edifact::FsdElem>& fsd, bool required = false);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setErd(const boost::optional<edifact::ErdElem>& erd, bool required = false);
    iatci::Result makeResult() const;
};

}//namespace TlgHandling
