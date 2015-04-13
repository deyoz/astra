#include "read_edi_elements.h"
#include "edi_elements.h"
#include "astra_ticket.h"

#include <serverlib/isdigit.h>
#include <serverlib/dates.h>
#include <edilib/edi_func_cpp.h>
#include <etick/edi_cast.h>
#include <etick/tick_data.h>

#include <numeric>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>


namespace Ticketing {
namespace TickReader{
    using namespace edilib;
    using namespace edifact;

boost::optional<TktElem> readEdiTkt(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder tkt_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "TKT"))
        return boost::optional<TktElem>();

    TktElem res;
    PushEdiPointG(pMes);
    SetEdiPointToCompositeG(pMes, "C667",0, "EtsErr::INV_TICKNUM");

    res.m_tickStatAction = GetDBNumCast<TickStatAction::TickStatAction_t>
            (EdiCast::TickActCast("EtErr::INV_TICK_ACT"), pMes, 9988);

    std::string ticktmp = GetDBNum(pMes, DataElement(1004,0,0), "EtsErr::INV_TICKNUM");
    if(res.m_tickStatAction == TickStatAction::oldtick)
        ticktmp = TicketNum_t::cut_check_digit(ticktmp);

    TicketNum_t::check(ticktmp);
    res.m_ticketNum = TicketNum_t(ticktmp);
    res.m_docType  = GetDBNumCast<DocType>(EdiCast::DocTypeCast("EtsErr::INV_DOC_TYPE"),
                                           pMes, 1001,0, "EtsErr::INV_DOC_TYPE");
    int Nbooklets  = GetDBNumCast<int>(EdiDigitCast<int>("EtsErr::INV_COUPON", "-1"), pMes, 7240);
    if(Nbooklets >= 0)
        res.m_nBooklets = Nbooklets;

    if(res.m_tickStatAction == TickStatAction::inConnectionWith) {
        ticktmp = GetDBNum(pMes, DataElement(1004,0,1), "EtsErr::INV_TICKNUM");
        TicketNum_t::check(ticktmp);
        res.m_inConnectionTicketNum = TicketNum_t(ticktmp);
    }
    PopEdiPointG(pMes);

    LogTrace(TRACE3) << res;

    return res;
}

boost::optional<CpnElem> readEdiCpn(_EDI_REAL_MES_STRUCT_ *pMes, int numCpn)
{
    EdiPointHolder cpn_holder(pMes); (void) cpn_holder;
    boost::optional<CpnElem> res;

    if(!SetEdiPointToSegmentG(pMes, "CPN", numCpn))
        return res;

    int numCPN = GetNumComposite(pMes, "C640", "INV_COUPON");
    if( numCPN > 1 ) {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) <<
                            "Bad number of C640 (" << numCPN << ")! (More then 1)";
    }

    res.reset(CpnElem());

    SetEdiPointToCompositeG(pMes, "C640");
    const unsigned numberOfCpnNumbers = GetNumDataElem(pMes, 1050);
    if(numberOfCpnNumbers) {
        unsigned num = GetDBNumCast<unsigned>(Ticketing::EdiCast::EdiDigitCast<unsigned>("INV_COUPON"),
                                              pMes, 1050);
        if(num < 1 || num > 4)
            throw EXCEPTIONS::ExceptionFmt(STDLOG) << "invalid coupon num (" << num <<")!";
        res->m_num = Ticketing::CouponNum_t(num);
    }

    if(GetNumDataElem(pMes, 1159)) {
        res->m_media = GetDBNumCast<Ticketing::TicketMedia>(Ticketing::EdiCast::TicketMediaCast("INV_MEDIA"),
                                                            pMes, 1159);
    }

    if(GetNumDataElem(pMes, 4405)) {
        res->m_status = GetDBNumCast<Ticketing::CouponStatus>(Ticketing::EdiCast::CoupStatCast("INV_COUPON"),
                                                              pMes, 4405);
        if(res->m_status == Ticketing::CouponStatus::Paper || res->m_status == Ticketing::CouponStatus::Printed) {
            if(res->m_media && res->m_media == Ticketing::TicketMedia::Electronic) {
                throw EXCEPTIONS::ExceptionFmt(STDLOG) <<
                                "Invalid coupon media [" << res->m_media->code() << "]!";
            }
        }
    }

    std::string amount = GetDBNum(pMes, 5004);
    if(!amount.empty())
        res->m_amount = Ticketing::TaxAmount::Amount(amount);

    res->m_sac    = GetDBNum(pMes, 9887);
    res->m_action = GetDBNum(pMes, 1229);

    if(numberOfCpnNumbers > 1) {
        unsigned num = GetDBNumCast<unsigned>(Ticketing::EdiCast::EdiDigitCast<unsigned>("INV_COUPON"),
                                              pMes, 1050, 1);
        if(num < 1 || num > 4)
            throw EXCEPTIONS::ExceptionFmt(STDLOG) << "invalid coupon num (" << num <<")!";
        res->m_connectedNum = Ticketing::CouponNum_t(num);
    }

    LogTrace(TRACE3) << "CPN: " << *res;

    return res;
}

