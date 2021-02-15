#include "iapi_interaction.h"
#include "apis_creator.h"
#include "alarms.h"
#include "base_callbacks.h"
#include "PgOraConfig.h"
#include "tlg/remote_system_context.h"

#include <serverlib/testmode.h>
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

namespace IAPI
{

const int MaxPassengerCountInRequest=10;

bool needCheckStatus(const TCompleteAPICheckInfo& checkInfo)
{
  return find_first_of(checkInfo.apis_formats().begin(),
                       checkInfo.apis_formats().end(),
                       getIAPIFormats().begin(),
                       getIAPIFormats().end())!=checkInfo.apis_formats().end();
}

void RequestCollector::clear()
{
  groupedPassengers.clear();
  apisDataset.clear();
  requestParamsList.clear();
}

#ifdef XP_TESTING
static int requestIdGenerator=0;
void initRequestIdGenerator(int id) { requestIdGenerator=id; }

static std::string lastRequestId;

std::string getLastRequestId()               { return lastRequestId; }
void setLastRequestId(const std::string& id) { lastRequestId = id;   }
#endif/*XP_TESTING*/

std::string RequestCollector::getRequestId()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apis_id__seq.nextval AS id FROM dual";
  Qry.Execute();
  std::ostringstream s;
  s << std::setw(7) << std::setfill('0');
#ifdef XP_TESTING
  if (inTestMode())
  {
    s << (requestIdGenerator++);
    setLastRequestId(s.str());
  }
  else
#endif/*XP_TESTING*/
  {
    s << Qry.FieldAsInteger("id");
  }
  return s.str();
}

bool RequestCollector::resendNeeded(const CheckIn::PaxRems& rems)
{
  for(const CheckIn::TPaxRemItem& rem : rems)
    if (rem.code=="RSIA") return true;
  return false;
}

void RequestCollector::addPassengerIfNeed(const PaxId_t& paxId,
                                          const TPaxSegmentPair& paxSegment,
                                          const TCompleteAPICheckInfo& checkInfo)
{
  if (checkInfo.apis_formats().empty()) return;

  auto i=groupedPassengers.find(paxSegment);
  if (i==groupedPassengers.end())
  {
    if (!needCheckStatus(checkInfo)) return;
    i=groupedPassengers.emplace(paxSegment, PaxIds()).first;
  }
  i->second.insert(paxId);
}

void RequestCollector::collectApisDataset()
{
  apisDataset.lstRouteData.clear();

  for(const auto& i : groupedPassengers)
  {
    const TPaxSegmentPair& paxSegment=i.first;
    const PaxIds& paxIds=i.second;

    TApisRouteData rd;
    if (!rd.depInfo.getByPointId(paxSegment.point_dep,
                                 FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn)))
    {
      //что-то написать
      continue;
    };

    rd.paxLegs.getRouteBetween(paxSegment);
    if(rd.paxLegs.size()<2)
    {
      ProgTrace(TRACE5,
                "%s: route.size()=%zu (point_id=%d, airp_arv=%s)", __func__,
                rd.paxLegs.size(), paxSegment.point_dep, paxSegment.airp_arv.c_str());
      continue;
    }

    rd.lstSetsData.getByCountries(rd.depInfo.airline, rd.country_dep(), rd.country_arv());
    rd.lstSetsData.filterFormatsFromList(getIAPIFormats());
    if (rd.lstSetsData.empty()) continue;

    CheckIn::TSimplePaxGrpItem grp;
    for(const PaxId_t& paxId : paxIds)
    {
      TApisPaxData pax;

      if (!pax.getByPaxId(paxId.get())) continue;
      if (!pax.refuse.empty()) continue;

      if (grp.id!=pax.grp_id)
      {
        if (!grp.getByGrpId(pax.grp_id)) continue;
      }

      if (grp.status==psCrew) continue;

      pax.status=grp.status;
      pax.airp_arv=rd.arvInfo().airp;
      pax.airp_arv_final=pax.airp_arv;
      pax.processingIndicator=TReqInfo::Instance()->client_type==ctTerm?"173":"174";

      CheckIn::LoadPaxDoc(paxId.get(), pax.doc);
      pax.doco = CheckIn::TPaxDocoItem::get(paxCheckIn, paxId);
      rd.lstPaxData.push_back(pax);
      if (rd.lstPaxData.size()==MaxPassengerCountInRequest)
      {
        apisDataset.lstRouteData.push_back(rd);
        rd.lstPaxData.clear();
      }
    }

    if (!rd.lstPaxData.empty())
      apisDataset.lstRouteData.push_back(rd);
  }
}

