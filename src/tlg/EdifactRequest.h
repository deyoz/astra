//
// C++ Interface: EdifactRequest
//
// Description: Makes an EDIFACT request structure
//
//
// Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
//
//
#pragma once

#include "request_params.h"

#include <edilib/edi_types.h>
#include <edilib/edi_request.h>
#include <edilib/edi_astra_msg_types.h>

struct _edi_mes_head_;
struct _EDI_REAL_MES_STRUCT_;

class AstraEdiSessWR;
namespace Ticketing {
namespace RemoteSystemContext{
    class SystemContext;
}//namespace RemoteSystemContext
}//namespace Ticketing

namespace TlgHandling{
    class TlgSourceEdifact;
}//namespace TlgHandling


namespace edifact
{

/**
 * @class EdifactRequest
 * @brief Makes an EDIFACT request structure
 */
class EdifactRequest : public edilib::EdifactRequest
{
   TlgHandling::TlgSourceEdifact *TlgOut;
   std::string ediSessCtxt;
   KickInfo m_kickInfo;
   const Ticketing::RemoteSystemContext::SystemContext* SysCont;

public:
    /**
     * @brief EdifactRequest
     * @param msg_type ⨯ ᮮ�饭��
     */
    EdifactRequest(const std::string& pult,
                   const std::string& ctxt,
                   const edifact::KickInfo& v_kickInfo,
                   edi_msg_types_t msg_type,
                   const Ticketing::RemoteSystemContext::SystemContext* sysCont);


    virtual void collectMessage() = 0;

    virtual void sendTlg();

    // ������� ��� ᮮ�饭�� - ᮢ������ � mesFuncCode �ᥣ��, �஬� iatci ᮮ�饭��
    virtual std::string funcCode() const;

    const std::string& context() const { return ediSessCtxt; }
    const KickInfo& kickInfo() const { return m_kickInfo; }

    const TlgHandling::TlgSourceEdifact* tlgOut() const;

    const Ticketing::RemoteSystemContext::SystemContext* sysCont();

    edilib::EdiSessionId_t ediSessionId() const;
    virtual ~EdifactRequest();
};

} // namespace edifact
