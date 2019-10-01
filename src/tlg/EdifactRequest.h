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
     * @param msg_type тип сообщения
     */
    EdifactRequest(const std::string& pult,
                   const std::string& ctxt,
                   const edifact::KickInfo& v_kickInfo,
                   edi_msg_types_t msg_type,
                   const Ticketing::RemoteSystemContext::SystemContext* sysCont);

    virtual void updateMesHead() {}
    virtual void collectMessage() = 0;

    virtual bool needRemoteResults() const { return true; }
    virtual bool needSaveEdiSessionContext() const { return true; }
    virtual bool needConfigAgentToWait() const { return true; }

    virtual void sendTlg();

    // Обобщённый код сообщения - совпадает с mesFuncCode всегда, кроме iatci сообщений
    virtual std::string funcCode() const;

    const std::string& context() const { return ediSessCtxt; }
    const KickInfo& kickInfo() const { return m_kickInfo; }

    const TlgHandling::TlgSourceEdifact* tlgOut() const;

    const Ticketing::RemoteSystemContext::SystemContext* sysCont();

    virtual ~EdifactRequest();
};

} // namespace edifact
