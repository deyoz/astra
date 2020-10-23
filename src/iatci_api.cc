#include "iatci_api.h"
#include "iatci_help.h"
#include "iatci.h"
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
                         boost::none,
                         dcqcku::FlightGroup(iatci::makeFlight(xmlSeg.toSeg()),
                                             boost::none,
                                             ckuPaxGroups));
    }

    
    return boost::none;
}

static std::list<dcrcka::Result> loadDeferredData(tlgnum_t postponeTlgNum)
{
    auto ddata = iatci::loadDeferredCkiData(postponeTlgNum);
    if(ddata) {
        if(ddata->status() == iatci::DefferedIatciData::Status_e::Success) {
            return ddata->lRes();
        } else {
            if(ddata->error() == iatci::DefferedIatciData::Error_e::Timeout) {
                throw tick_soft_except(STDLOG, AstraErr::TIMEOUT_ON_HOST_3);
            } else {
                throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
            }
        }
    }

    return {};
}

static void saveRemoteCkiResults(const GrpId_t& grpId, xmlNodePtr reqNode,
                                 const std::list<iatci::dcrcka::Result>& lRes)
{
    if(lRes.empty()) return;

    XMLDoc iatciResCtxt = ASTRA::createXmlDoc("<iatci_result/>");
    xmlNodePtr iatciResNode = NodeAsNode(std::string("/iatci_result").c_str(), iatciResCtxt.docPtr());
    xmlNodePtr segmentsNode = newChild(iatciResNode, "segments");
    xmlNodePtr logSegmentsNode = newChild(iatciResNode, "segments_for_log");

    iatci::iatci2xml(logSegmentsNode, lRes, IatciViewXmlParams());
    iatci::iatci2xml(segmentsNode, lRes, IatciViewXmlParams());
    iatci::saveCkiGrp(grpId, reqNode, iatciResNode);
}

static void saveRemoteCkxResults(const GrpId_t& grpId, const std::list<iatci::dcrcka::Result>& lRes,
                                 xmlNodePtr reqNode)
{
    if(lRes.empty()) return;

    XMLDoc iatciResCtxt = ASTRA::createXmlDoc("<iatci_result/>");
    xmlNodePtr iatciResNode = NodeAsNode(std::string("/iatci_result").c_str(), iatciResCtxt.docPtr());
    xmlNodePtr segmentsNode = newChild(iatciResNode, "segments");
    /*xmlNodePtr logSegmentsNode = */newChild(iatciResNode, "segments_for_log");

    xmlNodePtr iatciSegmentsNode = findNodeR(reqNode, "iatci_segments");
    ASSERT(iatciSegmentsNode);
    CopyNode(segmentsNode, iatciSegmentsNode);
    iatci::saveCkxGrp(grpId, reqNode, iatciResNode);
}

static void saveRemoteCkuResults(const GrpId_t& grpId,
                                 const std::list<iatci::dcrcka::Result>& lRes,
                                 xmlNodePtr reqNode)
{
    if(lRes.empty()) return;

    XMLDoc iatciResCtxt = ASTRA::createXmlDoc("<iatci_result/>");
    xmlNodePtr iatciResNode = NodeAsNode(std::string("/iatci_result").c_str(), iatciResCtxt.docPtr());
    /*xmlNodePtr segmentsNode = */newChild(iatciResNode, "segments");
    /*xmlNodePtr logSegmentsNode = */newChild(iatciResNode, "segments_for_log");

    iatci::saveCkuGrp(grpId, reqNode, iatciResNode);
}

dcrcka::Result checkin(const CkiParams& ckiParams)
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

std::list<dcrcka::Result> checkin(tlgnum_t postponeTlgNum)
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

    std::list<dcrcka::Result> lRes = {};
    OciCpp::Savepoint sp("iatci_checkin");
    try {
        auto defferedData = loadDeferredData(postponeTlgNum);
        auto segNode = astra_api::xml_entities::XmlEntityReader::readSeg(findNodeR(termReqNode, "segment"));
        auto fakeCkuParams = makeFakeCkuParamsWithBaggage(segNode);
        auto checkinResWithGrpId = astra_api::checkinIatciPaxes(termReqNode, ediResNode);
        auto checkinRes = checkinResWithGrpId.m_iatciResult;
        if(fakeCkuParams && checkinRes.status() == dcrcka::Result::Ok) {
            checkinRes = updateCheckin(fakeCkuParams.get());
        }
        lRes = defferedData;
        saveRemoteCkiResults(checkinResWithGrpId.m_grpId, termReqNode, lRes);
        lRes.push_front(checkinRes);
    } catch(std::exception& e) {
        LogTrace(TRACE0) << e.what();
        sp.rollback();
        ETRollbackStatus_local(ediResNode->doc);
        throw;
    }

    return lRes;
}

std::list<dcrcka::Result> cancelCheckin(tlgnum_t postponeTlgNum)
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
    
    std::list<dcrcka::Result> lRes = {};
    OciCpp::Savepoint sp("iatci_checkin");
    try {
        auto defferedData = loadDeferredData(postponeTlgNum);
        auto cancelResWithGrpId = astra_api::cancelCheckinIatciPax(termReqNode, ediResNode);
        auto cancelRes = cancelResWithGrpId.m_iatciResult;
        saveRemoteCkxResults(cancelResWithGrpId.m_grpId, defferedData, termReqNode);
        lRes.push_back(cancelRes);
    } catch(std::exception& e) {
        LogTrace(TRACE0) << e.what();
        sp.rollback();
        ETRollbackStatus_local(ediResNode->doc);
        throw;
    }

    return lRes;
}

std::list<dcrcka::Result> updateCheckin(tlgnum_t postponeTlgNum)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " by tlgnum " << postponeTlgNum;

    int reqCtxtId = AstraEdifact::ReadPostponedContext(postponeTlgNum, true/*clear*/);
    LogTrace(TRACE3) << "req_ctxt_id=" << reqCtxtId;

    if(!reqCtxtId) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }

    XMLDoc termReqCtxt;
    AstraEdifact::getTermRequestCtxt(reqCtxtId, true,
                                     "iatci::updateCheckinPax", termReqCtxt);
    xmlNodePtr termReqNode = NodeAsNode("/query", termReqCtxt.docPtr())->children;
    ASSERT(termReqNode != NULL);

    XMLDoc ediResCtxt;
    AstraEdifact::getEdiResponseCtxt(reqCtxtId, true,
                                     "iatci::updateCheckinPax", ediResCtxt, false/*throw*/);
    if(ediResCtxt.docPtr() == NULL) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
    }

    xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
    ASSERT(ediResNode != NULL);
    
    std::list<dcrcka::Result> lRes = {};
    auto defferedData = loadDeferredData(postponeTlgNum);
    auto updateResWithGrpId = astra_api::updateIatciPaxes(termReqNode, ediResNode);
    auto updateRes = updateResWithGrpId.m_iatciResult;
    saveRemoteCkuResults(updateResWithGrpId.m_grpId, defferedData, termReqNode);
    lRes.push_back(updateRes);
    
    return lRes;
}

}//namespace iatci
