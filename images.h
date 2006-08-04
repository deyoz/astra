#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"


class ImagesInterface : public JxtInterface
{
private:
public:
  ImagesInterface() : JxtInterface("","images")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ImagesInterface>::CreateHandler(&ImagesInterface::GetImages);
     AddEvent("getimages",evHandle);
     evHandle=JxtHandler<ImagesInterface>::CreateHandler(&ImagesInterface::SetImages);
     AddEvent("setimages",evHandle);     
  };

  void GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void GetImages( xmlNodePtr reqNode, xmlNodePtr imagesNode );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_IMAGES_H_*/

