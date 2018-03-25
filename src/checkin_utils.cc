#include "apis_utils.h"
#include "checkin_utils.h"
#include "salons.h"
#include "seats.h"
#include "salonform.h"
#include "date_time.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "ANNA"
#include <serverlib/slogger.h>

#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace SALONS2;

namespace CheckIn
{

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
        "SELECT pax.reg_no, pax.grp_id FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND reg_no<0 AND point_dep=:point_id ";
  else
    Qry.SQLText=
        "SELECT pax.reg_no, pax.grp_id FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND reg_no>0 AND point_dep=:point_id ";
  Qry.CreateVariable("point_id", otInteger, _point_id);
  Qry.Execute();

  for(; !Qry.Eof; Qry.Next())
  {
    int reg_no=Qry.FieldAsInteger("reg_no");
    int grp_id=Qry.FieldAsInteger("grp_id");
    if ((_type==Negative && (reg_no<-_maxAbsRegNo || reg_no>=0))||
        (_type==Positive && (reg_no<=0 || reg_no>_maxAbsRegNo)))
    {
      LogError(STDLOG) << __FUNCTION__ << ": wrong reg_no=" << reg_no << "!";
      continue;
    }
    if (!usedRegNo.emplace(_type==Negative?-reg_no:reg_no, grp_id).second)
      LogError(STDLOG) << __FUNCTION__ << ": reg_no=" << reg_no << " duplicated!";
  }
}

void RegNoGenerator::fillUnusedRanges(const std::set<UsedRegNo> &usedRegNo)
{
  unusedRanges.clear();
  lastUnusedRange=boost::none;

  UsedRegNo prior(0, ASTRA::NoExists);
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

bool TWebPaxFromReq::mergePaxFQT(set<CheckIn::TPaxFQTItem> &fqts) const
{
  if (!fqtv_rems_present) return false;
  multiset<string> prior, curr;
  for(set<CheckIn::TPaxFQTItem>::iterator f=fqts.begin(); f!=fqts.end();)
  {
    if (f->rem=="FQTV")
    {
      prior.insert(f->rem_text(false));
      f=Erase(fqts, f);
    }
    else
      ++f;
  };
  for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqtv_rems.begin(); f!=fqtv_rems.end(); ++f)
    curr.insert(f->rem_text(false));

  fqts.insert(fqtv_rems.begin(), fqtv_rems.end());

  return prior!=curr;
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
    // надо номализовать старое и новое место, сравнить их, если изменены, то вызвать пересадку
    string prior_xname, prior_yname;
    getXYName( point_id, prior_seat_no, prior_xname, prior_yname );

    if ( prior_xname + prior_yname != curr_xname + curr_yname )
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
/*      все бы хорошо в закомментированном методе, но IntChangeSeatsN и IntChangeSeats не работают  с cltPNLBeforePay и cltPNLAfterPay
 *      возможно, потом докрутим
        "SELECT tlg_comp_layers.layer_type "
        "FROM tlg_comp_layers, comp_layer_types "
        "WHERE tlg_comp_layers.layer_type=comp_layer_types.code AND "
        "      crs_pax_id=:crs_pax_id AND "
        "      tlg_comp_layers.layer_type IN (:cltPNLBeforePay, "
        "                                     :cltPNLAfterPay, "
        "                                     :cltProtBeforePay, "
        "                                     :cltProtAfterPay) "
        "ORDER BY comp_layer_types.priority ";
      Qry.CreateVariable("crs_pax_id", otInteger, crs_pax_id);
      Qry.CreateVariable("cltPNLBeforePay", otString, EncodeCompLayerType(cltPNLBeforePay));
      Qry.CreateVariable("cltPNLAfterPay", otString, EncodeCompLayerType(cltPNLAfterPay));
      Qry.CreateVariable("cltProtBeforePay", otString, EncodeCompLayerType(cltProtBeforePay));
      Qry.CreateVariable("cltProtAfterPay", otString, EncodeCompLayerType(cltProtAfterPay));
      Qry.Execute();
      if (!Qry.Eof)
      {
        TCompLayerType layer_type=DecodeCompLayerType(Qry.FieldAsString("layer_type"));
        bool isTranzitSalonsVersion = isTranzitSalons( point_id );
        BitSet<SEATS2::TChangeLayerFlags> change_layer_flags;
        change_layer_flags.setFlag(SEATS2::flCheckPayLayer);

        bool changedOrNotPay;
        int tid=NoExists;
        if ( isTranzitSalonsVersion ) {
          changedOrNotPay = IntChangeSeatsN( point_id,
                                             crs_pax_id,
                                             tid,
                                             curr_xname,
                                             curr_yname,
                                             SEATS2::stSeat,
                                             layer_type,
                                             change_layer_flags,
                                             NULL );
        }
        else {
          changedOrNotPay = IntChangeSeats( point_id,
                                            crs_pax_id,
                                            tid,
                                            curr_xname,
                                            curr_yname,
                                            SEATS2::stSeat,
                                            layer_type,
                                            change_layer_flags,
                                            NULL );
        };

      };

*/
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

void CreateEmulRems(xmlNodePtr paxNode, const multiset<CheckIn::TPaxRemItem> &rems, const set<CheckIn::TPaxFQTItem> &fqts)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    if (isDisabledRem(r->code, r->text)) continue;
    r->toXML(remsNode);
  }

  //добавим fqts
  for(set<CheckIn::TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
    CheckIn::TPaxRemItem(*r, false).toXML(remsNode);
}