boost::optional<PtsElem> readEdiPts(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder pts_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PTS"))
        return boost::optional<PtsElem>();

    PtsElem res;

    res.m_itemNumber = GetDBNumCast<int>(EdiCast::EdiDigitCast<int>("EtsErr::EDI_PROC_ERR", "-1"), pMes, 7140);
    res.m_fareBasis  = GetDBFName(pMes, DataElement(5242), CompElement("C643"));
    res.m_rfic       = GetDBNum  (pMes, DataElement(4183, 0, 0));
    res.m_rfisc      = GetDBNum  (pMes, DataElement(4183, 0, 1));

    //LogTrace(TRACE3) << res;

    return res;
}

boost::optional<EbdElem> readEdiEbd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ebd_holer(pMes);
    if(!SetEdiPointToSegmentG(pMes, "EBD") || !SetEdiPointToCompositeG(pMes, "C675"))
        return boost::optional<EbdElem>();

    int quantity       = GetDBNumCast<int>(EdiDigitCast<int>("EtErr::ETS_INV_LUGGAGE", "0"), pMes, 6060, 0);
    std::string charge = GetDBNum(pMes, DataElement(5463), "EtErr::ETS_INV_LUGGAGE");
    std::string measure= GetDBNum(pMes, 6411);

    //LogTrace(TRACE3) << res;

    return EbdElem(quantity, charge, measure);
}

boost::optional<TvlElem> readEdiTvl(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder tvl_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "TVL"))
        return boost::optional<TvlElem>();

    const std::string depDate  = GetDBFName(pMes, DataElement(9916,0), CompElement("C310",0));
    const std::string depTime  = GetDBFName(pMes, DataElement(9918,0), CompElement("C310",0));
    const std::string arrDate  = GetDBFName(pMes, DataElement(9920,0), CompElement("C310",0));
    const std::string arrTime  = GetDBFName(pMes, DataElement(9922,0), CompElement("C310",0));
    const std::string depPoint = GetDBFName(pMes, DataElement(3225,0), CompElement("C328",0));
    const std::string arrPoint = GetDBFName(pMes, DataElement(3225,0), CompElement("C328",1));
    const std::string comp     = GetDBFName(pMes, DataElement(9906,0), CompElement("C306",0));
    const std::string operComp = GetDBFName(pMes, DataElement(9906,1), CompElement("C306",0));
    const std::string flNum    = GetDBFName(pMes, DataElement(9908,0), CompElement("C308",0));
    const std::string operFlNum= GetDBFName(pMes, DataElement(9908,1), CompElement("C308",0));

    TvlElem res;
    if(!depDate.empty())
        res.m_depDate = Dates::ddmmrr(depDate);
    if(!depTime.empty())
        res.m_depTime = Dates::hh24mi(depTime);
    if(!arrDate.empty())
        res.m_arrDate = Dates::ddmmrr(arrDate);
    if(!arrTime.empty())
        res.m_arrTime = Dates::hh24mi(arrTime);
    if(!depPoint.empty())
        res.m_depPoint = depPoint; // getPointId(depPoint);
    if(!arrPoint.empty())
        res.m_arrPoint = arrPoint; // getPointId(arrPoint);
    if(!comp.empty())
        res.m_airline = comp; // getAirlineId(comp);
    if(!operComp.empty())
        res.m_operAirline = operComp; // getAirlineId(operComp);
    if(!flNum.empty())
        res.m_flNum = getFlightNum(flNum);
    if(!operFlNum.empty())
        res.m_operFlNum = getFlightNum(operFlNum);

    //LogTrace(TRACE3) << res;

    return res;
}

