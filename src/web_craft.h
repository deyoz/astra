#pragma once

#include <vector>
#include <string>

#include "astra_consts.h"
#include "web_main.h"
#include "xml_unit.h"
#include "salons.h"
#include "seats.h"
#include "astra_locale.h"

namespace AstraWeb
{
namespace WebCraft {
  struct FilterWebSeat { // для расчета свойств мест веб регистрации
    int point_id;
    bool pr_lat;
    bool pr_CHIN;
    std::vector<AstraWeb::TWebPax> pnr;
    std::string pass_rem;
    std::set<std::string> cabin_classes;
    std::string crs_pax_cabin_class;
    FilterWebSeat() {
      pr_lat = true;
      pr_CHIN = false;
      point_id = ASTRA::NoExists;
    }
    std::string toString() const {
      std::ostringstream buf;
      buf << "FilterWebSeat: point_id=" << point_id << ",pr_lat=" << pr_lat;
      if ( pr_CHIN ) {
        buf << ",pr_CHIN";
      }
      if( !pass_rem.empty() ) {
        buf << ",pass_rem=" << pass_rem;
      }
      buf << ",cabin_classes=";
      for ( const auto& str : cabin_classes ) {
        buf << " " << str;
      }
      return buf.str();
    }
  };

  class TWebPlace {
    private:
      int x, y;
      std::string xname;
      std::string yname;
      std::string seat_no;
      std::string elem_type;
      std::string cls;
      int pr_free;
      bool reserv_owner;
      int pr_CHIN;
      int pax_id;
      ASTRA::TCompLayerType layer_type;
      int layer_pax_id;
      std::vector<SEATS2::TRem> rems;
      SALONS2::TSeatTariff SeatTariff;
      SALONS2::TRFISC rfisc;
      std::string getSubCls( const std::vector<SALONS2::TRem> &rems );
      void layerFromSeats( SALONS2::IPlace &seat, const FilterWebSeat &filterWebSeat );
      void propsFromPlace( SALONS2::IPlace &seat, const FilterWebSeat &filterWebSeat );
    public:
      TWebPlace() {
        pr_free = 0;
        pr_CHIN = 0;
        pax_id = ASTRA::NoExists;
        layer_type = ASTRA::TCompLayerType::cltUnknown;
        layer_pax_id = ASTRA::NoExists;
        reserv_owner = false;
      }
      TWebPlace( SALONS2::IPlace seat, const FilterWebSeat &filterWebSeat ):TWebPlace() {
        propsFromPlace( seat, filterWebSeat );
      }
      bool isFreeSubcls() const {
        return pr_free == 3;
      }
      int get_seat_status( bool pr_find_free_subcls_place, bool view_craft ) const;
      int getX() const {
        return x;
      }
      int getY() const {
        return y;
      }
      int getPaxId() const {
        return pax_id;
      }
      void setPaxId( int pax_id ) {
         this->pax_id = pax_id;
      }
      std::string getSeatNo() const {
        return seat_no;
      }
      void setSeatNo( const std::string &seat_no ) {
        this->seat_no = seat_no;
      }

      std::string getXName() const {
        return xname;
      }
      std::string getYName() const {
        return yname;
      }
      SALONS2::TSeatTariff getTariff() const {
        return SeatTariff;
      }
      void toXML( int version, bool isFreeSubclsSeats, xmlNodePtr placeListNode ) const;
  };

  typedef std::vector<TWebPlace> TWebPlaces;

  class TWebPlaceList: public TWebPlaces {
    private:
      int xcount, ycount;
      bool pr_find_free_subcls_place;
    public:
      TWebPlaceList() {
        xcount = 0;
        ycount = 0;
        pr_find_free_subcls_place = false;
      }
      void add( const TWebPlace &seat ) {
        if ( seat.getX() > xcount ) {
          xcount = seat.getX();
        }
        if ( seat.getY() > ycount ) {
          ycount = seat.getY();
        }
        pr_find_free_subcls_place |= seat.isFreeSubcls();
        push_back( seat );
      }
      bool FreeSubclsExists() const {
        return pr_find_free_subcls_place;
      }
      void toXML( int version, int num, bool isFreeSubclsSeats, xmlNodePtr viewCraftNode ) const;
  };

  class WebCraft: public std::map<int, TWebPlaceList> {
    private:
      int version;
      SALONS2::TSalonList salonList;
      bool pr_find_free_subcls_place;
      void add( int num, const TWebPlaceList &placeList );
    public:
      WebCraft( int version ) {
        pr_find_free_subcls_place = false;
        this->version = version;
      }
      bool FreeSubclsExists() {
        return pr_find_free_subcls_place;
      }
      void Read( int point_id, int crs_pax_id, const std::vector<AstraWeb::TWebPax> &pnr );
      void toXML( xmlNodePtr craftNode );
      SALONS2::TSalonList &getSalonList() {
        return salonList;
      }
      bool findSeat( std::string seat_no, TWebPlace &wsp );
  };


  void ViewCraft(const std::vector<TWebPax> &paxs, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetCrsPaxSeats( int point_id, const std::vector<TWebPax> &pnr,
                       std::vector< std::pair<TWebPlace, AstraLocale::LexemaData> > &pax_seats );
} //namespace WebCraft
}

