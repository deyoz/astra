#ifndef _SALONSFORM_H_
#define _SALONFORM_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "jxtlib/JxtInterface.h"
#include "seats.h"
#include "salons.h"
#include "astra_misc.h"
#include "astra_consts.h"


class SalonFormInterface : public JxtInterface
{
private:
public:
  SalonFormInterface() : JxtInterface("","salonform")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Show);
     AddEvent("Show",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::ComponShow);
     AddEvent("ComponShow",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Write);
     AddEvent("Write",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::ComponWrite);
     AddEvent("ComponWrite",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Reseat);
     AddEvent("Reseat",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::DropSeats);
     AddEvent("DropSeats",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::DeleteProtCkinSeat);
     AddEvent("DeleteProtCkinSeat",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::WaitList);
     AddEvent("WaitList",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::AutoSeats);
     AddEvent("AutoSeats",evHandle);
  };
  void Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ComponShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ComponWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WaitList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

struct TZoneOccupiedSeats {
  std::string name;
  int total_seats;
  SALONS2::TPlaces seats;
  TZoneOccupiedSeats(): total_seats(0) {};
};

bool filterCompons( const std::string &airline, const std::string &airp );
void SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void ZoneLoads(int point_id, std::map<std::string, int> &zones, bool occupied);
void ZoneLoads(int point_id,
               bool only_checkin_layers, bool only_high_layer, bool drop_not_used_pax_layers,
               std::vector<TZoneOccupiedSeats> &zones,
               std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers);
void ZoneLoads(int point_id,
               bool only_checkin_layers, bool only_high_layer, bool drop_not_used_pax_layers,
               std::vector<TZoneOccupiedSeats> &zones,
               std::vector<SALONS2::TCompSectionLayers> &CompSectionsLayers,
               std::vector<SALONS2::TCompSection> &compSections );
void IntChangeSeats( int point_id, int pax_id,
                     int &tid, std::string xname, std::string yname,
	                   SEATS2::TSeatsType seat_type,
	                   ASTRA::TCompLayerType layer_type,
                     bool pr_waitlist, bool pr_question_reseat,
                     xmlNodePtr resNode );
void IntChangeSeatsN( int point_id, int pax_id, int &tid,
                      std::string xname, std::string yname,
                      SEATS2::TSeatsType seat_type,
                      ASTRA::TCompLayerType layer_type,
                      bool pr_waitlist, bool pr_question_reseat,
                      xmlNodePtr resNode );
void trace( int pax_id, int grp_id, int parent_pax_id, int crs_pax_id, const std::string &pers_type, int seats );
template <class T1>
void ZonePax( int point_id, std::vector<T1> &PaxItems, std::vector<SALONS2::TCompSection> &compSections )
{
  compSections.clear();
  std::vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
  std::vector<TZoneOccupiedSeats> zones;
  ZoneLoads( point_id, false, true, true, zones, CompSectionsLayers, compSections );
  std::vector<T1> InfItems, AdultItems;
  for ( typename std::vector<T1>::iterator i=PaxItems.begin(); i!=PaxItems.end(); i++ ) {
    trace( i->pax_id, i->grp_id, i->parent_pax_id, i->temp_parent_id, i->pers_type, i->seats );
    if ( i->seats == 0 )
      InfItems.push_back( *i );
    else
      if ( i->pers_type == "ВЗ" )
        AdultItems.push_back( *i );
  }
  //привязали детей к взрослым
  SetInfantsToAdults<T1,T1>( InfItems, AdultItems );
  bool pr_tranzit_salons = ( SALONS2::isTranzitSalons( point_id ) );
  for ( typename std::vector<T1>::iterator i=InfItems.begin(); i!=InfItems.end(); i++ ) {
    trace( i->pax_id, i->grp_id, i->parent_pax_id, i->temp_parent_id, i->pers_type, i->seats );
  }
  for ( typename std::vector<T1>::iterator i=PaxItems.begin(); i!=PaxItems.end(); i++ ) {
    i->zone.clear();
    int pax_id = i->pax_id;
    if ( i->seats == 0 ) {
      for ( typename std::vector<T1>::iterator j=InfItems.begin(); j!=InfItems.end(); j++ ) {
        if ( i->pax_id == j->pax_id ) {
          pax_id = j->parent_pax_id;
          break;
        }
      }
    }
    for ( std::vector<TZoneOccupiedSeats>::iterator z=zones.begin(); z!=zones.end(); z++ ) {
      for ( SALONS2::TPlaces::iterator p=z->seats.begin(); p!=z->seats.end(); p++ ) {
        if ( pr_tranzit_salons ) {
          if ( p->getCurrLayer( point_id ).getPaxId() == pax_id ) {
            i->zone = z->name;
            break;
          }
        }
        else {
          if ( p->layers.empty() )
            throw EXCEPTIONS::Exception( "ZonePax: p->layers.empty()" );
          if ( p->layers.begin()->pax_id == pax_id ) {
            i->zone = z->name;
            break;
          }
        }
      }
      if ( !i->zone.empty() )
        break;
    }
  }
};



#endif /*_SALONFORM_H_*/