RciElements readEdiResControlInfo(_EDI_REAL_MES_STRUCT_ *pMes)
{
    RciElements elems;

    // TODO
//    EdiPointHolder ph(pMes);
//    if(!SetEdiPointToSegmentG(pMes, "RCI")) {
//        return elems;
//    }

//    unsigned Num = GetNumComposite(pMes, "C330", "EtsErr::INV_SYS_CONTROL");

//    if(Num == 0) {
//        throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_SYS_CONTROL",
//                                              "Bad number of record locators [%d]. "
//                                              "Must be 1,2 or more", Num);
//    }

//    EdiPointHolder ph2(pMes);
//    for(unsigned i = 0; i < Num; i++)
//    {
//        SetEdiPointToCompositeG(pMes, "C330", i, "EtErr::ProgErr");
//        std::string recloc = GetDBNum(pMes, 9956,0, "EtsErr::NEED_RECLOC");

//        std::string typeChr = GetDBNum(pMes, 9958);
//        if (typeChr.empty())
//        {
//            typeChr = "1";
//        }

//        if(typeChr == "1")
//        {
//            std::string Awk = GetDBFName(pMes, 9906,0, "EtsErr::INV_AIRLINE");
//            elems.Reclocs.push_back(RciElement(getAirlineId(Awk), recloc, typeChr));
//        }
//        else
//        {
//            if(typeChr == "6" || typeChr == "8")
//            {
//                elems.Foid.push_back(FormOfId(FoidType(boost::lexical_cast<int>(typeChr)), recloc, ""));
//            }
//            else
//            {
//                LogWarning(STDLOG) << "Ignore rci type " << typeChr << ":" << recloc;
//            }
//        }

//        PopEdiPoint_wdG(pMes);
//    }

    return elems;
}

RciElements readEdiResControlInfoCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    RciElements rci2 = readEdiResControlInfo(pMes);
    ResetEdiPointG(pMes);
    RciElements rci0 = readEdiResControlInfo(pMes);

    return rci0 + rci2;
}

static void readSingleTxd(_EDI_REAL_MES_STRUCT_ *pMes, std::list<TaxDetails> &ltxd)
{
    TaxCategory TCat = GetDBNumCast<TaxCategory>
            (EdiCast::TaxCategoryCast("EtErr::INV_TAX_AMOUNT", TaxCategory::Current),
             pMes, DataElement(5305)); //Category

    unsigned tax_num = GetNumComposite(pMes, "C668", "EtErr::INV_TAX_AMOUNT");

    EdiPointHolder c668_holder(pMes);

    for (unsigned i = 0; i < tax_num; i++)
    {
        SetEdiPointToCompositeG(pMes, "C668", i, "EtErr::INV_TAX_AMOUNT");

        std::string tax_amount = GetDBNum(pMes, 5278,0);
        // �� ��������� �������� ����� �ਤ� ⠪� ��� �㬬�, �ந�����㥬 ��.
        if(!tax_amount.empty() && tax_amount != TaxAmount::Amount::TaxExempt)
        {
            TaxAmount::Amount Am = EdiCast::amountFromString(tax_amount);

            std::string Type = GetDBNum(pMes, 5153,0, "EtsErr::INV_TAX_CODE");

            ltxd.push_back(TaxDetails(Am, TCat, Type));
        }
        else if(!tax_amount.empty())
        {
            LogTrace(TRACE2) << TaxAmount::Amount::TaxExempt;
            std::string Type = GetDBNum(pMes, 5153,0); //Code
            ltxd.push_back(TaxDetails(TaxAmount::Amount::TaxExempt, TCat, Type));
        }
        PopEdiPoint_wdG(pMes);
    }
}

