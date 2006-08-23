#ifndef _PREPREG_H_
#define _PREPREG_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class PrepRegInterface : public JxtInterface
{
private:
  void readTripHeader( int point_id, xmlNodePtr dataNode );
  void readTripCounters( int point_id, xmlNodePtr dataNode );
  void readTripData( int point_id, xmlNodePtr dataNode );
public:
  PrepRegInterface() : JxtInterface("","prepreg")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::ReadTripInfo);
     AddEvent("ReadTripInfo",evHandle);               
     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::CrsDataApplyUpdates);
     AddEvent("CrsDataApplyUpdates",evHandle);                    
     evHandle=JxtHandler<PrepRegInterface>::CreateHandler(&PrepRegInterface::ViewPNL);
     AddEvent("ViewPNL",evHandle);                         
  };

  void ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CrsDataApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewPNL(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_PREPREG_H_*/