Paxlst::PaxlstInfo& getPaxlstInfo(const TApisRouteData& route,
                                  const TAPISFormat& format,
                                  const TApisPaxData& pax,
                                  Paxlst::PaxlstInfo& paxlst1,
                                  Paxlst::PaxlstInfo& paxlst2)
{
  bool notRequestedBefore;
  PassengerStatus(pax.paxId(), format.settings, route.depInfo.point_id).
      updateByRequest(paxlst1.settings().RFF_TN(),
                      paxlst2.settings().RFF_TN(),
                      notRequestedBefore);

  return (notRequestedBefore?paxlst1:paxlst2);
}

void RequestCollector::collectRequestParamsList()
{
  using namespace Paxlst;

  for(const TApisRouteData& route : apisDataset.lstRouteData)
    for(const auto& i : route.lstSetsData)
    {
      const APIS::Settings& settings = i.second;
      std::unique_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(settings));

      PaxlstInfo paxlst1(PaxlstInfo::IAPIClearPassengerRequest, "");
      PaxlstInfo paxlst2(PaxlstInfo::IAPIChangePassengerData, "");
      paxlst1.settings().set_RFF_TN(getRequestId());
      paxlst2.settings().set_RFF_TN(getRequestId());

      CreateEdi(route, *pFormat, paxlst1, paxlst2, getPaxlstInfo);

      if (!paxlst1.passengersList().empty())
        requestParamsList.emplace_back(settings, paxlst1);
      if (!paxlst2.passengersList().empty())
        requestParamsList.emplace_back(settings, paxlst2);

    }
}

void RequestCollector::send()
{
  collectApisDataset();
  collectRequestParamsList();
  for(const edifact::PaxlstReqParams& params : requestParamsList)
  {
//    LogTrace(TRACE5) << params.paxlst().toEdiString();
    edifact::PaxlstRequest(params).sendTlg();
  }
}

void syncAlarms(const int point_id_spp)
{
  TTripAlarmHook::set(Alarm::IAPIProblem, point_id_spp);
}

static void addAlarm( const int pax_id,
                      const std::initializer_list<Alarm::Enum>& alarms,
                      const int point_id_spp )
{
  if (!addAlarmByPaxId(pax_id, alarms, {paxCheckIn})) return; //ничего не изменилось
  syncAlarms(point_id_spp);
}

static void deleteAlarm( const int pax_id,
                         const std::initializer_list<Alarm::Enum>& alarms,
                         const int point_id_spp )
{
  if (!deleteAlarmByPaxId(pax_id, alarms, {paxCheckIn})) return; //ничего не изменилось
  syncAlarms(point_id_spp);
}

void deleteAlarms(const int pax_id, const int point_id_spp)
{
  if (!deleteAlarmByPaxId(pax_id,
                          {Alarm::IAPINegativeDirective},
                          {paxCheckIn})) return; //ничего не изменилось

  if (point_id_spp!=ASTRA::NoExists)
    syncAlarms(point_id_spp);
  else
    LogError(STDLOG) << __func__ << ": point_id_spp==ASTRA::NoExists";
}

static bool updateIapiPaxData(const PointId_t& pointId,
                              const PaxId_t& paxId,
                              const CountryCode_t& countryControl,
                              const std::string& freeText,
                              const std::string& status,
                              const std::string& msgId)
{
    auto cur = make_db_curs(
"UPDATE iapi_pax_data "
"SET status=:status, free_text=:free_text, msg_id=:msg_id, point_id=:point_id, pr_del=0 "
"WHERE pax_id=:pax_id AND country_control=:country_control",
                PgOra::getRWSession("IAPI_PAX_DATA"));

    cur
            .bind(":point_id",        pointId.get())
            .bind(":pax_id",          paxId.get())
            .bind(":country_control", countryControl.get())
            .bind(":free_text",       freeText)
            .bind(":status",          status)
            .bind(":msg_id",          msgId)
            .exec();

    return cur.rowcount() > 0;
}