TxdElements readEdiTxd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    TxdElements result;
    EdiPointHolder txd_holder(pMes);

    unsigned tax_mnum = GetNumSegment(pMes, "TXD");
    for(unsigned t = 0; t < tax_mnum; t++) {

        SetEdiPointToSegmentG(pMes, "TXD", t);
        readSingleTxd(pMes, result.m_lTax);
        PopEdiPoint_wdG(pMes);
    }

    return result;
}

TxdElements readEdiTxdCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    TxdElements txt2 = readEdiTxd(pMes);
    ResetEdiPointG(pMes);
    TxdElements txt0 = readEdiTxd(pMes);

    return txt2 + txt0;
}

MonElements readEdiMon(_EDI_REAL_MES_STRUCT_ *pMes)
{
    MonElements result;
    EdiPointHolder mon_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "MON", 0))
        return result;

    unsigned MonNum = GetNumComposite(pMes, "C663", "EtErr::MISS_MONETARY_INF");

    EdiPointHolder C663_holder(pMes);
    for(unsigned i=0; i < MonNum; i++)
    {
        MonElem mon;
        C663_holder.popNoDel();
        SetEdiPointToCompositeG(pMes, "C663", i, "EtErr::ProgErr");

        mon.m_code = GetDBNumCast<AmountCode>
                (EdiCast::AmountCodeCast("EtErr::MISS_MONETARY_INF"), pMes, DataElement(5025),
                 "EtErr::MISS_MONETARY_INF");

        mon.m_value   = GetDBNum(pMes, DataElement(1230));
        mon.m_currency= GetDBNum(pMes, DataElement(6345));
        if(!mon.m_value.empty() && mon.m_value[mon.m_value.length() - 1] == 'A') {
            mon.m_addCollect = true;
            mon.m_value = mon.m_value.substr(0, mon.m_value.length()-1);
        }

        result.m_lMon.push_back(mon);
    }

    return result;
}

MonElements readEdiMonCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    MonElements mon2 = readEdiMon(pMes);
    ResetEdiPointG(pMes);
    MonElements mon0 = readEdiMon(pMes);

    return mon2 + mon0;
}

void readEdiMonetaryInfo(const std::list<MonElem> &mons, std::list<MonetaryInfo> &lmon)
{
    BOOST_FOREACH(const MonElem &mon, mons) {
        lmon.push_back(MonetaryInfo(mon.m_code, TaxAmount::Amount(mon.m_value), mon.m_currency));
    }
}

FopElements readEdiFopCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    FopElements fop2 = readEdiFop(pMes);
    ResetEdiPointG(pMes);
    FopElements fop0 = readEdiFop(pMes);

    return fop2 + fop0;
}

FopElements readEdiFop(_EDI_REAL_MES_STRUCT_ *pMes)
{
    FopElements result;
    EdiPointHolder fop_holder(pMes);

    if(!SetEdiPointToSegmentG(pMes, SegmElement("FOP"))) {
        tst();
        return result;
    }

    unsigned Num = GetNumComposite(pMes, "C641", "EtsErr::INV_FOP");

    EdiPointHolder C641_holder(pMes);
    for(unsigned i=0; i < Num; i++) {
        SetEdiPointToCompositeG(pMes, "C641", i, "EtErr::ProgErr");

        //FOP type
        std::string FOPcode = GetDBNum(pMes, 9888);

        FopIndicator FopInd = GetDBNumCast<FopIndicator>
                (EdiCast::FopIndicatorCast("EtsErr::INV_FOP", FopIndicator::New), pMes,
                 DataElement(9988)); //Indicator

        // Amount
        TaxAmount::Amount Am = GetDBNumCast<TaxAmount::Amount>
                (EdiCast::AmountCast("EtsErr::INV_AMOUNT"), pMes, 5004); //Amount

        std::string Vendor = GetDBNum(pMes, 9906);

        if(Vendor.size() &&
           (Vendor.size() > FormOfPayment::VCMax ||
            Vendor.size() < FormOfPayment::VCMin))
        {
            throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_FOP", "Invalid vendor code");
        }

        std::string AccountNum = GetDBNum(pMes, 1154);

        if(!AccountNum.empty() && !Vendor.empty() &&
           (AccountNum.size() > FormOfPayment::CrdNoMax ||
            AccountNum.size() < FormOfPayment::CrdNoMin))
        {
            throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_FOP", "Invalid account number");
        }

        Dates::date date1 = GetDBNumCast<Dates::date>(EdiCast::DateCast("%m%y", "EtErr::INV_DATE"),
                                      pMes, 9916);

        std::string AppCode = GetDBNum(pMes, 9889);
        if(AppCode.size() > FormOfPayment::AppCodeMax)
        {
            throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_FOP",
                                                   "Invalid approval code");
        }

        std::string free_text = GetDBNum(pMes, 4440);

        result.m_lFop.push_back(FormOfPayment(FOPcode, FopInd, Am, Vendor, AccountNum, date1, AppCode));
        result.m_lFop.back().setFreeText(free_text);

        PopEdiPoint_wdG(pMes); // Point to FOP
    }

    return result;
}

