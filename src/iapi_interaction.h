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

void syncAlarms(const PointId_t& pointId);
void deleteAlarms(const PaxId_t& paxId, const PointId_t& pointId);

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
    static bool allowedToBoarding(const PaxId_t& paxId);

    PaxId_t m_paxId;
    CountryCode_t m_countryControl;
    Enum m_status;
    std::string m_freeText;
    std::optional<PointId_t> m_pointId;

  public:
    PassengerStatus(const PaxId_t& paxId,
                    const APIS::SettingsKey& settingsKey,
                    const PointId_t& pointId) :
      m_paxId(paxId),
      m_countryControl(settingsKey.countryControl()),
      m_status(Unknown),
      m_freeText(""),
      m_pointId(pointId) {}

    PassengerStatus(const PaxId_t& paxId,
                    const APIS::SettingsKey& settingsKey,
                    const edifact::Cusres::SegGr4& gr4);

    bool operator < (const PassengerStatus &item) const
    {
      return m_paxId<item.m_paxId;
    }

    void setPaxId(const PaxId_t& paxId) { m_paxId=paxId; }
    const PaxId_t& paxId() const { return m_paxId; }

    const CountryCode_t& countryControl() const { return m_countryControl; }

    static bool allowedToBoarding(const PaxId_t& paxId, const TCompleteAPICheckInfo& checkInfo);
    const PassengerStatus& updateByRequest(const std::string& msgIdForClearPassengerRequest,
                                           const std::string& msgIdForChangePassengerData,
                                           bool& notRequestedBefore) const;
    const PassengerStatus& updateByResponse(const std::string& msgId) const;
    const PassengerStatus& updateByCusRequest(bool& notRequestedBefore) const;
    void writeToLogAndCheckAlarm(bool isRequest) const;

    static Level getStatusLevel(const edifact::Cusres::SegGr4& gr4);
    static std::optional<PaxId_t> getPaxId(const edifact::Cusres::SegGr4& gr4);
};

class PassengerStatusInspector : public APICheckInfoForPassenger
{
  public:
    bool allowedToPrintBP(const PaxId_t& paxId, const boost::optional<GrpId_t>& grpId=boost::none);
};

class PassengerStatusList : public std::set<PassengerStatus>
{
  public:
    void getByResponseHeaderLevel(const std::string& msgId,
                                  const APIS::SettingsKey& settingsKey,
                                  const edifact::Cusres::SegGr4& gr4);
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

std::vector<std::string> statusesFromDb(const PaxId_t& paxId);

void init_callbacks();

} //namespace IAPI

