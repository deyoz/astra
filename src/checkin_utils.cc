#include "apis_utils.h"
#include "checkin_utils.h"
#include "salons.h"
#include "seats.h"
#include "salonform.h"
#include "date_time.h"
#include "comp_layers.h"
#include "alarms.h"
#include "points.h"
#include "counters.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "ANNA"
#include <serverlib/slogger.h>
#include <serverlib/savepoint.h>
#include <serverlib/algo.h>

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace SALONS2;

void TSegListItem::setCabinClassAndSubclass(bool isTransitGrp)
{
  boost::optional<TCFG> cfg;
  for(CheckIn::TPaxListItem& p : paxs)
  {
    CheckIn::TSimplePaxItem& pax=p.pax;
    boost::optional<CheckIn::TComplexClass> crsClass;
    if (isTransitGrp)
    {
      if (p.originalCrsPaxId)
      {
        crsClass=CheckIn::TSimplePaxItem::getCrsClass(p.originalCrsPaxId.get(), true);
        if (crsClass)
        {
          if (!cfg) cfg=boost::in_place(flt.point_id);
          if (algo::none_of(cfg.get(), [&crsClass](const TCFGItem& item) { return item.cls==crsClass.get().cl && item.cfg>0; }))
            crsClass=boost::none;
        }
      }
    }
    else
    {
      if (pax.id!=NoExists)
        crsClass=CheckIn::TSimplePaxItem::getCrsClass(PaxId_t(pax.id), true);
    }

    if (crsClass)
      pax.cabin=crsClass.get();
    else
    {
      pax.cabin.cl=grp.cl;
      pax.cabin.subcl=pax.subcl;
    }
  }
}

bool TSegList::needCopyBaggage(const GrpId_t& grpId, const size_t trferSize) const
{
  size_t trferNum=0;
  for(const_iterator i=begin(); i!=end(); ++i)
  {
    if (i!=begin() && i->grp.status!=psTransit) trferNum++;
    if (i->grp.id==grpId.get())
      return (i!=begin() && trferNum<=trferSize);
  }
  //�� ��諨 grpId;
  return false;
}

PaxConfirmations::Segments TSegList::transformForPaxConfirmations(bool isNewCheckIn) const
{
  PaxConfirmations::Segments result;
  for(const TSegListItem& s : *this)
  {
    result.emplace_back();
    PaxConfirmations::Segment& segment=result.back();
    segment.flt=s.flt;
    segment.grp=s.grp;
    for(const CheckIn::TPaxListItem& p : s.paxs)
      segment.paxs.emplace(PaxId_t(p.getExistingPaxIdOrSwear()), p.pax);

    if (!isNewCheckIn &&
        (this->front().grp.group_bag ||
         this->front().grp.svc ||
         this->front().grp.svc_auto))
    {
      std::list<CheckIn::TSimplePaxItem> paxs=CheckIn::TSimplePaxItem::getByGrpId(GrpId_t(s.grp.id));
      for(const CheckIn::TSimplePaxItem& p : paxs)
        segment.paxs.emplace(PaxId_t(p.id), p);
    }
  }

  return result;
}

namespace CheckIn
{

void grpMktFlightFromXML(xmlNodePtr node, TGrpMktFlight& grpMktFlight)  //�����: ��।���� grpMktFlight ᮤ�ন� ����� �������饣� ३�
{
  if (node==nullptr) return; //�����: �� ���塞 grpMktFlight!

  TGrpMktFlight markFltInfo;
  markFltInfo.fromXML(node);

  ostringstream flt;
  flt << markFltInfo.airline << markFltInfo.flight_number();
  if (markFltInfo.scd_date_local!=grpMktFlight.scd_date_local)
    flt << "/" << DateTimeToStr(markFltInfo.scd_date_local,"dd");
  if (markFltInfo.airp_dep!=grpMktFlight.airp_dep)
    flt << "/" << markFltInfo.airp_dep;

  string str;
  TElemFmt fmt;

  str=ElemToElemId(etAirline,markFltInfo.airline,fmt);
  if (fmt==efmtUnknown)
    throw AstraLocale::UserException("MSG.COMMERCIAL_FLIGHT.AIRLINE.UNKNOWN_CODE",
                                     LParams()<<LParam("airline",markFltInfo.airline)
                                              <<LParam("flight", flt.str()));  //WEB
  markFltInfo.airline=str;

  if (!markFltInfo.suffix.empty())
  {
    str=ElemToElemId(etSuffix,markFltInfo.suffix,fmt);
    if (fmt==efmtUnknown)
      throw AstraLocale::UserException("MSG.COMMERCIAL_FLIGHT.SUFFIX.INVALID",
                                       LParams()<<LParam("suffix",markFltInfo.suffix)
                                                <<LParam("flight",flt.str())); //WEB
    markFltInfo.suffix=str;
  }

  str=ElemToElemId(etAirp,markFltInfo.airp_dep,fmt);
  if (fmt==efmtUnknown)
    throw AstraLocale::UserException("MSG.COMMERCIAL_FLIGHT.UNKNOWN_AIRP",
                                     LParams()<<LParam("airp",markFltInfo.airp_dep)
                                              <<LParam("flight",flt.str())); //WEB
  markFltInfo.airp_dep=str;

  grpMktFlight=markFltInfo; //�����: ⮫쪮 �᫨ �� ��� ���塞 grpMktFlight!
}

void setComplexClassGrp(const TFlightRbd& rbds, TComplexClass& complex)
{
  boost::optional<TSubclassGroup> pax_subclass_grp=rbds.getSubclassGroup(complex.subcl, complex.cl);
  if (!pax_subclass_grp)
    throw AstraLocale::UserException("MSG.CHECKIN.NOT_MADE_IN_CLASS",
                                     LParams()<<LParam("class",ElemIdToCodeNative(etClass,complex.cl)));
  complex.cl_grp=pax_subclass_grp.get().value();
}

class SeatingGroup
{
  public:
    TSegListItem seg;
    TGrpMktFlight grpMktFlight;
};

class SeatingGroups : public std::map<int/*grp_id*/, SeatingGroup>
{
  private:
    TAdvTripInfo fltInfo;
  public:
    SeatingGroups(const TAdvTripInfo& _fltInfo) : fltInfo(_fltInfo) {}
    void add(const TSimplePaxItem& pax);
};

void SeatingGroups::add(const TSimplePaxItem& pax)
{
  if (pax.id==ASTRA::NoExists) return;

  SeatingGroups::iterator iGroup=find(pax.grp_id);
  if (iGroup==end())
  {
    TSimplePaxGrpItem grp;
    if (!grp.getByGrpId(pax.grp_id)) return;
    //�������᪨� ३�
    TGrpMktFlight grpMktFlight;
    if (!grpMktFlight.getByGrpId(pax.grp_id)) return;

    iGroup=emplace(pax.grp_id, SeatingGroup()).first;
    iGroup->second.seg.flt=fltInfo;
    static_cast<TSimplePaxGrpItem&>(iGroup->second.seg.grp)=grp;
    iGroup->second.grpMktFlight=grpMktFlight;
  }

  if (iGroup==end()) return;

  iGroup->second.seg.paxs.emplace_back(pax);
  //६�ન �㦭� ��� ��ᠤ��
  LoadPaxRem(pax.id, iGroup->second.seg.paxs.back().rems);
  iGroup->second.seg.paxs.back().remsExists=true;
}

void syncCabinClassErrorToLog(int point_id,
                              const TSimplePaxItem& pax,
                              const AstraLocale::UserException& e)
{
  string err_id;
  LEvntPrms err_prms;
  e.getAdvParams(err_id, err_prms);

  TLogLocale locale;
  locale.ev_type=ASTRA::evtPax;
  locale.id1=point_id;
  locale.id2=pax.reg_no;
  locale.id3=pax.grp_id;
  locale.lexema_id = "EVT.PAX.SYNC_CABIN_CLASS_ERROR";

  locale.prms << PrmSmpl<string>("name", pax.full_name())
              << PrmLexema("what", err_id, err_prms);
  TReqInfo::Instance()->LocaleToLog(locale);
}

void newCabinClassMsgToLog(int point_id,
                           const TSimplePaxItem& pax)
{
  TLogLocale locale;
  locale.ev_type=ASTRA::evtPax;
  locale.id1=point_id;
  locale.id2=pax.reg_no;
  locale.id3=pax.grp_id;
  locale.lexema_id = "EVT.PASSENGER_SEATED_AUTOMATICALLY";

  locale.prms << PrmSmpl<string>("name", pax.full_name())
              << PrmSmpl<string>("seat", pax.seat_no);
  TReqInfo::Instance()->LocaleToLog(locale);
}

void syncCabinClass(const TTripTaskKey &task)
{
  std::set<PaxId_t> paxIds;
  getAlarmByPointId(PointId_t(task.point_id), Alarm::SyncCabinClass, paxIds);
  if (paxIds.empty()) return;

  TFlights flightsForLock;
  flightsForLock.Get(task.point_id, ftTranzit);
  flightsForLock.Lock(__FUNCTION__);

  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(task.point_id)) return;