FreeTextInfo readSingleIft(_EDI_REAL_MES_STRUCT_ *pMes, unsigned num, unsigned level
                           /*,TicketNum_t currTicket, CouponNum_t currCoupon*/)
{
    boost::optional<FreeTextInfo> ift;
    EdiPointHolder inside_ift(pMes);

    SetEdiPointToCompositeG(pMes, "C346",0, "EtErr::INV_IFT_QUALIFIER");
    std::string Qualifier   = GetDBNum(pMes, 4451,0, "EtErr::INV_IFT_QUALIFIER");
    std::string Type        = GetDBNum(pMes, 9980);
    std::string pricing_ind = GetDBNum(pMes, 4405);

    inside_ift.pop();

    unsigned numDText = GetNumDataElem(pMes, 4440);
    if (!numDText && pricing_ind.empty()) {
        throw TickExceptions::tick_soft_except(STDLOG,"EtErr::INV_IFT_QUALIFIER","Missing IFT data");
    }

    ift = FreeTextInfo(num, level, FreeTextType(Type, Qualifier), ""/*, currTicket, currCoupon*/);
    //ift->setPricingInd(pricing_ind);
    LogTrace(TRACE3) << "IFT DATA count=" << numDText;

    for(unsigned j = 0; j < numDText; j++) {

        ProgTrace(TRACE3,"getting 4440, %d", j);
        std::string Text = GetDBNum(pMes, 4440,j, "EtErr::ProgErr");
        LogTrace(TRACE3) << Qualifier << ":" << Type << ":" << pricing_ind << ":" << Text;
        ift->addText(Text);
    }

    return *ift;
}

IftElements readEdiIft(_EDI_REAL_MES_STRUCT_ *pMes, unsigned level
                       /*,TicketNum_t currTicket, CouponNum_t currCoupon*/)
{
    IftElements result;

    EdiPointHolder ift_holder(pMes);

    unsigned num = GetNumSegment(pMes, "IFT");
    if(!num) {
        ProgTrace(TRACE3,"There are no IFT information found at level %d", level);
        return result;
    }

    ProgTrace(TRACE3,"IFT count=%d", num);
    for(unsigned i=0; i < num; i++)
    {
        SetEdiPointToSegmentG(pMes, "IFT", i, "EtErr::ProgErr");
        result.m_lIft.push_back(readSingleIft(pMes, i, level/*, currTicket, currCoupon*/));
        PopEdiPoint_wdG(pMes);
    }

    return result;
}

IftElements readEdiIftCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes, unsigned level/*,
                               TicketNum_t currTicket, CouponNum_t currCoupon*/)
{
    EdiPointHolder ph(pMes);

    IftElements ift2 = readEdiIft(pMes, level/*, currTicket, currCoupon*/);
    ResetEdiPointG(pMes);
    IftElements ift0 = readEdiIft(pMes, 0/*, currTicket, currCoupon*/);

    return ift2 + ift0;
}

boost::optional<TourCode_t> readEdiTourCodeCurrOr0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    if(!GetNumSegment(pMes, "ATI"))
    {
        ResetEdiPointG(pMes);
    }

    return readEdiTourCode(pMes);;
}

boost::optional<TourCode_t> readEdiTourCode(_EDI_REAL_MES_STRUCT_ *pMes)
{
    if(!GetNumSegment(pMes, "ATI")) {
        return boost::optional<TourCode_t>();
    }

    // ��६ ATI ���� � ᠬ��� ����, ���� ������ (��� TIF)
    std::string tc = GetDBFName(pMes,
                                DataElement(9908),
                                CompElement("C993"),
                                SegmElement("ATI"));

    return TourCode_t(tc);
}

