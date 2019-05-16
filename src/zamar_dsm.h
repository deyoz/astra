#ifndef ZAMAR_DSM_H
#define ZAMAR_DSM_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "astra_consts.h"
#include "passenger.h"
#include "print.h"
#include "astra_misc.h"
#include "baggage_base.h"

using BASIC::date_time::TDateTime;

enum class ZamarType { DSM, SBDO };

struct ZamarDataInterface
{
  virtual void fromXML(xmlNodePtr reqNode, ZamarType type = ZamarType::SBDO) = 0;
  virtual void toXML(xmlNodePtr resNode, ZamarType type = ZamarType::SBDO) const = 0;
  virtual ~ZamarDataInterface() {}
};

//-----------------------------------------------------------------------------------
// DSM

class ZamarDSMInterface: public JxtInterface
{
    public:
        ZamarDSMInterface(): JxtInterface("456", "ZamarDSM")
        {
            AddEvent("PassengerSearch",    JXT_HANDLER(ZamarDSMInterface, PassengerSearch));
        }

        void PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class PassengerSearchResult : public ZamarDataInterface
{
  PrintInterface::BPPax bppax;
  int point_id = ASTRA::NoExists;
  int grp_id = ASTRA::NoExists;
  int pax_id = ASTRA::NoExists;
  TTripInfo trip_info;
  CheckIn::TPaxGrpItem grp_item; // для DSM было TSimplePaxGrpItem
  CheckIn::TSimplePaxItem pax_item;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  bool doc_exists;
  bool doco_exists;
  TMktFlight mkt_flt;
  
  // sessionId
  std::string sessionId;
  // flightStatus
  TStage flightCheckinStage = sNoActive;
  // pnr
  TPnrAddrs pnrs;
  // baggageTags
  std::multimap<TBagTagNumber, CheckIn::TBagItem> bagTagsExtended;
  
public:
  void fromXML(xmlNodePtr reqNode, ZamarType type = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType type = ZamarType::SBDO) const override final;
};

//-----------------------------------------------------------------------------------
// SBDO

class ZamarSBDOInterface: public JxtInterface
{
    public:
        ZamarSBDOInterface(): JxtInterface("457", "ZamarSBDO") // уточнить параметры конструктора
        {
            AddEvent("PassengerSearchSBDO",    JXT_HANDLER(ZamarSBDOInterface, PassengerSearchSBDO));
            AddEvent("PassengerBaggageTagAdd",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagAdd));
            AddEvent("PassengerBaggageTagConfirm",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagConfirm));
            AddEvent("PassengerBaggageTagRevoke",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagRevoke));
        }

        void PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

struct ZamarBagTag
{
  bool generated_ = false;
  bool activated_ = false;
  bool deactivated_ = false;
  
  TBagConcept::Enum bag_concept_ = TBagConcept::Unknown;
  double      no_dbl_ = 0.0;
  int pax_id_ = ASTRA::NoExists;
  int weight_ = ASTRA::NoExists;
  int list_id_ = ASTRA::NoExists;
  std::string bag_type_;
  std::string rfisc_;
  TServiceType::Enum service_type_ = TServiceType::Unknown;
  std::string airline_;
  
  std::string ServiceType() const { return ServiceTypes().encode(service_type_); }
  std::string NoToStr() const;
  void Generate(int grp_id);
  void GetListId(); // TODO const
  
  void fromXML_add(xmlNodePtr reqNode);
  void fromXML(xmlNodePtr reqNode);
  void toXML(xmlNodePtr resNode) const;
  
  void toDB_generated(); // TODO const
  void toDB_activated(); // TODO const
  void toDB_deactivated(); // TODO const
  void fromDB();
  
  void Activate();
  void Deactivate();
};

class ZamarBaggageTagAdd : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

class ZamarBaggageTagConfirm : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

class ZamarBaggageTagRevoke : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

#endif // ZAMAR_DSM_H
