#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <set>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"
#include "basic.h"
#include "astra_locale.h"

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

namespace BASIC_SALONS
{

class TCompElemType {
  private:
    std::string code;
    std::string name;
    std::string name_lat;
    bool is_seat;
    bool is_default;
    BASIC::TDateTime time_create;
    std::string image;
  public:
    TCompElemType( const std::string &vcode,
                   const std::string &vname,
                   const std::string &vname_lat,
                   const bool vis_seat,
                   const bool vis_default,
                   const BASIC::TDateTime &vtime_create,
                   const std::string &vimage ) {
      code = vcode;
      name = vname;
      name_lat = vname_lat;
      is_seat = vis_seat;
      is_default = vis_default;
      time_create = vtime_create;
      image = vimage;
    }
    TCompElemType() {
      Clear();
    }
    void operator = ( const TCompElemType &elem ) {
       code = elem.code;
       name = elem.name;
       is_seat = elem.is_seat;
       is_default = elem.is_default;
       time_create = elem.time_create;
       image = elem.image;
    }
    void Clear() {
      code.clear();
      name.clear();
      name_lat.clear();
      is_seat = false;
      is_default = false;
      time_create = -1;
      image.clear();
    }
    bool isSeat() {
      return is_seat;
    }
    std::string getCode() {
      return code;
    }
    std::string getName() {
      return name;
    }
    std::string getNameLat() {
      return name_lat;
    }
    std::string getImage() {
      return image;
    }
};

class TCompElemTypes {
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
      comp_elem.Clear();
      return false;
    }
    bool isSeat( const std::string &elem_type ) {
      TCompElemType elem;
      return ( getElem( elem_type, elem ) && elem.isSeat() );
    }
    TCompElemTypes();
};

////////////////////////////////////////////////////////////////////////////////
class TCompLayerType {
  private:
    std::string code;
    ASTRA::TCompLayerType layer_type;
    std::string name;
    std::string name_lat;
    bool del_if_comp_chg;
    std::string color;
    std::string figure;
    bool pr_occupy;
    int priority;
  public:
    TCompLayerType( ) {
      Clear();
    }
    TCompLayerType( const std::string &vcode,
                    ASTRA::TCompLayerType vlayer_type,
                    const std::string &vname,
                    const std::string &vname_lat,
                    bool vdel_if_comp_chg,
                    const std::string &vcolor,
                    const std::string &vfigure,
                    bool vpr_occupy,
                    int vpriority ) {
      code = vcode;
      layer_type = vlayer_type;
      name = vname;
      name_lat = vname_lat;
      del_if_comp_chg = vdel_if_comp_chg;
      color = vcolor;
      figure = vfigure;
      pr_occupy = vpr_occupy;
      priority = vpriority;
    }
    void Clear() {
      code.clear();
      layer_type = ASTRA::cltUnknown;
      name.clear();
      name_lat.clear();
      del_if_comp_chg = false;
      color.clear();
      figure.clear();
      pr_occupy = false;
      priority = -1;
    }
    void operator = ( const BASIC_SALONS::TCompLayerType &layer ) {
      code = layer.code;
      layer_type = layer.layer_type;
      name = layer.name;
      name_lat = layer.name_lat;
      if ( name_lat.empty() )
        name_lat = name;
      del_if_comp_chg = layer.del_if_comp_chg;
      color = layer.color;
      figure = layer.figure;
      pr_occupy = layer.pr_occupy;
      priority = layer.priority;
    }
    int getPriority() {
      return priority;
    }
    bool getOccupy() {
      return pr_occupy;
    }
    std::string getCode() {
      return code;
    }
    std::string getName( ) {
      return name;
    }
    std::string getNameLat() {
      return name_lat;
    }
};

class TCompLayerTypes {
  private:
    std::map<ASTRA::TCompLayerType,BASIC_SALONS::TCompLayerType> layers;
    std::map<ASTRA::TCompLayerType, std::map<ASTRA::TCompLayerType,int> > layers_priority_routes;
  public:
    static TCompLayerTypes *Instance() {
      static TCompLayerTypes *_instance = 0;
      if ( !_instance ) {
        _instance = new TCompLayerTypes();
      }
      return _instance;
    }
    void Update();
    TCompLayerTypes();
    bool getElem( const ASTRA::TCompLayerType &layer_type, TCompLayerType &layer_elem ) {
      if ( layers.find( layer_type ) != layers.end() ) {
        layer_elem = layers[ layer_type ];
        return true;
      }
      layer_elem.Clear();
      return false;
    }
    int priority( const ASTRA::TCompLayerType &layer_type ) {
      TCompLayerType elem;
      getElem( layer_type, elem );
      return elem.getPriority();
    }                        /*  ан  е = layer1, ╗озже layer2, ╗о  oо└ ани   ан  ий ╗ ио и е нее ╗озднего */
    bool priority_on_routes( const ASTRA::TCompLayerType &layer_type1,
                             const ASTRA::TCompLayerType &layer_type2,
                             int layer1_time_less ) {
      if ( layers_priority_routes.find( layer_type1 ) != layers_priority_routes.end() ) {
        if ( layers_priority_routes[ layer_type1 ].find( layer_type2 ) != layers_priority_routes[ layer_type1 ].end() ) {
          if ( layers_priority_routes[ layer_type1 ][ layer_type2 ] == ASTRA::NoExists ||
               layers_priority_routes[ layer_type1 ][ layer_type2 ] == layer1_time_less ) {
            return false; // ес└и найден  └еoен ,  о  ан  ий oен  е ╗озднего
          }
        }
      }
      return true;
    }
    std::string getCode( const ASTRA::TCompLayerType &layer_type ) {
      TCompLayerType elem;
      getElem( layer_type, elem );
      return elem.getCode();
    }
    std::string getName( const ASTRA::TCompLayerType &layer_type,
                         std::string Lang = AstraLocale::LANG_RU ) {
      TCompLayerType elem;
      getElem( layer_type, elem );
      if ( Lang == AstraLocale::LANG_RU || elem.getNameLat().empty() ) {
        return elem.getName();
      }
      else {
        return elem.getNameLat();
      }
    }
};

}; //


void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode);

#endif /*_IMAGES_H_*/