boost::optional<TicketingAgentInfo_t> readEdiTicketAgnInfoCurrOr0(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ph(pMes);

    if(!GetNumSegment(pMes, "TAI")) {
        ResetEdiPointG(pMes);
    }

    return readEdiTicketAgnInfo(pMes);
}

boost::optional<TicketingAgentInfo_t> readEdiTicketAgnInfo(EDI_REAL_MES_STRUCT *pMes)
{
    if(!GetNumSegment(pMes, "TAI"))
    {
        return boost::optional<TicketingAgentInfo_t>();
    }
    // ��६ TAI ���� � ᠬ��� ����, ���� ������ (��� TIF)
    std::string systemId = GetDBFName(pMes,
                                DataElement(9996),
                                SegmElement("TAI"));
    std::string agentId = GetDBFName(pMes,
                                DataElement(9902),
                                CompElement("C642"),
                                SegmElement("TAI"));
    std::string agentType = GetDBFName(pMes,
                                DataElement(9893),
                                CompElement("C642"),
                                SegmElement("TAI"));


    return TicketingAgentInfo_t(systemId, agentId, agentType);
}


// IATCI
boost::optional<LorElem> readEdiLor(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder lor_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "LOR")) {
        return boost::optional<LorElem>();
    }

    LorElem lor;
    lor.m_airline = GetDBFName(pMes, DataElement(3127), CompElement("C059"));
    lor.m_port    = GetDBFName(pMes, DataElement(3800), CompElement("C059"));

    LogTrace(TRACE3) << lor;

    return lor;
}

boost::optional<FdqElem> readEdiFdq(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder fdq_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "FDQ")) {
        return boost::optional<FdqElem>();
    }

    std::string outbAirl  = GetDBFName(pMes, DataElement(3127), CompElement("C013"));
    std::string outbFlNum = GetDBFName(pMes, DataElement(3802), CompElement("C014"));
    std::string outbDepDateTime = GetDBFName(pMes, DataElement(2281, 0, 0), CompElement());
    std::string outbDepPoint = GetDBFName(pMes, DataElement(3215, 0, 0), CompElement());
    std::string outbArrPoint = GetDBFName(pMes, DataElement(3259, 0, 0), CompElement());
    std::string inbAirl = GetDBFName(pMes, DataElement(3127), CompElement("C015"));
    std::string inbFlNum = GetDBFName(pMes, DataElement(3802), CompElement("C016"));
    std::string inbDepDateTime = GetDBFName(pMes, DataElement(2281, 0, 1), CompElement());
    std::string inbArrDateTime = GetDBFName(pMes, DataElement(2107), CompElement());
    std::string inbDepPoint = GetDBFName(pMes, DataElement(3215, 0, 1), CompElement());
    std::string inbArrPoint = GetDBFName(pMes, DataElement(3259, 0, 1), CompElement());

    FdqElem fdq;
    fdq.m_outbAirl = outbAirl;
    if(!outbFlNum.empty()) {
        fdq.m_outbFlNum = getFlightNum(outbFlNum);
    }
    if(outbDepDateTime.length() == 6) {
        fdq.m_outbDepDateTime = Dates::DateTime_t(Dates::rrmmdd(outbDepDateTime));
    } else if(outbDepDateTime.length() == 10) {
        fdq.m_outbDepDateTime = Dates::DateTime_t(Dates::rrmmdd(outbDepDateTime.substr(0, 6)),
                                                  Dates::hh24mi(outbDepDateTime.substr(6, 4)));
    }
    fdq.m_outbDepPoint = outbDepPoint;
    fdq.m_outbArrPoint = outbArrPoint;
    fdq.m_inbAirl = inbAirl;
    if(!inbFlNum.empty()) {
        fdq.m_inbFlNum = getFlightNum(inbFlNum);
    }
    if(inbDepDateTime.length() == 6) {
        fdq.m_inbDepDateTime = Dates::DateTime_t(Dates::rrmmdd(inbDepDateTime));
    } else if(inbDepDateTime.length() == 10) {
        fdq.m_inbDepDateTime = Dates::DateTime_t(Dates::rrmmdd(inbDepDateTime.substr(0, 6)),
                                                 Dates::hh24mi(inbDepDateTime.substr(6, 4)));
    }
    if(inbArrDateTime.length() == 6) {
        fdq.m_inbArrDateTime = Dates::DateTime_t(Dates::rrmmdd(inbArrDateTime));
    } else if(inbArrDateTime.length() == 10) {
        fdq.m_inbArrDateTime = Dates::DateTime_t(Dates::rrmmdd(inbArrDateTime.substr(0, 6)),
                                                 Dates::hh24mi(inbArrDateTime.substr(6, 4)));
    }
    fdq.m_inbDepPoint = inbDepPoint;
    fdq.m_inbArrPoint = inbArrPoint;

    LogTrace(TRACE3) << fdq;

    return fdq;
}