static bool insertIapiPaxData(const PointId_t& pointId,
                              const PaxId_t& paxId,
                              const CountryCode_t& countryControl,
                              const std::string& freeText,
                              const std::string& status,
                              const std::string& msgId)
{
    auto cur = make_db_curs(
"INSERT INTO iapi_pax_data(pax_id, country_control, status, free_text, msg_id, point_id, pr_del) "
"VALUES(:pax_id, :country_control, :status, :free_text, :msg_id, :point_id, 0)",
                PgOra::getRWSession("IAPI_PAX_DATA"));

    cur
            .bind(":point_id",        pointId.get())
            .bind(":pax_id",          paxId.get())
            .bind(":country_control", countryControl.get())
            .bind(":free_text",       freeText)
            .bind(":status",          status)
            .bind(":msg_id",          msgId)
            .exec();

    return cur.rowcount() > 0;
}

bool PassengerStatus::allowedToBoarding(const int paxId)
{
    auto cur = make_db_curs(
"SELECT status FROM iapi_pax_data WHERE pax_id=:pax_id AND pr_del=0",
                PgOra::getROSession("IAPI_PAX_DATA"));

    std::string status;
    cur
            .defNull(status, "")
            .bind(":pax_id", paxId)
            .exec();
    while(!cur.fen()) {
        if(!allowedToBoarding(statusTypes().decode(status))) {
            return false;
        }
    }

    return true;
}

bool PassengerStatus::allowedToBoarding(const int paxId, const TCompleteAPICheckInfo& checkInfo)
{
  if (needCheckStatus(checkInfo))
    return allowedToBoarding(paxId);
  return true;
}

bool PassengerStatusInspector::allowedToPrintBP(const int pax_id, const int grp_id)
{
  if (PassengerStatus::allowedToBoarding(pax_id)) return true; //специально вначале - оптимизируем обращения к БД (подавляющее большинство разрешено к посадке)

  PaxId_t paxId(pax_id);
  boost::optional<GrpId_t> grpId;
  if (grp_id!=ASTRA::NoExists) grpId=boost::in_place(grp_id);

  return (!needCheckStatus(get(paxId, grpId)));
}

const PassengerStatus& PassengerStatus::updateByRequest(const std::string& msgIdForClearPassengerRequest,
                                                        const std::string& msgIdForChangePassengerData,
                                                        bool& notRequestedBefore) const
{
    auto encStatus = statusTypes().encode(m_status);

    LogTrace(TRACE5) << __func__ << " pax_id=" << m_paxId
                                 << " point_Id=" << m_pointId
                                 << " country_control=" << m_countryControl
                                 << " msgIdForClearPassengerRequest=" << msgIdForClearPassengerRequest
                                 << " msgIdForChangePassengerData=" << msgIdForChangePassengerData
                                 << " status=" << m_status << "(" << encStatus << ")";

    notRequestedBefore = !updateIapiPaxData(PointId_t(m_pointId),
                                            PaxId_t(m_paxId),
                                            CountryCode_t(m_countryControl),
                                            m_freeText,
                                            encStatus,
                                            msgIdForChangePassengerData);

    if(notRequestedBefore) {
        insertIapiPaxData(PointId_t(m_pointId),
                          PaxId_t(m_paxId),
                          CountryCode_t(m_countryControl),
                          m_freeText,
                          encStatus,
                          msgIdForClearPassengerRequest);
    }

    return *this;
}