  TFlightRbd rbds(fltInfo);

  SeatingGroups seatingGroups(fltInfo);

  for(const PaxId_t paxId : paxIds)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": paxId=" << paxId;
    deleteAlarmByPaxId(paxId, {Alarm::SyncCabinClass}, {paxCheckIn});

    OciCpp::Savepoint spSyncCabinClass("sync_cabin_class");

    TSimplePaxItem pax;
    if (!pax.getByPaxId(paxId.get())) continue;

    try
    {
      boost::optional<TComplexClass> actualCabin=CheckIn::TSimplePaxItem::getCrsClass(PaxId_t(pax.id), false);
      if (!actualCabin) continue;

      if (pax.hasCabinSeatNumber())
      {
        //��ᠦ����� ���ᠦ�� �� ᠫ���
        BitSet<SEATS2::TChangeLayerFlags> change_layer_flags;
        change_layer_flags.setFlag(SEATS2::flSyncCabinClass);

        IntChangeSeatsN( fltInfo.point_id, pax.id, pax.tid, "", "",
                         SEATS2::stDropseat,
                         cltUnknown,
                         NoExists,
                         change_layer_flags,
                         0, NoExists, NULL, NULL,
                         __func__);
      };

      setComplexClassGrp(rbds, actualCabin.get());
      pax.cabin=actualCabin.get();
      if (!pax.cabinClassToDB())
        throw EXCEPTIONS::Exception("strange situation - !pax.cabinClassToDB()");

      if (pax.hasCabinSeatNumber())
        seatingGroups.add(pax);
    }
    catch(const AstraLocale::UserException &e)
    {
      spSyncCabinClass.rollback();
      syncCabinClassErrorToLog(fltInfo.point_id, pax, e);
    }
    catch(const std::exception &e)
    {
      spSyncCabinClass.rollback();
      LogError(STDLOG) << __func__ << " error (pax_id=" << paxId << "): " << e.what();
    }
  }

  for(auto& g : seatingGroups)
  {
    const SeatingGroup& seatingGroup=g.second;

    OciCpp::Savepoint spSyncCabinClass("sync_cabin_class");

    try
    {
      //���� �� ����� �஢���� ���稪� - CheckCounters?
      seatingWhenNewCheckIn(seatingGroup.seg,
                            fltInfo,
                            seatingGroup.grpMktFlight);
      for(TPaxListItem& i : g.second.seg.paxs)
      {
        i.pax.seat_no=i.pax.getSeatNo("seats");
        newCabinClassMsgToLog(fltInfo.point_id, i.pax);
      }
    }
    catch(const AstraLocale::UserException &e)
    {
      spSyncCabinClass.rollback();
      for(const TPaxListItem& i : seatingGroup.seg.paxs)
        syncCabinClassErrorToLog(fltInfo.point_id, i.pax, e);
    }
    catch(const std::exception &e)
    {
      spSyncCabinClass.rollback();
      for(const TPaxListItem& i : seatingGroup.seg.paxs)
        LogError(STDLOG) << __func__ << " error (grp_id=" << i.pax.grp_id << ", pax_id=" << i.pax.id << "): " << e.what();
    }
  }

  TCounters().recount(fltInfo.point_id, CheckIn::TCounters::Total, __func__);
  SALONS2::check_waitlist_alarm_on_tranzit_routes( fltInfo.point_id, __func__ );
}

