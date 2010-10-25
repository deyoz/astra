#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"

class ImagesInterface : public JxtInterface
{
private:
public:
  ImagesInterface() : JxtInterface("","images")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ImagesInterface>::CreateHandler(&ImagesInterface::GetImages);
     AddEvent("getimages",evHandle);
     evHandle=JxtHandler<ImagesInterface>::CreateHandler(&ImagesInterface::GetDrawSalonData);
     AddEvent("drawsalondata",evHandle);
     evHandle=JxtHandler<ImagesInterface>::CreateHandler(&ImagesInterface::SetImages);
     AddEvent("setimages",evHandle);
  };

  static void GetisPlaceMap( std::map<std::string,bool> &ispl );
  void GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetDrawSalonData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void GetImages( xmlNodePtr reqNode, xmlNodePtr imagesNode );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode);

#endif /*_IMAGES_H_*/

