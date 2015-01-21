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

#include "astra_consts.h"

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

struct KickInfo
{
  int reqCtxtId;
  std::string iface;
  std::string handle;
public:
  KickInfo() : reqCtxtId(ASTRA::NoExists) {};
  KickInfo(const int v_reqCtxtId,
           const std::string &v_iface)
    : reqCtxtId(v_reqCtxtId),
      iface(v_iface),
      handle("0") {};
  bool empty() const
  {
    return iface.empty();
  }
};

/**
 * @class EdifactRequest
 * @brief Makes an EDIFACT request structure
 */
class EdifactRequest : public edilib::EdifactRequest
{
   TlgHandling::TlgSourceEdifact *TlgOut;
   std::string ediSessCtxt;
   KickInfo m_kickInfo;
public:
    /**
     * @brief EdifactRequest
     * @param msg_type тип сообщения
     */
    EdifactRequest(const std::string &pult,
                   const std::string& ctxt,
                   const KickInfo &v_kickInfo,                   
                   edi_msg_types_t msg_type,
                   const Ticketing::RemoteSystemContext::SystemContext* sysCont);


    virtual void collectMessage() = 0;

    virtual void sendTlg();

    const std::string & context() const { return ediSessCtxt; }
    const KickInfo &kickInfo() const { return m_kickInfo; }

    const TlgHandling::TlgSourceEdifact *tlgOut() const;

    const Ticketing::RemoteSystemContext::SystemContext *sysCont();

    virtual ~EdifactRequest();
};
} // namespace edifact