boost::optional<PpdElem> readEdiPpd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder ppd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PPD")) {
        return boost::optional<PpdElem>();
    }

    PpdElem ppd;
    ppd.m_passSurname = GetDBFName(pMes, DataElement(3808), CompElement());
    ppd.m_passType    = GetDBFName(pMes, DataElement(9819), CompElement("C017"));
    ppd.m_passName    = GetDBFName(pMes, DataElement(3809), CompElement());
    ppd.m_passRespRef = GetDBFName(pMes, DataElement(9821), CompElement("C692"));
    ppd.m_passQryRef  = GetDBFName(pMes, DataElement(9821), CompElement("C690"));

    LogTrace(TRACE3) << ppd;

    return ppd;
}

boost::optional<PrdElem> readEdiPrd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder prd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PRD")) {
        return boost::optional<PrdElem>();
    }

    PrdElem prd;
    prd.m_rbd = GetDBFName(pMes, DataElement(9800), CompElement("C023"));

    LogTrace(TRACE3) << prd;

    return prd;
}

boost::optional<PsdElem> readEdiPsd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder psd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PSD")) {
        return boost::optional<PsdElem>();
    }

    PsdElem psd;
    psd.m_noSmokingInd   = GetDBFName(pMes, DataElement(9807), CompElement("C024"));
    psd.m_characteristic = GetDBFName(pMes, DataElement(9825), CompElement("C024"));

    LogTrace(TRACE3) << psd;

    return psd;
}

boost::optional<PbdElem> readEdiPbd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder pbd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PBD")) {
        return boost::optional<PbdElem>();
    }

    PbdElem pbd;
    pbd.m_numOfPieces = GetDBFNameCast<unsigned>(EdiDigitCast<unsigned>(), pMes,
                                                 DataElement(6806), "", CompElement("C027"));
    pbd.m_weight = GetDBFNameCast<unsigned>(EdiDigitCast<unsigned>(), pMes,
                                            DataElement(6803), "", CompElement("C027"));

    LogTrace(TRACE3) << pbd;

    return pbd;
}

boost::optional<edifact::FdrElem> readEdiFdr(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder fdr_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "FDR")) {
        return boost::optional<FdrElem>();
    }

    std::string airl  = GetDBFName(pMes, DataElement(3127), CompElement("C013"));
    std::string flNum = GetDBFName(pMes, DataElement(3802), CompElement("C014"));
    std::string depDateTime = GetDBFName(pMes, DataElement(2281, 0, 0), CompElement());
    std::string arrDateTime = GetDBFName(pMes, DataElement(2107, 0, 0), CompElement());
    std::string depPoint = GetDBFName(pMes, DataElement(3215, 0, 0), CompElement());
    std::string arrPoint = GetDBFName(pMes, DataElement(3259, 0, 0), CompElement());

    FdrElem fdr;
    fdr.m_airl = airl;
    if(!flNum.empty()) {
        fdr.m_flNum = getFlightNum(flNum);
    }
    if(depDateTime.length() == 6) {
        fdr.m_depDateTime = Dates::DateTime_t(Dates::rrmmdd(depDateTime));
    } else if(depDateTime.length() == 10) {
        fdr.m_depDateTime = Dates::DateTime_t(Dates::rrmmdd(depDateTime.substr(0, 6)),
                                              Dates::hh24mi(depDateTime.substr(6, 4)));
    }
    fdr.m_depPoint = depPoint;
    fdr.m_arrPoint = arrPoint;
    if(arrDateTime.length() == 6) {
        fdr.m_arrDateTime = Dates::DateTime_t(Dates::rrmmdd(arrDateTime));
    } else if(arrDateTime.length() == 10) {
        fdr.m_arrDateTime = Dates::DateTime_t(Dates::rrmmdd(arrDateTime.substr(0, 6)),
                                              Dates::hh24mi(arrDateTime.substr(6, 4)));
    }

    LogTrace(TRACE3) << fdr;

    return fdr;
}