const PassengerStatus& PassengerStatus::updateByResponse(const std::string& msgId) const
{
    if (msgId.empty()) return *this;

    auto cur = make_db_curs(
"UPDATE iapi_pax_data SET status=:status, free_text=:free_text "
"WHERE pax_id=:pax_id AND country_control=:country_control AND msg_id=:msg_id",
        PgOra::getRWSession("IAPI_PAX_DATA"));

    auto encStatus = statusTypes().encode(m_status);
    cur
            .bind(":pax_id",          m_paxId)
            .bind(":country_control", m_countryControl)
            .bind(":status",          encStatus)
            .bind(":free_text",       m_freeText)
            .bind(":msg_id",          msgId)
            .exec();

    if(cur.rowcount() > 0) {
        writeToLogAndCheckAlarm(false);
    } else {
        LogTrace(TRACE5) << __func__
                         << ": passenger not found (paxId=" << m_paxId
                         << ", countryControl=" << m_countryControl
                         << ", msg_id=" << msgId << ")";
    }

    return *this;
}

const PassengerStatus& PassengerStatus::updateByCusRequest(bool& notRequestedBefore) const
{
    auto cur = make_db_curs(
"UPDATE iapi_pax_data SET status=:status, free_text=:free_text "
"WHERE pax_id=:pax_id AND country_control=:country_control",
        PgOra::getRWSession("IAPI_PAX_DATA"));

    auto encStatus = statusTypes().encode(m_status);
    cur
            .bind(":pax_id",          m_paxId)
            .bind(":country_control", m_countryControl)
            .bind(":status",          encStatus)
            .bind(":free_text",       m_freeText)
            .exec();

    if(cur.rowcount() > 0) {
        writeToLogAndCheckAlarm(true);
    } else {
        LogTrace(TRACE5) << __func__
                         << ": passenger not found (paxId=" << m_paxId
                         << ", countryControl=" << m_countryControl << ")";
    }

    notRequestedBefore = cur.rowcount() > 0;

    return *this;
}

void PassengerStatus::writeToLogAndCheckAlarm(bool isRequest) const
{
  if (m_paxId==ASTRA::NoExists) return;

  CheckIn::TSimplePaxItem pax;
  if (!pax.getByPaxId(m_paxId)) return; //теоретически может быть удален по ошибке агента
  CheckIn::TSimplePaxGrpItem grp;
  if (!grp.getByGrpId(pax.grp_id)) return;

  TLogLocale msg;
  msg.ev_type=ASTRA::evtPax;
  if (isRequest)
    msg.lexema_id = allowedToBoarding(m_status)?"EVT.IAPI_REQUEST.BOARDING_PERMITTED":
                                                "EVT.IAPI_REQUEST.BOARDING_NOT_PERMITTED";
  else
    msg.lexema_id = allowedToBoarding(m_status)?"EVT.IAPI_RESPONSE.BOARDING_PERMITTED":
                                                "EVT.IAPI_RESPONSE.BOARDING_NOT_PERMITTED";

  msg.id1=grp.point_dep;
  msg.id2=pax.reg_no;
  msg.id3=pax.grp_id;

  string statusStr=statusTypes().encode(m_status);
  if (!statusStr.empty() && !m_freeText.empty()) statusStr+=" - ";
  statusStr+=m_freeText;

  msg.prms.clearPrms();
  msg.prms << PrmSmpl<string>("pax_name", pax.full_name())
           << PrmElem<string>("pers_type", etPersType, EncodePerson(pax.pers_type))
           << PrmElem<string>("country", etCountry, m_countryControl)
           << PrmSmpl<string>("status",  statusStr);

  TReqInfo::Instance()->LocaleToLog(msg);

  //тревоги
  if (allowedToBoarding(m_paxId))
    deleteAlarm(m_paxId, {Alarm::IAPINegativeDirective}, grp.point_dep);
  else
    addAlarm(m_paxId, {Alarm::IAPINegativeDirective}, grp.point_dep);
}

PassengerStatus::Level PassengerStatus::getStatusLevel(const edifact::Cusres::SegGr4& gr4)
{
  if (gr4.m_erp.m_msgSectionCode=="1")
    return HeaderLevel;
  else if (gr4.m_erp.m_msgSectionCode=="2")
    return DetailLevel;
  else
    return UnknownLevel;
}

PassengerStatus::PassengerStatus(const edifact::Cusres::SegGr4& gr4,
                                 const APIS::SettingsKey& settingsKey)
{
  clear();

  m_paxId=getPaxId(gr4);
  m_countryControl=settingsKey.countryControl();
  m_status=statusTypes().decode(gr4.m_erc.m_errorCode);
  if (gr4.m_ftx)
    m_freeText=gr4.m_ftx.get().m_freeText;
}

