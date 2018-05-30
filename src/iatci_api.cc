#include "iatci_api.h"

#include "etick.h" // ChangeOfStatus
#include "astra_ticket.h" // Ticket
#include "astra_context.h"
#include "astra_api.h"
#include "astra_msg.h"
#include "edi_utils.h"
#include "tlg/postpone_edifact.h"

#include <serverlib/xml_stuff.h>
#include <etick/exceptions.h>

#include <sstream>
#include <boost/lexical_cast.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci
{

using namespace AstraEdifact;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;


static void ETRollbackStatus_local(xmlDocPtr ediResDocPtr)
{
    try {
        ETStatusInterface::ETRollbackStatus(ediResDocPtr, false);
    } catch(const TlgHandling::TlgToBePostponed&) {
        ; // nop
    }
}

static UpdateBaggageDetails makeFakeUpdBaggage(const BaggageDetails& baggage)
{
    return UpdateBaggageDetails(UpdateDetails::Add,
                                baggage.bag(),
                                baggage.handBag(),
                                baggage.bagTags());
}

static bool haveBaggage(const dcqcki::PaxGroup& ckiPaxGrp)
{
    if(ckiPaxGrp.baggage()) 
    {
        auto baggage = ckiPaxGrp.baggage();
        if(baggage->bag() && baggage->bag()->numOfPieces()) {
            return true;
        }
        if(baggage->handBag() && baggage->handBag()->numOfPieces()) {
            return true;
        }
    }
    
    return false;         
}
                                   
static boost::optional<CkuParams> makeFakeCkuParamsWithBaggage(const CkiParams& ckiParams)
{
    std::list<dcqcku::PaxGroup> ckuPaxGroups;   
    for(const auto& ckiPaxGrp: ckiParams.fltGroup().paxGroups()) {
        if(haveBaggage(ckiPaxGrp)) {
            ckuPaxGroups.push_back(dcqcku::PaxGroup(ckiPaxGrp.pax(),
                                                    boost::none,/*reserv*/
                                                    boost::none,/*baggage*/
                                                    boost::none,/*service*/
                                                    ckiPaxGrp.infant(),
                                                    boost::none,/*upd personal*/
                                                    boost::none,/*upd seat*/
                                                    makeFakeUpdBaggage(ckiPaxGrp.baggage().get()),
                                                    boost::none,/*upd service*/
                                                    boost::none,/*upd doc*/
                                                    boost::none,/*upd address*/
                                                    boost::none /*upd visa*/
                                                    ));
        }                 
    }
    
    if(!ckuPaxGroups.empty()) {
        return CkuParams(ckiParams.org(),
                         ckiParams.cascade(),
                         dcqcku::FlightGroup(ckiParams.fltGroup().outboundFlight(),
                                             ckiParams.fltGroup().inboundFlight(),
                                             ckuPaxGroups));
    }
    
    return boost::none;    
}

dcrcka::Result checkinPaxes(const CkiParams& ckiParams)
{
    auto fakeCkuParams = makeFakeCkuParamsWithBaggage(ckiParams);
    if(fakeCkuParams) {
        LogTrace(TRACE1) << "CkiParams with baggage";
        auto checkinRes = astra_api::checkinIatciPaxes(ckiParams);
        if(checkinRes.status() == dcrcka::Result::Ok) {
            LogTrace(TRACE1) << "Apply baggage from CkiParams";
            return updateCheckin(fakeCkuParams.get());
        }
        return checkinRes;
    } else {
        return astra_api::checkinIatciPaxes(ckiParams);
    }
}

dcrcka::Result cancelCheckin(const CkxParams& ckxParams)
{
    return astra_api::cancelCheckinIatciPaxes(ckxParams);
}

dcrcka::Result updateCheckin(const CkuParams& ckuParams)
{
    return astra_api::updateIatciPaxes(ckuParams);
}

dcrcka::Result reprintBoardingPass(const BprParams& bprParams)
{
    return astra_api::printBoardingPass(bprParams);
}

dcrcka::Result fillPasslist(const PlfParams& plfParams)
{
    return astra_api::fillPaxList(plfParams);
}

dcrcka::Result fillSeatmap(const SmfParams& smfParams)
{
    return astra_api::fillSeatmap(smfParams);
}

dcrcka::Result checkinPax(tlgnum_t postponeTlgNum)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " by tlgnum " << postponeTlgNum;

    int reqCtxtId = AstraEdifact::ReadPostponedContext(postponeTlgNum, true/*clear*/);
    LogTrace(TRACE3) << "req_ctxt_id=" << reqCtxtId;

    if(!reqCtxtId) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }

    XMLDoc termReqCtxt;
    AstraEdifact::getTermRequestCtxt(reqCtxtId, true,
                                     "iatci::checkinPax", termReqCtxt);
    xmlNodePtr termReqNode = NodeAsNode("/query", termReqCtxt.docPtr())->children;
    ASSERT(termReqNode != NULL);

    XMLDoc ediResCtxt;
    AstraEdifact::getEdiResponseCtxt(reqCtxtId, true,
                                     "iatci::checkinPax", ediResCtxt, false/*throw*/);

    if(ediResCtxt.docPtr() == NULL) {
        // что-то пошло не так
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }
    xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
    ASSERT(ediResNode != NULL);

    try {
        return astra_api::checkinIatciPaxes(termReqNode, ediResNode);
    } catch(tick_soft_except&) {
        tst();
        // откатываем измененные статусы
        ETRollbackStatus_local(ediResNode->doc);
        throw;
    }
}

dcrcka::Result cancelCheckin(tlgnum_t postponeTlgNum)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " by tlgnum " << postponeTlgNum;

    int reqCtxtId = AstraEdifact::ReadPostponedContext(postponeTlgNum, true/*clear*/);
    LogTrace(TRACE3) << "req_ctxt_id=" << reqCtxtId;

    if(!reqCtxtId) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }

    XMLDoc termReqCtxt;
    AstraEdifact::getTermRequestCtxt(reqCtxtId, true,
                                     "iatci::cancelCheckinPax", termReqCtxt);
    xmlNodePtr termReqNode = NodeAsNode("/query", termReqCtxt.docPtr())->children;
    ASSERT(termReqNode != NULL);

    XMLDoc ediResCtxt;
    AstraEdifact::getEdiResponseCtxt(reqCtxtId, true,
                                     "iatci::cancelCheckinPax", ediResCtxt, false/*throw*/);
    if(ediResCtxt.docPtr() == NULL) {
        // что-то пошло не так
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }

    xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
    ASSERT(ediResNode != NULL);

    tst();
    return astra_api::cancelCheckinIatciPax(termReqNode, ediResNode);
}

}//namespace iatci
