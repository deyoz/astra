#include "iapi_interaction.h"
#include "apis_creator.h"
#include "alarms.h"
#include "tlg/remote_system_context.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

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

std::string RequestCollector::getRequestId()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apis_id__seq.nextval AS id FROM dual";
  Qry.Execute();
  std::ostringstream s;
  s << std::setw(7) << std::setfill('0') << Qry.FieldAsInteger("id");
  return s.str();
}

bool RequestCollector::resendNeeded(const CheckIn::PaxRems& rems)
{
  for(const CheckIn::TPaxRemItem& rem : rems)
    if (rem.code=="RSIA") return true;
  return false;
}

void RequestCollector::addPassengerIfNeed(const int pax_id,
                                          const int point_dep,
                                          const std::string& airp_arv,
                                          const TCompleteAPICheckInfo& checkInfo)
{
  if (checkInfo.apis_formats().empty()) return;

  PaxIdsKey key(point_dep, airp_arv);
  auto i=groupedPassengers.find(key);
  if (i==groupedPassengers.end())
  {
    if (!needCheckStatus(checkInfo)) return;
    i=groupedPassengers.emplace(key, PaxIds()).first;
  }
  i->second.insert(pax_id);
}

void RequestCollector::collectApisDataset()
{
  apisDataset.lstRouteData.clear();

  for(const auto& i : groupedPassengers)
  {
    const PaxIdsKey& paxIdsKey=i.first;
    const PaxIds& paxIds=i.second;

    TApisRouteData rd;
    if (!rd.depInfo.getByPointId(paxIdsKey.point_dep,
                                 FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn)))
    {
      //что-то написать
      continue;
    };

    rd.paxLegs.getRouteBetween(paxIdsKey.point_dep, paxIdsKey.airp_arv);
    if(rd.paxLegs.size()<2)
    {
      ProgTrace(TRACE5,
                "%s: route.size()=%zu (point_id=%d, airp_arv=%s)", __func__,
                rd.paxLegs.size(), paxIdsKey.point_dep, paxIdsKey.airp_arv.c_str());
      continue;
    }

    rd.lstSetsData.getByCountries(rd.depInfo.airline, rd.country_dep(), rd.country_arv());
    rd.lstSetsData.filterFormatsFromList(getIAPIFormats());
    if (rd.lstSetsData.empty()) continue;

    CheckIn::TSimplePaxGrpItem grp;
    for(const int& paxId : paxIds)
    {
      TApisPaxData pax;

      if (!pax.getByPaxId(paxId)) continue;
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

      CheckIn::LoadPaxDoc(paxId, pax.doc);
      pax.doco = CheckIn::LoadPaxDoco(paxId);
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


PassengerStatus& PassengerStatus::fromDB(TQuery &Qry)
{
  clear();

  m_paxId=Qry.FieldAsInteger("pax_id");
  m_countryControl=Qry.FieldAsString("country_control");
  m_status=statusTypes().decode(Qry.FieldAsString("status"));
  m_freeText=Qry.FieldAsString("free_text");
  m_pointId=Qry.FieldAsInteger("point_id");

  return *this;
}

const PassengerStatus& PassengerStatus::toDB(TQuery &Qry) const
{
  m_paxId!=ASTRA::NoExists?Qry.SetVariable("pax_id", m_paxId):
                           Qry.SetVariable("pax_id", FNull);
  Qry.SetVariable("country_control", m_countryControl);
  Qry.SetVariable("status", statusTypes().encode(m_status));
  Qry.SetVariable("free_text", m_freeText);
  if (Qry.GetVariableIndex("point_id")>=0)
    m_pointId!=ASTRA::NoExists?Qry.SetVariable("point_id", m_pointId):
                               Qry.SetVariable("point_id", FNull);

  return *this;
}

bool PassengerStatus::allowedToBoarding(const int paxId)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT status FROM iapi_pax_data WHERE pax_id=:pax_id AND pr_del=0";
  Qry.CreateVariable("pax_id", otInteger, paxId);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    if (!allowedToBoarding( statusTypes().decode(Qry.FieldAsString("status")) )) return false;

  return true;
}

bool PassengerStatus::allowedToBoarding(const int paxId, const TCompleteAPICheckInfo& checkInfo)
{
  if (needCheckStatus(checkInfo))
    return allowedToBoarding(paxId);
  return true;
}

bool PassengerStatusInspector::allowedToPrintBP(const int paxId, const int grpId)
{
  if (PassengerStatus::allowedToBoarding(paxId)) return true; //специально вначале - оптимизируем обращения к БД (подавляющее большинство разрешено к посадке)

  return (!needCheckStatus(get(paxId, grpId)));
}

const PassengerStatus& PassengerStatus::updateByRequest(const std::string& msgIdForClearPassengerRequest,
                                                        const std::string& msgIdForChangePassengerData,
                                                        bool& notRequestedBefore) const
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
      "BEGIN "
      "  :inserted:=0; "
      "  UPDATE iapi_pax_data "
      "  SET status=:status, free_text=:free_text, msg_id=:msg_id_change, point_id=:point_id, pr_del=0 "
      "  WHERE pax_id=:pax_id AND country_control=:country_control; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO iapi_pax_data(pax_id, country_control, status, free_text, msg_id, point_id, pr_del) "
      "    VALUES(:pax_id, :country_control, :status, :free_text, :msg_id_clear, :point_id, 0); "
      "    :inserted:=1; "
      "  END IF; "
      "END;";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("country_control", otString);
  Qry.DeclareVariable("status", otString);
  Qry.DeclareVariable("free_text", otString);
  Qry.DeclareVariable("point_id", otInteger);
  Qry.CreateVariable("msg_id_clear", otString, msgIdForClearPassengerRequest);
  Qry.CreateVariable("msg_id_change", otString, msgIdForChangePassengerData);
  Qry.CreateVariable("inserted", otInteger, FNull);

  toDB(Qry);
  Qry.Execute();

  notRequestedBefore=Qry.GetVariableAsInteger("inserted")!=0;

  return *this;
}

