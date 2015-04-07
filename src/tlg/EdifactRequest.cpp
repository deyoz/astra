/*
*  C++ Implementation: EdifactRequest
*
* Description: Makes an EDIFACT request structure
*
*
* Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
*
*/
#include "EdifactRequest.h"
#include "exceptions.h"
#include "remote_system_context.h"
#include "remote_results.h"
#include "edi_tlg.h"
#include "tlg.h"
#include "tlg_source_edifact.h"
#include "EdiSessionTimeOut.h"
#include "AgentWaitsForRemote.h"
#include "astra_context.h"

#include <edilib/edi_func_cpp.h>

#include <memory.h>
#include <boost/scoped_ptr.hpp>

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edifact
{
using namespace EXCEPTIONS;
using namespace TlgHandling;
using namespace Ticketing;

const KickInfo& KickInfo::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr kickInfoNode=NewTextChild(node,"kick_info");
  reqCtxtId==ASTRA::NoExists?NewTextChild(kickInfoNode, "req_ctxt_id"):
                             NewTextChild(kickInfoNode, "req_ctxt_id", reqCtxtId);
  NewTextChild(kickInfoNode, "iface", iface);
  NewTextChild(kickInfoNode, "handle", handle);
  parentSessId==ASTRA::NoExists?NewTextChild(kickInfoNode, "parent_sess_id"):
                                NewTextChild(kickInfoNode, "parent_sess_id", parentSessId);
  NewTextChild(kickInfoNode, "int_msg_id", msgId);
  NewTextChild(kickInfoNode, "desk", desk);
  return *this;
}

KickInfo& KickInfo::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr kickInfoNode=GetNode("kick_info",node);
  if (kickInfoNode==NULL) return *this;
  if (!NodeIsNULL("req_ctxt_id", kickInfoNode))
    reqCtxtId=NodeAsInteger("req_ctxt_id", kickInfoNode);
  iface=NodeAsString("iface", kickInfoNode);
  handle=NodeAsString("handle", kickInfoNode);
  if (!NodeIsNULL("parent_sess_id", kickInfoNode))
    parentSessId=NodeAsInteger("parent_sess_id", kickInfoNode);
  msgId=NodeAsString("int_msg_id", kickInfoNode);
  desk=NodeAsString("desk", kickInfoNode);
  return *this;
}

EdifactRequest::EdifactRequest(const std::string &pult,
                               const std::string& ctxt,
                               const KickInfo &v_kickInfo,
                               edi_msg_types_t msg_type,
                               const Ticketing::RemoteSystemContext::SystemContext* sysCont)
    :edilib::EdifactRequest(msg_type), TlgOut(0), ediSessCtxt(ctxt), m_kickInfo(v_kickInfo)
{
    setEdiSessionController(new NewAstraEdiSessWR(pult, msgHead(), sysCont));
    setEdiSessMesAttr();
}

EdifactRequest::~EdifactRequest()
{
    delete TlgOut;
}

void EdifactRequest::sendTlg()
{
    if(TlgOut)
        delete TlgOut;

    collectMessage();

    TlgOut = new TlgSourceEdifact(makeEdifactText(), ediSess()->hth());

    LogTrace(TRACE1) << *TlgOut;

    // Положить тлг в очередь на отправку
    ::sendTlg(sysCont()->routerCanonName().c_str(),
              OWN_CANON_NAME(),
              qpOutA,
              sysCont()->edifactResponseTimeOut(),
              TlgOut->text(),
              ASTRA::NoExists,
              ASTRA::NoExists);
    ediSess()->ediSession()->CommitEdiSession();

    //запишем контексты
    AstraContext::SetContext("EDI_SESSION",
                             ediSessId().get(),
                             context());


    // Записать информацию о timeout отправленной телеграммы
    edilib::EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
                                   mesFuncCode(),
                                   ediSessId(),
                                   sysCont()->edifactResponseTimeOut());

    RemoteResults::add(kickInfo().msgId,
                       kickInfo().desk,
                       ediSess()->ediSession()->ida(),
                       sysCont()->ida());

    Ticketing::ConfigAgentToWait(sysCont()->ida(),
                                 ediSess()->ediSession()->pult(),
                                 ediSess()->ediSession()->ida(),
                                 kickInfo());

}

const TlgSourceEdifact * EdifactRequest::tlgOut() const
{
    return TlgOut;
}

const Ticketing::RemoteSystemContext::SystemContext * EdifactRequest::sysCont()
{
    return dynamic_cast<NewAstraEdiSessWR*>(ediSess())->sysCont();
}

} // namespace edifact

