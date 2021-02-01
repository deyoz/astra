#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <set>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"
#include "date_time.h"
#include "astra_locale.h"
#include "astra_utils.h"

using BASIC::date_time::TDateTime;

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
    TDateTime time_create;
    std::string image;
    std::string filename;
  public:
    TCompElemType( const std::string &vcode,
                   const std::string &vname,
                   const std::string &vname_lat,
                   const bool vis_seat,
                   const bool vis_default,
                   const TDateTime &vtime_create,
                   const std::string &vimage,
                   const std::string &vfilename ) {
      code = vcode;
      name = vname;
      name_lat = vname_lat;
      is_seat = vis_seat;
      is_default = vis_default;
      time_create = vtime_create;
      image = vimage;
      filename = vfilename;
    }
    TCompElemType() {
      Clear();
    }
    void operator = ( const TCompElemType &elem ) {
       code = elem.code;
       name = elem.name;
       name_lat = elem.name_lat;
       is_seat = elem.is_seat;
       is_default = elem.is_default;
       time_create = elem.time_create;
       image = elem.image;
       filename = elem.filename;
    }
    void Clear() {
      code.clear();
      name.clear();
      name_lat.clear();
      is_seat = false;
      is_default = false;
      time_create = -1;
      image.clear();
      filename.clear();
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
    std::string getFilename() {
      return filename;
    }
};

class TCompElemTypes {
  private:
    std::map<std::string,TCompElemType> is_places;
    TDateTime max_time_create;
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
    TDateTime getLastUpdateDate() {
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
class TCompLayerElem {
  private:
    std::string code;
    ASTRA::TCompLayerType layer_type;
    std::string name;
    std::string name_lat;
    std::string color;
    std::string figure;
    bool pr_occupy;
  public:
    TCompLayerElem( ) {
      Clear();
    }
    TCompLayerElem( const std::string &vcode,
                    ASTRA::TCompLayerType vlayer_type,
                    const std::string &vname,
                    const std::string &vname_lat,
                    const std::string &vcolor,
                    const std::string &vfigure,
                    bool vpr_occupy ) {
      code = vcode;
      layer_type = vlayer_type;
      name = vname;
      name_lat = vname_lat;
      color = vcolor;
      figure = vfigure;
      pr_occupy = vpr_occupy;
    }
    void Clear() {
      code.clear();
      layer_type = ASTRA::cltUnknown;
      name.clear();
      name_lat.clear();
      color.clear();
      figure.clear();
      pr_occupy = false;
    }
    void operator = ( const BASIC_SALONS::TCompLayerElem &layer ) {
      code = layer.code;
      layer_type = layer.layer_type;
      name = layer.name;
      name_lat = layer.name_lat;
      if ( name_lat.empty() )
        name_lat = name;
      color = layer.color;
      figure = layer.figure;
      pr_occupy = layer.pr_occupy;
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
    std::string getFigure() {
      return figure;
    }
    ASTRA::TCompLayerType getLayerType() {
      return layer_type;
    }
    std::string getColor() {
      return color;
    }
};

class TCompLayerPriority: public TCompLayerElem {
  private:
    std::string code;
    ASTRA::TCompLayerType layer_type;
    std::string name;
    std::string name_lat;
    std::string color;
    std::string figure;
    bool pr_occupy;
    int priority;
  public:
    TCompLayerPriority( ) {
      Clear();
    }
    TCompLayerPriority( const std::string &vcode,
                        ASTRA::TCompLayerType vlayer_type,
                        const std::string &vname,
                        const std::string &vname_lat,
                        const std::string &vcolor,
                        const std::string &vfigure,
                        bool vpr_occupy,
                        int vpriority ):TCompLayerElem(vcode,
                                                       vlayer_type,
                                                       vname,
                                                       vname_lat,
                                                       vcolor,
                                                       vfigure,
                                                       vpr_occupy),priority(vpriority) {}
    void Clear() {
      TCompLayerElem::Clear();
      priority = -1;
    }
    void operator = ( const TCompLayerPriority &layer ) {
      TCompLayerElem::operator =(layer);
      priority = layer.priority;
    }
    int getPriority() {
      return priority;
    }
};

class TCompLayerTypes {
  public:
    enum Enum { useAirline, ignoreAirline };
    struct LayerKey {
      std::string airline;
      ASTRA::TCompLayerType layer_type;
      bool operator < ( const LayerKey& _apriority ) const {
          if ( airline != _apriority.airline )
            return ( airline < _apriority.airline );
        return ( EncodeCompLayerType( layer_type ) < EncodeCompLayerType( _apriority.layer_type ) );
      }
      LayerKey( const std::string& _airline, ASTRA::TCompLayerType _layer_type) {
        airline = _airline;
        layer_type = _layer_type;
      }
    };

  private:
    std::map<LayerKey,int> airline_priorities; //приоритеты для АК
    std::map<ASTRA::TCompLayerType,TCompLayerPriority> layers;
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
    bool getElem( const ASTRA::TCompLayerType layer_type, TCompLayerElem &layer_elem ) {
      if ( layers.find( layer_type ) != layers.end() ) {
        layer_elem = layers[ layer_type ];
        return true;
      }
      layer_elem.Clear();
      return false;
    }
    int priority( const LayerKey &key, const Enum &flag ) {
      if ( layers.find( key.layer_type ) != layers.end() ) {
        if ( flag != ignoreAirline &&
             !key.airline.empty() &&
             airline_priorities.find( key ) != airline_priorities.end() ) {
          return airline_priorities[ key ];
        }
        return layers[ key.layer_type ].getPriority();
      }
      throw EXCEPTIONS::Exception( "get layer priority: layer type is not found ");
    }                        /*  ан  е = layer1, ┐озже layer2, ┐о  oоL ани   ан  ий ┐ ио и е нее ┐озднего */
    bool priority_on_routes( const ASTRA::TCompLayerType &layer_type1,
                             const ASTRA::TCompLayerType &layer_type2,
                             int layer1_time_less ) {
      if ( layers_priority_routes.find( layer_type1 ) != layers_priority_routes.end() ) {
        if ( layers_priority_routes[ layer_type1 ].find( layer_type2 ) != layers_priority_routes[ layer_type1 ].end() ) {
          if ( layers_priority_routes[ layer_type1 ][ layer_type2 ] == ASTRA::NoExists ||
               layers_priority_routes[ layer_type1 ][ layer_type2 ] == layer1_time_less ) {
            return false; // есLи найден  Lеoен ,  о  ан  ий oен  е ┐озднего
          }
        }
      }
      return true;
    }
    std::string getCode( const ASTRA::TCompLayerType &layer_type ) {
      TCompLayerElem elem;
      getElem( layer_type, elem );
      return elem.getCode();
    }
    std::string getName( const ASTRA::TCompLayerType &layer_type,
                         std::string Lang = AstraLocale::LANG_RU ) {
      TCompLayerElem elem;
      getElem( layer_type, elem );
      if ( Lang == AstraLocale::LANG_RU || elem.getNameLat().empty() ) {
        return elem.getName();
      }
      else {
        return elem.getNameLat();
      }
    }
};

}


void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode);
void getTariffColors( std::map<std::string,std::string> &colors );

#endif /*_IMAGES_H_*/

