#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include "SeatsPlanter.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "seats.h"
#include "salons.h"
#include "salonform.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "flt_settings.h"
#include "points.h"
#include "dcs_services.h"
#include "comp_layers.h"
#include "term_version.h"
#include "seat_number.h"
#include "checkin.h"
#include "astra_context.h"

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC::date_time;
using namespace RESEAT;

struct ReseatRfiscData {
  enum EnumRFISC { doAdd, doDelete };
  int point_id;
  int pax_id;
  TPaxSegRFISCKey del_rfisc, add_rfisc;
  TSeat del_seat, add_seat;
  void RFISCToXML( xmlNodePtr node,
                   const TPaxSegRFISCKey& rfisc,
                   const TSeat& seat ) {
    xmlNodePtr n = NewTextChild( node, "rfisc" );
    rfisc.toXML(n);
    node = NewTextChild( node, "seat" );
    SetProp( node, "line", seat.line );
    SetProp( node, "row", seat.row );
  }
  void toXML( xmlNodePtr reqNode ) {
    RemoveNode( GetNode("rfiscs", reqNode) );
    xmlNodePtr node = NewTextChild( reqNode, "rfiscs");
    SetProp( node, "point_id", point_id );
    SetProp( node, "pax_id", pax_id );
    RFISCToXML( NewTextChild( node, "delete" ),
                del_rfisc, del_seat );
    RFISCToXML( NewTextChild( node, "add" ),
                add_rfisc, add_seat );
  }
  void RFISCFromXML( xmlNodePtr node,
                     TPaxSegRFISCKey& rfisc,
                     TSeat& seat ) {
    rfisc.fromXML( NodeAsNode( "rfisc", node ) );
    seat = TSeat(NodeAsString("seat/@line", node),
                 NodeAsString("seat/@row", node));
  }
  void fromXML( xmlNodePtr reqNode ) {
    xmlNodePtr node = NodeAsNode( "rfiscs", reqNode );
    point_id = NodeAsInteger( "@point_id", node );
    pax_id = NodeAsInteger( "@pax_id", node );
    RFISCFromXML( NodeAsNode( "delete", node), del_rfisc, del_seat );
    RFISCFromXML( NodeAsNode( "add", node), add_rfisc, add_seat );
  }
};

class ReseatPaxRFISCServiceChanger: public ReseatRfiscData {
  private:
    void doService( TPaxSegRFISCKey& rfisc,
                    TPaidRFISCList& paidRFISC,
                    ReseatRfiscData::EnumRFISC operation ) {
      if ( rfisc.RFISC.empty() ) return;
      try {
        rfisc.getListItemByPaxId(pax_id,0,TServiceCategory::Other,__func__);
      }
      catch(EXCEPTIONS::EConvertError& e) {
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
      }

      for ( TPaidRFISCList::iterator p=paidRFISC.begin(); p!=paidRFISC.end(); ++p ) {
        LogTrace(TRACE5) << p->second.need_positive()
                         << "|" << p->second.paid_positive()
                         << "|" << p->second.need << "|" << p->second.paid;
        if ( !p->second.need_positive() ||
             !p->second.paid_positive() ||
             (operation==ReseatRfiscData::EnumRFISC::doDelete &&
              p->second.need <= 0) )
          continue;
        if ( p->first == rfisc ) {
          p->second.service_quantity = p->second.service_quantity + (operation==ReseatRfiscData::EnumRFISC::doAdd?1:-1);
          if ( p->second.service_quantity == 0 )
            paidRFISC.erase(p);
          return;
        }
      }
      if ( operation==ReseatRfiscData::EnumRFISC::doAdd ) { //�� ����⨫� ����, ⮣�� ������塞
        TPaidRFISCItem item(TGrpServiceItem(rfisc,1));
        paidRFISC.insert( make_pair(rfisc,item));
      }
    }
    xmlNodePtr createTCkinSavePaxQuery(xmlNodePtr reqNode,xmlNodePtr resNode) {
      //������ doc ����� � �ᥬ� ��ਡ�⠬� - ���⮬, ���ᨥ� �ନ���� � ��稬
      LogTrace(TRACE5) << __func__;
      xmlNodePtr node = GetNode("TCkinSavePax", reqNode);
      RemoveNode(node);
      xmlNodePtr emulChngNode = NewTextChild(reqNode, "TCkinSavePax");

      list<CheckIn::TSimplePaxGrpItem> grps;
      CheckIn::TSimplePaxItem pax;
      if (!pax.getByPaxId(pax_id))
        throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
      TCkinRoute route;
      //!!!route.getRouteAfter(GrpId_t(pax.grp_id),
      boost::optional<int> current_grp_num =
      route.getRoute(GrpId_t(pax.grp_id),
                     TCkinRoute::WithCurrent,
                     TCkinRoute::OnlyDependent,
                     TCkinRoute::WithoutTransit);
      LogTrace(TRACE5) << pax_id << " " << pax.grp_id << " " << route.empty();
      const std::vector<PaxGrpRoute> paxRoute = PaxGrpRoute::load(PaxId_t(pax_id),0/*???*/,
                                                                  GrpId_t(pax.grp_id),
                                                                  GrpId_t(route.empty()?pax.grp_id:route.front().grp_id));
      if ( route.empty() ||
           paxRoute.empty() ) //???
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
      tst();
      LogTrace(TRACE5) << "dest pax_id " << paxRoute.front().dest.pax_id <<
                          " grp_id " << paxRoute.front().dest.grp_id <<
                          " seg_no " << paxRoute.front().dest.seg_no;
      LogTrace(TRACE5) << "src pax_id " << paxRoute.front().src.pax_id <<
                          " grp_id " << paxRoute.front().src.grp_id  <<
                          " seg_no " << paxRoute.front().src.seg_no;
      int trfer_num = 0; //!!! ���宩 ����
      if (!route.empty())
      {
        tst();
        bool f = false;
        for(TCkinRoute::const_iterator s=route.begin(); s!=route.end(); ++s)
        {
          CheckIn::TSimplePaxGrpItem grp;
          if (!grp.getByGrpId(s->grp_id))
            throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
          grps.push_back(grp);
          if ( !f && grp.id == pax.grp_id ) {
            f = true;
          }
          if ( !f )
            trfer_num++;
        }
      }
      else //??? ��� ⠪ ����� ����? �� ��-�� ⮣�, �� �⮨� 䫠� TCkinRoute::OnlyDependent � �롨����� ३�� ᪢���猪?
      {
        tst();
        CheckIn::TSimplePaxGrpItem grp;
        if (!grp.getByGrpId(pax.grp_id))
          throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        grps.push_back(grp);
      }
      if (grps.empty())
        throw UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
tst();
      //???const CheckIn::TSimplePaxGrpItem& first=grps.front();
      xmlNodePtr segsNode=NewTextChild(emulChngNode,"segments");
      for(list<CheckIn::TSimplePaxGrpItem>::const_iterator g=grps.begin(); g!=grps.end(); ++g)
      {
          tst();
        xmlNodePtr segNode=NewTextChild(segsNode,"segment");
        g->toEmulXML((g==grps.begin())?emulChngNode:nullptr, segNode);
        NewTextChild(segNode,"passengers");
        if (g!=grps.begin()) continue;
        //TGrpServiceListWithAuto svc; //�� ����� ᯨ᮪ ���?
        //svc.fromDB(g->id);
        TPaidRFISCList paidRFISC;
        //paidRFISC.fromDB(paxRoute.front().dest.grp_id);//�� ������� � �ନ���� � �業���
        paidRFISC.fromDB((int)paxRoute.front().dest.grp_id.get(),true); //�� ������� � �ନ���� � �業���
        tst();
        //del_rfisc - ��� ���� ���� 㤠����, �᫨ ��� �� �� ����祭�
        //add_rfisc - ��� ���� ���� ��������, �᫨ �� ��� ⠪�� �����(�� ����� ����砭� ��� ��� ���
        add_rfisc.pax_id = paxRoute.front().dest.pax_id.get();
        add_rfisc.airline = route.front().operFlt.airline;
        add_rfisc.trfer_num = trfer_num;
        add_rfisc.service_type = TServiceType::FlightRelated;
tst();
        doService( add_rfisc,
                   paidRFISC,
                   ReseatRfiscData::EnumRFISC::doAdd );
        del_rfisc.pax_id = paxRoute.front().dest.pax_id.get();
        del_rfisc.airline = route.front().operFlt.airline;
        del_rfisc.trfer_num = trfer_num;
        del_rfisc.service_type = TServiceType::FlightRelated;
tst();
        doService( del_rfisc,
                   paidRFISC,
                   ReseatRfiscData::EnumRFISC::doDelete );
        //����祭� �� ��㣠? ���� �� 㦥 ⠪��? ���-��?
        xmlNodePtr servsNode = NewTextChild( emulChngNode, "services" );
        for ( auto& p : paidRFISC ) {
          /*if ( !p.second.need_positive()   ||
               !p.second.paid_positive() )
            continue;*/
tst();
          const TGrpServiceItem& s = static_cast<const TGrpServiceItem&>(p.second);
          xmlNodePtr itemNode;
          s.toXML( itemNode=NewTextChild( servsNode, "item" ) );
          RemoveNode(GetNode("name_view",itemNode)); //��� ⥣ ��譨� ��� SavePax
tst();
        }
      }
      tst();
      NewTextChild(emulChngNode, "agent_stat_period", -1);
      //��� ⥣� <segment><bag_refuse/>
      return emulChngNode;
    }
  public:
    ReseatPaxRFISCServiceChanger(){}

