#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <etick/edi_cast.h>
#include <numeric>
#include <serverlib/isdigit.h>
#include <serverlib/dates.h>
#include <edilib/edi_func_cpp.h>
#include "astra_ticket.h"
#include "edi_elements.h"
#include "read_edi_elements.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace Ticketing {
namespace TickReader{
    using namespace edilib;
    using namespace ASTRA::edifact;

boost::optional<TktElement> readEdiTkt(_EDI_REAL_MES_STRUCT_ *pMes)
{
    edilib::EdiPointHolder tkt_holder(pMes);
    if(!edilib::SetEdiPointToSegmentG(pMes, "TKT"))
        return boost::optional<TktElement>();

    TktElement res;
    PushEdiPointG(pMes);
    SetEdiPointToCompositeG(pMes, "C667",0, "EtsErr::INV_TICKNUM");

    res.tickStatAction = GetDBNumCast<TickStatAction::TickStatAction_t>
            (EdiCast::TickActCast("EtErr::INV_TICK_ACT"), pMes, 9988);

    std::string ticktmp = GetDBNum(pMes, DataElement(1004,0,0), "EtsErr::INV_TICKNUM");
    if(res.tickStatAction == TickStatAction::oldtick)
        ticktmp = TicketNum_t::cut_check_digit(ticktmp);

    TicketNum_t::check(ticktmp);
    res.ticketNum = TicketNum_t(ticktmp);
    res.docType  = GetDBNumCast<DocType>(EdiCast::DocTypeCast("EtsErr::INV_DOC_TYPE"),
                                           pMes, 1001,0, "EtsErr::INV_DOC_TYPE");
    int Nbooklets  = GetDBNumCast<int>(EdiDigitCast<int>("EtsErr::INV_COUPON", "-1"), pMes, 7240);
    if(Nbooklets >= 0)
        res.nBooklets = Nbooklets;

    if(res.tickStatAction == TickStatAction::inConnectionWith) {
        ticktmp = GetDBNum(pMes, DataElement(1004,0,1), "EtsErr::INV_TICKNUM");
        TicketNum_t::check(ticktmp);
        res.inConnectionTicketNum = TicketNum_t(ticktmp);
    }
    PopEdiPointG(pMes);

    LogTrace(TRACE3) << res;

    return res;
}

boost::optional<CpnElement> readEdiCpn(EDI_REAL_MES_STRUCT *pMes, int numCpn)
{
    edilib::EdiPointHolder cpn_holder(pMes); (void) cpn_holder;
    boost::optional<CpnElement> res;

    if(!SetEdiPointToSegmentG(pMes, "CPN", numCpn))
        return res;

    int numCPN = GetNumComposite(pMes, "C640", "INV_COUPON");
    if( numCPN > 1 ) {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) <<
                            "Bad number of C640 (" << numCPN << ")! (More then 1)";
    }

    res.reset(CpnElement());

    SetEdiPointToCompositeG(pMes, "C640");
    const unsigned numberOfCpnNumbers = GetNumDataElem(pMes, 1050);
    if(numberOfCpnNumbers) {
        unsigned num = GetDBNumCast<unsigned>(Ticketing::EdiCast::EdiDigitCast<unsigned>("INV_COUPON"),
                                              pMes, 1050);
        if(num < 1 || num > 4)
            throw EXCEPTIONS::ExceptionFmt(STDLOG) << "invalid coupon num (" << num <<")!";
        res->num = Ticketing::CouponNum_t(num);
    }

    if(GetNumDataElem(pMes, 1159)) {
        res->media = GetDBNumCast<Ticketing::TicketMedia>(Ticketing::EdiCast::TicketMediaCast("INV_MEDIA"),
                                                          pMes, 1159);
    }

    if(GetNumDataElem(pMes, 4405)) {
        res->status = GetDBNumCast<Ticketing::CouponStatus>(Ticketing::EdiCast::CoupStatCast("INV_COUPON"),
                                                  pMes, 4405);
        if(res->status == Ticketing::CouponStatus::Paper || res->status == Ticketing::CouponStatus::Printed) {
            if(res->media && res->media == Ticketing::TicketMedia::Electronic) {
                throw EXCEPTIONS::ExceptionFmt(STDLOG) <<
                                "Invalid coupon media [" << res->media->code() << "]!";
            }
        }
    }

    std::string amount = GetDBNum(pMes, 5004);
    if(!amount.empty())
        res->amount = Ticketing::TaxAmount::Amount(amount);

    res->sac    = GetDBNum(pMes, 9887);
    res->action = GetDBNum(pMes, 1229);

    if(numberOfCpnNumbers > 1) {
        unsigned num = GetDBNumCast<unsigned>(Ticketing::EdiCast::EdiDigitCast<unsigned>("INV_COUPON"),
                                              pMes, 1050, 1);
        if(num < 1 || num > 4)
            throw EXCEPTIONS::ExceptionFmt(STDLOG) << "invalid coupon num (" << num <<")!";
        res->connectedNum = Ticketing::CouponNum_t(num);
    }

    LogTrace(TRACE3) << "CPN: " << *res;

    return res;
}

} // namespace TickReader
} // namespace Ticketing
