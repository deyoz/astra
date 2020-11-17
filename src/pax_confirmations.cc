#include "pax_confirmations.h"
#include <serverlib/cursctl.h>
#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;

namespace PaxConfirmations
{

std::ostream& operator<<(std::ostream& os, const SettingsFilter& filter)
{
  os << endl << "  airline:        " << filter.airline
     << endl << "  airpDep:        " << filter.airpDep
     << endl << "  cl:             " << filter.cl
     << endl << "  subcl:          " << filter.subcl
     << endl << "  dcsAction:      " << dcsActions().encode(filter.dcsAction);

  return os;
}

std::ostream& operator<<(std::ostream& os, const Setting& setting)
{
  os << "  id: " << setting.id
     << ", rfisc: " << setting.rfisc
     << ", rem_code: " << setting.rem_code;
  os << ", brand: ";
  if (setting.brand)
    os << setting.brand.get();
  os << ", tierLevel: ";
  if (setting.tierLevel)
    os << setting.tierLevel.get();
  os << ", fqtShouldNotExists: " << boolalpha << setting.fqtShouldNotExists;

  return os;
}

std::ostream& operator<<(std::ostream& os, const Settings& settings)
{
  for(const auto& i : settings)
    os << endl << i;

  return os;
}

bool SettingsFilter::suitable(const boost::optional<TBrand::Key>& brand, TBrands& brands) const
{
  if (!brand) return true;

  if (!brandKeyList)
  {
    brands.get(paxId.get());

    brandKeyList=boost::in_place();
    brandKeyList.get()=algo::transform<std::list<TBrand::Key>>(brands, [](const auto& i) { return i.key(); } );
  }

  return algo::any_of(brandKeyList.get(), [&brand](const auto& i) { return i==brand.get(); } );
}

bool SettingsFilter::suitable(const boost::optional<TierLevelKey>& tierLevel,
                              const bool fqtShouldNotExists) const
{
  if (!tierLevel && !fqtShouldNotExists) return true;

  if (!fqts)
  {
    fqts=boost::in_place();
    LoadPaxFQT(paxId.get(), fqts.get());
  }

  if (fqtShouldNotExists) return fqts.get().empty();

  return algo::any_of(fqts.get(), [&tierLevel](const auto& i) { return i.tierLevelKey()==tierLevel; } );
}

bool SettingsFilter::suitableRfisc(const std::string& rfisc) const
{
  if (rfisc.empty()) return true;

  if (!rfiscs)
  {
    TPaidRFISCListWithAuto paid;
    paid.fromDB(paxId.get(), false);

    rfiscs=boost::in_place();
    paid.getUniqRFISCSs(paxId.get(), rfiscs.get());
  }

  return algo::any_of(rfiscs.get(), [&rfisc](const auto& i) { return i==rfisc; } );
}

bool SettingsFilter::suitableRemCode(const std::string& rem_code) const
{
  if (rem_code.empty()) return true;

  if (!rems)
  {
    rems=boost::in_place();
    LoadPaxRem(paxId.get(), rems.get());
  }

  return algo::any_of(rems.get(), [&rem_code](const auto& i) { return i.code==rem_code; } );
}

const std::set<AppliedMessage>& AppliedMessages::getMessages(const Segment& seg,
                                                             const int paxIndex,
                                                             const std::pair<PaxId_t, CheckIn::TSimplePaxItem>& paxPair) const
{
  static std::set<AppliedMessage> emptyMessages;

  if (paxPair.second.id!=ASTRA::NoExists)
  {
    const auto i=messagesByPaxId.find(PaxId_t(paxPair.second.id));
    if (i!=messagesByPaxId.end()) return i->second;
  }
  else
  {
    const auto i=messagesByNorecId.find(NorecId(PointId_t(seg.flt.point_id), paxIndex));
    if (i!=messagesByNorecId.end()) return i->second;
  }

  return emptyMessages;
}

void AppliedMessages::toDB(const Segments& segs) const
{
  if (messagesByPaxId.empty() && messagesByNorecId.empty()) return;

  auto cur = make_curs("INSERT INTO pax_confirmations(pax_id, id, text) "
                       "VALUES(:pax_id, :id, :text)");

  for(const Segment& seg : segs)
  {
    int paxIndex=0;
    for(const auto& paxPair : seg.paxs)
    {
      paxIndex++;
      const auto& messages=getMessages(seg, paxIndex, paxPair);

      for(const AppliedMessage& m : messages)
      {
        cur.noThrowError(CERR_U_CONSTRAINT)
           .bind(":pax_id", paxPair.first.get())
           .bind(":id", m.id)
           .bind(":text", m.text)
           .exec();
      }
    }
  }
}

void AppliedMessages::toLog(const Segments& segs) const
{
  for(const Segment& seg : segs)
  {
    TLogLocale msg;
    msg.ev_type=ASTRA::evtPax;
    msg.lexema_id = "EVT.PASSENGER_DATA";
    msg.id1=seg.flt.point_id;
    msg.id3=seg.grp.id;

    int paxIndex=0;
    for(const auto& paxPair : seg.paxs)
    {
      paxIndex++;
      const auto& messages=getMessages(seg, paxIndex, paxPair);

      const CheckIn::TSimplePaxItem& pax=paxPair.second;

      msg.id2=pax.reg_no;

      set<string> alreadyUsedText;
      for(const AppliedMessage& m : messages)
      {
        if (!alreadyUsedText.insert(m.text).second) continue;
        msg.prms.clearPrms();
        msg.prms << PrmSmpl<string>("pax_name", pax.full_name())
                 << PrmElem<string>("pers_type", etPersType, EncodePerson(pax.pers_type))
                 << PrmLexema("param",
                              "EVT.PAX_CONFIRMATION_REQUESTED",
                              LEvntPrms() << PrmSmpl<string>("text", m.text));

        TReqInfo::Instance()->LocaleToLog(msg);
      }
    }
  }
}

void AppliedMessages::get(int id, bool isGrpId)
{
  auto cur = make_curs(isGrpId?"SELECT pax_confirmations.pax_id, pax_confirmations.id, pax_confirmations.text "
                               "FROM pax, pax_confirmations "
                               "WHERE pax.pax_id=pax_confirmations.pax_id AND pax.grp_id=:id":
                               "SELECT pax_id, id, text "
                               "FROM pax_confirmations "
                               "WHERE pax_id=:id");
  int pax_id;
  int msg_id;
  string msg_text;

  cur.def(pax_id)
     .def(msg_id)
     .def(msg_text)
     .bind(":id", id)
     .exec();

  while(!cur.fen())
    messagesByPaxId[PaxId_t(pax_id)].emplace(msg_id, msg_text);
}

bool AppliedMessages::exists(xmlNodePtr node)
{
  return GetNode("confirmations/pax_confirmations", node)!=nullptr;
}

void AppliedMessages::get(xmlNodePtr node)
{
  if (node==nullptr) return;

  xmlNodePtr replyInfoNode=GetNode("confirmations/pax_confirmations/reply_info", node);
  for(; replyInfoNode!=nullptr; replyInfoNode=replyInfoNode->next)
  {
    xmlNodePtr paxNode=NodeAsNode("passenger", replyInfoNode);

    boost::optional<PaxId_t> paxId;
    boost::optional<NorecId> norecId;
    xmlNodePtr paxIdNode=GetNode("@id", paxNode);
    if (paxIdNode!=nullptr)
      paxId=boost::in_place(NodeAsInteger(paxIdNode));
    else
      norecId=boost::in_place(PointId_t(NodeAsInteger("@point_id", paxNode)),
                              NodeAsInteger("@index", paxNode));

    xmlNodePtr confirmationNode=GetNode("confirmation", paxNode);
    for(; confirmationNode!=nullptr; confirmationNode=confirmationNode->next)
    {
      if (paxId)
        messagesByPaxId[paxId.get()].emplace(NodeAsInteger("@id", confirmationNode),
                                             NodeAsString(confirmationNode));
      if (norecId)
        messagesByNorecId[norecId.get()].emplace(NodeAsInteger("@id", confirmationNode),
                                                 NodeAsString(confirmationNode));
    }
  }
}

bool AppliedMessages::exists(const PaxId_t& paxId, const int id) const
{
  const auto i=messagesByPaxId.find(paxId);
  if (i==messagesByPaxId.end()) return false;

  return algo::any_of(i->second, [&id](const auto& message) { return message.id==id; });
}

AppliedMessages::AppliedMessages(const PaxId_t& paxId)
{
  get(paxId.get(), false);
}

AppliedMessages::AppliedMessages(const GrpId_t& grpId)
{
  get(grpId.get(), true);
}

AppliedMessages::AppliedMessages(xmlNodePtr node)
{
  get(node);
}

const Settings& Messages::filterRoughly(const SettingsFilter& filter)
{
  auto iSettings=std::find_if(settingsCache.begin(), settingsCache.end(),
                              [&filter](const auto& i) {  return filter.airline==i.first.airline &&
                                                                 filter.airpDep==i.first.airpDep &&
                                                                 filter.cl==i.first.cl &&
                                                                 filter.subcl==i.first.subcl &&
                                                                 filter.dcsAction==i.first.dcsAction; } );

  if (iSettings!=settingsCache.end()) return iSettings->second;


  iSettings=settingsCache.emplace(settingsCache.end(), filter, Settings());

  Settings& settings=iSettings->second;

  LogTrace(TRACE5) << __func__ << ": " << filter;

  auto cur = make_curs(
    "SELECT " + Setting::selectedFields() +
    "FROM confirmation_sets "
    "WHERE airline=:airline AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (class IS NULL OR class=:class) AND "
    "      (subclass IS NULL OR subclass=:subclass) AND "
    "      dcs_action=:dcs_action ");



  Setting setting;

  Setting::curDef(cur, setting);

  cur.bind(":airline", filter.airline.get())
     .bind(":airp_dep", filter.airpDep.get())
     .bind(":class", filter.cl.get())
     .bind(":subclass", filter.subcl.get())
     .bind(":dcs_action", dcsActions().encode(filter.dcsAction))
     .exec();

  while(!cur.fen())
  {
    settings.insert(setting.afterFetchProcessing());
  }

  if (!settings.empty())
    LogTrace(TRACE5) << __func__ << ": " << settings;

  return settings;
}

void Messages::add(const SettingsFilter& filter,
                   const boost::optional<AppliedMessages>& messages)
{
  const Settings& settingsRoughly=filterRoughly(filter);

  for(const Setting& setting : settingsRoughly)
  {
    if (messages && messages.get().exists(filter.paxId, setting.id)) continue;

    if (filter.suitable(setting.brand, brands) &&
        filter.suitable(setting.tierLevel, setting.fqtShouldNotExists) &&
        filter.suitableRfisc(setting.rfisc) &&
        filter.suitableRemCode(setting.rem_code))
    {
      settingsByPaxId[filter.paxId].insert(setting);
    }
  }
}

Messages::Messages(const DCSAction::Enum dcsAction,
                   const Segments& segs,
                   const bool excludeAppliedMessages) : segments(segs)
{
  for(const Segment& seg : segments)
  {
    if (seg.grp.grpCategory()!=CheckIn::TPaxGrpCategory::Passenges) continue;

    boost::optional<AppliedMessages> appliedMessages;
    if (excludeAppliedMessages)
    {
      if (seg.paxs.size()>1)
      {
        appliedMessages=boost::in_place(GrpId_t(seg.grp.id));
      }
      else
      {
        if (!seg.paxs.empty())
        {
          appliedMessages=boost::in_place(seg.paxs.front().first);
        }
      }
    }

    for(const auto& paxPair : seg.paxs)
    {
      const CheckIn::TSimplePaxItem& pax=paxPair.second;

      if (!pax.refuse.empty()) continue;

      add(SettingsFilter(PaxId_t(paxPair.first),
                         AirlineCode_t(seg.flt.airline),
                         AirportCode_t(seg.grp.airp_dep),
                         Class_t(seg.grp.cl),
                         SubClass_t(pax.subcl),
                         dcsAction),
          appliedMessages);
    }
  }
}

bool Messages::toXML(xmlNodePtr node, const OutputLang &lang) const
{
  bool result=false;

  if (node==nullptr) return result;

  if (settingsByPaxId.empty()) return result;

  xmlNodePtr messagesNode=nullptr;
  for(const Segment& seg : segments)
  {
    int paxIndex=0;
    for(const auto& paxPair : seg.paxs)
    {
      paxIndex++;
      const CheckIn::TSimplePaxItem& pax=paxPair.second;

      SettingsByPaxId::const_iterator i=settingsByPaxId.find(paxPair.first);
      if (i==settingsByPaxId.end()) continue;

      if (messagesNode==nullptr)
      {
        xmlNodePtr confirmationNode=NewTextChild(node, "confirmation");
        NewTextChild(confirmationNode, "type", "pax_confirmations");
        messagesNode=NewTextChild(confirmationNode, "messages");
      }

      xmlNodePtr messageNode=NewTextChild(messagesNode, "message");
      SetProp(messageNode, "use_big_dialog", (int)false);
      NewTextChild(messageNode, "text", getText(pax, seg.flt, segments.size()>1, i->second, lang));
      if (pax.id!=ASTRA::NoExists)
        replyInfoToXML(messageNode, PaxId_t(pax.id), boost::none, i->second, lang);
      else
        replyInfoToXML(messageNode, boost::none, NorecId(PointId_t(seg.flt.point_id), paxIndex), i->second, lang);

      result=true;
    }
  }

  return result;
}

void Messages::replyInfoToXML(xmlNodePtr messageNode,
                              const boost::optional<PaxId_t>& paxId,
                              const boost::optional<NorecId>& norecId,
                              const Settings& settings,
                              const OutputLang &lang)
{
  if (messageNode==nullptr) return;

  xmlNodePtr paxNode=NewTextChild(NewTextChild(messageNode, "reply_info"), "passenger");
  if (paxId)
  {
    SetProp(paxNode, "id", paxId.get().get());
  }
  if (norecId)
  {
    SetProp(paxNode, "point_id", norecId.get().pointId.get());
    SetProp(paxNode, "index",  norecId.get().paxIndex);
  }

  for(const Setting& setting : settings)
  {
    xmlNodePtr confirmationNode=NewTextChild(paxNode, "confirmation",
                                             lang.get()==LANG_RU?setting.text:setting.text_lat);
    SetProp(confirmationNode, "id", setting.id);
  }

}

std::string Messages::getText(const CheckIn::TSimplePaxItem& pax,
                              const TTripInfo& flt,
                              const bool showFlight,
                              const Settings& settings,
                              const OutputLang &lang)
{
  string paxStr=getLocaleText("MSG.PASSENGER_INFO",
                              LParams() << LParam("name", pax.full_name())
                                        << LParam("pers_type", ElemIdToPrefferedElem(etPersType,
                                                                                     EncodePerson(pax.pers_type),
                                                                                     efmtCodeNative,
                                                                                     lang.get())),
                              lang.get());

  ostringstream result;
  if (showFlight)
    result << getLocaleText("WRAP.PASSENGER_INFO",
                            LParams() << LParam("pass", paxStr)
                                      << LParam("flight", flt.flight_view()),
                            lang.get());
  else
    result << paxStr;

  set<string> alreadyUsedText;
  for(const Setting& setting : settings)
  {
    const string& text=(lang.get()==LANG_RU?setting.text:setting.text_lat);
    if (!alreadyUsedText.insert(text).second) continue;
    result << endl << "    " << text;
  }

  result << endl << endl << " " << getLocaleText("QST.CONTINUE", lang.get());

  return result.str();
}

} //namespace PaxConfirmations