boost::optional<edifact::RadElem> readEdiRad(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder rad_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "RAD")) {
        return boost::optional<RadElem>();
    }

    RadElem rad;
    rad.m_respType = GetDBFName(pMes, DataElement(9868), CompElement());
    rad.m_status   = GetDBFName(pMes, DataElement(9869), CompElement());

    LogTrace(TRACE3) << rad;

    return rad;
}

boost::optional<edifact::PfdElem> readEdiPfd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder pfd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "PFD")) {
        return boost::optional<PfdElem>();
    }

    PfdElem pfd;
    pfd.m_seat = GetDBFName(pMes, DataElement(9809), CompElement("C043"));
    pfd.m_noSmokingInd = GetDBFName(pMes, DataElement(9807), CompElement("C044"));
    pfd.m_cabinClass = GetDBFName(pMes, DataElement(9854), CompElement("C044"));
    pfd.m_securityId = GetDBFName(pMes, DataElement(9874), CompElement("C045"));

    LogTrace(TRACE3) << pfd;

    return pfd;
}

boost::optional<edifact::ChdElem> readEdiChd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder pfd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "CHD")) {
        return boost::optional<ChdElem>();
    }

    ChdElem chd;
    chd.m_origAirline = GetDBFName(pMes, DataElement(3127), CompElement("C059"));
    chd.m_origPoint = GetDBFName(pMes, DataElement(3800), CompElement("C059"));

    unsigned num_hosts = GetNumComposite(pMes, "C696");

    EdiPointHolder c696_holder(pMes);

    for (unsigned i = 0; i < num_hosts; i++)
    {
        SetEdiPointToCompositeG(pMes, "C696", i, "EtErr::INV_HOST_DEFINITION");

        std::string hostAirline = GetDBNum(pMes, 3127);
        chd.m_hostAirlines.push_back(hostAirline);

        PopEdiPoint_wdG(pMes);
    }

    LogTrace(TRACE3) << chd;

    return chd;
}

boost::optional<edifact::FsdElem> readEdiFsd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder fsd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "FSD")) {
        return boost::optional<FsdElem>();
    }

    std::string boardingTime = GetDBFName(pMes, DataElement(2804), CompElement());
    if(boardingTime.empty()) {
        return boost::optional<FsdElem>();
    }

    FsdElem fsd;
    if(boardingTime.length() == 4) {
        fsd.m_boardingTime = Dates::hh24mi(boardingTime);
    } else {
        LogError(STDLOG) << "Invalid boarding time string [" << boardingTime << "]";
    }


    LogTrace(TRACE3) << fsd;

    return fsd;
}

boost::optional<edifact::ErdElem> readEdiErd(_EDI_REAL_MES_STRUCT_ *pMes)
{
    EdiPointHolder erd_holder(pMes);
    if(!SetEdiPointToSegmentG(pMes, "ERD")) {
        return boost::optional<ErdElem>();
    }

    ErdElem erd;
    erd.m_level = GetDBFNameCast<unsigned>(EdiDigitCast<unsigned>(), pMes,
                                           DataElement(9876), "", CompElement("C056"));
    erd.m_messageNumber = GetDBFNameCast<unsigned>(EdiDigitCast<unsigned>(), pMes,
                                                   DataElement(9845), "", CompElement("C056"));
    erd.m_messageText = GetDBFName(pMes, DataElement(4440), CompElement("C056"));

    LogTrace(TRACE3) << erd;

    return erd;
}

} // namespace TickReader
} // namespace Ticketing
