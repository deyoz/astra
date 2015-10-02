#include "apis_utils.h"
#include "checkin.h"
#include "checkin_utils.h"
#include "salons.h"
#include "seats.h"
#include "salonform.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "ANNA"

#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC;
using namespace SALONS2;

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

void CreateEmulRems(xmlNodePtr paxNode, TQuery &RemQry, const vector<string> &fqtv_rems)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(;!RemQry.Eof;RemQry.Next())
  {
    const char* rem_code=RemQry.FieldAsString("rem_code");
    const char* rem_text=RemQry.FieldAsString("rem");
    if (isDisabledRem(rem_code, rem_text)) continue;
    if (strcmp(rem_code,"FQTV")==0) continue;
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code",rem_code);
    NewTextChild(remNode,"rem_text",rem_text);
  };
  //добавим переданные fqtv_rems
  for(vector<string>::const_iterator r=fqtv_rems.begin();r!=fqtv_rems.end();r++)
  {
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code","FQTV");
    NewTextChild(remNode,"rem_text",*r);
  };
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
      WebSearch::TPNRAddrInfo pnr_addr;
      pnr_addr.airline=Qry.FieldAsString("pnr_airline");
      pnr_addr.addr=Qry.FieldAsString("pnr_addr");
      pnrData.seg.pnr_addrs.push_back(pnr_addr);
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

  const char* PaxRemQrySQL=
      "SELECT rem_code,rem FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";

  const char* CrsPaxRemQrySQL=
      "SELECT rem_code,rem FROM crs_pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";

  TQuery RemQry(&OraSession);
  RemQry.DeclareVariable("pax_id",otInteger);

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

      TCheckDocInfo checkDocInfo=GetCheckDocInfo(iPnrData->flt.point_dep, iPnrData->dest.airp_arv).pass;

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

            if (iPaxForCkin->present_in_req.find(ciDoc) != iPaxForCkin->present_in_req.end())
              CheckDoc(iPaxForCkin->apis.doc, checkDocInfo.doc, now_local);
            iPaxForCkin->apis.doc.toXML(paxNode);

            if (iPaxForCkin->present_in_req.find(ciDoco) != iPaxForCkin->present_in_req.end())
              CheckDoco(iPaxForCkin->apis.doco, checkDocInfo.doco, now_local);
            iPaxForCkin->apis.doco.toXML(paxNode);

            for(list<CheckIn::TPaxDocaItem>::const_iterator d=iPaxForCkin->apis.doca.begin(); d!=iPaxForCkin->apis.doca.end(); ++d) {
                if (d->type == "B" && iPaxForCkin->present_in_req.find(ciDocaB) != iPaxForCkin->present_in_req.end())
                  CheckDoca(*d, checkDocInfo.docaB);
                else if (d->type == "R" && iPaxForCkin->present_in_req.find(ciDocaR) != iPaxForCkin->present_in_req.end())
                  CheckDoca(*d, checkDocInfo.docaR);
                else if (d->type == "D" && iPaxForCkin->present_in_req.find(ciDocaD) != iPaxForCkin->present_in_req.end())
                  CheckDoca(*d, checkDocInfo.docaD);
            }

            xmlNodePtr docaNode=NewTextChild(paxNode, "addresses");
            for(list<CheckIn::TPaxDocaItem>::const_iterator d=iPaxForCkin->apis.doca.begin(); d!=iPaxForCkin->apis.doca.end(); ++d)
              d->toXML(docaNode);

            NewTextChild(paxNode,"subclass",iPaxForCkin->subclass);
            NewTextChild(paxNode,"transfer"); //пустой тег - трансфера нет
            NewTextChild(paxNode,"bag_pool_num");
            if (iPaxForCkin->reg_no!=NoExists)
              NewTextChild(paxNode,"reg_no",iPaxForCkin->reg_no);

            //ремарки
            RemQry.SQLText=CrsPaxRemQrySQL;
            RemQry.SetVariable("pax_id",iPaxForCkin->crs_pax_id);
            RemQry.Execute();
            CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);

            NewTextChild(paxNode,"norms"); //пустой тег - норм нет
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

          int pax_tid=iPaxFromReq->pax_tid;
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
                                 change_layer_flags,
                                 NULL );
              }
              else {
                IntChangeSeats( iPnrData->flt.point_dep,
                                iPaxForChng->crs_pax_id,
                                pax_tid,
                                curr_xname, curr_yname,
                                SEATS2::stReseat,
                                cltUnknown,
                                change_layer_flags,
                                NULL );
              }
            };
          };

          bool DocUpdatesPending=false;
          if (iPaxForChng->present_in_req.find(ciDoc) != iPaxForChng->present_in_req.end()) //тег <document> пришел
          {
            CheckDoc(iPaxForChng->doc, checkDocInfo.doc, now_local);
            CheckIn::TPaxDocItem prior_doc;
            LoadPaxDoc(iPaxForChng->crs_pax_id, prior_doc);
            DocUpdatesPending=!(prior_doc.equal(iPaxForChng->doc)); //реагируем также на изменение scanned_attrs
          };

          bool DocoUpdatesPending=false;
          if (iPaxForChng->present_in_req.find(ciDoco) != iPaxForChng->present_in_req.end()) //тег <doco> пришел
          {
            CheckDoco(iPaxForChng->doco, checkDocInfo.doco, now_local);
            CheckIn::TPaxDocoItem prior_doco;
            LoadPaxDoco(iPaxForChng->crs_pax_id, prior_doco);
            DocoUpdatesPending=!(prior_doco.equal(iPaxForChng->doco)); //реагируем также на изменение scanned_attrs
          };

          bool FQTRemUpdatesPending=false;
          if (iPaxFromReq->fqt_rems_present) //тег <fqt_rems> пришел
          {
            vector<string> prior_fqt_rems;
            //читаем уже записанные ремарки FQTV
            RemQry.SQLText="SELECT rem FROM pax_rem WHERE pax_id=:pax_id AND rem_code='FQTV'";
            RemQry.SetVariable("pax_id", iPaxForChng->crs_pax_id);
            RemQry.Execute();
            for(;!RemQry.Eof;RemQry.Next()) prior_fqt_rems.push_back(RemQry.FieldAsString("rem"));
            //сортируем и сравниваем
            sort(prior_fqt_rems.begin(),prior_fqt_rems.end());
            FQTRemUpdatesPending=prior_fqt_rems!=iPaxFromReq->fqt_rems;
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
              NewTextChild(segNode,"tid",iPaxFromReq->pax_grp_tid);
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
              RemQry.SQLText=PaxRemQrySQL;
              RemQry.SetVariable("pax_id",iPaxForChng->crs_pax_id);
              RemQry.Execute();
              CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);
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