    static bool isContinueServices( xmlNodePtr reqNode ) {
      return GetNode( "rfiscs", reqNode ) != nullptr;
    }
    static void toDB( xmlNodePtr reqNode ) {
      if ( !isContinueServices(reqNode) ) return;
//      fromXML(reqNode);
    }

    static std::string getContextName() {
      return TReqInfo::Instance()->desk.code+"reseat";
    }

    static bool reseatRFISCDispatcher(xmlNodePtr reqNode, xmlNodePtr externalSysResNode,
                                      xmlNodePtr resNode ) {
      LogTrace(TRACE5) << __func__ << " isDoomedToWait " << isDoomedToWait();
      LogTrace(TRACE5) << "request " << XMLTreeToText(reqNode->doc);
      if (externalSysResNode)
        LogTrace(TRACE5) << "externalSysResNode " << XMLTreeToText(externalSysResNode->doc);
      LogTrace(TRACE5) << "answer " << XMLTreeToText(resNode->doc);
      ReseatPaxRFISCServiceChanger r;
      std::string value;
      AstraContext::GetContext(getContextName(), 0, value);
      LogTrace(TRACE5) << value;
      XMLDoc doc = createXmlDoc(value);
      r.fromXML(doc.docPtr()->children);
      xmlNodePtr emulChngNode = r.createTCkinSavePaxQuery( reqNode, resNode);
      r.toXML(doc.docPtr()->children);
      LogTrace(TRACE5) <<  doc.text();
      AstraContext::ClearContext(getContextName(),0);
      AstraContext::SetContext(getContextName(),0,doc.text());
      return CheckInInterface::SavePax(emulChngNode, externalSysResNode, nullptr);
    }

    bool reseat( xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode ) {
      LogTrace(TRACE5) << __func__;
      std::string value;
      AstraContext::GetContext(getContextName(), 0, value);
      XMLDoc doc = createXmlDoc(value);
      toXML( doc.docPtr()->children );
      AstraContext::ClearContext(getContextName(),0);
      AstraContext::SetContext(getContextName(),0,doc.text());
      return reseatRFISCDispatcher( reqNode, externalSysResNode, resNode );
    }

};