void seatingWhenNewCheckIn(const TSegListItem& seg,
                           const TAdvTripInfo& fltAdvInfo,
                           const TSimpleMktFlight& markFltInfo)
{
  const TTripInfo& fltInfo=seg.flt;
  const CheckIn::TSimplePaxGrpItem& grp=seg.grp;
  const CheckIn::TPaxList& paxs=seg.paxs;

  //ࠧ��⪠ ��⥩ �� �����
  vector<TInfantAdults> InfItems, AdultItems;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT pax_id FROM crs_inf WHERE inf_id=:inf_id";
  Qry.DeclareVariable("inf_id", otInteger);
  for(int k=0;k<=1;k++)
  {
    int pax_no=1;
    for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
    {
      const CheckIn::TSimplePaxItem &pax=p->pax;
      int pax_id=p->getExistingPaxIdOrSwear();

      if ((pax.seats<=0&&k==0)||(pax.seats>0&&k==1)) continue;
      TInfantAdults pass;       // infant �� ⠡���� crs_pax, pax, crs_inf
      pass.grp_id = 1;
      pass.pax_id = pax_id;
      pass.reg_no = pax_no;
      pass.surname = pax.surname;
      if (pax.seats<=0)
      {
        // ��� ����
        if ( pax.id!=NoExists )
        {
          Qry.SetVariable("inf_id", pax.id);
          Qry.Execute();
          if ( !Qry.Eof )
            pass.parent_pax_id = Qry.FieldAsInteger( "pax_id" );
        }
        InfItems.push_back( pass );
      }
      else
      {
        if (pax.pers_type == ASTRA::adult) AdultItems.push_back( pass );
      }
    }
  }

  SALONS2::TSeatTariffMap tariffMap;
  //ProgTrace( TRACE5, "InfItems.size()=%zu, AdultItems.size()=%zu", InfItems.size(), AdultItems.size() );
  SetInfantsToAdults( InfItems, AdultItems );
  SEATS2::Passengers.Clear();
  SEATS2::TSublsRems subcls_rems( fltInfo.airline );

  SALONS2::TSalonList salonList(true);
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( grp.point_dep, grp.point_arv ), "", ASTRA::NoExists );
  //�������� ���ᨢ ��� ��ᠤ��
  TRemGrp remGrp;
  remGrp.Load(retFORBIDDEN_FREE_SEAT,grp.point_dep);

  for(int k=0;k<=1;k++)
  {
    for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    {
      const CheckIn::TSimplePaxItem &pax=p->pax;
      int pax_id=p->getExistingPaxIdOrSwear();
      try
      {
        if (pax.seats<=0||(pax.seats>0&&k==1)) continue;
        if (pax.is_jmp) continue;
        SEATS2::TPassenger pas;

        pas.cabin_clname=pax.cabin.cl.empty()?grp.cl:pax.cabin.cl;
        pas.paxId = pax_id;
        pas.grp_status = grp.getCheckInLayerType();
        if (pas.grp_status==cltUnknown)
          throw EXCEPTIONS::Exception("%s: Not applied for grp.status=%s", __func__, EncodePaxStatus(grp.status));
        pas.preseat_no=pax.seat_no; // crs or hand made
        pas.countPlace=pax.seats;
        pas.is_jmp=pax.is_jmp;
        pas.placeRem=pax.seat_type;
        pas.pers_type = EncodePerson(pax.pers_type);
        bool flagCHIN=pax.pers_type != ASTRA::adult;
        bool flagINFT = false;
        for(multiset<CheckIn::TPaxRemItem>::const_iterator r=p->rems.begin(); r!=p->rems.end(); ++r)
        {
          if (r->code=="BLND" ||
              r->code=="STCR" ||
              r->code=="UMNR" ||
              r->code=="WCHS" ||
              r->code=="MEDA") flagCHIN=true;
          pas.add_rem(r->code,remGrp);
        }
        string pass_rem;
        if ( subcls_rems.IsSubClsRem( pax.getCabinSubclass(), pass_rem ) )  pas.add_rem(pass_rem,remGrp);
        if ( AdultsWithBaby( pax_id, InfItems ) ) {
          flagCHIN = true;
          flagINFT = true;
        }
        if ( flagCHIN ) {
          pas.add_rem("CHIN",remGrp);
        }
        if ( flagINFT ) {
          pas.add_rem("INFT",remGrp);
        }

        //����� ����ࠥ�
        if ( SALONS2::selfckin_client() ) {
          tariffMap.get_rfisc_colors( fltAdvInfo.airline );
          SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
          SelfCkinSalonTariff.setTariffMap( grp.point_dep, tariffMap );
        }
        else {
          tariffMap.get(fltAdvInfo, markFltInfo, pax.tkn,grp.airp_arv);
        }
        pas.tariffs=tariffMap;
        pas.tariffStatus = tariffMap.status();
        tariffMap.trace(TRACE5);

        pas.dont_check_payment = p->dont_check_payment;

        SEATS2::Passengers.Add(salonList,pas);
      }
      catch(const CheckIn::UserException&)
      {
        throw;
      }
      catch(const AstraLocale::UserException &e)
      {
        throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax_id);
      }
    }
  }

  //��।���� ������ ��ᠤ��
  SEATS2::TSeatAlgoParams algo=SEATS2::GetSeatAlgo(Qry,fltInfo.airline,fltInfo.flt_no,fltInfo.airp);
  boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();
  //��ᠤ��
  SALONS2::TAutoSeats autoSeats;
  SEATS2::SeatsPassengers( salonList, algo, TReqInfo::Instance()->client_type, SEATS2::Passengers, autoSeats, remGrp );
  bool pr_do_check_wait_list_alarm = salonList.check_waitlist_alarm_on_tranzit_routes( autoSeats );
  //!!! ������ True - �������� ��ᠤ�� �� ���஭�஢���� ����, �����
  // ���� �ࠢ� �� ॣ������, ����� ३� ����砭��, ���� �ࠢ� ᠦ��� �� �㦨� ���஭. ����
  bool pr_lat_seat=salonList.isCraftLat();
  boost::posix_time::ptime mst2 = boost::posix_time::microsec_clock::local_time();
  LogTrace(TRACE5) << "SeatsPassengers: " << boost::posix_time::time_duration(mst2 - mst1).total_milliseconds() << " msecs";

  int i=0;
  bool change_agent_seat_no = false;
  bool change_preseat_no = false;
  bool exists_preseats = false; //���� �� � ��㯯� ���ᠦ�஢ �।���⥫�� ����
  bool invalid_seat_no = false; //���� ����饭�� ����
  std::set<int> paxs_external_logged;
  for(int k=0;k<=1;k++)
  {
    for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    {
      const CheckIn::TSimplePaxItem &pax=p->pax;
      int pax_id=p->getExistingPaxIdOrSwear();
      try
      {
        if ((pax.seats<=0&&k==0)||(pax.seats>0&&k==1)) continue;

        //������ ����஢ ����
        if (pax.seats>0 && !pax.is_jmp && i<SEATS2::Passengers.getCount())
        {
          SEATS2::TPassenger pas = SEATS2::Passengers.Get(i);
          ProgTrace( TRACE5, "pas.pax_id=%d, pax.id=%d, pax_id=%d", pas.paxId, pax.id, pax_id );
          for ( SALONS2::TAutoSeats::iterator iseat=autoSeats.begin();
                iseat!=autoSeats.end(); iseat++ ) {
            ProgTrace( TRACE5, "pas.paxId=%d, iseat->pax_id=%d, pax_id=%d",
                       pas.paxId, iseat->pax_id, pax_id );
            if ( pas.paxId == iseat->pax_id ) {
              iseat->pax_id = pax_id;
              break;
            }
          }
          paxs_external_logged.insert( pax_id );

          if ( pas.preseat_pax_id > 0 )
            exists_preseats = true;
          if ( !pas.isValidPlace )
            invalid_seat_no = true;

          if (pas.seat_no.empty()) throw EXCEPTIONS::Exception("SeatsPassengers: empty seat_no");
              string pas_seat_no;
              bool pr_found_preseat_no = false;
              bool pr_found_agent_no = false;
              for( std::vector<TSeat>::iterator iseat=pas.seat_no.begin(); iseat!=pas.seat_no.end(); iseat++ ) {
                pas_seat_no = iseat->denorm_view(pr_lat_seat);
              if ( pas_seat_no == pas.preseat_no ) {
                pr_found_preseat_no = true;
              }
              if ( pas_seat_no == pax.seat_no )
                pr_found_agent_no = true;
            }
            if ( !pax.seat_no.empty() &&
                 !pr_found_agent_no ) {
              change_agent_seat_no = true;
            }
            if ( pas.preseat_pax_id > 0 &&
                 !pas.preseat_no.empty() && !pr_found_preseat_no ) {
              change_preseat_no = true;
            }

          TSeatRanges ranges;
          for(vector<TSeat>::iterator iSeat=pas.seat_no.begin();iSeat!=pas.seat_no.end();iSeat++)
          {
            TSeatRange range(*iSeat,*iSeat);
            ranges.push_back(range);
          }
          ProgTrace( TRACE5, "ranges.size=%zu", ranges.size() );
          //������ � ����
          TCompLayerType layer_type = grp.getCheckInLayerType();
          if (layer_type==cltUnknown)
            throw EXCEPTIONS::Exception("%s: Not applied for grp.status=%s", __func__, EncodePaxStatus(grp.status));

          SEATS2::SaveTripSeatRanges( grp.point_dep, layer_type, ranges, pax_id, grp.point_dep, grp.point_arv, NowUTC() );
          DeleteTlgSeatRanges( {ASTRA::cltProtSelfCkin, ASTRA::cltProtBeforePay} , pax_id, pas.tid );

          if ( !pr_do_check_wait_list_alarm ) {
            autoSeats.WritePaxSeats( grp.point_dep, pax_id );
          }
          i++;
        }
        if ( invalid_seat_no )
            AstraLocale::showErrorMessage("MSG.SEATS.PASSENGERS_FORBIDDEN_PLACES");
        else
              if ( change_agent_seat_no && exists_preseats && !change_preseat_no )
              AstraLocale::showErrorMessage("MSG.SEATS.PASSENGERS_PRESEAT_PLACES");
          else
            if ( change_agent_seat_no || change_preseat_no )
                    AstraLocale::showErrorMessage("MSG.SEATS.PART_REQUIRED_PLACES_NOT_AVAIL");
      }
      catch(const CheckIn::UserException&)
      {
        throw;
      }
      catch(const AstraLocale::UserException &e)
      {
        throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax_id);
      }

    } // end for paxs
  } //end for k

  //!!!⮫쪮 ��� ॣ����樨 ���ᠦ�஢
  //��।���� �� ���⠬ ���ᠦ�஢ �㦭� �� ������ ������� �ॢ��� �� �
  //�᫨ �㦭� ������ �������
  if ( pr_do_check_wait_list_alarm )
    SALONS2::check_waitlist_alarm_on_tranzit_routes( grp.point_dep, paxs_external_logged, __FUNCTION__ );
}

void showError(const std::map<int, std::map<int, AstraLocale::LexemaData> > &segs)
{
  if (segs.empty()) throw EXCEPTIONS::Exception("CheckIn::showError: empty segs!");
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc==NULL) return;
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  xmlNodePtr cmdNode = GetNode( "command", resNode );
  if (cmdNode==NULL) cmdNode=NewTextChild( resNode, "command" );
  resNode = ReplaceTextChild( cmdNode, "checkin_user_error" );
  xmlNodePtr segsNode = NewTextChild(resNode, "segments");
  for(std::map<int, std::map<int, AstraLocale::LexemaData> >::const_iterator s=segs.begin(); s!=segs.end(); s++)
  {
    xmlNodePtr segNode = NewTextChild(segsNode, "segment");
    if (s->first!=NoExists)
      NewTextChild(segNode, "point_id", s->first);
    xmlNodePtr paxsNode=NewTextChild(segNode, "passengers");
    for(std::map<int, AstraLocale::LexemaData>::const_iterator pax=s->second.begin(); pax!=s->second.end(); pax++)
    {
      string text, master_lexema_id;
      getLexemaText( pax->second, text, master_lexema_id );
      if (pax->first!=NoExists)
      {
        xmlNodePtr paxNode=NewTextChild(paxsNode, "pax");
        NewTextChild(paxNode, "crs_pax_id", pax->first);
        NewTextChild(paxNode, "error_code", master_lexema_id);
        NewTextChild(paxNode, "error_message", text);
      }
      else
      {
        NewTextChild(segNode, "error_code", master_lexema_id);
        NewTextChild(segNode, "error_message", text);
      }
    }
  }
}

