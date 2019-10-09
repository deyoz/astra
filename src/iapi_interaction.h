#pragma once
#include <string>

#include "apis_utils.h"
#include "apis_creator.h"
#include "apis_edi_file.h"
#include "tlg/paxlst_request.h"

namespace IAPI
{

bool needCheckStatus(const TCompleteAPICheckInfo& checkInfo);

class RequestCollector
{
  class PaxIdsKey
  {
    public:
      int point_dep;
      std::string airp_arv;

      PaxIdsKey(const int pointDep, const std::string& airpArv) :
        point_dep(pointDep),
        airp_arv(airpArv) {}

      bool operator < (const PaxIdsKey &key) const
      {
        if (point_dep!=key.point_dep)
          return point_dep<key.point_dep;
        return airp_arv<key.airp_arv;
      }
  };

  class PaxIds : public std::set<int> {};

  std::map<PaxIdsKey, PaxIds> groupedPassengers;
  TApisDataset apisDataset;
  std::list<edifact::PaxlstReqParams> requestParamsList;

protected:
  void collectApisDataset();
  void collectRequestParamsList();
  static std::string getRequestId();
public:
  void addPassengerIfNeed(const int pax_id,
                          const int point_dep,
                          const std::string& airp_arv,
                          const TCompleteAPICheckInfo& checkInfo);
  void send();
};

class PassengerStatus
{
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

    PassengerStatus& fromDB(TQuery &Qry);
    const PassengerStatus& toDB(TQuery &Qry) const;

    static bool allowedToBoarding(int paxId);
    const PassengerStatus& updateByRequest(const std::string& msgIdForClearPassengerRequest,
                                           const std::string& msgIdForChangePassengerData,
                                           bool& notRequestedBefore) const;
    const PassengerStatus& updateByResponse(const std::string& msgId) const;
    const PassengerStatus& updateByCusRequest(bool& notRequestedBefore) const;
    void toLog(bool isRequest) const;

    static Level getStatusLevel(const edifact::Cusres::SegGr4& gr4);
    static int getPaxId(const edifact::Cusres::SegGr4& gr4);
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

void init_callbacks();

} //namespace IAPI