const PassengerStatus& PassengerStatus::updateByResponse(const std::string& msgId) const
{
  if (msgId.empty()) return *this;

  TQuery Qry(&OraSession);
  Qry.SQLText=
      "UPDATE iapi_pax_data SET status=:status, free_text=:free_text "
      "WHERE pax_id=:pax_id AND country_control=:country_control AND msg_id=:msg_id";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("country_control", otString);
  Qry.DeclareVariable("status", otString);
  Qry.DeclareVariable("free_text", otString);
  Qry.CreateVariable("msg_id", otString, msgId);

  toDB(Qry);
  Qry.Execute();
  if (Qry.RowsProcessed()>0) writeToLogAndCheckAlarm(false);

  return *this;
}

const PassengerStatus& PassengerStatus::updateByCusRequest(bool& notRequestedBefore) const
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
      "UPDATE iapi_pax_data SET status=:status, free_text=:free_text "
      "WHERE pax_id=:pax_id AND country_control=:country_control";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("country_control", otString);
  Qry.DeclareVariable("status", otString);
  Qry.DeclareVariable("free_text", otString);

  toDB(Qry);
  Qry.Execute();
  if (Qry.RowsProcessed()>0)
    writeToLogAndCheckAlarm(true);
  else
    LogTrace(TRACE5) << __func__ << ": passenger not found (paxId=" << m_paxId << ", countryControl=" << m_countryControl << ")";

  notRequestedBefore=Qry.RowsProcessed()>0;

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

  TQuery Qry(&OraSession);
  Qry.SQLText=
      "SELECT pax_id FROM iapi_pax_data WHERE country_control=:country_control AND msg_id=:msg_id";
  Qry.CreateVariable("msg_id", otString, msgId);
  Qry.CreateVariable("country_control", otString, statusPattern.countryControl());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    PassengerStatus status(statusPattern);
    status.setPaxId(Qry.FieldAsInteger("pax_id"));
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

void init_callbacks()
{
  CallbacksSingleton<edifact::CusresCallbacks>::Instance()->setCallbacks(new CusresCallbacks);
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


} //namespace IAPI