void RegNoGenerator::getUsedRegNo(std::set<UsedRegNo> &usedRegNo) const
{
  usedRegNo.clear();

  TQuery Qry(&OraSession);
  if (_type==Negative)
    Qry.SQLText=
        "SELECT pax.reg_no, pax.grp_id, pax.seats FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND reg_no<0 AND point_dep=:point_id ";
  else
    Qry.SQLText=
        "SELECT pax.reg_no, pax.grp_id, pax.seats FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND reg_no>0 AND point_dep=:point_id ";
  Qry.CreateVariable("point_id", otInteger, _point_id);
  Qry.Execute();

  for(; !Qry.Eof; Qry.Next())
  {
    UsedRegNo curr(Qry.FieldAsInteger("reg_no"),
                   Qry.FieldAsInteger("grp_id"),
                   Qry.FieldAsInteger("seats")==0);

    if ((_type==Negative && (curr.value<-_maxAbsRegNo || curr.value>=0))||
        (_type==Positive && (curr.value<=0 || curr.value>_maxAbsRegNo)))
    {
      LogError(STDLOG) << __FUNCTION__ << ": wrong reg_no=" << curr.value << "!";
      continue;
    }
    const auto res=usedRegNo.emplace(_type==Negative?-curr.value:curr.value, curr.grp_id, curr.without_seat);
    if (!res.second && (res.first->grp_id!=curr.grp_id || res.first->without_seat==curr.without_seat))
      LogError(STDLOG) << __FUNCTION__ << ": reg_no=" << curr.value << " duplicated!";
  }
}

void RegNoGenerator::fillUnusedRanges(const std::set<UsedRegNo> &usedRegNo)
{
  unusedRanges.clear();
  lastUnusedRange=boost::none;

  UsedRegNo prior(0, ASTRA::NoExists, false);
  for(const UsedRegNo& curr : usedRegNo)
  {
    if (prior.hasGap(curr) && curr.value!=prior.value+1)
      unusedRanges.emplace(prior.value+1, curr.value-1);
    prior=curr;
  }

  if (prior.value<_maxAbsRegNo)
  {
    lastUnusedRange=RegNoRange(prior.value+1, _maxAbsRegNo);
    unusedRanges.insert(lastUnusedRange.get());
  }
}

void RegNoGenerator::traceUnusedRanges() const
{
  ostringstream s;
  for(const RegNoRange& range : unusedRanges)
    s << traceStr(range) << " ";
  LogTrace(TRACE5) << s.str();
}

RegNoGenerator::RegNoGenerator(int point_id, Type type, int maxAbsRegNo) :
  _point_id(point_id), _type(type), _maxAbsRegNo(abs(maxAbsRegNo))
{
  set<UsedRegNo> usedRegNo;
  getUsedRegNo(usedRegNo);
  fillUnusedRanges(usedRegNo);
}

RegNoGenerator::RegNoGenerator(const std::set<UsedRegNo> &usedRegNo, Type type, int maxAbsRegNo) :
  _point_id(ASTRA::NoExists), _type(type), _maxAbsRegNo(abs(maxAbsRegNo))
{
  fillUnusedRanges(usedRegNo);
}

boost::optional<RegNoRange> RegNoGenerator::getRange(int count, Strategy strategy) const
{
  if (count<=0 || unusedRanges.empty()) return boost::none;
  try
  {
    if (strategy==AbsoluteDefragAtLast ||
        strategy==DefragAtLast)
    {
      if (lastUnusedRange && lastUnusedRange.get().count>=count)
        throw lastUnusedRange.get();
    }

    std::set<RegNoRange>::const_iterator r=unusedRanges.lower_bound(RegNoRange(1,count));
    for(; r!=unusedRanges.end(); ++r)
    {
      const RegNoRange& range=*r;
      if ((strategy==AbsoluteDefragAnytime ||
           strategy==AbsoluteDefragAtLast) && range.count!=count) break;
      if (range.count>=count) throw range;
    }

    if (strategy==AbsoluteDefragAnytime)
    {
      if (lastUnusedRange && lastUnusedRange.get().count>=count)
        throw lastUnusedRange.get();
    }
  }
  catch(RegNoRange &result)
  {
    if (_type==Negative) result.first_no=-result.first_no;
    return result;
  }

  return boost::none;
}

std::string RegNoGenerator::traceStr(const boost::optional<RegNoRange>& range)
{
  ostringstream s;
  if (!range)
    s << "[boost::none]";
  else
    s << "[" << range.get().first_no << ":"
      << range.get().first_no+(range.get().first_no<0?-1:1)*(range.get().count-1) << "]";
  return s.str();
}

} //namespace CheckIn

TWebTids& TWebTids::fromDB(TQuery &Qry)
{
  clear();
  if (Qry.GetFieldIndex("crs_pnr_tid")>=0 && !Qry.FieldIsNULL("crs_pnr_tid"))
    crs_pnr_tid = Qry.FieldAsInteger("crs_pnr_tid");
  if (Qry.GetFieldIndex("crs_pax_tid")>=0 && !Qry.FieldIsNULL("crs_pax_tid"))
    crs_pax_tid = Qry.FieldAsInteger("crs_pax_tid");
  if (Qry.GetFieldIndex("pax_grp_tid")>=0 && !Qry.FieldIsNULL("pax_grp_tid"))
    pax_grp_tid = Qry.FieldAsInteger("pax_grp_tid");
  if (Qry.GetFieldIndex("pax_tid")>=0 && !Qry.FieldIsNULL("pax_tid"))
    pax_tid = Qry.FieldAsInteger("pax_tid");
  return *this;
}

TWebTids& TWebTids::fromXML(xmlNodePtr node)
{
  clear();
  if (node==nullptr) return *this;
  xmlNodePtr node2=NodeAsNode("tids", node)->children;

  crs_pnr_tid=NodeAsIntegerFast("crs_pnr_tid", node2);
  crs_pax_tid=NodeAsIntegerFast("crs_pax_tid", node2);
  xmlNodePtr tidNode;
  tidNode=GetNodeFast("pax_grp_tid", node2);
  if (tidNode!=nullptr && !NodeIsNULL(tidNode)) pax_grp_tid=NodeAsInteger(tidNode);
  tidNode=GetNodeFast("pax_tid", node2);
  if (tidNode!=nullptr && !NodeIsNULL(tidNode)) pax_tid=NodeAsInteger(tidNode);

  return *this;
}

const TWebTids& TWebTids::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  xmlNodePtr tidsNode=NewTextChild(node, "tids");

  NewTextChild(tidsNode, "crs_pnr_tid" , crs_pnr_tid, ASTRA::NoExists);
  NewTextChild(tidsNode, "crs_pax_tid" , crs_pax_tid, ASTRA::NoExists);
  NewTextChild(tidsNode, "pax_grp_tid" , pax_grp_tid, ASTRA::NoExists);
  NewTextChild(tidsNode, "pax_tid"     , pax_tid    , ASTRA::NoExists);

  return *this;
}

TWebAPISItem& TWebAPISItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==nullptr) return *this;
  xmlNodePtr node2=node->children;

  xmlNodePtr docNode = GetNodeFast("document", node2);
  if (docNode!=nullptr)
  {
    doc.fromWebXML(docNode);
    presentAPITypes.insert(apiDoc);
  }

  xmlNodePtr docoNode = GetNodeFast("doco", node2);
  if (docoNode!=nullptr)
  {
    doco.fromWebXML(docoNode);
    presentAPITypes.insert(apiDoco);
  }

  return *this;
}