int PassengerStatus::getPaxId(const edifact::Cusres::SegGr4& gr4)
{
  if (gr4.m_vRff.empty()) return ASTRA::NoExists;
  int paxId=NoExists;
  auto beg = gr4.m_vRff.cbegin();
  auto end = gr4.m_vRff.cend();
  auto abo = find_if(beg, end, [](const auto &x){return x.m_qualifier == "ABO";});
  if (abo != end)
    if (StrToInt(abo->m_ref.c_str(), paxId)==EOF)
    {
      LogTrace(TRACE5) << __func__ << ": wrong ABO=" << abo->m_ref;
      paxId=NoExists;
    }

  return paxId;
}

std::string PassengerStatusList::getMsgId(const edifact::Cusres& cusres)
{
  if (cusres.m_rff && cusres.m_rff.get().m_qualifier=="TN")
    return cusres.m_rff.get().m_ref;
  return "";
}

void PassengerStatusList::processCusres(const edifact::Cusres& cusres, bool isRequest)
{
  using namespace Ticketing::RemoteSystemContext;
  const SystemContext& ctxt=SystemContext::Instance(STDLOG);

  APIS::SettingsList settingsList;
  settingsList.getByAddrs(ctxt.remoteAddrEdifact(),
                          ctxt.remoteAddrEdifactExt(),
                          ctxt.ourAddrEdifact(),
                          ctxt.ourAddrEdifactExt());
  if (settingsList.empty())
  {
    LogTrace(TRACE5) << "settingsList.empty()!";
    return;
  }

  PassengerStatusList statuses;

  string msgId=getMsgId(cusres);

  for (const edifact::Cusres::SegGr4& gr4 : cusres.m_vSegGr4)
  {
    PassengerStatus::Level level=PassengerStatus::getStatusLevel(gr4);

    if (level==PassengerStatus::UnknownLevel) continue;

    PassengerStatus status(gr4, settingsList.begin()->first);

    if (level==PassengerStatus::HeaderLevel)
    {
      statuses.getByResponseHeaderLevel(msgId, status);
    }
    else
    {
      if (status.paxId()!=ASTRA::NoExists)
        statuses.insert(status);
    }
  }

  if (isRequest)
    statuses.updateByCusRequest();
  else
    statuses.updateByResponse(msgId);
}

void PassengerStatusList::getByResponseHeaderLevel(const std::string& msgId,
                                                   const PassengerStatus& statusPattern)
{
    if (msgId.empty()) return;

    auto cur = make_db_curs(
"SELECT pax_id FROM iapi_pax_data "
"WHERE country_control=:country_control AND msg_id=:msg_id",
              PgOra::getROSession("IAPI_PAX_DATA"));

    int paxId = ASTRA::NoExists;

    cur
            .def(paxId)
            .bind(":country_control", statusPattern.countryControl())
            .bind(":msg_id",          msgId)
            .exec();
    while(!cur.fen()) {
        PassengerStatus status(statusPattern);
        status.setPaxId(paxId);
        insert(status);
    }
}

void PassengerStatusList::updateByResponse(const std::string& msgId) const
{
  for(const PassengerStatus& i : *this)
    i.updateByResponse(msgId);
}

void PassengerStatusList::updateByCusRequest() const
{
  for(const PassengerStatus& i : *this)
  {
    bool notRequestedBefore;
    i.updateByCusRequest(notRequestedBefore);
  }
}

void CusresCallbacks::onCusResponseHandle(TRACE_SIGNATURE, const edifact::Cusres& cusres)
{
  LogTrace(TRACE_PARAMS) << __func__ << " started";
  PassengerStatusList::processCusres(cusres, false);
}

void CusresCallbacks::onCusRequestHandle(TRACE_SIGNATURE, const edifact::Cusres& cusres)
{
  LogTrace(TRACE_PARAMS) << __func__ << " started";
  PassengerStatusList::processCusres(cusres, true);
}

