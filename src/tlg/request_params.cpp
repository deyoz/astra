#include "request_params.h"
#include "xml_unit.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

KickInfo::KickInfo() :
    reqCtxtId(ASTRA::NoExists),
    parentSessId(ASTRA::NoExists)
{}

KickInfo::KickInfo(const int v_reqCtxtId,
                   const std::string &v_iface,
                   const std::string &v_msgid,
                   const std::string &v_desk)
  : reqCtxtId(v_reqCtxtId),
    iface(v_iface),
    handle("0"),
    parentSessId(ASTRA::NoExists),
    msgId(v_msgid),
    desk(v_desk)
{}

void KickInfo::clear()
{
  reqCtxtId=ASTRA::NoExists;
  iface.clear();
  handle.clear();
  parentSessId=ASTRA::NoExists;
  msgId.clear();
  desk.clear();
}

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

bool KickInfo::background_mode() const
{
  return msgId.empty();
}

}//namespace edifact
