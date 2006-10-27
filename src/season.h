#ifndef _SEASON_H_
#define _SEASON_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

class SeasonInterface : public JxtInterface
{
public:
  SeasonInterface() : JxtInterface("123","season")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Filter);
     AddEvent("filter",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Read);
     AddEvent("season_read",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Write);
     AddEvent("write",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::GetSPP);
     AddEvent("get_spp",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::DelRangeList);
     AddEvent("del_range_list",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::RemovalGangWayTimes);
     AddEvent("RemovalGangWayTimes",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Edit);
     AddEvent("edit",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::ViewSPP);
     AddEvent("spp",evHandle);
  };

  void Filter(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RemovalGangWayTimes(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Edit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
