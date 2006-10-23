#ifndef _CENT_H_
#define _CENT_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class CentInterface : public JxtInterface
{
private:
  static void readTripHeader( int point_id, xmlNodePtr dataNode );
public:
  CentInterface() : JxtInterface("","cent")
  {
  };

  static void ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};


#endif /*_CENT_H_*/

