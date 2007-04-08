#ifndef _ETICK_H_
#define _ETICK_H_

#include "JxtInterface.h"
#include "xmllibcpp.h"

class ETSearchInterface : public JxtInterface
{
public:
  ETSearchInterface() : JxtInterface("ETSearchForm","ETSearchForm")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::SearchETByTickNo);
     AddEvent("TickPanel",evHandle);
     AddEvent("kick", JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::KickHandler));
  };

  void SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ETChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

class ETStatusInterface : public JxtInterface
{
public:
  ETStatusInterface() : JxtInterface("ETStatus","ETStatus")
  {
     AddEvent("ChangePaxStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangePaxStatus));
     AddEvent("ChangeGrpStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeGrpStatus));
     AddEvent("ChangeFltStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeFltStatus));
     AddEvent("kick", JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::KickHandler));
  };

  void ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

enum TETCheckStatusArea {csaFlt,csaGrp,csaPax};
bool ETCheckStatus(int id, TETCheckStatusArea area, int point_id=-1);

inline xmlNodePtr astra_iface(xmlNodePtr resNode, const std::string &iface_id)
{

    xmlSetProp(resNode,"handle","1");

    xmlNodePtr ifaceNode=getNode(resNode,"interface");
    xmlSetProp(ifaceNode,"id",iface_id);
    xmlSetProp(ifaceNode,"ver","0");

    return ifaceNode;
}

#endif