class SeatsPlanter {
public:
  enum EnumClientSets {
    flWaitList,  // �맢���� ���ଠ�� � ����묨 �� ��
    flQuestionReseat, //������ ����� ���짮��⥫�, �᫨ �� �� ��訢���
    flSetPayLayer, //��������� ᫮� ⥪. ���� ��� ��� ॣ����樨 - ����� (����� ᫮� ��। � ��᫥ ������)
    flSyncCabinClass}; //�⫨砥��� ������� � ���
private:
  inline static const std::string WAITLIST_SET = "waitlist";
  inline static const std::string QUESTION_RESEAT_SET = "question_reseat";
  static const std::map<std::string,EnumClientSets>& pairs() {
    static std::map<std::string,EnumClientSets> m =
    {
      {WAITLIST_SET, flWaitList},
      {QUESTION_RESEAT_SET, flQuestionReseat} // ����� ���������� ⮫쪮 �� ���ᠤ�� ��ॣ����஢������ ���ᠦ��
    };
    return m;
  }
  enum EnumSeatsProps { propNone,
                        propExit, //��� �� �����
                        propRFISC,  //�ਧ��� ⮣�, �� ��࠭��� ���� ����� RFISC
                        propChangeRFISCService,
                        propRFISCQuestion, //�ਧ��� ⮣�, �� �� ������ �㤥� �뢥��� ����� � ���ᠤ�� � RFISC
                        propPriorProtLayerQuestion //�ਧ��� ⮣�, �� �� ������ �뢥����� ����� � ���ᠤ��,
                        //�.�. ���ᠦ�� ������� ᢮� ����祭��� ��� १�ࢭ�� ���� �� ��㣮�
                      };
  enum EnumRecalc { rMust, rNone };
  std::set<TCompLayerType> checkinLayers { cltGoShow, cltTranzit, cltCheckin, cltTCheckin };
private:
  int _point_id;
  std::optional<int> _point_arv;
  int _pax_id;
  int _crcComp;
  int _tid;
  multiset<CheckIn::TPaxRemItem> rems;
  std::optional<int> _tariffPaxId;
  std::optional<TCompLayerType> _layer_type;
  std::optional<TSeat> _seat;
  BitSet<EnumClientSets> _clientSets;
  BitSet<EnumSeatsProps> _resPropsSets;
  BitSet<EnumRecalc> _recalcSets;
private:
  EnumSitDown _seat_type;
  std::optional<int> _time_limit;
private: //�ᯮ����⥫�� ��६����, ����塞� � ����� ���ᠤ��
  TAdvTripInfo fltInfo;
  SALONS2::TSalonList salonList; //��६ ��� ����
  SALONS2::TSalonList NewSalonList;
  std::optional< std::set<SALONS2::TPlace*,SALONS2::CompareSeats> > currSeats;
  std::optional< pair<TPlaceList*,std::set<SALONS2::TPlace*,SALONS2::CompareSeats> > > newSeats;
  std::optional<TCompLayerType> curr_layer_type;
  CheckIn::TSimplePaxItem pax;
  std::optional<CheckIn::TSimplePaxGrpItem> grp;
  std::optional<CheckIn::TSimplePnrItem> pnr;
  BASIC_SALONS::TCompLayerTypes::Enum flagCompLayerPriority;
  TSeatTariffMap passTariffs;
  vector<pair<TSeatRange,TRFISC> > logSeats;
public:
  SeatsPlanter( xmlNodePtr reqNode ) {
    fromXML( reqNode );
  }
  SeatsPlanter( int point_id,
                std::optional<int> point_arv,
                int pax_id,
                int crcComp,
                int tid,
                const std::optional<int>& tariffPaxId,
                const std::optional<TSeat>& seat,
                const std::optional<TCompLayerType>& layer_type,
                const BitSet<EnumClientSets> &clientSets ):
                _point_id(point_id),
                _point_arv(point_arv?std::optional<int>(point_arv.value()):std::nullopt), //�����-� ᫮���
                _pax_id(pax_id),
                _crcComp(crcComp),
                _tid(tid),
                _tariffPaxId(tariffPaxId?std::optional<int>(tariffPaxId.value()):std::nullopt),
                _layer_type(layer_type?std::optional<TCompLayerType>(layer_type.value()):std::nullopt),
                _seat(seat?std::optional<TSeat>(seat.value()):std::nullopt),
                salonList(true),
                NewSalonList(true) {
    _clientSets = clientSets;
  }
  void fromXMLClientSets( const std::string &nameSet,
                          xmlNodePtr reqNode ) {
    if (!GetNode( nameSet.c_str(), reqNode ))
      return;
    const auto &s = pairs().find( nameSet );
    if ( s != pairs().end() ) {
      _clientSets.setFlag(s->second);
    }
  }
  void fromXML( xmlNodePtr reqNode ) {
    LogTrace(TRACE5) << __func__;
    if ( reqNode == nullptr ) return;
    xmlNodePtr node;
    _point_id = NodeAsInteger( "trip_id", reqNode );
    _point_arv = (node=GetNode("point_arv", reqNode))?std::optional<int>(NodeAsInteger(node)):std::nullopt;
    _pax_id = NodeAsInteger( "pax_id", reqNode );
    _crcComp = NodeAsInteger( "comp_crc", reqNode, 0 );
    _tid = NodeAsInteger( "tid", reqNode );
    _tariffPaxId = (node=GetNode("tariff_pax_id", reqNode))?std::optional<int>(NodeAsInteger(node)):std::nullopt;
    _layer_type = (node=GetNode("layer", reqNode))?std::optional<TCompLayerType>(DecodeCompLayerType(NodeAsString(node))):std::nullopt;
    _seat.emplace(TSeat(NodeAsString( "yname", reqNode, "" ),
                        NodeAsString( "xname", reqNode, "" )));
    LogTrace(TRACE5) << ( _seat != std::nullopt );
    LogTrace(TRACE5) << _seat.value().toString();
    fromXMLClientSets(WAITLIST_SET,reqNode);
    fromXMLClientSets(QUESTION_RESEAT_SET,reqNode);
  }
  bool isCheckLayer( const std::optional<TCompLayerType> &layer_type ) {
    return ( layer_type && (checkinLayers.find( layer_type.value() ) != checkinLayers.end()) );
  }
  bool isRemExists( const std::set<std::string>& searchRems ) {
    return ( find_if( rems.begin(), rems.end(), [searchRems](const auto& item)
                        {return (find(searchRems.begin(), searchRems.end(), item.code) != searchRems.end());}) != rems.end() );
  }
  void fromDB() {
    LogTrace(TRACE5) << __func__;
    currSeats.reset();
    curr_layer_type.reset();
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( _point_id,
                                                      _point_arv.value_or(ASTRA::NoExists) ),
                          "", _pax_id );
    std::set<SALONS2::TPlace*,SALONS2::CompareSeats> seats;
    SALONS2::TLayerPrioritySeat layerPrioritySeat = SALONS2::TLayerPrioritySeat::emptyLayer();
    salonList.getPaxLayer( _point_id, _pax_id, layerPrioritySeat, seats );
    if ( layerPrioritySeat.layerType() != cltUnknown ) {
      if ( !seats.empty() )
        currSeats.emplace( seats );
    }
    if ( layerPrioritySeat.layerType() != cltUnknown ) {
      curr_layer_type.emplace( layerPrioritySeat.layerType() );
      //�ନ��� �� 㬥�� ������ ���ᠤ�� ���⭮�� ᫮�, ���⮬� ����� ���� ���� �� �᭮�� ⥪. ᫮� ���� ���ᠦ��
      if ( curr_layer_type.value() == cltProtAfterPay ||
           curr_layer_type.value() == cltPNLAfterPay ) {
        if ( _layer_type && _layer_type.value() == cltProtCkin &&
             BASIC_SALONS::TCompLayerTypes::Instance()->priority(
                      BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, curr_layer_type.value() ), flagCompLayerPriority ) <
             BASIC_SALONS::TCompLayerTypes::Instance()->priority(
                      BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, _layer_type.value() ), flagCompLayerPriority ) )
          _layer_type.emplace( curr_layer_type.value() );
      }
      if ( !_layer_type  ) //᫮� � ����� �� ����� - �� ��, �� � ⥪�騩
        _layer_type.emplace( curr_layer_type.value() );
    }

    LogTrace(TRACE5) << __func__ << " " <<_point_id << " " << _pax_id << " "
                     << EncodeCompLayerType(_layer_type.value())
                     << " isCheckLayer " << isCheckLayer( _layer_type );
    CheckIn::LoadPaxRem( !isCheckLayer( _layer_type ) /*crs*/, _pax_id, rems );
    switch ( _layer_type.value() ) {
      case cltGoShow:
      case cltTranzit:
      case cltCheckin:
      case cltTCheckin:
        if ( !pax.getByPaxId(_pax_id) )
          throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA" );
        tst();
        grp = CheckIn::TSimplePaxGrpItem();
        tst();
        grp.value().getByPaxId(_pax_id);
        tst();
        break;
      case cltProtCkin:
      case cltProtBeforePay: //WEB ChangeProtPaidLayer
      case cltProtAfterPay:
      case cltPNLAfterPay:
      case cltProtSelfCkin:
        if ( !pax.getCrsByPaxId( PaxId_t(_pax_id) ) )
          throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA" );
        pnr = CheckIn::TSimplePnrItem();
        pnr.value().getByPaxId(_pax_id);
        break;
      default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( _layer_type.value() ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
    flagCompLayerPriority = (GetTripSets( tsAirlineCompLayerPriority, fltInfo )?
                                BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
    tst();
  }
private:
  void checkComponChanged() {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return;
    int ncomp_crc = CompCheckSum::keyFromDB( _point_id ).total_crc32; //��६ �� ��, �� ���� ������ ����뢠��
    if ( (_crcComp != 0 && ncomp_crc != 0 && _crcComp != ncomp_crc) ) { //update
      throw UserException( "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH" );
    }
  }
  void checkBabyEmergencySeats() {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return;
    TEmergencySeats emergencySeats( _point_id );
    TBabyZones  babyZones( _point_id );
    TPaxsCover grpPaxs;
    grpPaxs.emplace_back( TPaxCover( _pax_id, ASTRA::NoExists ) );
    std::set<int> pax_with_baby_lists;
    std::map<int,TPaxList>::const_iterator ipaxs = salonList.pax_lists.find( _point_id );
    if ( ipaxs != salonList.pax_lists.end() ) {
      ipaxs->second.setPaxWithInfant( pax_with_baby_lists );
    }
    if ( emergencySeats.deniedEmergencySection() ||
         babyZones.useInfantSection() ) {
      if ( emergencySeats.deniedEmergencySection() &&
           ( pax.pers_type != ASTRA::adult ||
             pax_with_baby_lists.find( _pax_id ) != pax_with_baby_lists.end() ||
             isRemExists(std::set<std::string>{"BLND","STCR","UMNR","WCHS","MEDA"}) ) )
         emergencySeats.setDisabledEmergencySeats( nullptr, newSeats.value().first,
                                                   grpPaxs, TEmergencySeats::DisableMode::dlrss );
      if ( babyZones.useInfantSection() &&
           pax_with_baby_lists.find( _pax_id ) != pax_with_baby_lists.end() )
        babyZones.setDisabledBabySection( _point_id, nullptr, newSeats.value().first,
                                          grpPaxs, pax_with_baby_lists, TBabyZones::DisableMode::dlrss );
    }
    //�஡�� �� ���� ���⠬ � �஢�ઠ �� ����������� ��ᠤ��� �㤠 ���ᠦ��
    try {
      for ( const auto& seat : newSeats.value().second ) {
        if ( seat->isLayer( cltDisable ) ) {
         emergencySeats.in( newSeats.value().first->num, seat )
          ?throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" )
          :throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL_WITH_INFT" );
        }
      }
    }
    catch(...) {
      babyZones.rollbackDisabledBabySection();
      emergencySeats.rollbackDisabledEmergencySeats();
      throw;
    }
    babyZones.rollbackDisabledBabySection();
    emergencySeats.rollbackDisabledEmergencySeats();

    //���� �஢�ઠ �� ���� �� � ���਩���� ��室�
    SEATS2::TAllowedAttributesSeat AllowedAttrsSeat;
    if ( TReqInfo::Instance()->client_type != ctTerm &&
         TReqInfo::Instance()->client_type != ctPNL &&
         !_clientSets.isFlag( flSetPayLayer ) && // ࠧ��⪠ ����� ᫮�� �१
         AllowedAttrsSeat.isWorkINFT( _point_id ) &&
         !AllowedAttrsSeat.passSeats( EncodePerson( pax.pers_type ),
                                      pax_with_baby_lists.find( _pax_id ) != pax_with_baby_lists.end(),
                                      algo::transform<std::vector>(newSeats.value().second, [](const auto& s) { return *s; }) ) ) { //web-���ᠤ�� INFT ����饭�
      tst();
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
  }
  EnumSeatsProps checkChangeSeatsForPay() {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return EnumSeatsProps::propNone;
    std::set<TCompLayerType> layers = { cltProtBeforePay, cltProtAfterPay };
    if ( _clientSets.isFlag( flSetPayLayer ) &&
         layers.find( _layer_type.value() ) != layers.end() &&
         currSeats ) { // �뫨 ����� ���� - �� ������ ����������!!!
      ProgTrace( TRACE5, "seats.size()=%zu, nseats.size()=%zu", currSeats.value().size(), newSeats.value().second.size() );
      //�ࠢ������
      if ( currSeats.value().size() != newSeats.value().second.size() ||
           currSeats.value() != newSeats.value().second ) {
        tst();
        throw  UserException( "MSG.SEATS.SEAT_NO.NOT_COINCIDE_WITH_PREPAID" );
      }
      if ( TReqInfo::Instance()->client_type != ctWeb &&
           TReqInfo::Instance()->client_type != ctMobile )
        return EnumSeatsProps::propExit;
    }
    return EnumSeatsProps::propNone;
  }
  void checkPreseatingAccess()  {
    if ( (_layer_type.value() == cltProtAfterPay ||
          _layer_type.value() == cltPNLAfterPay) &&
         !TReqInfo::Instance()->user.access.profiledRightPermittedForCrsPax(PaxId_t(_pax_id), 193) )
      throw UserException( "MSG.SEATS.CHANGE_PAY_SEATS_DENIED" );

    if (_layer_type.value()!=cltProtCkin) return;

    if (TAccess::profiledRightPermitted(AirportCode_t(fltInfo.airp), AirlineCode_t(fltInfo.airline), 192)) return;

    if (TAccess::profiledRightPermitted(AirportCode_t(fltInfo.airp), AirlineCode_t(fltInfo.airline), 191))
    {
      if (fltInfo.airline=="��") {
        //�ࠢ��� ��� ��: ࠧ�蠥� �� ࠭��, 祬 �� 6 �ᮢ �� �뫥�
        if (fltInfo.scd_out-NowUTC()<=(6/24.0)) return;
      } else {
        //��� ��⠫��� �������� �।���⥫쭠� ��ᠤ�� ࠧ�襭�
        return;
      }
    }

    throw UserException("MSG.PRESEATING.ACCESS_DENIED");
  }
  void checkLayerNewSeats() {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return;
    TQuery QrySeatRules( &OraSession );
    QrySeatRules.SQLText =
        "SELECT pr_owner FROM comp_layer_rules "
        "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
    QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( _layer_type.value() ) );
    QrySeatRules.DeclareVariable( "old_layer", otString );
    tst();
    TPassenger pass;
    pass.preseat_pax_id = _pax_id;
    pass.preseat_layer = _layer_type.value();
    tst();

    for ( const auto& seat : newSeats.value().second ) {
      TLayerPrioritySeat tmp_layer = seat->getDropBlockedLayer( _point_id );
      if ( tmp_layer.layerType() != cltUnknown ) {
        ProgTrace( TRACE5, "getDropBlockedLayer: %s add %s",
                   string(seat->yname+seat->xname).c_str(), tmp_layer.toString().c_str() );
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }
      tst();
      std::map<int, TSetOfLayerPriority,classcomp > layers;
      seat->GetLayers( layers, glAll );
      tst();
      const auto& lrs = layers.find( _point_id );
      if ( lrs == layers.end() ||
           lrs->second.empty() )
        continue;
      if ( lrs->second.begin()->getPaxId() == _pax_id &&
           lrs->second.begin()->layerType() == _layer_type.value() &&
           !_clientSets.isFlag(flSetPayLayer) )
        throw UserException( "MSG.SEATS.SEAT_NO.PASSENGER_OWNER" );
      tst();

      QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( lrs->second.begin()->layerType() ) );
      ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( lrs->second.begin()->layerType() ) );
      QrySeatRules.Execute();
      tst();
      if ( QrySeatRules.Eof ||
         ( QrySeatRules.FieldAsInteger( "pr_owner" ) &&
           _pax_id != lrs->second.begin()->getPaxId() ) ) {
        if ( lrs->second.begin()->getPaxId() == NoExists )
          throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
        TTripInfo fltDep, fltArv;
        fltDep.getByPointId( lrs->second.begin()->point_dep() );
        fltArv.getByPointId( lrs->second.begin()->point_arv() );
        throw UserException( "MSG.SEATS.SEAT_NO.OCCUPIED_OTHER_LEG_PASSENGER",
                             LParams()<<LParam("airp_dep", ElemIdToCodeNative(etAirp,fltDep.airp) )
                                      <<LParam("airp_arv", ElemIdToCodeNative(etAirp,fltArv.airp) ) );
      }
      tst();
      if ( !UsedPayedPreseatForPassenger( fltInfo, flagCompLayerPriority, *seat, pass, false ) )
        throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
    }
    tst();
  }
  void checkRequiredRfiscs() {
    LogTrace(TRACE5) << __func__;
    if ( !_clientSets.isFlag( flWaitList ) && //  �� �㣠����, �᫨ ���ᠤ�� ���� � ��
         _seat_type == EnumSitDown::stReseat && //���ᠤ��
         isCheckLayer( _layer_type ) ) // 㦥 ��ॣ����஢������
      //����� ������ ���ᠤ��, �.�. ������ ���� ��㣨
      DCSServiceApplying::RequiredRfiscs( DCSAction::ChangeSeatOnDesk, PaxId_t(_pax_id) ).throwIfNotExists();
  }

  void getRFISC( const std::set<TPlace*,CompareSeats>& _seats,
                 TPaxSegRFISCKey& rfisc, TSeat& seat ) {
    rfisc.clear();
    TRFISC r;
    for ( const auto & s : _seats ) {
      r = s->getRFISC( _point_id );
      if ( !r.empty() ) {
        seat = TSeat(s->yname,s->xname);
        rfisc.RFISC = r.code;
        return; //⮫쪮 ����
      }
    }
  }

  EnumSeatsProps checkRequeredServiceOnRfics( xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode ) {
    LogTrace(TRACE5) << __func__;
    if ( reqNode == nullptr ||
         resNode == nullptr )
      return EnumSeatsProps::propNone;
    if ( _seat_type == EnumSitDown::stReseat && //���ᠤ��
         TReqInfo::Instance()->client_type == ctTerm &&
         salonList.getRFISCMode() == rRFISC &&
         !_clientSets.isFlag( flWaitList ) &&
         isCheckLayer( _layer_type ) &&
         GetTripSets(tsReseatRequiredRFISCService,fltInfo) &&
         !ReseatPaxRFISCServiceChanger::isContinueServices( reqNode ) ) {
      ReseatPaxRFISCServiceChanger r;
      r.point_id = _point_id;
      r.pax_id = _pax_id;
      getRFISC( currSeats.value(), r.del_rfisc, r.del_seat );
      getRFISC( newSeats.value().second, r.add_rfisc, r.add_seat );
      LogTrace(TRACE5) << r.del_rfisc.RFISC << "|" << r.add_rfisc.RFISC;
      if ( r.del_rfisc.RFISC == r.add_rfisc.RFISC )
        return EnumSeatsProps::propNone;
      CheckIn::TPaxTknItem tkn;
      CheckIn::LoadPaxTkn(_pax_id,tkn); //??? �� ��������
      if ( !tkn.validET() )
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
      if ( !r.reseat( reqNode, externalSysResNode, resNode ) ) //�᫨ ��� ����� ������ �� ���ᠦ��᪨� ��㣠�, � ���� ��� � �����, ���� ������ ������ ���ᠤ��
        return EnumSeatsProps::propExit;
      _resPropsSets.setFlag(propChangeRFISCService);
      LogTrace(TRACE5) << __func__ << " saving..";
    }
    return EnumSeatsProps::propNone;
  }

  EnumSeatsProps checkRFISC(xmlNodePtr reqNode, xmlNodePtr resNode) {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ||
         reqNode == nullptr ||
         resNode == nullptr )
      return EnumSeatsProps::propNone;
    if ( isCheckLayer( _layer_type ) ) {
      passTariffs.get( _pax_id );
    }
    else {
      TMktFlight mktFlight;
      mktFlight.getByCrsPaxId( _pax_id );
      CheckIn::TPaxTknItem tkn;
      CheckIn::LoadCrsPaxTkn( _pax_id, tkn);
      passTariffs.get( fltInfo, mktFlight, tkn, pnr.value().airp_arv );
    }
    for ( const auto& seat : newSeats.value().second ) {
      std::map<int, TRFISC,classcomp> vrfiscs;
      TRFISC rfisc;
      if ( salonList.getRFISCMode() ) {
        seat->GetRFISCs( vrfiscs );
        const auto &r = vrfiscs.find( _point_id );
        TSeatTariffMap::const_iterator c;
        if ( r != vrfiscs.end() &&
             !r->second.empty() ) {
          if ( (c=passTariffs.find( r->second.color )) != passTariffs.end() &&
               _layer_type.value() == cltProtCkin &&
               !c->second.pr_prot_ckin )
            throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL_WITH_RFISC",
                                LParams()<<LParam("code", r->second.code) );
          _resPropsSets.setFlag(propRFISC);
          if ( setRFISCQuestion( reqNode, resNode ) ) {
             tst();
            _resPropsSets.setFlag(propRFISCQuestion);
            return EnumSeatsProps::propRFISCQuestion;
          }
        }
        tst();
        SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
        SelfCkinSalonTariff.setTariffMap( _point_id, passTariffs );
        seat->SetRFISC( _point_id, passTariffs );
        seat->GetRFISCs( vrfiscs );
        const auto &r1 = vrfiscs.find( _point_id );
        if ( r1 != vrfiscs.end() )
          rfisc = r1->second;
      }
      else { //���� ०�� ࠡ��� ???
        seat->convertSeatTariffs( _point_id );
        rfisc.color = seat->SeatTariff.color;
        rfisc.rate = seat->SeatTariff.rate;
        rfisc.currency_id = seat->SeatTariff.currency_id;
      }
      TSeat st(seat->yname,seat->xname);
      logSeats.emplace_back( make_pair(TSeatRange(st,st), rfisc ) );
      tst();
    }
    return EnumSeatsProps::propNone;
  }
  void checkSeatNo() {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return;
    if ( !_seat )
      throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
    tst();
    strcpy(_seat.value().row, SeatNumber::tryNormalizeRow( _seat.value().row ).c_str() );
    strcpy(_seat.value().line, SeatNumber::tryNormalizeLine( _seat.value().line ).c_str() );
    tst();
    if (_seat.value().Empty() )
      throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
    tst();
    //��室�� ���न���� ������ ����
    TSalonPoint p;
    for ( const auto &s : salonList._seats ) {
      if ( s->GetIsNormalizePlaceXY(_seat.value().line,
                                    _seat.value().row,
                                    p) ) {
        break;
      }
    }
    tst();
    if ( p.isEmpty() )
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    tst();
    //�஢��塞 ����� ����
    //�஢��塞 ���ᠦ�� �� �ਧ��� ������� � ���������� ᨤ��� � ���਩���� ��室� ��� �冷� � ��㣨� ������楬
    //������ ⠪�� ���� ������㯭묨
    SALONS2::CraftSeats::iterator slist = std::find_if( salonList._seats.begin(),
                                                        salonList._seats.end(),
                                                        [&](const auto&item) { return item->num == p.num; });
    ASSERT(slist!=salonList._seats.end());
    newSeats.emplace(std::make_pair(*slist,std::set<SALONS2::TPlace*,SALONS2::CompareSeats>()));
    bool pr_down = isRemExists(std::set<std::string>{"STCR"});
    std::string cls = pax.cabin.cl.empty()?(grp?grp.value().cl:pnr.value().cl):pax.cabin.cl;
    for ( int i=0; i<pax.seats; i++ ) { // �஡�� �� ���-�� ���� � �� ���� ���⠬ ��� ���ᠦ��
      TPlace* seat = (*slist)->place(TPoint(p.x,p.y));
      if ( !seat->visible ||
           !seat->isplace ||
            seat->clname != cls ) {
          throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }
      newSeats.value().second.emplace( seat );
      tst();
      pr_down?p.y++:p.x++;
    }

  }
  void checkLayerSet() {
    LogTrace(TRACE5) << __func__;
    std::set<TCompLayerType> layers = { cltProtBeforePay, cltProtAfterPay, cltProtSelfCkin };
    tst();
    if ( _clientSets.isFlag( flSetPayLayer ) ) {
      if ( _seat_type != EnumSitDown::stSeat ||
           !_layer_type ||
           layers.find( _layer_type.value() ) == layers.end() )
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
      if ( grp )
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
    }
    tst(); //!!!������
    if ( layers.find( _layer_type.value() ) != layers.end() &&
       (!( _layer_type.value() == cltProtCkin ||
             _layer_type.value() == cltProtSelfCkin ||
            ( !_clientSets.isFlag( flSetPayLayer ) && ( _layer_type.value() == cltPNLAfterPay ||
                                                        _layer_type.value() == cltProtAfterPay )) ||
             _clientSets.isFlag( flSetPayLayer ) )) )
        throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");

    tst();
    if ( grp &&
         !isCheckLayer(_layer_type) )
     throw UserException( "MSG.PASSENGER.CHECKED.REFRESH_DATA" );
    tst();
    if ( curr_layer_type ) {
      if ( BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, curr_layer_type.value() ),
                                                                flagCompLayerPriority ) <
           BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, _layer_type.value() ),
                                                                flagCompLayerPriority ) ) {
        throw UserException( "MSG.SEATS.SEAT_NO.EXIST_MORE_PRIORITY" ); //���ᠦ�� ����� ����� �ਮ��⭮� ����
      }
    }
  }
  void checkCrewChangeSeat() {
    LogTrace(TRACE5) << __func__;
    if ( grp && grp.value().status == psCrew )
      throw UserException("MSG.CREW.IMPOSSIBLE_CHANGE_SEAT");
  }
  void checkIsJmp() {
    LogTrace(TRACE5) << __func__;
    if ( pax.is_jmp )
      throw UserException( "MSG.SEATS.NOT_RESEATS_PASSENGER_FROM_JUMP_SEAT" );
  }
  std::string fullname() {
    LogTrace(TRACE5) << __func__;
    string res = pax.surname;
    TrimString( res );
    res += string(" ") + pax.name;
    return res;
  }
  void checkTid() {
    LogTrace(TRACE5) << __func__ << " " << pax.tid << " " << _tid;
    if ( pax.tid != _tid  ) {
      throw UserException( "MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                           LParams()<<LParam("surname", fullname() ) );
    }
  }
  void checkSeatsCount() {
    LogTrace(TRACE5) << __func__;
    if ( !pax.seats )
      throw UserException( "MSG.SEATS.NOT_RESEATS_SEATS_ZERO" );
  }
  EnumSeatsProps setProtLayerQuestion( xmlNodePtr reqNode, xmlNodePtr resNode ) {
    LogTrace(TRACE5) << __func__;
    // �।�०����� � ���ᠤ�� ��ॣ����஢������ ���ᠦ�� � ���� � �।�. ࠧ��⪮�
    if ( _seat_type != EnumSitDown::stReseat ||
         _clientSets.isFlag( flWaitList ) ||
         resNode == nullptr ||
         reqNode == nullptr ||
         !_clientSets.isFlag( flQuestionReseat ) ||
         !currSeats ||
         _clientSets.isFlag(flSetPayLayer) )
      return EnumSeatsProps::propNone;
    const auto& pl = salonList.pax_lists.find(_point_id); //ᯨ᮪ ���ᠦ�஢ �� ३��
    if ( pl == salonList.pax_lists.end() ) return EnumSeatsProps::propNone;
    const auto& p = pl->second.find( _pax_id ); //���ᠦ��
    if ( p == pl->second.end() ) return EnumSeatsProps::propNone;
    //�� ����� ⥪�騩 ���� ���ᠦ��+᫮� � ��������� ����+᫮� ���ᠦ�� (��� �㦭� �, ����� �������� ��-�� ����� ������� �ਮ���
    //��� ���� 㡥�����, �� ⥪. ���� ������� � ���������� ����� � ᫮��
    //�� �㤥� �������, �� ���ᠦ�� ��ॣ����஢���� �� �� �����, ����� ����� ��� ���� १��
    // ���� �।�०���� ᮮ�饭��� ����� �� �⮩ ���樨
    std::set ls = { cltProtCkin, cltPNLAfterPay, cltProtAfterPay, cltProtSelfCkin };
    std::set<SALONS2::TPlace*,SALONS2::CompareSeats> sts;
    for ( const auto& l : p->second.layers ) { //std::map<TLayerPrioritySeat,TPaxLayerSeats,LayerPrioritySeatCompare> TLayersPax
      if ( l.second.waitListReason.status == layerLess &&
           ls.find( l.first.layerType() ) != ls.end() ) {
        set_intersection( l.second.seats.begin(),
                          l.second.seats.end(),
                          currSeats.value().begin(),
                          currSeats.value().end(),
                          inserter(sts, sts.end()) );
      }
      if ( !sts.empty() ) {
        if (TReqInfo::Instance()->desk.compatible(RESEAT_QUESTION_VERSION) ) {
            bool reset=false;
            xmlNodePtr dataNode = GetNode( "data", resNode );
            if (GetNode("confirmations/msg2",reqNode)==NULL) {
              tst();
              if ( dataNode == nullptr )
                dataNode =  NewTextChild(resNode,"data");
              xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
              NewTextChild(confirmNode,"reset",(int)reset);
              NewTextChild(confirmNode,"type","msg2");
              NewTextChild(confirmNode,"dialogMode","");
              ostringstream msg;
              msg << ((l.first.layerType() == cltProtCkin)?
                                                getLocaleText("QST.PAX_HAS_PRESEAT_SEATS.RESEAT"):
                                                getLocaleText("QST.PAX_HAS_PAID_SEATS.RESEAT"))
                  << endl
                  << getLocaleText("QST.CONTINUE_RESEAT");
              NewTextChild(confirmNode,"message",msg.str());
              tst();
              _resPropsSets.setFlag( propRFISCQuestion );
              return propRFISCQuestion;
            }
            else
             if ( NodeAsInteger("confirmations/msg2",reqNode) == 7 )
               throw UserException("MSG.PASSENGER.RESEAT_BREAK");
        }
        else {
          if ( l.first.layerType() == cltProtCkin )
            NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PRESEAT_SEATS.RESEAT") );
          else
            if ( l.first.layerType() != cltProtSelfCkin ) //not web pay layers
              NewTextChild( resNode, "question_reseat", getLocaleText("QST.PAX_HAS_PAID_SEATS.RESEAT"));
          _resPropsSets.setFlag( propRFISCQuestion );
          return propRFISCQuestion;
        }
      }
    }
    return EnumSeatsProps::propNone;
  }
  bool setRFISCQuestion(xmlNodePtr reqNode, xmlNodePtr resNode )
  {
    LogTrace(TRACE5) << __func__;
    bool reset=false;
    if ( TReqInfo::Instance()->client_type == ctTerm &&
         _seat_type == EnumSitDown::stReseat && //���ᠤ��
          !_clientSets.isFlag( flWaitList ) && //  �� �㣠����, �᫨ ���ᠤ�� ���� � ��
          reqNode != nullptr &&
          resNode != nullptr &&
          isCheckLayer( _layer_type ) ) { // 㦥 ��ॣ����஢������
      if ( GetTripSets(tsReseatOnRFISC,fltInfo) ) {
        if (TReqInfo::Instance()->desk.compatible(RESEAT_QUESTION_VERSION) ) {
          tst();
          xmlNodePtr dataNode = GetNode( "data", resNode );
          if (GetNode("confirmations/msg1",reqNode)==NULL) {
            tst();
            if ( dataNode == nullptr ) {
              dataNode =  NewTextChild(resNode,"data");
            }
            xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
            NewTextChild(confirmNode,"reset",(int)reset);
            NewTextChild(confirmNode,"type","msg1");
            NewTextChild(confirmNode,"dialogMode","");
            ostringstream msg;
            msg << getLocaleText("MSG.PASSENGER.RESEAT_TO_RFISC") << endl
                << getLocaleText("QST.CONTINUE_RESEAT");
            NewTextChild(confirmNode,"message",msg.str());
            tst();
            return true;
          }
          else
           if ( NodeAsInteger("confirmations/msg1",reqNode) == 7 ) {
             throw UserException("MSG.PASSENGER.RESEAT_BREAK");
          }
        }
      }
    }
    return false;
  }
  void doDeleteLayer( int &curr_tid ) {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stSeat ) return;
    TPointIdsForCheck point_ids_spp;
    point_ids_spp.insert( make_pair(_point_id, _layer_type.value()) );
    curr_tid = NoExists;
    if ( isCheckLayer( _layer_type ) ) {
      TQuery Qry( &OraSession );
      Qry.SQLText =
        "BEGIN "
        " DELETE FROM trip_comp_layers "
        "  WHERE point_id=:point_id AND "
        "        layer_type=:layer_type AND "
        "        pax_id=:pax_id; "
        " IF :tid IS NULL THEN "
        "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
        "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
        " END IF;"
        "END;";
      Qry.CreateVariable( "point_id", otInteger, _point_id );
      Qry.CreateVariable( "pax_id", otInteger, _pax_id );
      Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( _layer_type.value() ) );
      if ( curr_tid == NoExists )
        Qry.CreateVariable( "tid", otInteger, FNull );
      else
        Qry.CreateVariable( "tid", otInteger, curr_tid );
      Qry.Execute();
      curr_tid = Qry.GetVariableAsInteger( "tid" );
    }
    else {
      DeleteTlgSeatRanges( {_layer_type.value()}, _pax_id, curr_tid, point_ids_spp );
    }
  }
  void doAddLayer( int& curr_tid ) {
    LogTrace(TRACE5) << __func__;
    if ( _seat_type == EnumSitDown::stDropseat ) return; // ��ᠤ�� �� ����� ����
    TSeatRanges seatRanges;
    for ( const auto &s : newSeats.value().second ) {
      TSeat st(s->yname,s->xname);
      seatRanges.emplace_back( TSeatRange(st,st) );
    }
    if ( isCheckLayer( _layer_type ) ) {
      SaveTripSeatRanges( _point_id, _layer_type.value(), seatRanges, _pax_id, _point_id, grp.value().point_arv, NowUTC() );
      TQuery Qry( &OraSession );
      Qry.SQLText =
        "BEGIN "
        " IF :tid IS NULL THEN "
        "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
        "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
        " END IF;"
        "END;";
      Qry.CreateVariable( "pax_id", otInteger, _pax_id );
      if ( curr_tid == NoExists )
        Qry.CreateVariable( "tid", otInteger, FNull );
      else
        Qry.CreateVariable( "tid", otInteger, curr_tid );
      Qry.Execute();
      curr_tid = Qry.GetVariableAsInteger( "tid" );
    }
    else {
      TPointIdsForCheck point_ids_spp;
      point_ids_spp.insert( make_pair(_point_id, _layer_type.value()) );
      InsertTlgSeatRanges( pnr.value().point_id_tlg, pnr.value().airp_arv, _layer_type.value(), seatRanges, _pax_id,
                           NoExists, _time_limit.value_or(ASTRA::NoExists), false, curr_tid, point_ids_spp );
    }
  }
  void doLog() {
    LogTrace(TRACE5) << __func__;
    TReqInfo *reqinfo = TReqInfo::Instance();
    PrmEnum seatPrmEnum("seat", "");
    if ( _seat_type == EnumSitDown::stDropseat ) {
      logSeats.clear(); //�� ��直�
      for ( const auto &seat : currSeats.value() ) {
        TSeat st(seat->yname,seat->xname);
        logSeats.emplace_back( make_pair(TSeatRange(st,st), TRFISC() ) );
      }
    }
    for ( std::vector<std::pair<TSeatRange,TRFISC> >::iterator it=logSeats.begin(); it!=logSeats.end(); it++ ) {
      if (it!=logSeats.begin())
        seatPrmEnum.prms << PrmSmpl<string>("", " ");
      seatPrmEnum.prms << PrmSmpl<string>("", it->first.first.denorm_view(salonList.isCraftLat()));

      if ( !it->second.code.empty() || !it->second.empty())
      {
        seatPrmEnum.prms << PrmSmpl<string>("", "(");
        if ( !it->second.code.empty() )
          seatPrmEnum.prms << PrmSmpl<string>("", it->second.code);

        if ( !it->second.empty() )
        {
          if ( !it->second.code.empty() )
            seatPrmEnum.prms << PrmSmpl<string>("", "/");

          if (passTariffs.status()==TSeatTariffMap::stUseRFISC ||
              passTariffs.status()==TSeatTariffMap::stNotRFISC)
            seatPrmEnum.prms << PrmSmpl<string>("", it->second.rateView())
                             << PrmElem<string>("", etCurrency, it->second.currency_id);
          else
            seatPrmEnum.prms << PrmLexema("", "EVT.UNKNOWN_RATE");
        }
        seatPrmEnum.prms << PrmSmpl<string>("", ")");
      }
    }
    switch( _seat_type ) {
      case EnumSitDown::stSeat:
        if ( isCheckLayer( _layer_type ) )
          reqinfo->LocaleToLog(_clientSets.isFlag( flWaitList )?"EVT.PASSENGER_SEATED_MANUALLY_WAITLIST":"EVT.PASSENGER_SEATED_MANUALLY",
                               LEvntPrms() << PrmSmpl<std::string>("name", fullname())
                                           << seatPrmEnum,
                               evtPax, _point_id, pax.reg_no, pax.grp_id);
        break;
      case EnumSitDown::stReseat:
        if ( isCheckLayer( _layer_type ) ) {
          reqinfo->LocaleToLog(_clientSets.isFlag( flWaitList )?"EVT.PASSENGER_CHANGE_SEAT_MANUALLY_WAITLIST":"EVT.PASSENGER_CHANGE_SEAT_MANUALLY",
                               LEvntPrms() << PrmSmpl<std::string>("name", fullname())
                                           << seatPrmEnum,
                               evtPax, _point_id, pax.reg_no, pax.grp_id);
          SyncPRSA( fltInfo.airline, _pax_id, passTariffs.status(), logSeats );
        }
        break;
      case EnumSitDown::stDropseat:
        if ( isCheckLayer( _layer_type ) )
          if (!logSeats.empty())
            reqinfo->LocaleToLog(_clientSets.isFlag( flSyncCabinClass )?"EVT.PASSENGER_DISEMBARKED_DUE_TO_CLASS_CHANGE":"EVT.PASSENGER_DISEMBARKED_MANUALLY",
                                 LEvntPrms() << PrmSmpl<std::string>("name", fullname())
                                             << seatPrmEnum,
                                 evtPax, _point_id, pax.reg_no, pax.grp_id);
        break;
    }
  }
  void doChangeSeats(const std::string& whence) {
    LogTrace(TRACE5) << __func__;
    int curr_tid;
    doDeleteLayer( curr_tid );
    doAddLayer( curr_tid );
    _tid = curr_tid;
    doLog();
    std::set<int> paxs_external_logged;
    paxs_external_logged.insert( _pax_id );
    TPointIdsForCheck point_ids_spp;
    point_ids_spp.insert( make_pair(_point_id, _layer_type.value()) );
    check_layer_change( point_ids_spp, paxs_external_logged, whence );
  }
  void xmlWaitListFinishMessage( xmlNodePtr reqNode, xmlNodePtr resNode ) {
    LogTrace(TRACE5) << __func__;
    if ( !_clientSets.isFlag( flWaitList ) ||
         _seat_type == EnumSitDown::stDropseat ||
         reqNode == nullptr ||
         resNode == nullptr ) return;
    SALONS2::TGetPassFlags flags;
    flags.setFlag( SALONS2::gpPassenger );
    flags.setFlag( SALONS2::gpWaitList );
    SALONS2::TSalonPassengers passengers;
    NewSalonList.getPassengers( passengers, flags );
    passengers.BuildWaitList( _point_id, NewSalonList.getSeatDescription(), NodeAsNode( "data", resNode ) );
    if ( !passengers.isWaitList( _point_id ) )
      AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_FINISHED" );
  }
  void xmlRFISCWarning( xmlNodePtr reqNode, xmlNodePtr resNode ) { //!!!�������� ���� ���⭮� ��⥣�ਨ
    LogTrace(TRACE5) << __func__;
    if ( TReqInfo::Instance()->client_type == ctTerm &&
         _seat_type == EnumSitDown::stReseat && //���ᠤ��
         reqNode != nullptr &&
         resNode != nullptr &&
         _resPropsSets.isFlag(propRFISC) && //!!!
         !_clientSets.isFlag( flWaitList ) && //  �� �㣠����, �᫨ ���ᠤ�� ���� � ��
         isCheckLayer( _layer_type ) && // 㦥 ��ॣ����஢������
         GetTripSets(tsReseatOnRFISC,fltInfo) ) { //ᮮ�饭�� �� ���ᠤ�� ���ᠦ�� �� ���� � ࠧ��⪮� RFISC
      TRemGrp remGrp;
      remGrp.Load(retFORBIDDEN_FREE_SEAT, _point_id); //६�ન ����饭�� � ᢮������ ��ᠤ��
      for ( TRemGrp::iterator it=remGrp.begin(); it!=remGrp.end(); ++it ) {
        if ( isRemExists( std::set<std::string>{*it} ) ) {
          for ( const auto &seat : newSeats.value().second ) {
            std::vector<TSeatRemark> sremarks;
            seat->GetRemarks( _point_id, sremarks );
            if ( std::find( sremarks.begin(), sremarks.end(), TSeatRemark( *it, false ) ) != sremarks.end() )
              return;
          }
        }
      }
      AstraLocale::showErrorMessage( "MSG.SEATS.SEATS_WARNING_RFISC" );
    }
  }
  void xmlChangeRFISCServices( xmlNodePtr reqNode, xmlNodePtr resNode ) {
    LogTrace(TRACE5) << __func__;
    if ( TReqInfo::Instance()->client_type == ctTerm &&
         _seat_type == EnumSitDown::stReseat && //���ᠤ��
         reqNode != nullptr &&
         resNode != nullptr &&
         _resPropsSets.isFlag(propChangeRFISCService) ) {
      if (TReqInfo::Instance()->desk.compatible(RESEAT_TO_RFISC_VERSION) )
        AstraLocale::showErrorMessage( "MSG.PASSENGER.CHANGE_SERVICE" );
      else
        AstraLocale::showErrorMessage( "MSG.MSG.PASSENGER.CHANGE_SERVICE_UPDATE_PAX" );
    }
  }
  void toXML( xmlNodePtr reqNode, xmlNodePtr resNode ) {
    LogTrace(TRACE5) << __func__;
    if ( reqNode == nullptr ||
         resNode == nullptr )
      return;
    std::set ls = { cltProtCkin, cltPNLAfterPay, cltProtAfterPay, cltProtSelfCkin };
    string seat_no, slayer_type;
    //!!!���� - ���� ���� �� ᠫ����
    if ( ls.find( _layer_type.value() ) != ls.end() )
      getSeat_no( _pax_id, true, string("_seats"), seat_no, slayer_type, _tid );
    else
      getSeat_no( _pax_id, false, string("one"), seat_no, slayer_type, _tid );
    /* ���� ��।��� ����� ���� tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", _tid );
    if ( !seat_no.empty() ) {
      NewTextChild( dataNode, "seat_no", seat_no );
      NewTextChild( dataNode, "layer_type", slayer_type );
    }
    if ( mustUpdateSalon(reqNode) ) {
      TSalonChanges changes( _point_id, NewSalonList.getRFISCMode() );
      changes.get(salonList,NewSalonList).toXML(dataNode,
                                                NewSalonList.isCraftLat(),
                                                std::nullopt);
                                                //!!!NewSalonList.pax_lists); �� �ᯮ�����???
    }
    xmlWaitListFinishMessage( reqNode, resNode );
    xmlRFISCWarning( reqNode, resNode );
    xmlChangeRFISCServices( reqNode, resNode );
  }
  void FullUpdateToXML(xmlNodePtr reqNode,xmlNodePtr resNode) {
    LogTrace(TRACE5) << __func__;
    if ( !mustUpdateSalon(reqNode) ||
         resNode == nullptr ||
         TReqInfo::Instance()->client_type != ctTerm ) return;
    xmlNodePtr dataNode = GetNode( "data", resNode );
    RemoveNode( dataNode ); // 㤠����� �ᥩ ����, �.�. ��稫��� �訡��
    dataNode = NewTextChild( resNode, "data" );
    if ( !TReqInfo::Instance()->desk.compatible(RFISC_VERSION) )
      salonList.ReadFlight( salonList.getFilterRoutes(), salonList.getFilterClass(),
                            _tariffPaxId.value_or(ASTRA::NoExists) );
    SALONS2::GetTripParams( _point_id, dataNode );
    salonList.JumpToLeg( SALONS2::TFilterRoutesSets( _point_id, ASTRA::NoExists ) );
    salonList.Build( NewTextChild( dataNode, "salons" ) );
    if ( _clientSets.isFlag( flWaitList ) ) {
      SALONS2::TSalonPassengers passengers;
      SALONS2::TGetPassFlags flags;
      flags.setFlag( SALONS2::gpPassenger );
      flags.setFlag( SALONS2::gpWaitList );
      salonList.getPassengers( passengers, flags );
      passengers.BuildWaitList( _point_id, salonList.getSeatDescription(), dataNode );
    }
    if ( salonList.getCompId() > 0 &&
         isComponSeatsNoChanges( fltInfo ) ) //��ண� ���易�� ������ ���������� � �����祭�묨 �� ३�
      componPropCodes::Instance()->buildSections( salonList.getCompId(), TReqInfo::Instance()->desk.lang,
                                                  dataNode, TAdjustmentRows().get( salonList ) );
  }
  bool mustUpdateSalon(xmlNodePtr reqNode) {
    return ( reqNode != nullptr &&
             (std::string("DeleteProtCkinSeat") != (const char*)reqNode->name ||
              GetNode( "update_salons", reqNode ) != nullptr ));
  }
  class MessageSaver {
    //��࠭塞 ᮮ�饭�� �� SavePax, ��⮬ �ய�襬 ᮮ�饭�� � resNode �� ���ᠤ��, � ��⮬ ����⠭����
   private:
    std::string tag_name;
    std::string content;
    std::string lexema_id;
    int code;
   public:
    bool operator==(const MessageSaver& v) {
      return (tag_name == v.tag_name &&
              content == v.content &&
              lexema_id == v.lexema_id &&
              code == v.code);
    }
    void fromXML(xmlNodePtr resNode) {
      tag_name.clear();
      content.clear();
      lexema_id.clear();
      code = 0;
      xmlNodePtr node = GetNode("command", resNode);
      if ( node == nullptr ) return;
      tag_name = (const char*)node->children->name;
      content = NodeAsString( node->children );
      lexema_id = NodeAsString( "@lexema_id", node->children, "" );
      code = NodeAsInteger( "@code", node->children, 0 );
    }
    void toXML(xmlNodePtr resNode, bool clear_if_empty = false) {
      if (tag_name.empty() && !clear_if_empty) return;
      if(tag_name.empty())
        RemoveNode(GetNode("command", resNode));
      else {
        resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), tag_name.c_str(), content );
        SetProp(resNode, "lexema_id", lexema_id);
        SetProp(resNode, "code", code);
      }
    }
  };
public:
  BitSet<EnumSeatsProps>& SitDownPlease( xmlNodePtr reqNode,
                                         xmlNodePtr externalSysResNode,
                                         xmlNodePtr resNode,
                                         EnumSitDown seat_type, //��ᠤ��, ���ᠤ��, �����祭�� ����
                                         const std::string& whence,
                                         const std::optional<int>& time_limit = std::nullopt ) {
    _seat_type = seat_type;
    _time_limit = time_limit?std::optional<int>(time_limit.value()):std::nullopt;
    _recalcSets.clearFlags();
    _resPropsSets.clearFlags();

    if ( !fltInfo.getByPointId(_point_id) )
      throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

    TFlights flights;
    flights.Get( _point_id, ftTranzit );
    flights.Lock(__func__); //��稬 ३� � �஢��塞 ����� �� ��

    if ( SALONS2::isFreeSeating(_point_id) ) { //᢮������ ��ᠤ��
      throw UserException( "MSG.SALONS.FREE_SEATING" );
    }
    XMLDoc doc("reseat");
    xmlNodePtr emulResNode = NewTextChild(doc.docPtr()->children,"answer");
    AstraContext::ClearContext(ReseatPaxRFISCServiceChanger::getContextName(),0);
    tst();
    AstraContext::SetContext(ReseatPaxRFISCServiceChanger::getContextName(),0,doc.text());//ConvertCodepage(doc.text(), "UTF-8", "CP866"));
    tst();
    MessageSaver savePaxMessage;

    try {
      fromDB(); // ���⪠ ������ �� ���ᠦ���
      checkSeatNo(); //����� ����
      checkComponChanged(); //���������� ����������

      checkCrewChangeSeat(); //����� ������ ���� ��� crew
      checkLayerSet(); //��������� ���⭮�� ����(᫮�) �� ���� � ��㣮�
      checkIsJmp(); // �⪨��� ����
      checkTid(); // ��������� �� ���ᠦ���
      checkSeatsCount(); // ��� ���� � ���ᠦ��
      checkBabyEmergencySeats(); //�஢�ઠ �� ����饭�� ���� ��� ��������� � ��+��
      checkPreseatingAccess(); //�஢�ઠ �ࠢ ࠧ��⪨ ᫮��
      checkLayerNewSeats(); //�஢�ઠ ����� ���� �� ᫮�
      if ( EnumSeatsProps::propNone != checkRFISC( reqNode, resNode ) ) //�஢�ઠ RFISC | Tariff
        return _resPropsSets;

      if ( EnumSeatsProps::propNone != setProtLayerQuestion( reqNode, resNode ) ) //����� �� ���ᠤ�� � ⮬, 㢥७ �� �����, �� ���ᠦ����� � ��ண� १�ࢭ��� ��� ����祭���� ����
        return _resPropsSets;

      if ( EnumSeatsProps::propExit == checkChangeSeatsForPay() ) //�஢�ઠ ��������� ���� �� �����
        return _resPropsSets;
      checkRequiredRfiscs();
      if ( EnumSeatsProps::propExit == checkRequeredServiceOnRfics( reqNode, externalSysResNode, resNode ) ) //��砫��� ���������� ��� 㤠����� ��㣨
        return _resPropsSets;
      savePaxMessage.fromXML(resNode);
      doChangeSeats(whence);
      salonList.JumpToLeg( SALONS2::TFilterRoutesSets( _point_id, ASTRA::NoExists ) );
      //�ନ஢���� �⢥�
      if ( TReqInfo::Instance()->client_type != ctTerm ||
           resNode == NULL )
          return _resPropsSets; // web-ॣ������
/*      if ( _resPropsSets.isFlag(propRFISCQuestion) || //���� ������
           _resPropsSets.isFlag(propPriorProtLayerQuestion) )
        return _resPropsSets;*/
      //����뢠�� ���� ᠫ��, ����砥� ��������� �� �ࠢ����� � �।��騬
      if ( mustUpdateSalon(reqNode) ) {
        NewSalonList.ReadFlight( salonList.getFilterRoutes(),
                                 salonList.getFilterClass(),
                                 _tariffPaxId.value_or(ASTRA::NoExists) );
      }
      //� ������ ����� SavePax �㦭� �ନ஢�� XML, �.�. ������� ��।����� ��������� ᠫ��� � ��砫� � ����砭�� ���ᠤ��
      //���� �᢮�������� � ��㣮� ���﫮��, �᫨ �ନ஢��� �⢥� �� ��室� �⢥� �� EMDRefreh, � ��������� �� ����稬 � �⢥�
      //���� ��������� �⢥� � ���⥪�� ��� � reqNode �� ⮣�� �� �맮�� SavePax, �.�. �� ⠬ ��࠭����?
      //��࠭塞 ॠ�쭣� �⢥� � ���⥪��, � ��⮬ �����㥬 ��� ��� message � ⥪. �⢥� - �� �㦥� �᫨ �� �㤥� �裡 � EMD
      toXML(reqNode,emulResNode);
    }
    catch( UserException& ue ) {
      tst();
      if ( (TReqInfo::Instance()->client_type != ctTerm &&
            ue.getLexemaData().lexema_id != "MSG.SALONS.CHANGE_CONFIGURE_CRAFT_ALL_DATA_REFRESH") ||
           resNode == NULL )
          throw;
      xmlNodePtr emulResNode = GetNode("answer",doc.docPtr()->children);
      if (!emulResNode) emulResNode = NewTextChild(doc.docPtr()->children,"answer");
      FullUpdateToXML(reqNode,emulResNode);
      showErrorMessageAndRollback( ue.getLexemaData( ) );
    }
    if (_resPropsSets.isFlag(propChangeRFISCService)) { //���� �������� �⢥� �� SavePax
      CheckInInterface::LoadPaxByGrpId(GrpId_t(pax.grp_id), nullptr, NewTextChild( emulResNode, "SavePax"), true);
    }
    MessageSaver currMessage;
    currMessage.fromXML(resNode);
    if ( currMessage == savePaxMessage )
      currMessage = MessageSaver();
    savePaxMessage.toXML(resNode); //����⠭����� SavePax  message ETS.. ERROR
    currMessage.toXML(emulResNode,true); //��࠭��� ॠ�쭮� ᮮ�饭�� �� ���ᠤ��
    CopyNode(resNode,GetNode("data",emulResNode),true); //�����㥬 �� �஬� ᮮ�饭��
    LogTrace(TRACE5) << doc.text();
    LogTrace(TRACE5) << XMLTreeToText(resNode->doc);
    AstraContext::ClearContext(ReseatPaxRFISCServiceChanger::getContextName(),0);
    AstraContext::SetContext(ReseatPaxRFISCServiceChanger::getContextName(),0,doc.text());//ConvertCodepage(doc.text(), "UTF-8", "CP866"));
    return _resPropsSets;
  }
};


