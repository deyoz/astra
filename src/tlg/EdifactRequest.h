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

#include <string>

#include <edilib/edi_types.h>
#include <edilib/edi_request.h>


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
   int ReqCtxtId;
public:
    /**
     * @brief EdifactRequest
     * @param msg_type тип сообщения
     */
    EdifactRequest(const std::string &pult, int ctxtId, edi_msg_types_t msg_type,
                   const Ticketing::RemoteSystemContext::SystemContext* sysCont);


    virtual void collectMessage() = 0;

    virtual void sendTlg();

    int reqCtxtId() const;

    const TlgHandling::TlgSourceEdifact *tlgOut() const;

    const Ticketing::RemoteSystemContext::SystemContext *sysCont();

    virtual ~EdifactRequest();
};
} // namespace edifact
