#ifndef ZAMAR_DSM_H
#define ZAMAR_DSM_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "astra_consts.h"
#include "passenger.h"
//#include "print.h"
#include "astra_misc.h"
#include "baggage_base.h"
#include "baggage_tags.h"

using BASIC::date_time::TDateTime;

enum class ZamarType { PaxCtl, SBDO, CUWS };
enum class ZamarActionType { CONFIRM, REVOKE };

const char *EncodeZamarType(ZamarType type);
ZamarType DecodeZamarType(const char *type);

struct ZamarDataInterface
{
  virtual void fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType type = ZamarType::SBDO) = 0;
  virtual void toXML(xmlNodePtr resNode, ZamarType type = ZamarType::SBDO) const = 0;
  virtual ~ZamarDataInterface() {}
};

//-----------------------------------------------------------------------------------
// PaxCtl

class ZamarPaxCtlInterface: public JxtInterface
{
    public:
        ZamarPaxCtlInterface(): JxtInterface("456", "ZamarPaxCtl")
        {
            AddEvent("PassengerSearchPaxCtl",    JXT_HANDLER(ZamarPaxCtlInterface, PassengerSearchPaxCtl));
        }

        void PassengerSearchPaxCtl(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class PassengerSearchResult : public ZamarDataInterface
{
//  PrintInterface::BPPax bppax;
  int point_id = ASTRA::NoExists;
  int grp_id = ASTRA::NoExists;
  int pax_id = ASTRA::NoExists;
  TTripInfo trip_info;
  CheckIn::TPaxGrpItem grp_item; // для PaxCtl было TSimplePaxGrpItem
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
  std::list<std::pair<double, int>> bagTagsGenerated;

  void DateTimeToXML(xmlNodePtr resNode, const std::string &name, TDateTime value, const std::string &airp = {}) const;

public:
  void fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType type = ZamarType::SBDO) override final;
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

        static void PassengerBaggageTagAdd(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
        static void PassengerBaggageTagConfirm(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
        static void PassengerBaggageTagRevoke(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
};

struct ZamarBagTag
{
  bool generated_ = false;
  bool activated_ = false;
  bool deactivated_ = false;

  int pax_id_ = ASTRA::NoExists;
  CheckIn::TSimpleBagItem bag_;
  boost::optional<TBagTagNumber> tagNumber_;

  std::string tagNumber() const;
  void tagNumberToDB(TQuery &Qry) const;
  void paxIdToDB(TQuery &Qry) const;
  TBagConcept::Enum bagConcept() const;
  void Generate(int grp_id, ZamarType type);
  void SetListId();

  void fromXML_add(xmlNodePtr reqNode);
  void fromXML(xmlNodePtr reqNode, ZamarType type, ZamarActionType actionType);
  void toXML(xmlNodePtr resNode) const;

  void toDB_generated(ZamarType type) const;
  void toDB_activated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode) const;
  void toDB_deactivated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode) const;
  void fromDB(ZamarType type, ZamarActionType actionType);

  void Activate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode);
  void Deactivate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, bool force);
};

class ZamarBaggageTagAdd : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

class ZamarBaggageTagConfirm : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

class ZamarBaggageTagRevoke : public ZamarDataInterface
{
  ZamarBagTag tag_;
  std::string session_id_;
public:
  void fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType = ZamarType::SBDO) override final;
  void toXML(xmlNodePtr resNode, ZamarType = ZamarType::SBDO) const override final;
};

struct ZamarXML {
    static void ProcessXML(ZamarDataInterface& data, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode, ZamarType type);
};

#endif // ZAMAR_DSM_H