void TWebAPISItem::set(const CheckIn::TPaxAPIItem& item)
{
  TAPIType type=item.apiType();
  try
  {
    switch(type)
    {
      case apiDoc:
        doc=static_cast<const CheckIn::TPaxDocItem&>(item);
        break;
      case apiDoco:
        doco=static_cast<const CheckIn::TPaxDocoItem&>(item);
        break;
      case apiDocaB:
      case apiDocaD:
      case apiDocaR:
        doca_map[type]=static_cast<const CheckIn::TPaxDocaItem&>(item);
        break;
      default:
        throw EXCEPTIONS::Exception("TWebAPISItem::set: wrong type=%d", type);
    }
  }
  catch(const bad_cast&)
  {
    throw EXCEPTIONS::Exception("TWebAPISItem::set: wrong item for type=%d", type);
  }
  presentAPITypes.insert(type);
}

TWebPaxFromReq& TWebPaxFromReq::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  TWebTids::fromDB(Qry);
  return *this;
}

bool TWebPaxFromReq::isRemProcessingAllowed(const CheckIn::TPaxFQTItem& fqt)
{
  return fqt.rem=="FQTV";
}

bool TWebPaxFromReq::isRemProcessingAllowed(const CheckIn::TPaxRemItem& rem)
{
  return rem.code=="JMP";
}

bool TWebPaxFromReq::mergePaxFQT(std::set<CheckIn::TPaxFQTItem> &dest) const
{
  return mergeRems< std::set<CheckIn::TPaxFQTItem> >(fqtv_rems, dest);
}

bool TWebPaxFromReq::mergePaxRem(CheckIn::PaxRems &dest) const
{
  return mergeRems< CheckIn::PaxRems >(rems, dest);
}

void TWebPaxFromReq::paxFQTFromXML(xmlNodePtr paxNode)
{
  remsFromXML< std::set<CheckIn::TPaxFQTItem>, CheckIn::TPaxFQTItem >(paxNode, fqtv_rems);
}

void TWebPaxFromReq::paxRemFromXML(xmlNodePtr paxNode)
{
  remsFromXML< CheckIn::PaxRems, CheckIn::TPaxRemItem >(paxNode, rems);
}

TWebPaxFromReq& TWebPaxFromReq::fromXML(xmlNodePtr paxNode)
{
  clear();
  if (paxNode==nullptr) return *this;

  xmlNodePtr node2=paxNode->children;

  if (GetNodeFast("passengerAlreadyChecked", node2)!=nullptr)
    passengerAlreadyChecked=NodeAsBooleanFast("passengerAlreadyChecked", node2);
  else
    TWebTids::fromXML(paxNode);

  id=NodeAsIntegerFast("crs_pax_id", node2);
  dont_check_payment=NodeAsBooleanFast("dont_check_payment", node2, false);
  seat_no=NodeAsStringFast("seat_no", node2, "");

  apis.fromXML(paxNode);

  paxFQTFromXML(paxNode);
  paxRemFromXML(paxNode);

  refuse=NodeAsBooleanFast("refuse", node2, false);

  return *this;
}

const TWebPaxFromReq& TWebPaxFromReqList::get(int id, const std::string& whence) const
{
  for(const TWebPaxFromReq& pax : *this)
    if (pax.id==id) return pax;
  throw EXCEPTIONS::Exception("%s: TWebPaxFromReqList: passenger not found (id=%d)", whence.c_str(), id);
}

void TWebPaxForChngList::checkUniquenessAndAdd(const TWebPaxForChng& newPax)
{
  for(const TWebPaxForChng& pax : *this)
    if (pax.paxId()==newPax.paxId())
      throw EXCEPTIONS::Exception("�assengers are duplicated (id=%d)", newPax.paxId());
  push_back(newPax);
}

void TWebPaxForCkinList::checkUniquenessAndAdd(const TWebPaxForCkin& newPax)
{
  for(const TWebPaxForCkin& pax : *this)
    if (pax.paxId()==newPax.paxId())
      throw EXCEPTIONS::Exception("�assengers are duplicated (id=%d)", newPax.paxId());
  push_back(newPax);
}

void TWebPaxForSaveSeg::checkAndSortPaxForCkin(const TWebPaxForSaveSeg& seg)
{
  if (paxForCkin.empty() || seg.paxForCkin.empty()) return;

  if (paxForCkin.size()!=seg.paxForCkin.size())
    throw EXCEPTIONS::Exception("%s: Different number of passengers on segments for through check-in (point_id=%d)",
                                __FUNCTION__, point_id);

  TWebPaxForCkinList::const_iterator iPax=seg.paxForCkin.begin();
  for(;iPax!=seg.paxForCkin.end();iPax++)
  {
    TWebPaxForCkinList::iterator iPax2=find(paxForCkin.begin(),paxForCkin.end(),*iPax);
    if (iPax2==paxForCkin.end())
      throw EXCEPTIONS::Exception("Passenger not found (point_id=%d, %s)",
                                  point_id, iPax->traceStr().c_str());

    TWebPaxForCkinList::iterator iPax3=iPax2;
    if (find(++iPax3,paxForCkin.end(),*iPax)!=paxForCkin.end())
      throw EXCEPTIONS::Exception("�assengers are duplicated (point_id=%d, %s)",
                                  point_id, iPax->traceStr().c_str());

    paxForCkin.splice(paxForCkin.end(),paxForCkin,iPax2,iPax3); //��६�頥� ���������� ���ᠦ�� � �����
  }
}

void TWebPaxForSaveSegs::checkAndSortPaxForCkin()
{
  TWebPaxForSaveSegs::const_iterator firstSegmentForCkin=begin();
  for(; firstSegmentForCkin!=end(); ++firstSegmentForCkin)
    if (!firstSegmentForCkin->paxForCkin.empty()) break;

  if (firstSegmentForCkin==end()) return;

  for(TWebPaxForSaveSegs::iterator s=begin(); s!=end(); ++s)
  {
    if (firstSegmentForCkin==s) continue;
    s->checkAndSortPaxForCkin(*firstSegmentForCkin);
  }
}

void TWebPaxForSaveSegs::checkSegmentsFromReq(int& firstPointIdForCkin)
{
  firstPointIdForCkin=ASTRA::NoExists;

  int prevNotCheckedCount=NoExists;
  set<int> pointIds;
  for(TWebPaxForSaveSeg& s : *this)
  {
    if (!pointIds.insert(s.point_id).second)
      throw EXCEPTIONS::Exception("Segments are duplicated (point_id=%d)", s.point_id);

    int currNotCheckedCount=s.paxFromReq.notCheckedCount();

    if (currNotCheckedCount==0) continue;

    if (firstPointIdForCkin==NoExists) firstPointIdForCkin=s.point_id;

    if (prevNotCheckedCount!=NoExists && prevNotCheckedCount!=currNotCheckedCount)
      throw EXCEPTIONS::Exception("Different number of passengers on segments for through check-in (point_id=%d)",
                                  s.point_id);

    prevNotCheckedCount=currNotCheckedCount;
  };
}

TWebPaxForChng& TWebPaxForChng::fromDB(TQuery &Qry)
{
  clear();
  CheckIn::TSimplePaxGrpItem::fromDB(Qry);
  CheckIn::TSimplePaxItem::fromDB(Qry);
  TWebTids::fromDB(Qry);
  CheckIn::TSimplePaxGrpItem::tid=TWebTids::pax_grp_tid;
  CheckIn::TSimplePaxItem::tid=TWebTids::pax_tid;
  return *this;
}

TWebPaxForCkin& TWebPaxForCkin::fromDB(TQuery &Qry)
{
  clear();
  CheckIn::TSimplePnrItem::fromDB(Qry);
  CheckIn::TSimplePaxItem::fromDBCrs(Qry, true);
  TWebTids::fromDB(Qry);
  if (isTest())
  {
    pnr_addrs.emplace_back(Qry.FieldAsString("pnr_airline"),
                           Qry.FieldAsString("pnr_addr"));
    apis.doc.no = Qry.FieldAsString("doc_no");
  }
  else
  {
    pnr_addrs.getByPaxId(paxId());
    CheckIn::LoadCrsPaxDoc(paxId(), apis.doc);
    CheckIn::LoadCrsPaxVisa(paxId(), apis.doco);
    CheckIn::LoadCrsPaxDoca(paxId(), apis.doca_map);
  }
  return *this;
}