namespace RESEAT
{


void Reseat( int point_id, int pax_id, int& tid,
             const TSeat& seat,
             const std::string& whence )
{
  BitSet<SeatsPlanter::EnumClientSets> clientSets;
  SeatsPlanter( point_id, std::nullopt,
                pax_id, 0, tid,
                std::nullopt,
                seat,
                std::nullopt,
                clientSets ).SitDownPlease(nullptr,nullptr,nullptr,EnumSitDown::stReseat, whence);
}

void FreeUpSeatsSyncCabinClass( int point_id, int pax_id, int& tid,
                                const std::string& whence ) { //��ᠤ���
  BitSet<SeatsPlanter::EnumClientSets> clientSets;
  clientSets.setFlag(SeatsPlanter::EnumClientSets::flSyncCabinClass);
  SeatsPlanter( point_id, std::nullopt,
                pax_id, 0, tid,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                clientSets ).SitDownPlease(nullptr,nullptr,nullptr,EnumSitDown::stDropseat, whence);
}

void ChangeLayer( int point_id, int pax_id, int& tid,
                  const TSeat& seat,
                  const std::optional<TCompLayerType>& layer_type,
                  const std::string& whence,
                  const std::optional<int>& time_limit ) {
  BitSet<SeatsPlanter::EnumClientSets> clientSets;
  clientSets.setFlag(SeatsPlanter::EnumClientSets::flSetPayLayer);
  SeatsPlanter( point_id, std::nullopt,
                pax_id, 0, tid,
                std::nullopt,
                seat,
                layer_type,
                clientSets ).SitDownPlease(nullptr,nullptr,nullptr,EnumSitDown::stSeat,whence,time_limit);
}

void SitDownPlease( xmlNodePtr reqNode,
                    xmlNodePtr externalSysResNode,
                    xmlNodePtr resNode,
                    EnumSitDown seat_type, //��ᠤ��, ���ᠤ��, �����祭�� ����
                    const std::string& whence,
                    const std::optional<int>& time_limit ) {
  SeatsPlanter( reqNode ).SitDownPlease(reqNode,externalSysResNode,resNode,seat_type,whence,time_limit);
}

void AfterRefreshEMD(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << XMLTreeToText( resNode->doc );
  std::string value;
  AstraContext::GetContext(ReseatPaxRFISCServiceChanger::getContextName(),0,value);
  AstraContext::ClearContext(ReseatPaxRFISCServiceChanger::getContextName(),0);
  LogTrace(TRACE5) << value;
  XMLDoc doc = createXmlDoc(value);
  xmlDocPtr d = resNode->doc;
  RemoveNode( resNode );
  CopyNode( d->children, GetNode( "answer", doc.docPtr()->children), true );
}

/*
 * type:
 * changeSeatsAndUpdateTids
 * syncCabinClass
 * ChangeSeats
 * DeleteProtCkinSeat
 * changeLayer
 */

bool isSitDownPlease( int point_id, const std::string& whence ) {
  TTripInfo info;
  if (!info.getByPointId(point_id)) return false;
  //�६����� ��� �⫠���, ��ॢ����� �� PG �� ����!!!
  TQuery Qry( &OraSession );
  Qry.SQLText=
    "SELECT "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM tmp_reseat_algo "
    "WHERE type=:type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("type",otString,whence);
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  try {
    Qry.Execute();
    if ( !Qry.Eof ) LogTrace(TRACE5) << __func__;
    return !Qry.Eof;
  }
  catch(...) {
    return false;
  }
}



} //end namespace RESEAT

