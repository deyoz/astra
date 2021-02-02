#pragma once
#include <string>

#include "apis_utils.h"
#include "apis_creator.h"
#include "apis_edi_file.h"
#include "tlg/paxlst_request.h"
#include "astra_types.h"

namespace IAPI
{

#ifdef XP_TESTING
void initRequestIdGenerator(int id);
std::string getLastRequestId();
#endif/*XP_TESTING*/

bool needCheckStatus(const TCompleteAPICheckInfo& checkInfo);

class RequestCollector
{
  class PaxIds : public std::set<PaxId_t> {};

  std::map<TPaxSegmentPair, PaxIds> groupedPassengers;
  TApisDataset apisDataset;
  std::list<edifact::PaxlstReqParams> requestParamsList;

protected:
  void collectApisDataset();
  void collectRequestParamsList();
  static std::string getRequestId();
public:
  void clear();
  void addPassengerIfNeed(const PaxId_t& paxId,
                          const TPaxSegmentPair& paxSegment,
                          const TCompleteAPICheckInfo& checkInfo);
  void send();
  static bool resendNeeded(const CheckIn::PaxRems& rems);
};

void syncAlarms(const int point_id_spp);
void deleteAlarms(const int pax_id, const int point_id_spp);

class PassengerStatus
{
  friend class PassengerStatusInspector;

  public:
    enum Enum {OnBoard, NoBoard, Advisory, InsufficientData, Accepted, Exception, Unknown};
    enum Level {HeaderLevel, DetailLevel, UnknownLevel};

  protected:
    typedef std::list< std::pair<Enum, std::string> > TPairs;

    static const TPairs& pairs()
    {
      static TPairs l=
       {
        {OnBoard,                "0Z"},
        {NoBoard,                "1Z"},
        {Advisory,               "2Z"},
        {InsufficientData,       "4Z"},
        {Accepted,               "0" },
        {Exception,              "1" },
        {Unknown,                ""  },
       };
      return l;
    }

    class StatusTypes : public ASTRA::PairList<Enum, std::string>
    {
      private:
        virtual std::string className() const { return "StatusTypes"; }
      public:
        StatusTypes() : ASTRA::PairList<Enum, std::string>(pairs(),
                                                           Unknown,
                                                           boost::none) {}
    };

    static const StatusTypes& statusTypes() { return ASTRA::singletone<StatusTypes>(); }
    static bool allowedToBoarding(Enum status) { return status==OnBoard; }
    static bool allowedToBoarding(const int paxId);

    int m_paxId;
    std::string m_countryControl;
    Enum m_status;
    std::string m_freeText;
    int m_pointId;

  public:
    PassengerStatus(const int paxId,
                    const APIS::SettingsKey& settingsKey,
                    int pointId) :
      m_paxId(paxId),
      m_countryControl(settingsKey.countryControl()),
      m_status(Unknown),
      m_freeText(""),
      m_pointId(pointId) {}

    PassengerStatus(const edifact::Cusres::SegGr4& gr4,
                    const APIS::SettingsKey& settingsKey);

    void clear()
    {
      m_paxId=ASTRA::NoExists;
      m_countryControl.clear();
      m_status=Unknown;
      m_freeText.clear();
      m_pointId=ASTRA::NoExists;
    }

    bool operator < (const PassengerStatus &item) const
    {
      return m_paxId<item.m_paxId;
    }

    void setPaxId(const int paxId) { m_paxId=paxId; }
    const int& paxId() const { return m_paxId; }

    const std::string& countryControl() const { return m_countryControl; }

    static bool allowedToBoarding(const int paxId, const TCompleteAPICheckInfo& checkInfo);
    const PassengerStatus& updateByRequest(const std::string& msgIdForClearPassengerRequest,
                                           const std::string& msgIdForChangePassengerData,
                                           bool& notRequestedBefore) const;
    const PassengerStatus& updateByResponse(const std::string& msgId) const;
    const PassengerStatus& updateByCusRequest(bool& notRequestedBefore) const;
    void writeToLogAndCheckAlarm(bool isRequest) const;

    static Level getStatusLevel(const edifact::Cusres::SegGr4& gr4);
    static int getPaxId(const edifact::Cusres::SegGr4& gr4);
};

class PassengerStatusInspector : public APICheckInfoForPassenger
{
  public:
    bool allowedToPrintBP(const int pax_id, const int grp_id=ASTRA::NoExists);
};

class PassengerStatusList : public std::set<PassengerStatus>
{
  public:
    void getByResponseHeaderLevel(const std::string& msgId,
                                  const PassengerStatus& statusPattern);
    void updateByResponse(const std::string& msgId) const;
    void updateByCusRequest() const;

    static std::string getMsgId(const edifact::Cusres& cusres);
    static void processCusres(const edifact::Cusres& cusres, bool isRequest);
};

class CusresCallbacks: public edifact::CusresCallbacks
{
  public:
    virtual void onCusResponseHandle(TRACE_SIGNATURE, const edifact::Cusres& cusres);
    virtual void onCusRequestHandle(TRACE_SIGNATURE, const edifact::Cusres& cusres);
};

std::vector<std::string> statusesFromDb(const PaxId_t& pax_id);

void init_callbacks();

} //namespace IAPI