void CheckSeatNoFromReq(int point_id,
                        int crs_pax_id,
                        const string &prior_seat_no,
                        const string &curr_seat_no,
                        string &curr_xname,
                        string &curr_yname,
                        bool &changed)
{
  curr_xname.clear();
  curr_yname.clear();
  changed=false;
  if (curr_seat_no.empty()) return;

  getXYName( point_id, curr_seat_no, curr_xname, curr_yname );
    if ( curr_xname.empty() && curr_yname.empty() )
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_FOUND" );

  if (!prior_seat_no.empty())
  {
    // ���� ������������ ��஥ � ����� ����, �ࠢ���� ��, �᫨ ��������, � �맢��� ���ᠤ��
    string prior_xname, prior_yname;
    getXYName( point_id, prior_seat_no, prior_xname, prior_yname );

    if ( prior_xname + prior_yname != curr_xname + curr_yname )
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
/*      �� �� ��� � ���������஢����� ��⮤�, �� IntChangeSeatsN � IntChangeSeats �� ࠡ����  � cltPNLBeforePay � cltPNLAfterPay
 *      ��������, ��⮬ �����⨬*/
        "DECLARE "
        "vseat_xname crs_pax.seat_xname%TYPE; "
        "vseat_yname crs_pax.seat_yname%TYPE; "
        "vseats      crs_pax.seats%TYPE; "
        "vpoint_id   crs_pnr.point_id%TYPE; "
        "BEGIN "
        "  SELECT crs_pax.seat_xname, crs_pax.seat_yname, crs_pax.seats, crs_pnr.point_id "
        "  INTO vseat_xname, vseat_yname, vseats, vpoint_id "
        "  FROM crs_pnr,crs_pax "
        "  WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:crs_pax_id AND crs_pax.pr_del=0; "
        "  :crs_seat_no:=salons.get_crs_seat_no(:crs_pax_id, vseat_xname, vseat_yname, vseats, vpoint_id, :layer_type, 'one', 1); "
        "EXCEPTION "
        "  WHEN NO_DATA_FOUND THEN NULL; "
        "END;";
      Qry.CreateVariable("crs_pax_id", otInteger, crs_pax_id);
      Qry.CreateVariable("layer_type", otString, FNull);
      Qry.CreateVariable("crs_seat_no", otString, FNull);
      Qry.Execute();
      TCompLayerType layer_type=DecodeCompLayerType(Qry.GetVariableAsString("layer_type"));
      if (layer_type==cltPNLBeforePay ||
          layer_type==cltPNLAfterPay ||
          layer_type==cltProtBeforePay ||
          layer_type==cltProtAfterPay)
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_COINCIDE_WITH_PREPAID" );
      changed=true;
    };
  };
}

void getForbiddenRemGrp(const TTripInfo &flt, TRemGrp& forbiddenRemGrp)
{
  forbiddenRemGrp.clear();

  switch (TReqInfo::Instance()->client_type)
  {
    case ctWeb:
      forbiddenRemGrp.Load(retFORBIDDEN_WEB, flt.airline);
      break;
    case ctKiosk:
      forbiddenRemGrp.Load(retFORBIDDEN_KIOSK, flt.airline);
      break;
    case ctMobile:
      forbiddenRemGrp.Load(retFORBIDDEN_MOB, flt.airline);
      break;
    default:
      break;
  }
}

bool forbiddenRemExists(const TTripInfo &flt,
                        const multiset<CheckIn::TPaxRemItem> &rems,
                        boost::optional<TRemGrp>& forbiddenRemGrp)
{
  if (!forbiddenRemGrp)
  {
    forbiddenRemGrp=boost::in_place();
    getForbiddenRemGrp(flt, forbiddenRemGrp.get());
  }
  return forbiddenRemExists(forbiddenRemGrp.get(), rems);
}

void CreateEmulRems(xmlNodePtr paxNode, const multiset<CheckIn::TPaxRemItem> &rems, const set<CheckIn::TPaxFQTItem> &fqts)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    if (isDisabledRem(*r)) continue;
    r->toXML(remsNode);
  }

  //������� fqts
  for(set<CheckIn::TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
    CheckIn::TPaxRemItem(*r, false).toXML(remsNode);
}

void TMultiPNRSegInfo::add(const TAdvTripInfo &flt, const TWebPaxForCkin& pax, bool first_segment)
{
  if (!empty())
  {
    if (_cls!=pax.cl)
      throw UserException("MSG.PASSENGERS.COMM_FLIGHT_DEST_CLASS_NOT_EQUAL");
  }

  if (find(pax.pnrId())!=end()) return;

  auto iRoute=routes.find(flt.point_id);
  if (iRoute==routes.end())
  {
    iRoute=routes.emplace(flt.point_id, TTripRoute()).first;
    iRoute->second.GetRouteAfter( NoExists,
                                  flt.point_id,
                                  flt.point_num,
                                  flt.first_point,
                                  flt.pr_tranzit,
                                  trtNotCurrent,
                                  trtNotCancelled );
  }
  if (iRoute==routes.end())
    throw EXCEPTIONS::Exception("%s: iRoute==routes.end()!", __FUNCTION__);


  WebSearch::TPNRSegInfo& seg=emplace(pax.pnrId(), WebSearch::TPNRSegInfo()).first->second;

  boost::optional<TTripRouteItem> arv=pax.isTest()?iRoute->second.getFirstAirp():
                                                   iRoute->second.findFirstAirp(pax.airp_arv);
  if (!arv)
    throw UserException("MSG.FLIGHT.DEST_AIRP_NOT_FOUND");

  seg.point_dep=flt.point_id;
  seg.point_arv=arv.get().point_id;
  seg.pnr_id=pax.pnrId();
  seg.pnr_addrs=pax.pnr_addrs;
  if (!pax.isTest() && pax.pnrId()!=ASTRA::NoExists)
  {
    seg.mktFlight=TMktFlight();
    seg.mktFlight.get().getByPnrId(pax.pnrId());
    if (seg.mktFlight.get().empty())
      throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }

  if (first_segment)
  {
    //�஢�ਬ �����
    if (!WebSearch::TPNRFilter::userAccessIsAllowed(flt, boost::optional<TSimpleMktFlight>(seg.mktFlight)))
      throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
  }

  if (size()>1)
  {
    if (!(_point_arv==seg.point_arv &&
          _cls==pax.cl &&
          _mktFlight==seg.mktFlight))
      throw UserException("MSG.PASSENGERS.COMM_FLIGHT_DEST_CLASS_NOT_EQUAL");
  }
  else
  {
    _point_arv=seg.point_arv;
    _cls=pax.cl;
    _mktFlight=seg.mktFlight;
  }
}

void TMultiPnrData::checkJointCheckInAndComplete()
{
  if (segs.empty()) return;
  TFlightRbd flightRbd(flt.oper);
  std::list<AstraLocale::LexemaData> errors;
  for(TMultiPNRSegInfo::const_iterator i=segs.begin(); i!=segs.end(); ++i)
  {
    if (i==segs.begin()) continue;

    if (!WebSearch::TPNRSegInfo::isJointCheckInPossible(segs.begin()->second, i->second, flightRbd, errors))
      throw UserException(errors.front().lexema_id, errors.front().lparams);
  }
  dest.fromDB(segs.begin()->second.point_arv, true);
}

TMultiPnrData& TMultiPnrDataSegs::add(int point_id, bool first_segment, bool with_additional)
{
  emplace_back();
  TMultiPnrData& multiPnrData=back();
  multiPnrData.flt.fromDB(point_id, true);
  if (with_additional)
    multiPnrData.flt.fromDBadditional(first_segment, true);
  return multiPnrData;
}


