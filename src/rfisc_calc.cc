#include "rfisc_calc.h"

#include <serverlib/xml_stuff.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;

static bool updatePaidRFISC(const CheckIn::TServicePaymentItem &emd,
                            TPaidRFISCList &paid_rfisc,
                            bool only_check)
{
  if (!emd.isEMD() ||
      !emd.pc ||
      !emd.service_quantity_valid()) return false;
  TPaxSegRFISCKey key(Sirena::TPaxSegKey(emd.pax_id, emd.trfer_num), emd.pc.get());
  TPaidRFISCList::iterator p=paid_rfisc.find(key);
  if (p!=paid_rfisc.end() && p->second.need_positive())
  {
    TPaidRFISCItem& item=p->second;
    if (!only_check)
    {
      item.need-=emd.service_quantity;
      if (item.need<0) item.need=0;
    };
    return true;
  };
  return false;
}

static bool confirmed(const CheckIn::TServicePaymentItem &emd,
                      const boost::optional< list<TEMDCtxtItem> > &confirmed_emd)
{
  if (!emd.isEMD() ||
      !emd.pc ||
      !emd.service_quantity_valid()) return false;
  if (!confirmed_emd) return true;
  if (emd.trfer_num!=0) return true;
  for(list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    if (emd.doc_no==i->emd.no &&
        emd.doc_coupon==i->emd.coupon &&
        emd.pax_id==i->pax.id) return true;
  return false;
}

static bool confirmed(const TGrpServiceAutoItem &emd,
                      const boost::optional< list<TEMDCtxtItem> > &confirmed_emd)
{
  if (!emd.isEMD() ||
      !emd.service_quantity_valid()) return false;
  if (!confirmed_emd) return true;
  if (emd.trfer_num!=0) return true;
  for(list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    if (emd.emd_no==i->emd.no &&
        emd.emd_coupon==i->emd.coupon &&
        emd.pax_id==i->pax.id) return true;
  return false;
}

class TQuantityPerSeg : public map<int/*trfer_num*/, int/*кол-во*/>
{
  public:
    void add(int trfer_num, int quantity)
    {
      TQuantityPerSeg::iterator i=insert(make_pair(trfer_num, 0)).first;
      if (i==end()) throw Exception("%s: i==TQuantityPerSeg.end()", __FUNCTION__);
      i->second+=quantity;
    }
    void remove(int trfer_num, int quantity)
    {
      TQuantityPerSeg::iterator i=find(trfer_num);
      if (i!=end())
      {
        i->second-=quantity;
        if (i->second<=0) erase(i);
      };
    }
    string traceStr() const
    {
      ostringstream s;
      for(TQuantityPerSeg::const_iterator i=begin(); i!=end(); ++i)
      {
        if (i!=begin()) s << ", ";
        s << i->first << ":" << i->second;
      }
      return s.str();
    }
};

bool tryEnlargeServicePayment(TPaidRFISCList &paid_rfisc,
                              CheckIn::TServicePaymentList &payment,
                              const TGrpServiceAutoList &svcsAuto,
                              const TCkinGrpIds &tckin_grp_ids,
                              const CheckIn::TGrpEMDProps &emdProps,
                              const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd)
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  if (confirmed_emd)
  {
    ostringstream s;
    for(std::list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    {
      if (i!=confirmed_emd.get().begin()) s << ", ";
      s << i->no_str();
    }
    ProgTrace(TRACE5, "%s: confirmed_emd=%s", __FUNCTION__, s.str().c_str());
  };


  bool result=false;

  class TAddedEmdItem
  {
    public:
      string rfisc;
      string emd_no_base;
      int continuous_segs;
      bool manual_bind;
      list<CheckIn::TServicePaymentItem> coupons;

      TAddedEmdItem(): continuous_segs(0), manual_bind(false) {}
      TAddedEmdItem(const string& _rfisc, const string& _emd_no_base):
        rfisc(_rfisc), emd_no_base(_emd_no_base), continuous_segs(0), manual_bind(false) {}

      bool operator < (const TAddedEmdItem &item) const
      {
        if (continuous_segs!=item.continuous_segs)
          return continuous_segs<item.continuous_segs;
        if (manual_bind!=item.manual_bind)
          return manual_bind;
        return coupons.size()>item.coupons.size();
      }
      bool empty() const
      {
        return rfisc.empty() || emd_no_base.empty() || continuous_segs==0 || coupons.empty();
      }
      string traceStr() const
      {
        ostringstream s;
        s << "rfisc=" << rfisc << ", "
             "emd_no_base=" << emd_no_base << ", "
             "continuous_segs=" << continuous_segs << ", "
             "manual_bind=" << (manual_bind?"true":"false") << ", "
             "coupons=";
        for(list<CheckIn::TServicePaymentItem>::const_iterator i=coupons.begin(); i!=coupons.end(); ++i)
        {
          if (i!=coupons.begin()) s << ", ";
          s << i->no_str();
        };
        return s.str();
      }
  };

  class TBaseEmdMap : public map< string/*emd_no_base*/, set<string/*emd_no*/> >
  {
    public:
      string rfisc;
      TBaseEmdMap(const string& _rfisc) : rfisc(_rfisc) {}
      void add(const TPaxEMDItem& emd)
      {
        if (emd.RFISC!=rfisc) return;
        string emd_no_base=emd.emd_no_base.empty()?emd.emd_no:emd.emd_no_base;
        pair< TBaseEmdMap::iterator, bool > res=insert(make_pair(emd_no_base, set<string>()));
        if (res.first==end()) throw Exception("%s: res.first==end()!", __FUNCTION__);
        res.first->second.insert(emd.emd_no);
      }
      string traceStr() const
      {
        ostringstream s;
        for(TBaseEmdMap::const_iterator i=begin(); i!=end(); ++i)
        {
          if (i!=begin()) s << ", ";
          for(set<string>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
          {
            if (j!=i->second.begin()) s << "/";
            s << *j;
          }
        }
        return s.str();
      }
  };

  class TNeedMap : public map<int/*pax_id*/, map<TRFISCListKey, TQuantityPerSeg> >
  {
    public:
      void add(const TPaidRFISCItem &item)
      {
        if (!item.need_positive()) return;
        TNeedMap::iterator i=find(item.pax_id);
        if (i==end())
          i=insert(make_pair(item.pax_id, map<TRFISCListKey, TQuantityPerSeg>())).first;
        if (i==end()) throw Exception("%s: i==TNeedMap.end()!", __FUNCTION__);
        map<TRFISCListKey, TQuantityPerSeg>::iterator j=i->second.insert(make_pair(item, TQuantityPerSeg())).first;
        if (j==i->second.end()) throw Exception("%s: j==i->second.end()!", __FUNCTION__);
        j->second.add(item.trfer_num, item.need);
      }

      void dump(const string &where) const
      {
        ProgTrace(TRACE5, "%s: === TNeedMap dump ===", where.c_str());
        for(TNeedMap::const_iterator i=begin(); i!=end(); ++i)
        {
          ProgTrace(TRACE5, "%s: pax_id=%d", where.c_str(), i->first);
          for(map<TRFISCListKey, TQuantityPerSeg >::const_iterator j=i->second.begin();
                                                                   j!=i->second.end(); ++j)
          {
            ProgTrace(TRACE5, "%s:     rfisc=%s", where.c_str(), j->first.str(LANG_EN).c_str());
            for(TQuantityPerSeg::const_iterator k=j->second.begin();
                                                k!=j->second.end(); ++k)
              ProgTrace(TRACE5, "%s:         trfer_num=%d, quantity=%d", where.c_str(), k->first, k->second);
          }
        }
      }
  };

  TNeedMap need;
  int max_trfer_num=0;
  for(TPaidRFISCList::iterator p=paid_rfisc.begin(); p!=paid_rfisc.end(); ++p)
  {
    const TPaidRFISCItem& item=p->second;
    if (max_trfer_num<item.trfer_num) max_trfer_num=item.trfer_num;
    need.add(item);
  };

  //need.dump(__FUNCTION__);

  for(TNeedMap::iterator i=need.begin(); i!=need.end(); ++i)
  {
    TPaxEMDList emds;
    emds.getAllPaxEMD(i->first, tckin_grp_ids.size()==1);
    for(const TGrpServiceAutoItem& svcAuto : svcsAuto)
      if (svcAuto.withEMD())
        emds.erase(TPaxEMDItem(svcAuto)); //удаляем уже автопривязанные

    if (emds.empty()) continue; //по пассажиру нет ничего
    for(map<TRFISCListKey, TQuantityPerSeg>::iterator irfisc=i->second.begin(); irfisc!=i->second.end(); ++irfisc)
    {
      TBaseEmdMap base_emds(irfisc->first.RFISC);
      for(multiset<TPaxEMDItem>::const_iterator e=emds.begin(); e!=emds.end(); ++e) base_emds.add(*e);

      ProgTrace(TRACE5, "%s: pax_id=%d, rfisc=%s(%s), emds: %s",
                __FUNCTION__,
                i->first,
                irfisc->first.str(LANG_EN).c_str(),
                irfisc->second.traceStr().c_str(),
                base_emds.traceStr().c_str());

      for(int initial_trfer_num=0; initial_trfer_num<=max_trfer_num && !irfisc->second.empty(); initial_trfer_num++)
      {
        for(;!irfisc->second.empty();)
        {
          TAddedEmdItem best_added;

          for(TBaseEmdMap::const_iterator be=base_emds.begin(); be!=base_emds.end(); ++be)
          {
            TAddedEmdItem curr_added(base_emds.rfisc, be->first);

            for(;curr_added.continuous_segs<=max_trfer_num;curr_added.continuous_segs++)
            {
              {
                //ищем среди привязанных
                CheckIn::TServicePaymentList::const_iterator p=payment.begin();
                for(; p!=payment.end(); ++p)
                  if (p->isEMD() &&
                      p->pc &&
                      p->pc.get().RFISC==curr_added.rfisc &&
                      p->trfer_num==curr_added.continuous_segs &&
                      p->pax_id==i->first &&
                      be->second.find(p->doc_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && p==payment.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && p!=payment.end()) continue;
              }
              {
                //ищем среди непривязанных
                multiset<TPaxEMDItem>::const_iterator e=emds.begin();
                for(; e!=emds.end(); ++e)
                  if (e->service_quantity_valid() &&
                      e->RFISC==curr_added.rfisc &&
                      e->trfer_num==curr_added.continuous_segs &&
                      be->second.find(e->emd_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && e==emds.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && e!=emds.end())
                {
                  boost::optional<TRFISCKey> emdKey=paid_rfisc.getKeyIfSingleRFISC(i->first, e->trfer_num, irfisc->first.RFISC);
                  if (emdKey)  //!emdKey - у пассажира как минимум 2 разных услуги с одним RFISC на одном сегменте и мы не можем определить к какой услуге относится ASVC или EMD
                  {
                    CheckIn::TServicePaymentItem item;
                    item.doc_type="EMD"+e->emd_type;
                    item.doc_no=e->emd_no;
                    item.doc_coupon=e->emd_coupon;
                    item.service_quantity=e->service_quantity;
                    item.pax_id=i->first;
                    item.trfer_num=e->trfer_num;
                    item.pc=emdKey.get();

                    if (confirmed(item, confirmed_emd) &&
                        updatePaidRFISC(item, paid_rfisc, true))
                    {
                      if (emdProps.get(e->emd_no, e->emd_coupon).get_manual_bind())
                        curr_added.manual_bind=true;
                      else
                      {
                        curr_added.coupons.push_back(item);
                        continue;
                      }
                    }
                  }
                }
              }
              break; //сюда дошли - значит к данному сегменту не можем привязать
            }

            if (curr_added.coupons.empty()) continue;
            if (best_added<curr_added) best_added=curr_added;
          }; //emd_no

          if (best_added.empty()) break; //ни одного EMD из непривязанных не можем привязать по текущему RFISC

          ProgTrace(TRACE5, "%s: pax_id=%d, %s", __FUNCTION__, i->first, best_added.traceStr().c_str());

          for(list<CheckIn::TServicePaymentItem>::const_iterator p=best_added.coupons.begin(); p!=best_added.coupons.end(); ++p)
          {
            if (!updatePaidRFISC(*p, paid_rfisc, false)) throw Exception("%s: updatePaidRFISC strange situation!", __FUNCTION__);
            payment.push_back(*p);
            if (p->service_quantity_valid())
              irfisc->second.remove(p->trfer_num, p->service_quantity);
            result=true;
          }
          base_emds.erase(best_added.emd_no_base);
        }
      }; //initial_trfer_num
    } //rfisc
  }

  return result;
}

bool tryCheckinServicesAuto(TGrpServiceAutoList &svcsAuto,
                            const CheckIn::TServicePaymentList &payment,
                            const TCkinGrpIds &tckin_grp_ids,
                            const CheckIn::TGrpEMDProps &emdProps,
                            const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd)
{

  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  bool result=false;

  TPaxEMDList emds;
  emds.getAllEMD(tckin_grp_ids);

  vector< pair< int/*grp_id*/, boost::optional<TTripInfo> > > flts;
  for(const int& grp_id : tckin_grp_ids)
    flts.emplace_back(grp_id, boost::none);

  //попробуем автоматически зарегистрировать
  for(const TPaxEMDItem& emd : emds)
  {
    TGrpServiceAutoItem svcAuto(Sirena::TPaxSegKey(emd.pax_id, emd.trfer_num), emd);

    if (!svcAuto.isSuitableForAutoCheckin()) continue;
    if (svcsAuto.sameDocExists(svcAuto)) continue;
    if (payment.sameDocExists(svcAuto)) continue;
    if (emdProps.get(emd.emd_no, emd.emd_coupon).get_not_auto_checkin()) continue;
    if (!confirmed(svcAuto, confirmed_emd)) continue;
    if (svcAuto.trfer_num<0 || svcAuto.trfer_num>=(int)flts.size()) continue;
    auto& flt=flts.at(svcAuto.trfer_num);
    if (!flt.second)
    {
      flt.second=boost::in_place();
      flt.second.get().getByGrpId(flt.first);
    }
    if (!svcAuto.permittedForAutoCheckin(flt.second.get())) continue;
    svcsAuto.push_back(svcAuto);
    result=true;
  }

  TGrpServiceAutoList asvcWithoutEMD;
  PaxASVCList::getWithoutEMD(tckin_grp_ids.front(), asvcWithoutEMD, false);
  //формируем новый список с подходящими
  TGrpServiceAutoList actualWithoutEMD;
  for(const TGrpServiceAutoItem& svcAuto : asvcWithoutEMD)
  {
    if (!svcAuto.isSuitableForAutoCheckin()) continue;
    if (svcAuto.trfer_num<0 || svcAuto.trfer_num>=(int)flts.size()) continue;
    auto& flt=flts.at(svcAuto.trfer_num);
    if (!flt.second)
    {
      flt.second=boost::in_place();
      flt.second.get().getByGrpId(flt.first);
    }
    if (!svcAuto.permittedForAutoCheckin(flt.second.get())) continue;

    actualWithoutEMD.push_back(svcAuto);

    if (!svcsAuto.removeEqualWithoutEMD(svcAuto)) result=true;
  }

  if (any_of(svcsAuto.begin(), svcsAuto.end(), not1(mem_fun_ref(&TGrpServiceAutoItem::withEMD)))) result=true;

  svcsAuto.replaceWithoutEMDFrom(actualWithoutEMD);

  return result;
}

#include "rfisc_sirena.h"

bool getSvcPaymentStatus(int first_grp_id,
                         xmlNodePtr reqNode,
                         xmlNodePtr externalSysResNode,
                         const RollbackBeforeRequestFunction& rollbackFunction,
                         const SvcSirenaResponseHandler& resHandler,
                         SirenaExchange::TLastExchangeList &SirenaExchangeList,
                         TCkinGrpIds& tckin_grp_ids,
                         TPaidRFISCList& paid,
                         bool& httpWasSent)
{
  tckin_grp_ids.clear();
  paid.clear();

  if (TReqInfo::Instance()->api_mode)
    throw SvcPaymentStatusNotApplicable("SvcPaymentStatus request not applicable for api_mode");

  CheckIn::TPaxGrpCategory::Enum grp_cat;

  SirenaExchange::TPaymentStatusReq req;
  SirenaExchange::fillPaxsBags(first_grp_id, req, grp_cat, tckin_grp_ids);

  if (tckin_grp_ids.empty() || grp_cat!=CheckIn::TPaxGrpCategory::Passenges)
    throw SvcPaymentStatusNotApplicable(first_grp_id);

  LogTrace(TRACE5) << __FUNCTION__ << ": req.svcs.size()=" << req.svcs.size();
  if (req.svcs.empty()) return true;

  try
  {
    SirenaExchange::TPaymentStatusRes res;
//    res.setSrcFile("svc_payment_status_res.xml");
//    res.setDestFile("svc_payment_status_res.xml");

    SirenaExchange::TLastExchangeInfo prior, curr;
    prior.fromDB(first_grp_id);
    req.build(curr.pc_payment_req);
    if (curr.possibleToUseLastResult(prior))
      res.parse(prior.pc_payment_res);
    else
    {
//#define SVC_PAYMENT_STATUS_SYNC_MODE
#ifndef SVC_PAYMENT_STATUS_SYNC_MODE
      if(SirenaExchange::needSync(externalSysResNode)) {
        if (httpWasSent)
          throw Exception("%s: very bad situation! needSyncSirena again!", __FUNCTION__);

        if (rollbackFunction)
          rollbackFunction();

        SvcSirenaInterface::PaymentStatusRequest(reqNode, externalSysResNode, req, resHandler);

        httpWasSent = true;
        return false;
      } else {
        xmlNodePtr answerNode = SirenaExchange::findAnswerNode(externalSysResNode);
        ASSERT(answerNode != NULL);
        res.parseResponse(answerNode);

        curr.grp_id=first_grp_id;

        XMLDoc answerDoc("answer");
        CopyNodeList(NodeAsNode("/answer", answerDoc.docPtr()), answerNode);
        xml_encode_nodelist(answerDoc.docPtr()->children);

        curr.pc_payment_res=XMLTreeToText(answerDoc.docPtr());

        SirenaExchangeList.push_back(curr);
      }
#else
      RequestInfo requestInfo;
      ResponseInfo responseInfo;
      SirenaExchange::SendRequest(req, res, requestInfo, responseInfo);
      curr.grp_id=first_grp_id;
      curr.pc_payment_res=responseInfo.content;
      SirenaExchangeList.push_back(curr);
#endif
    }

    if (req.paxs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: req.paxs.empty()", __FUNCTION__);
    const SirenaExchange::TPaxSegMap &segs=req.paxs.front().segs;
    if (segs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: segs.empty()", __FUNCTION__);
    for(SirenaExchange::TPaxSegMap::const_iterator s=segs.begin(); s!=segs.end(); ++s)
    {
      set<TRFISCListKey> rfiscs;
      res.check_unknown_status(s->second.id, rfiscs);
      if (!rfiscs.empty())
      {
        string flight_view=GetTripName(s->second.operFlt, ecCkin);
        throw UserException("MSG.CHECKIN.UNKNOWN_PAYMENT_STATUS_FOR_BAG_TYPE_ON_SEGMENT",
                            LParams() << LParam("flight", flight_view)
                            << LParam("bag_type", rfiscs.begin()->str()));
      }
    }
    //мы не должны допустить запись в БД статуса unknown
    for(SirenaExchange::TSvcList::const_iterator i=res.svcs.begin(); i!=res.svcs.end(); ++i)
      if (i->status==TServiceStatus::Unknown)
        throw EXCEPTIONS::Exception("%s: strange situation: TServiceStatus::Unknown for trfer_num=%d", __FUNCTION__, i->trfer_num);

    res.normsToDB(tckin_grp_ids);
    res.svcs.get(req.svcs.autoChecked(), paid);
  }
  catch(UserException &e)
  {
    throw;
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
    throw UserException("MSG.CHECKIN.UNABLE_CALC_PAID_BAG_TRY_RE_CHECKIN");
  }

  return true;
}


