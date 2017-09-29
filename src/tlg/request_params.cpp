#include "request_params.h"
#include "xml_unit.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

KickInfo::KickInfo(const int v_reqCtxtId,
                   const std::string &v_iface,
                   const std::string &v_msgid,
                   const std::string &v_desk)
  : reqCtxtId(v_reqCtxtId),
    parentSessId(ASTRA::NoExists),
    msgId(v_msgid),
    desk(v_desk)
{
  jxt=JxtHandlerForKick(v_iface, "0");
}

KickInfo::KickInfo(const int v_reqCtxtId,
                   const int v_point_id,
                   const std::string &v_task_name,
                   const std::string &v_msgid,
                   const std::string &v_desk)
  : reqCtxtId(v_reqCtxtId),
    parentSessId(ASTRA::NoExists),
    msgId(v_msgid),
    desk(v_desk)
{
  task=TripTaskForPostpone(v_point_id, v_task_name);
}

void JxtHandlerForKick::clear()
{
  iface.clear();
  handle.clear();
}

void TripTaskForPostpone::clear()
{
  point_id=ASTRA::NoExists;
  name.clear();
}

void KickInfo::clear()
{
  reqCtxtId=ASTRA::NoExists;
  parentSessId=ASTRA::NoExists;
  msgId.clear();
  desk.clear();
  jxt=boost::none;
  task=boost::none;
}

const JxtHandlerForKick& JxtHandlerForKick::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "iface", iface);
  NewTextChild(node, "handle", handle);

  return *this;
}

const TripTaskForPostpone& TripTaskForPostpone::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "point_id", point_id);
  NewTextChild(node, "name", name);

  return *this;
}

const KickInfo& KickInfo::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr kickInfoNode=NewTextChild(node,"kick_info");
  reqCtxtId==ASTRA::NoExists?NewTextChild(kickInfoNode, "req_ctxt_id"):
                             NewTextChild(kickInfoNode, "req_ctxt_id", reqCtxtId);
  parentSessId==ASTRA::NoExists?NewTextChild(kickInfoNode, "parent_sess_id"):
                                NewTextChild(kickInfoNode, "parent_sess_id", parentSessId);
  NewTextChild(kickInfoNode, "int_msg_id", msgId);
  NewTextChild(kickInfoNode, "desk", desk);
  if (jxt) jxt.get().toXML(NewTextChild(kickInfoNode, "jxt"));
  if (task) task.get().toXML(NewTextChild(kickInfoNode, "task"));
  return *this;
}

JxtHandlerForKick& JxtHandlerForKick::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  iface=NodeAsString("iface", node);
  handle=NodeAsString("handle", node);

  return *this;
}

TripTaskForPostpone& TripTaskForPostpone::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  point_id=NodeAsInteger("point_id", node);
  name=NodeAsString("name", node);

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
  if (!NodeIsNULL("parent_sess_id", kickInfoNode))
    parentSessId=NodeAsInteger("parent_sess_id", kickInfoNode);
  msgId=NodeAsString("int_msg_id", kickInfoNode);
  desk=NodeAsString("desk", kickInfoNode);
  xmlNodePtr jxtNode=GetNode("jxt", kickInfoNode);
  if (jxtNode!=nullptr)
    jxt=JxtHandlerForKick(jxtNode);
  xmlNodePtr taskNode=GetNode("task", kickInfoNode);
  if (taskNode!=nullptr)
    task=TripTaskForPostpone(taskNode);
  return *this;
}

bool KickInfo::background_mode() const
{
  return msgId.empty();
}

}//namespace edifact