void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc)
{
  emulDoc.set("term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //�����㥬 ��������� ⥣ query
  xmlNodePtr node=NodeAsNode("/term/query",emulDoc.docPtr())->children;
  if (node!=NULL)
  {
    xmlUnlinkNode(node);
    xmlFreeNode(node);
  };
}

void CreateEmulXMLDoc(XMLDoc &emulDoc)
{
  emulDoc.set("term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  /*xmlNodePtr node=*/NewTextChild(NodeAsNode("/term",emulDoc.docPtr()),"query");
}

void CopyEmulXMLDoc(const XMLDoc &srcDoc, XMLDoc &destDoc)
{
  destDoc.set("term");
  if (destDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CopyEmulXMLDoc: CreateXMLDoc failed");
  xmlNodePtr destNode=NodeAsNode("/term",destDoc.docPtr());
  xmlNodePtr srcNode=NodeAsNode("/term",srcDoc.docPtr())->children;
  for(;srcNode!=NULL;srcNode=srcNode->next)
    CopyNode(destNode, srcNode, true); //�����㥬 ��������� XML
}

void changeSeatsAndUpdateTids(TWebPaxForSaveSegs &segs)
{
  for(TWebPaxForSaveSeg& currSeg : segs)
    for(TWebPaxForChng& currPaxForChng : currSeg.paxForChng)
    try
    {
      const TWebPaxFromReq& currPaxFromReq=currSeg.paxFromReq.get(currPaxForChng.paxId(), __FUNCTION__);

      //���ᠦ�� ��ॣ����஢��
      if (!currPaxFromReq.refuse &&!currPaxFromReq.seat_no.empty() && currPaxForChng.seats > 0)
      {
        string curr_xname, curr_yname;
        bool changed;
        CheckSeatNoFromReq(currPaxForChng.point_dep,
                           currPaxForChng.paxId(),
                           currPaxForChng.seat_no,
                           currPaxFromReq.seat_no,
                           curr_xname,
                           curr_yname,
                           changed);
        if (changed)
        {
          IntChangeSeatsN( currPaxForChng.point_dep,
                           currPaxForChng.paxId(),
                           currPaxForChng.pax_tid,
                           curr_xname, curr_yname,
                           SEATS2::stReseat,
                           cltUnknown,
                           NoExists,
                           BitSet<SEATS2::TChangeLayerFlags>(),
                           0, NoExists,
                           NULL, NULL,
                           __func__ );
          static_cast<CheckIn::TSimplePaxItem&>(currPaxForChng).tid=currPaxForChng.pax_tid;
        };
      }
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), currSeg.point_id, currPaxForChng.paxId());
    }

}

void CreateEmulDocs(const TWebPaxForSaveSegs &segs,
                    const XMLDoc &emulDocHeader,
                    map<int,XMLDoc> &emulChngDocs)
{
  for(const TWebPaxForSaveSeg& currSeg : segs)
    for(const TWebPaxForChng& currPaxForChng : currSeg.paxForChng)
    try
    {
      const TWebPaxFromReq& currPaxFromReq=currSeg.paxFromReq.get(currPaxForChng.paxId(), __FUNCTION__);

      bool DocUpdatesPending=false;
      if (currPaxForChng.apis.isPresent(apiDoc)) //⥣ <document> ��襫
      {
        CheckIn::TPaxDocItem prior_doc;
        LoadPaxDoc(currPaxForChng.paxId(), prior_doc);
        DocUpdatesPending=!(prior_doc.equal(currPaxForChng.apis.doc)); //ॠ���㥬 ⠪�� �� ��������� scanned_attrs
      };

      bool DocoUpdatesPending=false;
      if (currPaxForChng.apis.isPresent(apiDoco)) //⥣ <doco> ��襫
      {
        CheckIn::TPaxDocoItem prior_doco;
        LoadPaxDoco(currPaxForChng.paxId(), prior_doco);
        DocoUpdatesPending=!(prior_doco.equal(currPaxForChng.apis.doco)); //ॠ���㥬 ⠪�� �� ��������� scanned_attrs
      };

      bool RemUpdatesPending=false;
      set<CheckIn::TPaxFQTItem> fqts;
      CheckIn::PaxRems rems;
      if (currPaxFromReq.fqtv_rems || currPaxFromReq.rems) //⥣ <fqt_rems> ��� ⥣ <rems> ��襫
      {
        CheckIn::LoadPaxFQT(currPaxForChng.paxId(), fqts);
        CheckIn::LoadPaxRem(currPaxForChng.paxId(), rems);
        if (currPaxFromReq.mergePaxFQT(fqts))
          RemUpdatesPending=true;
        if (currPaxFromReq.mergePaxRem(rems))
          RemUpdatesPending=true;
      };

      if (currPaxFromReq.refuse ||
          DocUpdatesPending ||
          DocoUpdatesPending ||
          RemUpdatesPending)
      {
        //�ਤ���� �맢��� �࠭����� �� ������ ���������
        XMLDoc &emulChngDoc=emulChngDocs[currPaxForChng.grp_id];
        if (emulChngDoc.docPtr()==NULL)
        {
          CopyEmulXMLDoc(emulDocHeader, emulChngDoc);

          xmlNodePtr emulChngNode=NodeAsNode("/term/query",emulChngDoc.docPtr());
          emulChngNode=NewTextChild(emulChngNode,"TCkinSavePax");

          xmlNodePtr segNode=NewTextChild(NewTextChild(emulChngNode,"segments"),"segment");
          static_cast<const CheckIn::TSimplePaxGrpItem&>(currPaxForChng).toEmulXML(emulChngNode, segNode);
          NewTextChild(segNode,"passengers");
        };
        xmlNodePtr paxsNode=NodeAsNode("/term/query/TCkinSavePax/segments/segment/passengers",emulChngDoc.docPtr());

        xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
        CheckIn::TSimplePaxItem pax(currPaxForChng);
        pax.refuse=currPaxFromReq.refuse?refuseAgentError:"";
        bool PaxUpdatesPending=currPaxFromReq.refuse ||
                               DocUpdatesPending ||
                               DocoUpdatesPending;
        pax.toEmulXML(paxNode, PaxUpdatesPending);

        if (DocUpdatesPending)
          currPaxForChng.apis.doc.toXML(paxNode);

        if (DocoUpdatesPending)
          currPaxForChng.apis.doco.toXML(paxNode);

        if (RemUpdatesPending)
          CreateEmulRems(paxNode, rems, fqts);
      };
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), currSeg.point_id, currPaxForChng.paxId());
    }
}

