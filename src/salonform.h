#ifndef _SALONSFORM_H_
#define _SALONFORM_H_

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"
#include "seats.h"
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

bool filterCompons( const std::string &airline, const std::string &airp );
void SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void ZoneLoads(int point_id, std::map<std::string, int> &zones);
void IntChangeSeats( int point_id, int pax_id, int &tid, std::string xname, std::string yname,
	                   SEATS2::TSeatsType seat_type,
	                   ASTRA::TCompLayerType layer_type,
                     bool pr_waitlist, bool pr_question_reseat,
                     xmlNodePtr resNode );


#endif /*_SALONFORM_H_*/

