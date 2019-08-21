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
#include "comp_props.h"

namespace iatci { namespace dcrcka { class Result; } }
namespace iatci { class Seat; }


class SalonFormInterface : public JxtInterface
{
private:
public:
  SalonFormInterface() : JxtInterface("","salonform")
  {
     AddEvent("Show",                 JXT_HANDLER(SalonFormInterface, Show));
     AddEvent("ComponShow",           JXT_HANDLER(SalonFormInterface, ComponShow));
     AddEvent("Write",                JXT_HANDLER(SalonFormInterface, Write));
     AddEvent("ComponWrite",          JXT_HANDLER(SalonFormInterface, ComponWrite));
     AddEvent("Reseat",               JXT_HANDLER(SalonFormInterface, Reseat));
     AddEvent("DropSeats",            JXT_HANDLER(SalonFormInterface, DropSeats));
     AddEvent("DeleteProtCkinSeat",   JXT_HANDLER(SalonFormInterface, DeleteProtCkinSeat));
     AddEvent("WaitList",             JXT_HANDLER(SalonFormInterface, WaitList));
     AddEvent("AutoSeats",            JXT_HANDLER(SalonFormInterface, AutoSeats));
     AddEvent("Tranzit",              JXT_HANDLER(SalonFormInterface, Tranzit));
     AddEvent("RefreshPaxSalons",     JXT_HANDLER(SalonFormInterface, RefreshPaxSalons));
  }
  void Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RefreshPaxSalons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ComponShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ComponWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WaitList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Tranzit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  // iatci
  static void ShowRemote(xmlNodePtr resNode, const iatci::dcrcka::Result& res);
  static void ReseatRemote(xmlNodePtr resNode,
                           const iatci::Seat& oldSeat,
                           const iatci::Seat& newSeat,
                           const iatci::dcrcka::Result& res);

  static SalonFormInterface* instance();
};

struct TZoneOccupiedSeats {
  std::string name;
  int total_seats;
  SALONS2::TPlaces seats;
  TZoneOccupiedSeats(): total_seats(0) {}
};

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
BitSet<SEATS2::TChangeLayerSeatsProps>
     IntChangeSeatsN( int point_id, int pax_id, int &tid, std::string xname, std::string yname,
                      SEATS2::TSeatsType seat_type,
                      ASTRA::TCompLayerType layer_type,
                      int time_limit,
                      const BitSet<SEATS2::TChangeLayerFlags> &flags,
                      int comp_crc, int tariff_pax_id,
                      xmlNodePtr resNode,
                      const std::string& whence );
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
      if ( i->pers_type == "��" )
        AdultItems.push_back( *i );
  }
  //�ਢ易�� ��⥩ � �����
  SetInfantsToAdults<T1,T1>( InfItems, AdultItems );
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
        if ( p->getCurrLayer( point_id ).getPaxId() == pax_id ) {
          i->zone = z->name;
          break;
        }
      }
      if ( !i->zone.empty() )
        break;
    }
  }
}

#endif /*_SALONFORM_H_*/