void CreateEmulDocs(const TWebPaxForSaveSegs &segs,
                    const TMultiPnrDataSegs &multiPnrDataSegs,
                    const XMLDoc &emulDocHeader,
                    XMLDoc &emulCkinDoc)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  //��⠢�塞 XML-�����
  TMultiPnrDataSegs::const_iterator iPnrData=multiPnrDataSegs.begin();
  for(const TWebPaxForSaveSeg& currSeg : segs)
  {
    if (currSeg.paxForCkin.empty()) continue;

    try
    {
      if (iPnrData==multiPnrDataSegs.end()) //��譨� ᥣ����� � ����� �� ॣ������
        throw EXCEPTIONS::Exception("%s: iPnrData==multiPnrDataSegs.end() (point_id=%d)", __FUNCTION__, currSeg.point_id);
      if (iPnrData->segs.empty()) //��� ���ଠ樨 � pnr
        throw EXCEPTIONS::Exception("%s: iPnrData->segs.empty() (point_id=%d)", __FUNCTION__, currSeg.point_id);

      if (emulCkinDoc.docPtr()==NULL)
      {
        CopyEmulXMLDoc(emulDocHeader, emulCkinDoc);
        xmlNodePtr emulCkinNode=NodeAsNode("/term/query",emulCkinDoc.docPtr());
        emulCkinNode=NewTextChild(emulCkinNode,"TCkinSavePax");
        NewTextChild(emulCkinNode,"transfer"); //���⮩ ⥣ - �࠭��� ���
        NewTextChild(emulCkinNode,"segments");
        NewTextChild(emulCkinNode,"hall");
      };
      xmlNodePtr segsNode=NodeAsNode("/term/query/TCkinSavePax/segments",emulCkinDoc.docPtr());

      xmlNodePtr segNode=NewTextChild(segsNode, "segment");
      NewTextChild(segNode,"point_dep",iPnrData->flt.oper.point_id);
      NewTextChild(segNode,"point_arv",iPnrData->dest.point_arv);
      NewTextChild(segNode,"airp_dep",iPnrData->flt.oper.airp);
      NewTextChild(segNode,"airp_arv",iPnrData->dest.airp_arv);
      NewTextChild(segNode,"class",iPnrData->segs.origClass());
      NewTextChild(segNode,"status",EncodePaxStatus(currSeg.paxForCkin.status));
      NewTextChild(segNode,"wl_type");

      //�������᪨� ३� PNR
      TTripInfo pnrMarkFlt;
      iPnrData->segs.begin()->second.getMarkFlt(iPnrData->flt, pnrMarkFlt);
      TCodeShareSets codeshareSets;
      codeshareSets.get(iPnrData->flt.oper,pnrMarkFlt);

      xmlNodePtr node=NewTextChild(segNode,"mark_flight");
      NewTextChild(node,"airline",pnrMarkFlt.airline);
      NewTextChild(node,"flt_no",pnrMarkFlt.flt_no);
      NewTextChild(node,"suffix",pnrMarkFlt.suffix);
      NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //�����쭠� ���
      NewTextChild(node,"airp_dep",pnrMarkFlt.airp);
      NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);

      //�ਧ��� ����� ᠬ�ॣ����樨 ��� �롮� ����
      bool with_seat_choice=false;
      if (reqInfo->isSelfCkinClientType())
        with_seat_choice=GetSelfCkinSets(tsRegWithSeatChoice, iPnrData->flt.oper, reqInfo->client_type);

      xmlNodePtr paxsNode=NewTextChild(segNode,"passengers");
      for(const TWebPaxForCkin& currPaxForCkin : currSeg.paxForCkin)
      try
      {
        const TWebPaxFromReq& currPaxFromReq=currSeg.paxFromReq.get(currPaxForCkin.paxId(), __FUNCTION__);

        if (!currPaxFromReq.seat_no.empty())
        {
          string curr_xname, curr_yname;
          bool changed;
          CheckSeatNoFromReq(iPnrData->flt.oper.point_id,
                             currPaxForCkin.paxId(),
                             currPaxForCkin.seat_no,
                             currPaxFromReq.seat_no,
                             curr_xname,
                             curr_yname,
                             changed);
        }
        else
        {
          if (currPaxForCkin.seats>0 && with_seat_choice)
            throw UserException("MSG.CHECKIN.NEED_TO_SELECT_SEAT_NO");
        };

        xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
        if (currPaxForCkin.paxId()!=NoExists)
          NewTextChild(paxNode,"pax_id",currPaxForCkin.paxId());
        else
          NewTextChild(paxNode,"pax_id");
        NewTextChild(paxNode,"surname",currPaxForCkin.surname);
        NewTextChild(paxNode,"name",currPaxForCkin.name);
        NewTextChild(paxNode,"pers_type",EncodePerson(currPaxForCkin.pers_type));
//        NewTextChild(paxNode,"crew_type",CrewTypes().encode(currPaxForCkin.crew_type)); //ᥩ�� �� ������� �� ���⠢�塞 pax.crew_type ��� �����, ����� ���� ��⮬....
        if (!currPaxFromReq.seat_no.empty())
          NewTextChild(paxNode,"seat_no",currPaxFromReq.seat_no);
        else
          NewTextChild(paxNode,"seat_no",currPaxForCkin.seat_no);
        NewTextChild(paxNode,"seat_type",currPaxForCkin.seat_type);
        NewTextChild(paxNode,"seats",currPaxForCkin.seats);
        //��ࠡ�⪠ ����⮢
        currPaxForCkin.tkn.toXML(paxNode);
        currPaxForCkin.apis.doc.toXML(paxNode);
        currPaxForCkin.apis.doco.toXML(paxNode);
        currPaxForCkin.apis.doca_map.toXML(paxNode);

        NewTextChild(paxNode,"subclass",currPaxForCkin.subcl);
        NewTextChild(paxNode,"transfer"); //���⮩ ⥣ - �࠭��� ���
        NewTextChild(paxNode,"bag_pool_num");
        if (currPaxForCkin.reg_no!=NoExists)
          NewTextChild(paxNode,"reg_no",currPaxForCkin.reg_no);

        //६�ન
        multiset<CheckIn::TPaxRemItem> rems;
        CheckIn::LoadCrsPaxRem(currPaxForCkin.paxId(), rems);
        currPaxFromReq.mergePaxRem(rems);
        set<CheckIn::TPaxFQTItem> fqts;
        CheckIn::LoadCrsPaxFQT(currPaxForCkin.paxId(), fqts);
        currPaxFromReq.mergePaxFQT(fqts);
        CreateEmulRems(paxNode, rems, fqts);

        NewTextChild(paxNode, "dont_check_payment", (int)currPaxForCkin.dont_check_payment, (int)false);
      }
      catch(const CheckIn::UserException&)
      {
        throw;
      }
      catch(const UserException &e)
      {
        throw CheckIn::UserException(e.getLexemaData(), currSeg.point_id, currPaxForCkin.paxId());
      };
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), currSeg.point_id);
    };

    ++iPnrData;
  }
}

#include "baggage.h"

void tryGenerateBagTags(xmlNodePtr reqNode)
{
  LogTrace(TRACE5) << __FUNCTION__;

  xmlNodePtr segNode=NodeAsNode("segments/segment",reqNode);

  CheckIn::TPaxGrpItem grp;
  if (!grp.fromXML(segNode))
    throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB
  if (grp.id==ASTRA::NoExists) return;

  if (!CheckIn::TGroupBagItem::completeXMLForIatci(grp.id, reqNode, segNode))
    throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

  CheckIn::TGroupBagItem group_bag;
  if (!group_bag.fromXMLsimple(reqNode, grp.baggage_pc)) return;

  group_bag.checkAndGenerateTags(grp.point_dep, grp.id, true);

  group_bag.generatedToXML(reqNode);


  //LogTrace(TRACE5) << XMLTreeToText(reqNode->doc);
}

void createEmulDocForSBDO(int pax_id,
                          const TBagTagNumber& tag,
                          const boost::optional<CheckIn::TSimpleBagItem>& bag,
                          xmlNodePtr emulChngNode)
{
  if (emulChngNode==nullptr)
    throw EXCEPTIONS::Exception("%s: emulChngNode==nullptr", __FUNCTION__);

  CheckIn::TSimplePaxItem pax;
  if (!pax.getByPaxId(pax_id))
   throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  list<CheckIn::TSimplePaxGrpItem> grps;

  TCkinRoute route;
  route.getRouteAfter(GrpId_t(pax.grp_id),
                      TCkinRoute::WithCurrent,
                      TCkinRoute::OnlyDependent,
                      TCkinRoute::WithoutTransit);

  if (!route.empty())
  {
    for(TCkinRoute::const_iterator s=route.begin(); s!=route.end(); ++s)
    {
      CheckIn::TSimplePaxGrpItem grp;
      if (!grp.getByGrpId(s->grp_id))
        throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
      grps.push_back(grp);
    }
  }
  else
  {
    CheckIn::TSimplePaxGrpItem grp;
    if (!grp.getByGrpId(pax.grp_id))
      throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    grps.push_back(grp);
  }

  if (grps.empty())
    throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  const CheckIn::TSimplePaxGrpItem& first=grps.front();

  CheckIn::TGroupBagItem groupBag;
  groupBag.fromDB(first.id, ASTRA::NoExists, false);

  CheckIn::TSimplePaxItem editablePax(pax);
  if (bag)
    groupBag.add(first.point_dep, tag, bag.get(), editablePax.bag_pool_num);
  else
    groupBag.remove(tag, editablePax.bag_pool_num);

  xmlNodePtr segsNode=NewTextChild(emulChngNode,"segments");

  for(list<CheckIn::TSimplePaxGrpItem>::const_iterator g=grps.begin(); g!=grps.end(); ++g)
  {
    xmlNodePtr segNode=NewTextChild(segsNode,"segment");
    g->toEmulXML(emulChngNode, segNode);

    xmlNodePtr paxsNode=NewTextChild(segNode,"passengers");

    if (g!=grps.begin()) continue;

    if (pax.bag_pool_num!=editablePax.bag_pool_num)
    {
      xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
      editablePax.toEmulXML(paxNode, true);
    }
  }

  groupBag.toXML(emulChngNode, false);
}