class PaxEvents: public PaxEventCallbacks<TRemCategory>,
                 public PaxEventCallbacks<PaxChanges>
{
  private:
    CompleteAPICheckInfoCache checkInfoCache;
    IAPI::RequestCollector iapiCollector;
    map<TTripTaskKey, TDateTime> flightTasks;
  public:
    void clear()
    {
      checkInfoCache.clear();
      iapiCollector.clear();
      flightTasks.clear();
    }

    void apply()
    {
      iapiCollector.send();
      for(const auto& i : flightTasks)
        add_trip_task(i.first, i.second);
    }

    void processIfNeed(const PaxOrigin& paxOrigin,
                       const PaxIdWithSegmentPair& paxId,
                       const bool refused)
    {
      if (!paxId.getSegmentPair()) return;
      if (paxOrigin==paxPnl) return;

      const TPaxSegmentPair& paxSegment=paxId.getSegmentPair().get();
      const TCompleteAPICheckInfo& checkInfo=checkInfoCache.get(paxSegment);

      switch(paxOrigin)
      {
        case paxPnl:
          if (IAPI::needCheckStatus(checkInfo))
          {
            addAlarmByPaxId(paxId().get(), {Alarm::SyncIAPI}, {paxOrigin});
            flightTasks.emplace(TTripTaskKey(paxSegment.point_dep, AlarmTypes().encode(Alarm::SyncIAPI), ""), ASTRA::NoExists);
          }
          break;
        case paxCheckIn:
          if (refused)
            IAPI::deleteAlarms(paxId().get(), paxSegment.point_dep);
          else
            iapiCollector.addPassengerIfNeed(paxId(), paxSegment, checkInfo);
          break;
        default: break;
      }
    }

    //ByPaxId
    void initialize(TRACE_SIGNATURE,
                    const PaxOrigin& paxOrigin)
    {
      clear();
    }

    void finalize(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin)
    {
      apply();
      clear();
    }

    void onChange(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin,
                  const PaxIdWithSegmentPair& paxId,
                  const std::set<TRemCategory>& categories);

    void onChange(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin,
                  const PaxIdWithSegmentPair& paxId,
                  const std::set<PaxChanges>& categories);
};

void PaxEvents::onChange(TRACE_SIGNATURE,
                         const PaxOrigin& paxOrigin,
                         const PaxIdWithSegmentPair& paxId,
                         const std::set<TRemCategory>& categories)
{
  if (!paxId.getSegmentPair()) return;

  if (algo::none_of(categories, [](const TRemCategory& cat)
                                { return cat==remTKN ||
                                         cat==remDOC ||
                                         cat==remDOCO ||
                                         cat==remAPPSOverride; }
                    )) return;

  processIfNeed(paxOrigin, paxId, false);
}

void PaxEvents::onChange(TRACE_SIGNATURE,
                         const PaxOrigin& paxOrigin,
                         const PaxIdWithSegmentPair& paxId,
                         const std::set<PaxChanges>& categories)
{
  if (!paxId.getSegmentPair()) return;

  if (categories.count(PaxChanges::Cancel)>0)
  {
    processIfNeed(paxOrigin, paxId, true);
  }

  if (categories.count(PaxChanges::New)>0)
  {
    processIfNeed(paxOrigin, paxId, false);
  }
}

std::vector<std::string> statusesFromDb(const PaxId_t& pax_id)
{
    std::vector<std::string> res;
    auto cur = make_db_curs(
"SELECT status FROM iapi_pax_data WHERE pax_id = :pax_id",
                PgOra::getROSession("IAPI_PAX_DATA"));

    std::string status;
    cur
            .defNull(status, "")
            .bind(":pax_id", pax_id.get())
            .exec();

    while(!cur.fen()) {
        res.emplace_back(status);
    }
    return res;
}

void init_callbacks()
{
  static bool init=false;
  if (init) return;

  CallbacksSingleton<edifact::CusresCallbacks>::Instance()->setCallbacks(new CusresCallbacks);
  PaxEvents* paxEvents=new PaxEvents;
  CallbacksSuite< PaxEventCallbacks<TRemCategory> >::Instance()->addCallbacks(paxEvents);
  CallbacksSuite< PaxEventCallbacks<PaxChanges> >::Instance()->addCallbacks(paxEvents);
  init=true;
}

} //namespace IAPI

