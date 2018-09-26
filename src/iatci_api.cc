#include "iatci_api.h"
#include "iatci_help.h"
#include "etick.h" // ChangeOfStatus
#include "astra_ticket.h" // Ticket
#include "astra_context.h"
#include "astra_api.h"
#include "astra_msg.h"
#include "edi_utils.h"
#include "tlg/postpone_edifact.h"

#include <serverlib/xml_stuff.h>
#include <serverlib/savepoint.h>
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

    ASTRA::commit();
}

static UpdateBaggageDetails makeFakeUpdBaggage(const BaggageDetails& baggage)
{
    return UpdateBaggageDetails(UpdateDetails::Add,
                                baggage.bag(),
                                baggage.handBag(),
                                baggage.bagTags());
}

static UpdateBaggageDetails makeFakeUpdBaggage(const astra_api::xml_entities::XmlIatciBags& iatciBags,
                                               const boost::optional<astra_api::xml_entities::XmlIatciBagTags>& iatciBagTags)
{
    boost::optional<BaggageDetails::BagInfo> bag, handBag;
    std::list<BaggageDetails::BagTagInfo> bagTags;
    
    for(const auto& b: iatciBags.bags) {
        if(!b.num_of_pieces || !b.weight) continue;
        if(!b.is_hand) {
            bag = BaggageDetails::BagInfo(b.num_of_pieces, b.weight);
        } else {
            handBag = BaggageDetails::BagInfo(b.num_of_pieces, b.weight);
        }
    }
    
    if(bag) {
        ASSERT(iatciBagTags && !iatciBagTags->tags.empty());
        for(const auto& t: iatciBagTags->tags) {
            bagTags.push_back(BaggageDetails::BagTagInfo(t.carrier_code,
                                                         t.dest,
                                                         BaggageDetails::BagTagInfo::makeFullTag(t.accode, t.tag_num),
                                                         t.qtty));
        }
    }
    
    return UpdateBaggageDetails(UpdateDetails::Add,
                                bag,
                                handBag,
                                bagTags);        
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

static boost::optional<CkuParams> makeFakeCkuParamsWithBaggage(const astra_api::xml_entities::XmlSegment& xmlSeg)
{
    std::list<dcqcku::PaxGroup> ckuPaxGroups; 
    for(const auto& xmlPax: xmlSeg.passengers) {
        if(xmlPax.iatci_bags) {
            ckuPaxGroups.push_back(dcqcku::PaxGroup(iatci::makeQryPax(xmlPax.toPax()),
                                                    boost::none,/*reserv*/
                                                    boost::none,/*baggage*/
                                                    boost::none,/*service*/
                                                    boost::none,/*infant*/
                                                    boost::none,/*upd personal*/
                                                    boost::none,/*upd seat*/
                                                    makeFakeUpdBaggage(xmlPax.iatci_bags.get(), xmlPax.iatci_bag_tags),
                                                    boost::none,/*upd service*/
                                                    boost::none,/*upd doc*/
                                                    boost::none,/*upd address*/
                                                    boost::none /*upd visa*/
                                                    ));
        }
    }
    
    if(!ckuPaxGroups.empty()) {
        return CkuParams(iatci::makeOrg(xmlSeg.toSeg()),
                         iatci::makeCascade(),
                         dcqcku::FlightGroup(iatci::makeFlight(xmlSeg.toSeg()),
                                             boost::none,
                                             ckuPaxGroups));
    }

    
    return boost::none;
}

dcrcka::Result checkinPaxes(const CkiParams& ckiParams)
{
    auto fakeCkuParams = makeFakeCkuParamsWithBaggage(ckiParams);
    auto checkinRes = astra_api::checkinIatciPaxes(ckiParams);
    if(fakeCkuParams && checkinRes.status() == dcrcka::Result::Ok) {
        checkinRes = updateCheckin(fakeCkuParams.get());
    }
    return checkinRes;
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
    
    OciCpp::Savepoint sp("iatci_checkin");
    try {
        auto segNode = astra_api::xml_entities::XmlEntityReader::readSeg(findNodeR(termReqNode, "segment"));
        auto fakeCkuParams = makeFakeCkuParamsWithBaggage(segNode);
        auto checkinRes = astra_api::checkinIatciPaxes(termReqNode, ediResNode);        
        if(fakeCkuParams && checkinRes.status() == dcrcka::Result::Ok) {
            checkinRes = updateCheckin(fakeCkuParams.get());
        }
        return checkinRes;
    } catch(std::exception& e) {
        LogTrace(TRACE0) << e.what();
        sp.rollback();
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
    
    OciCpp::Savepoint sp("iatci_checkin");
    try {
        return astra_api::cancelCheckinIatciPax(termReqNode, ediResNode);
    } catch(std::exception& e) {
        LogTrace(TRACE0) << e.what();
        sp.rollback();
        ETRollbackStatus_local(ediResNode->doc);
        throw;
    }
}

}//namespace iatci
