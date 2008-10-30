#ifndef _PREPREG_H_
#define _PREPREG_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class PrepRegInterface : public JxtInterface
{
public:
  PrepRegInterface() : JxtInterface("","prepreg")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::CrsDataApplyUpdates);
     AddEvent("CrsDataApplyUpdates",evHandle);
/*     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::ViewPNL);
     AddEvent("ViewPNL",evHandle);*/
     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::ViewCRSList);
     AddEvent("ViewCRSList",evHandle);
  };

  void CrsDataApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
 /* void ViewPNL(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);*/
  void ViewCRSList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
};


#endif /*_PREPREG_H_*/