void CompletePnrDataForCrew(const string &airp_arv, WebSearch::TPnrData &pnrData)
{
    try
    {
        TTripRoute route; //маршрут рейса
        route.GetRouteAfter( NoExists,
                             pnrData.flt.point_dep,
                             pnrData.flt.point_num,
                             pnrData.flt.first_point,
                             pnrData.flt.pr_tranzit,
                             trtNotCurrent,
                             trtNotCancelled );
        TTripRoute::const_iterator r=route.begin();
        for(; r!=route.end(); ++r)
          if (r->airp == airp_arv) break;
        if (r==route.end()) throw UserException("MSG.FLIGHT.DEST_AIRP_NOT_FOUND");
        pnrData.seg.point_dep=pnrData.flt.point_dep;
        ProgTrace(TRACE5, "r->point_id: %d, pnrData.flt.point_dep: %d", r->point_id, pnrData.flt.point_dep);
        ProgTrace(TRACE5, "r->airp: %s, airp_arv: %s", r->airp.c_str(), airp_arv.c_str());
        pnrData.seg.point_arv=r->point_id;
        pnrData.dest.fromDB(pnrData.seg.point_arv, true);
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), pnrData.flt.point_dep);
    };
}

void CompletePnrData(bool is_test, int pnr_id, WebSearch::TPnrData &pnrData)
{
  try
  {
    if (!is_test)
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
        "SELECT crs_pnr.pnr_id, airp_arv, subclass, class "
        "FROM crs_pnr, crs_pax "
        "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
        "      crs_pnr.pnr_id=:pnr_id AND "
        "      crs_pax.pr_del=0 AND rownum<2";
      Qry.CreateVariable("pnr_id", otInteger, pnr_id);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

      TTripRoute route; //маршрут рейса
      route.GetRouteAfter( NoExists,
                           pnrData.flt.point_dep,
                           pnrData.flt.point_num,
                           pnrData.flt.first_point,
                           pnrData.flt.pr_tranzit,
                           trtNotCurrent,
                           trtNotCancelled );

      if (!pnrData.seg.fromDB(pnrData.flt.point_dep, route, Qry))
        throw UserException("MSG.FLIGHT.DEST_AIRP_NOT_FOUND");
    }
    else
    {
      TTripRouteItem next;
      TTripRoute().GetNextAirp(NoExists,
                               pnrData.flt.point_dep,
                               pnrData.flt.point_num,
                               pnrData.flt.first_point,
                               pnrData.flt.pr_tranzit,
                               trtNotCancelled,
                               next);
      if (next.point_id==NoExists || next.airp.empty())
        throw UserException("MSG.FLIGHT.DEST_AIRP_NOT_FOUND");

      TQuery Qry(&OraSession);
      Qry.Clear();
        Qry.SQLText=
          "SELECT subcls.class, subclass, "
          "       pnr_airline, pnr_addr "
          "FROM test_pax, subcls "
          "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
        Qry.CreateVariable("pnr_id", otInteger, pnr_id);
        Qry.Execute();
        if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

        pnrData.seg.point_dep=pnrData.flt.point_dep;
      pnrData.seg.point_arv=next.point_id;
      pnrData.seg.pnr_id=pnr_id;
      pnrData.seg.cls=Qry.FieldAsString("class");
      pnrData.seg.subcls=Qry.FieldAsString("subclass");
      pnrData.seg.pnr_addrs.emplace_back(Qry.FieldAsString("pnr_airline"),
                                         Qry.FieldAsString("pnr_addr"));
    };

    pnrData.dest.fromDB(pnrData.seg.point_arv, true);
  }
  catch(CheckIn::UserException)
  {
    throw;
  }
  catch(UserException &e)
  {
    throw CheckIn::UserException(e.getLexemaData(), pnrData.flt.point_dep);
  };
}

