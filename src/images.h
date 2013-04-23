#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"
#include "basic.h"

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

  void GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetDrawSalonData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void GetImages( xmlNodePtr reqNode, xmlNodePtr imagesNode );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

struct TCompElemType {
  std::string code;
  std::string name;
  bool is_seat;
  bool is_default;
  BASIC::TDateTime time_create;
  std::string image;
  void operator = ( const TCompElemType &elem ) {
     code = elem.code;
     name = elem.name;
     is_seat = elem.is_seat;
     is_default = elem.is_default;
     time_create = elem.time_create;
     image = elem.image;
  }
};

struct TCompElemTypes {
  private:
    std::map<std::string,TCompElemType> is_places;
    BASIC::TDateTime max_time_create;
    std::string default_elem_code;
  public:
    static TCompElemTypes *Instance() {
      static TCompElemTypes *_instance = 0;
      if ( !_instance ) {
        _instance = new TCompElemTypes();
      }
      return _instance;
    }
    void Update();
    std::string getDefaultElemType() {
      return default_elem_code;
    }
    BASIC::TDateTime getLastUpdateDate() {
      return max_time_create;
    }
    void getElemTypes( std::vector<std::string> &elem_types ) {
      elem_types.clear();
      for ( std::map<std::string,TCompElemType>::iterator iis_pl=is_places.begin();
            iis_pl!=is_places.end(); iis_pl++ ) {
        elem_types.push_back( iis_pl->first );
      }
    }
    bool getElem( const std::string &elem_type, TCompElemType &comp_elem ) {
      if ( is_places.find( elem_type ) != is_places.end() ) {
        comp_elem = is_places[ elem_type ];
        return true;
      }
      comp_elem.code.clear();
      comp_elem.name.clear();
      comp_elem.is_seat = false;
      comp_elem.is_default = false;
      return false;
    }
    bool isSeat( const std::string &elem_type ) {
      TCompElemType elem;
      return ( getElem( elem_type, elem ) && elem.is_seat );
    }
    TCompElemTypes();
};

void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode);

#endif /*_IMAGES_H_*/