void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc)
{
  emulDoc.set("term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //копируем полностью тег query
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
    CopyNode(destNode, srcNode, true); //копируем полностью XML
}

void CreateEmulDocs(const vector< pair<int/*point_id*/, TWebPnrForSave > > &segs,
                    const vector<WebSearch::TPnrData> &PNRs,
                    const XMLDoc &emulDocHeader,
                    XMLDoc &emulCkinDoc, map<int,XMLDoc> &emulChngDocs)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TDateTime now_local;
  if  (!PNRs.empty())
    now_local=UTCToLocal(NowUTC(), AirpTZRegion(PNRs.begin()->flt.oper.airp));
  else
    now_local=NowUTC();

  //составляем XML-запрос
  vector<WebSearch::TPnrData>::const_iterator iPnrData=PNRs.begin();
  int seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData==PNRs.end()) //лишние сегменты в запросе на регистрацию
        throw EXCEPTIONS::Exception("CreateEmulDocs: iPnrData==PNRs.end() (seg_no=%d)", seg_no);

      TCompleteAPICheckInfo checkInfo(iPnrData->flt.point_dep, iPnrData->dest.airp_arv);

      const TWebPnrForSave &currPnr=s->second;
      //пассажиры для регистрации
      if (!currPnr.paxForCkin.empty())
      {
        if (emulCkinDoc.docPtr()==NULL)
        {
          CopyEmulXMLDoc(emulDocHeader, emulCkinDoc);
          xmlNodePtr emulCkinNode=NodeAsNode("/term/query",emulCkinDoc.docPtr());
          emulCkinNode=NewTextChild(emulCkinNode,"TCkinSavePax");
            NewTextChild(emulCkinNode,"transfer"); //пустой тег - трансфера нет
          NewTextChild(emulCkinNode,"segments");
          NewTextChild(emulCkinNode,"excess",(int)0);
          NewTextChild(emulCkinNode,"hall");
        };
        xmlNodePtr segsNode=NodeAsNode("/term/query/TCkinSavePax/segments",emulCkinDoc.docPtr());

        xmlNodePtr segNode=NewTextChild(segsNode, "segment");
        NewTextChild(segNode,"point_dep",iPnrData->flt.point_dep);
        NewTextChild(segNode,"point_arv",iPnrData->dest.point_arv);
        NewTextChild(segNode,"airp_dep",iPnrData->flt.oper.airp);
        NewTextChild(segNode,"airp_arv",iPnrData->dest.airp_arv);
        NewTextChild(segNode,"class",iPnrData->seg.cls);
        NewTextChild(segNode,"status",EncodePaxStatus(currPnr.status));
        NewTextChild(segNode,"wl_type");

        //коммерческий рейс PNR
        TTripInfo pnrMarkFlt;
        iPnrData->seg.getMarkFlt(iPnrData->flt, false/*is_test*/, pnrMarkFlt);
        TCodeShareSets codeshareSets;
        codeshareSets.get(iPnrData->flt.oper,pnrMarkFlt);

        xmlNodePtr node=NewTextChild(segNode,"mark_flight");
        NewTextChild(node,"airline",pnrMarkFlt.airline);
        NewTextChild(node,"flt_no",pnrMarkFlt.flt_no);
        NewTextChild(node,"suffix",pnrMarkFlt.suffix);
        NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //локальная дата
        NewTextChild(node,"airp_dep",pnrMarkFlt.airp);
        NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);

        //признак запрета саморегистрации без выбора места
        bool with_seat_choice=false;
        if (reqInfo->client_type==ctWeb ||
            reqInfo->client_type==ctKiosk ||
            reqInfo->client_type==ctMobile )
          with_seat_choice=GetSelfCkinSets(tsRegWithSeatChoice, iPnrData->flt.oper, reqInfo->client_type);

        xmlNodePtr paxsNode=NewTextChild(segNode,"passengers");
        for(list<TWebPaxForCkin>::const_iterator iPaxForCkin=currPnr.paxForCkin.begin();iPaxForCkin!=currPnr.paxForCkin.end();iPaxForCkin++)
        {
          try
          {
            vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
            for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
              if (iPaxFromReq->crs_pax_id==iPaxForCkin->crs_pax_id) break;
            if (iPaxFromReq==currPnr.paxFromReq.end())
              throw EXCEPTIONS::Exception("CreateEmulDocs: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForCkin->crs_pax_id);

            if (!iPaxFromReq->seat_no.empty())
            {
              string curr_xname, curr_yname;
              bool changed;
              CheckSeatNoFromReq(iPnrData->flt.point_dep,
                                 iPaxForCkin->crs_pax_id,
                                 iPaxForCkin->seat_no,
                                 iPaxFromReq->seat_no,
                                 curr_xname,
                                 curr_yname,
                                 changed);
            }
            else
            {
              if (iPaxForCkin->seats>0 && with_seat_choice)
                throw UserException("MSG.CHECKIN.NEED_TO_SELECT_SEAT_NO");
            };

            xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
            if (iPaxForCkin->crs_pax_id!=NoExists)
              NewTextChild(paxNode,"pax_id",iPaxForCkin->crs_pax_id);
            else
              NewTextChild(paxNode,"pax_id");
            NewTextChild(paxNode,"surname",iPaxForCkin->surname);
            NewTextChild(paxNode,"name",iPaxForCkin->name);
            NewTextChild(paxNode,"pers_type",iPaxForCkin->pers_type);
            NewTextChild(paxNode,"crew_type",iPaxForCkin->crew_type);
            if (!iPaxFromReq->seat_no.empty())
              NewTextChild(paxNode,"seat_no",iPaxFromReq->seat_no);
            else
              NewTextChild(paxNode,"seat_no",iPaxForCkin->seat_no);
            NewTextChild(paxNode,"seat_type",iPaxForCkin->seat_type);
            NewTextChild(paxNode,"seats",iPaxForCkin->seats);
            //обработка билетов
            string ticket_no;
            if (!iPaxForCkin->eticket.empty())
            {
              //билет TKNE
              ticket_no=iPaxForCkin->eticket;

              int coupon_no=0;
              string::size_type pos=ticket_no.find_last_of('/');
              if (pos!=string::npos)
              {
                if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
                    coupon_no>=1 && coupon_no<=4)
                  ticket_no.erase(pos);
                else
                  coupon_no=0;
              };

              if (ticket_no.empty())
                throw UserException("MSG.ETICK.NUMBER_NOT_SET");
              NewTextChild(paxNode,"ticket_no",ticket_no);
              if (coupon_no<=0)
                throw UserException("MSG.ETICK.COUPON_NOT_SET", LParams()<<LParam("etick", ticket_no ) );
              NewTextChild(paxNode,"coupon_no",coupon_no);
              NewTextChild(paxNode,"ticket_rem","TKNE");
              NewTextChild(paxNode,"ticket_confirm",(int)false);
            }
            else
            {
              ticket_no=iPaxForCkin->ticket;

              NewTextChild(paxNode,"ticket_no",ticket_no);
              NewTextChild(paxNode,"coupon_no");
              if (!ticket_no.empty())
                NewTextChild(paxNode,"ticket_rem","TKNA");
              else
                NewTextChild(paxNode,"ticket_rem");
              NewTextChild(paxNode,"ticket_confirm",(int)false);
            };

            ASTRA::TPaxTypeExt pax_ext(currPnr.status, iPaxForCkin->crew_type);

            if (iPaxForCkin->present_in_req.find(apiDoc) != iPaxForCkin->present_in_req.end())
              CheckDoc(iPaxForCkin->apis.doc, pax_ext, iPaxForCkin->surname, checkInfo, now_local);
            iPaxForCkin->apis.doc.toXML(paxNode);

            if (iPaxForCkin->present_in_req.find(apiDoco) != iPaxForCkin->present_in_req.end())
              CheckDoco(iPaxForCkin->apis.doco, pax_ext, checkInfo, now_local);
            iPaxForCkin->apis.doco.toXML(paxNode);

            for(CheckIn::TDocaMap::const_iterator d = iPaxForCkin->apis.doca_map.begin(); d != iPaxForCkin->apis.doca_map.end(); ++d)
            {
              const CheckIn::TPaxDocaItem &doca = d->second;
              if (iPaxForCkin->present_in_req.find(doca.apiType()) != iPaxForCkin->present_in_req.end())
                CheckDoca(doca, pax_ext, checkInfo);
            }

            xmlNodePtr docaNode=NewTextChild(paxNode, "addresses");
            for(CheckIn::TDocaMap::const_iterator d = iPaxForCkin->apis.doca_map.begin(); d != iPaxForCkin->apis.doca_map.end(); ++d)
              d->second.toXML(docaNode);

            NewTextChild(paxNode,"subclass",iPaxForCkin->subclass);
            NewTextChild(paxNode,"transfer"); //пустой тег - трансфера нет
            NewTextChild(paxNode,"bag_pool_num");
            if (iPaxForCkin->reg_no!=NoExists)
              NewTextChild(paxNode,"reg_no",iPaxForCkin->reg_no);

            //ремарки
            multiset<CheckIn::TPaxRemItem> rems;
            CheckIn::LoadCrsPaxRem(iPaxForCkin->crs_pax_id, rems);
            set<CheckIn::TPaxFQTItem> fqts;
            CheckIn::LoadCrsPaxFQT(iPaxForCkin->crs_pax_id, fqts);
            iPaxFromReq->mergePaxFQT(fqts);
            CreateEmulRems(paxNode, rems, fqts);

            NewTextChild(paxNode,"norms"); //пустой тег - норм нет
            NewTextChild(paxNode, "dont_check_payment", (int)iPaxForCkin->dont_check_payment, (int)false);
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForCkin->crs_pax_id);
          };
        };
      };

      bool isTranzitSalonsVersion = isTranzitSalons( iPnrData->flt.point_dep );
      BitSet<SEATS2::TChangeLayerFlags> change_layer_flags;
      //пассажиры для изменения
      for(list<TWebPaxForChng>::const_iterator iPaxForChng=currPnr.paxForChng.begin();iPaxForChng!=currPnr.paxForChng.end();iPaxForChng++)
      {
        try
        {
          vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
          for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
            if (iPaxFromReq->crs_pax_id==iPaxForChng->crs_pax_id) break;
          if (iPaxFromReq==currPnr.paxFromReq.end())
            throw EXCEPTIONS::Exception("CreateEmulDocs: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForChng->crs_pax_id);

          int pax_tid=iPaxForChng->pax_tid;
          //пассажир зарегистрирован
          if (!iPaxFromReq->refuse &&!iPaxFromReq->seat_no.empty() && iPaxForChng->seats > 0)
          {
            string curr_xname, curr_yname;
            bool changed;
            CheckSeatNoFromReq(iPnrData->flt.point_dep,
                               iPaxForChng->crs_pax_id,
                               iPaxForChng->seat_no,
                               iPaxFromReq->seat_no,
                               curr_xname,
                               curr_yname,
                               changed);
            if (changed)
            {
              if ( isTranzitSalonsVersion ) {
                IntChangeSeatsN( iPnrData->flt.point_dep,
                                 iPaxForChng->crs_pax_id,
                                 pax_tid,
                                 curr_xname, curr_yname,
                                 SEATS2::stReseat,
                                 cltUnknown,
                                 NoExists,
                                 change_layer_flags,
                                 0, NoExists,
                                 NULL );
              }
              else {
                IntChangeSeats( iPnrData->flt.point_dep,
                                iPaxForChng->crs_pax_id,
                                pax_tid,
                                curr_xname, curr_yname,
                                SEATS2::stReseat,
                                cltUnknown,
                                NoExists,
                                change_layer_flags,
                                NULL );
              }
            };
          };

          ASTRA::TPaxTypeExt pax_ext(currPnr.status, iPaxForChng->crew_type);

          bool DocUpdatesPending=false;
          if (iPaxForChng->present_in_req.find(apiDoc) != iPaxForChng->present_in_req.end()) //тег <document> пришел
          {
            CheckDoc(iPaxForChng->doc, pax_ext, iPaxForChng->surname, checkInfo, now_local);
            CheckIn::TPaxDocItem prior_doc;
            LoadPaxDoc(iPaxForChng->crs_pax_id, prior_doc);
            DocUpdatesPending=!(prior_doc.equal(iPaxForChng->doc)); //реагируем также на изменение scanned_attrs
          };

          bool DocoUpdatesPending=false;
          if (iPaxForChng->present_in_req.find(apiDoco) != iPaxForChng->present_in_req.end()) //тег <doco> пришел
          {
            CheckDoco(iPaxForChng->doco, pax_ext, checkInfo, now_local);
            CheckIn::TPaxDocoItem prior_doco;
            LoadPaxDoco(iPaxForChng->crs_pax_id, prior_doco);
            DocoUpdatesPending=!(prior_doco.equal(iPaxForChng->doco)); //реагируем также на изменение scanned_attrs
          };

          bool FQTRemUpdatesPending=false;
          set<CheckIn::TPaxFQTItem> fqts;
          if (iPaxFromReq->fqtv_rems_present) //тег <fqt_rems> пришел
          {
            CheckIn::LoadPaxFQT(iPaxForChng->crs_pax_id, fqts);
            FQTRemUpdatesPending=iPaxFromReq->mergePaxFQT(fqts);
          };

          if (iPaxFromReq->refuse ||
              DocUpdatesPending ||
              DocoUpdatesPending ||
              FQTRemUpdatesPending)
          {
            //придется вызвать транзакцию на запись изменений
            XMLDoc &emulChngDoc=emulChngDocs[iPaxForChng->grp_id];
            if (emulChngDoc.docPtr()==NULL)
            {
              CopyEmulXMLDoc(emulDocHeader, emulChngDoc);

              xmlNodePtr emulChngNode=NodeAsNode("/term/query",emulChngDoc.docPtr());
              emulChngNode=NewTextChild(emulChngNode,"TCkinSavePax");

              xmlNodePtr segNode=NewTextChild(NewTextChild(emulChngNode,"segments"),"segment");
              NewTextChild(segNode,"point_dep",iPaxForChng->point_dep);
              NewTextChild(segNode,"point_arv",iPaxForChng->point_arv);
              NewTextChild(segNode,"airp_dep",iPaxForChng->airp_dep);
              NewTextChild(segNode,"airp_arv",iPaxForChng->airp_arv);
              NewTextChild(segNode,"class",iPaxForChng->cl);
              NewTextChild(segNode,"grp_id",iPaxForChng->grp_id);
              NewTextChild(segNode,"tid",iPaxForChng->pax_grp_tid);
              NewTextChild(segNode,"passengers");

              NewTextChild(emulChngNode,"excess",iPaxForChng->excess);
              NewTextChild(emulChngNode,"hall");
              if (iPaxForChng->bag_refuse)
                NewTextChild(emulChngNode,"bag_refuse",refuseAgentError);
              else
                NewTextChild(emulChngNode,"bag_refuse");
            };
            xmlNodePtr paxsNode=NodeAsNode("/term/query/TCkinSavePax/segments/segment/passengers",emulChngDoc.docPtr());

            xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
            NewTextChild(paxNode,"pax_id",iPaxForChng->crs_pax_id);
            NewTextChild(paxNode,"surname",iPaxForChng->surname);
            NewTextChild(paxNode,"name",iPaxForChng->name);
            if (iPaxFromReq->refuse ||
                DocUpdatesPending ||
                DocoUpdatesPending)
            {
              //были ли изменения по пассажиру CheckInInterface::SavePax определяет по наличию тега refuse
              NewTextChild(paxNode,"refuse",iPaxFromReq->refuse?refuseAgentError:"");
              NewTextChild(paxNode,"pers_type",iPaxForChng->pers_type);
            };
            NewTextChild(paxNode,"tid",pax_tid);

            if (DocUpdatesPending)
              iPaxForChng->doc.toXML(paxNode);

            if (DocoUpdatesPending)
              iPaxForChng->doco.toXML(paxNode);

            if (FQTRemUpdatesPending)
            {
              //ремарки
              multiset<CheckIn::TPaxRemItem> rems;
              CheckIn::LoadPaxRem(iPaxForChng->crs_pax_id, rems);
              CreateEmulRems(paxNode, rems, fqts);
            };
          };
        }
        catch(CheckIn::UserException)
        {
          throw;
        }
        catch(UserException &e)
        {
          throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForChng->crs_pax_id);
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), s->first);
    };
  };
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

