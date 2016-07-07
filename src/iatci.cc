#include "iatci.h"
#include "iatci_api.h"
#include "iatci_help.h"
#include "edi_utils.h"
#include "astra_api.h"
#include "basic.h"
#include "checkin.h"
#include "print.h"
#include "salonform.h"
#include "passenger.h" // TPaxItem
#include "astra_context.h" // AstraContext
#include "tlg/IatciCkiRequest.h"
#include "tlg/IatciCkuRequest.h"
#include "tlg/IatciCkxRequest.h"
#include "tlg/IatciPlfRequest.h"
#include "tlg/IatciBprRequest.h"
#include "tlg/IatciSmfRequest.h"
#include "tlg/remote_system_context.h"
#include "tlg/edi_msg.h"

#include <serverlib/dates.h>
#include <serverlib/cursctl.h>
#include <serverlib/savepoint.h>
#include <serverlib/dump_table.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/algo.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace astra_api;
using namespace astra_api::xml_entities;


namespace
{
    class EdiResCtxtWrapper
    {
        XMLDoc m_doc;
        xmlNodePtr m_node;
    public:
        EdiResCtxtWrapper(int ctxtId, const std::string& from)
        {
            read(ctxtId, from);
        }

        EdiResCtxtWrapper(int ctxtId, xmlNodePtr addCtxtNode, const std::string& tagName,
                          const std::string& from)
        {
            AstraEdifact::addToEdiResponseCtxt(ctxtId, addCtxtNode, tagName);
            read(ctxtId, from);
        }

        xmlNodePtr node() const
        {
            ASSERT(m_node != NULL);
            return m_node;
        }

        xmlDocPtr docPtr() const
        {
            return m_doc.docPtr();
        }

    protected:
        void read(int ctxtId, const std::string& from)
        {
            AstraEdifact::getEdiResponseCtxt(ctxtId, true, from, m_doc);
            m_node = NodeAsNode("/context", m_doc.docPtr());
        }
    };

    //-----------------------------------------------------------------------------------

    class TermReqCtxtWrapper
    {
        XMLDoc m_doc;
        xmlNodePtr m_node;
    public:
        TermReqCtxtWrapper(int ctxtId, bool clear, const std::string& from)
        {
            AstraEdifact::getTermRequestCtxt(ctxtId, clear, from, m_doc);
            m_node = NodeAsNode("/term/query", m_doc.docPtr())->children;
        }

        xmlNodePtr node() const
        {
            LogTrace(TRACE3) << "m_node:" << m_node;
            ASSERT(m_node != NULL);
            return m_node;
        }
    };

    //-----------------------------------------------------------------------------------

    class IatciPaxSeg
    {
        iatci::FlightDetails m_seg;
        iatci::PaxDetails m_pax;

    public:
        IatciPaxSeg(const iatci::FlightDetails& seg,
                    const iatci::PaxDetails& pax)
            : m_seg(seg), m_pax(pax)
        {}

        static IatciPaxSeg readFirst(int grpId)
        {
            LogTrace(TRACE3) << "read for grpId: " << grpId;
            return read(grpId, 1);
        }

        static IatciPaxSeg read(int grpId, unsigned segInd)
        {
            LogTrace(TRACE3) << "read for grpId: " << grpId << " and segInd:" << segInd;
            XMLDoc loadedDoc = iatci::createXmlDoc(iatci::IatciXmlDb::load(grpId));
            XmlCheckInTabs loadedTabs(findNodeR(loadedDoc.docPtr()->children, "segments"));
            std::list<XmlSegment> lSeg = algo::transform< std::list<XmlSegment> >(loadedTabs.tabs(),
                [](const XmlCheckInTab& tab) { return tab.xmlSeg(); });
            std::vector<iatci::Result> lRes = LoadPaxXmlResult(lSeg).toIatci(iatci::Result::Passlist,
                                                                             iatci::Result::Ok);
            ASSERT(segInd > 0 && segInd <= lRes.size()); // TODO #21190 check pax_id
            iatci::Result res = lRes.at(segInd - 1);
            ASSERT(res.pax());
            return IatciPaxSeg(res.flight(), res.pax().get());
        }

        const iatci::FlightDetails& seg() const { return m_seg; }
        const iatci::PaxDetails&    pax() const { return m_pax; }
    };

    //---------------------------------------------------------------------------------------

    template<class EntityT>
    class ModifiedEntity
    {
        EntityT m_oldEntity;
        EntityT m_newEntity;

    public:
        typedef boost::optional<ModifiedEntity<EntityT> > Optional_t;

        ModifiedEntity(const EntityT& oldE, const EntityT& newE)
            : m_oldEntity(oldE), m_newEntity(newE)
        {}

        const EntityT& oldEntity() const { return m_oldEntity; }
        const EntityT& newEntity() const { return m_newEntity; }
    };

    //---------------------------------------------------------------------------------------

    template<class EntityT>
    class CheckInEntityDiff
    {
        std::vector<EntityT> m_added;
        std::vector<EntityT> m_removed;
        std::vector<ModifiedEntity<EntityT> > m_modified;

    public:
        typedef boost::optional<CheckInEntityDiff<EntityT> > Optional_t;

        CheckInEntityDiff(const std::list<EntityT>& lOld, const std::list<EntityT>& lNew)
        {
            for(const EntityT& oldEntity: lOld) {
                //if(oldEntity.id() == ASTRA::NoExists) continue; // неправильные сущности пропускаем
                const auto newEntityOpt = algo::find_opt_if<boost::optional>(lNew,
                    [&oldEntity](const EntityT& newEntity) { return newEntity.id() == oldEntity.id(); } );
                if(newEntityOpt) { // если найдена в новых и с изменениями -> помещаем в modified
                    if(*newEntityOpt != oldEntity) {
                        m_modified.push_back(ModifiedEntity<EntityT>(oldEntity, *newEntityOpt));
                    }
                } else { // если не найдена в новых-> помещаем в cancelled
                    m_removed.push_back(oldEntity);
                }
            }

            for(const EntityT& newEntity: lNew) {
                //if(newEntity.id() == ASTRA::NoExists) continue; // неправильных пассажиров пропускаем
                const auto oldEntityOpt = algo::find_opt_if<boost::optional>(lOld,
                    [&newEntity](const EntityT& oldEntity) { return oldEntity.id() == newEntity.id(); } );
                if(!oldEntityOpt) { // если в старых не найден -> помещаем в added
                    m_added.push_back(newEntity);
                }
            }
        }

        const std::vector<ModifiedEntity<EntityT> >& modified() const { return m_modified;  }
        const std::vector<EntityT>& added()    const { return m_added;     }
        const std::vector<EntityT>& removed()const { return m_removed; }
        bool empty() const { return m_modified.empty() &&
                                    m_added.empty() &&
                                    m_removed.empty(); }
    };

    //---------------------------------------------------------------------------------------

    class PaxChange
    {
    public:
        typedef ModifiedEntity<astra_entities::DocInfo> DocChange_t;
        typedef CheckInEntityDiff<astra_entities::Remark> RemChange_t;
        typedef DocChange_t::Optional_t DocChangeOpt_t;
        typedef RemChange_t::Optional_t RemChangeOpt_t;

    private:
        astra_entities::PaxInfo m_oldPax;
        astra_entities::PaxInfo m_newPax;

        DocChangeOpt_t          m_docChange;
        RemChangeOpt_t          m_remChange;

    public:
        PaxChange(const astra_entities::PaxInfo& oldPax,
                  const astra_entities::PaxInfo& newPax);

        const astra_entities::PaxInfo& oldPax() const { return m_oldPax; }
        const astra_entities::PaxInfo& newPax() const { return m_newPax; }

        const DocChangeOpt_t& docChange() const { return m_docChange; }
        const RemChangeOpt_t& remChange() const { return m_remChange; }
    };

    //

    PaxChange::PaxChange(const astra_entities::PaxInfo& oldPax,
                         const astra_entities::PaxInfo& newPax)
        : m_oldPax(oldPax), m_newPax(newPax)
    {
        if(oldPax.m_doc && newPax.m_doc &&
           oldPax.m_doc.get() != newPax.m_doc.get())
        {
            m_docChange = DocChange_t(oldPax.m_doc.get(), newPax.m_doc.get());
        }

        if(newPax.m_rems)
        {
            ASSERT(oldPax.m_rems); // в старом пассажире ремарки быть обязаны
            RemChange_t remChng(oldPax.m_rems->m_lRems, newPax.m_rems->m_lRems);
            if(!remChng.empty())
            {
                m_remChange = remChng;
            }
        }
    }

    //---------------------------------------------------------------------------------------

    class PaxDiff: public CheckInEntityDiff<astra_entities::PaxInfo>
    {
    public:
        typedef CheckInEntityDiff<astra_entities::PaxInfo> Base_t;
        typedef std::vector<PaxChange> PaxChanges_t;

    private:
        PaxChanges_t m_paxChanges;

    public:

        PaxDiff(const std::list<astra_entities::PaxInfo>& lPaxOld,
                const std::list<astra_entities::PaxInfo>& lPaxNew);

        const PaxChanges_t& paxChanges() const;
    };

    //

    PaxDiff::PaxDiff(const std::list<astra_entities::PaxInfo>& lPaxOld,
                     const std::list<astra_entities::PaxInfo>& lPaxNew)
        : Base_t(lPaxOld, lPaxNew)
    {
        for(auto& re: modified()) {
            m_paxChanges.push_back(PaxChange(re.oldEntity(), re.newEntity()));
        }
    }

    const PaxDiff::PaxChanges_t& PaxDiff::paxChanges() const
    {
        return m_paxChanges;
    }

    //---------------------------------------------------------------------------------------

    class TabDiff
    {
        PaxDiff m_paxDiff;

    protected:
        TabDiff(const PaxDiff& paxDiff);

    public:
        typedef boost::optional<TabDiff> Optional_t;

        static Optional_t diff(const XmlCheckInTab& oldTab, const XmlCheckInTab& newTab);
        const PaxDiff& paxDiff() const { return m_paxDiff; }
    };

    //

    TabDiff::TabDiff(const PaxDiff& paxDiff)
        : m_paxDiff(paxDiff)
    {}

    TabDiff::Optional_t TabDiff::diff(const XmlCheckInTab& oldTab, const XmlCheckInTab& newTab)
    {
       PaxDiff paxDiff(oldTab.lPax(), newTab.lPax());
       if(!paxDiff.empty()) {
           return TabDiff(paxDiff);
       }
       return boost::none;
    }

    //---------------------------------------------------------------------------------------

    class TabsDiff
    {
    public:
        typedef std::map<size_t, TabDiff::Optional_t> Map_t;

    private:
        Map_t m_tabsDiff;

    public:
        TabsDiff(const XmlCheckInTabs& oldTabs,
                 const XmlCheckInTabs& newTabs);

        const Map_t& tabsDiff() const { return m_tabsDiff; }
        TabDiff::Optional_t at(size_t i) const;
    };

    //

    TabsDiff::TabsDiff(const XmlCheckInTabs& oldTabs,
                       const XmlCheckInTabs& newTabs)
    {
        ASSERT(oldTabs.size() == newTabs.size());

        for(size_t i = 0; i < oldTabs.size(); ++i) {
            m_tabsDiff.insert(std::make_pair(i, TabDiff::diff(oldTabs.tabs().at(i),
                                                              newTabs.tabs().at(i))));
        }
    }

    TabDiff::Optional_t TabsDiff::at(size_t i) const
    {
        ASSERT(i < m_tabsDiff.size());
        return m_tabsDiff.at(i);
    }

    //-----------------------------------------------------------------------------------


    iatci::PaxDetails::PaxType_e astra2iatci(ASTRA::TPerson personType)
    {
        switch(personType)
        {
        case ASTRA::adult:
            return iatci::PaxDetails::Adult;
        case ASTRA::child:
            return iatci::PaxDetails::Child;
        default:
            throw EXCEPTIONS::Exception("Unknown astra person type: %d", personType);
        }
    }

    struct PointInfo
    {
        std::string m_airline;
        std::string m_airport;

        PointInfo()
        {}

        PointInfo(const std::string& airline,
                  const std::string& airport)
            : m_airline(airline),
              m_airport(airport)
        {}

        static PointInfo readByPointId(int pointId)
        {
            LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << " by pointId " << pointId;

            std::string airl, airp;
            OciCpp::CursCtl cur = make_curs(
                    "select AIRLINE, AIRP from POINTS where POINT_ID=:point_id");
            cur.bind(":point_id", pointId)
               .defNull(airl, "")
               .def(airp)
               .EXfet();
            if(cur.err() == NO_DATA_FOUND) {
                LogWarning(STDLOG) << "Point " << pointId << " not found!";
                throw EXCEPTIONS::Exception("Point %d not found", pointId);
            }

            return PointInfo(airl, airp);
        }

        static PointInfo readByGrpId(int grpId)
        {
            LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << " by grpId " << grpId;

            int pointId = ASTRA::NoExists;
            OciCpp::CursCtl cur = make_curs(
                    "select POINT_DEP from PAX_GRP where GRP_ID=:grp_id");
            cur.bind(":grp_id", grpId)
               .def(pointId)
               .EXfet();
            if(cur.err() == NO_DATA_FOUND) {
                LogWarning(STDLOG) << "Group " << grpId << " not found!";
                throw EXCEPTIONS::Exception("Group %d not found", grpId);
            }

            return readByPointId(pointId);
        }
    };

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

static bool isReseatReq(xmlNodePtr reqNode)
{
    return findNodeR(reqNode, "Reseat") ? true : false;
}

static std::string getOldSeat(xmlNodePtr reqNode)
{
    xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
    ASSERT(tripIdNode != NULL && !NodeIsNULL(tripIdNode));
    int magicId = NodeAsInteger(tripIdNode);
    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    XMLDoc oldXmlDoc = iatci::createXmlDoc(iatci::IatciXmlDb::load(magicTab.grpId()));
    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    const XmlCheckInTab& oldIatciTab = oldIatciTabs.tabs().at(magicTab.tabInd() - 1);
    ASSERT(!oldIatciTab.lPax().empty());
    std::string oldSeat = oldIatciTab.lPax().front().m_seatNo;
    LogTrace(TRACE3) << "old seat: " << oldSeat;
    return oldSeat;
}

static PointInfo readPointInfo(int grpId)
{
    TCkinRoute tckinRoute;
    if(tckinRoute.GetRouteBefore(grpId, crtWithCurrent, crtIgnoreDependent)) {
        // если на стороне Астры делалась локальная(не iatci) сквозная регистрация
        ASSERT(!tckinRoute.empty());
        return PointInfo::readByPointId(tckinRoute.front().point_dep);
    } else {
        // если один единственный сегмент
        return PointInfo::readByGrpId(grpId);
    }
}

static std::string getIatciRequestContext(const edifact::KickInfo& kickInfo = edifact::KickInfo())
{
    // TODO fill whole context
    XMLDoc xmlCtxt = iatci::createXmlDoc("<context/>");
    xmlNodePtr rootNode = NodeAsNode("/context", xmlCtxt.docPtr());
    kickInfo.toXML(rootNode);
    SetProp(rootNode,"req_ctxt_id", kickInfo.reqCtxtId);
    return XMLTreeToText(xmlCtxt.docPtr());
}

static int getGrpId(xmlNodePtr reqNode, xmlNodePtr resNode, IatciInterface::RequestType reqType)
{
    int grpId = 0;
    if(reqType == IatciInterface::Cki) {
        // при первичной регистрации grp_id появляется в ответном xml
        grpId = NodeAsInteger("grp_id", NodeAsNode("segments/segment", resNode));
    } else {
        // в остальных случаях grp_id уже содержится в запросе, либо её можно найти
        xmlNodePtr grpIdNode = findNodeR(reqNode, "grp_id");
        if(grpIdNode != NULL && !NodeIsNULL(grpIdNode)) {
            grpId = NodeAsInteger(grpIdNode);
        } else {
            xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
            if(tripIdNode != NULL && !NodeIsNULL(tripIdNode)) {
                int magicId = NodeAsInteger(tripIdNode);
                ASSERT(magicId < 0);
                iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
                grpId = magicTab.grpId();
            } else {
                xmlNodePtr pointIdNode = findNodeR(reqNode, "point_id");
                ASSERT((pointIdNode != NULL && !NodeIsNULL(pointIdNode)));
                int pointId = NodeAsInteger(pointIdNode);
                xmlNodePtr regNoNode = findNodeR(reqNode, "reg_no");
                if(regNoNode != NULL && !NodeIsNULL(regNoNode)) {
                    int regNo = NodeAsInteger(regNoNode);
                    grpId = astra_api::findGrpIdByRegNo(pointId, regNo);
                } else {
                    xmlNodePtr paxIdNode = findNodeR(reqNode, "pax_id");
                    if(paxIdNode != NULL && !NodeIsNULL(paxIdNode)) {
                        int paxId = NodeAsInteger(paxIdNode);
                        grpId = astra_api::findGrpIdByPaxId(pointId, paxId);
                    }
                }
            }
        }
    }

    LogTrace(TRACE3) << "grpId:" << grpId;
    return grpId;
}

static void specialPlfErrorHandler(xmlNodePtr reqNode, const Ticketing::AstraMsg_t& errCode)
{
    LogTrace(TRACE3) << __FUNCTION__ << " called for err:" << errCode;
    if(errCode == Ticketing::AstraErr::PAX_SURNAME_NOT_CHECKED_IN) {
        int grpId = getGrpId(reqNode, NULL, IatciInterface::Plf);
        ASSERT(grpId > 0);
        if(!iatci::IatciXmlDb::load(grpId).empty()) {
            LogWarning(STDLOG) << "Group " << grpId << " marked as checked-in at our side but not checked-in at iatci-partner side. Drop iatci information...";
            iatci::IatciXmlDb::del(grpId);
        }
    }
}

static std::string getIatciPult()
{
    return TReqInfo::Instance()->desk.code;
}

static edifact::KickInfo getIatciKickInfo(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    if(reqNode != NULL)
    {
        int termReqCtxtId = AstraContext::SetContext("TERM_REQUEST", XMLTreeToText(reqNode->doc));
        if(ediResNode != NULL) {
            AstraContext::SetContext("EDI_RESPONSE", termReqCtxtId, XMLTreeToText(ediResNode->doc));
        }
        return AstraEdifact::createKickInfo(termReqCtxtId, "IactiInterface");
    }

    return edifact::KickInfo();
}

static iatci::CkiParams getCkiParams(xmlNodePtr reqNode)
{
    XmlCheckInTabs ownTabs(findNodeR(reqNode, "segments"));
    ASSERT(!ownTabs.empty());

    XmlCheckInTabs ediTabs(findNodeR(reqNode, "iatci_segments"));
    ASSERT(ediTabs.size() == 1);

    const XmlCheckInTab& lastOwnTab = ownTabs.tabs().back();
    const XmlSegment& ownSeg = lastOwnTab.xmlSeg();

    iatci::OriginatorDetails ediOrg(ownSeg.mark_flight.airline,
                                    ownSeg.mark_flight.airp_dep);

    const XmlCheckInTab& firstEdiTab = ediTabs.tabs().front();
    const XmlSegment& firstEdiSeg = firstEdiTab.xmlSeg();
    ASSERT(!firstEdiSeg.passengers.empty());
    const XmlPax& pax = firstEdiSeg.passengers.front(); // TODO #21190

    iatci::PaxDetails ediPax(pax.surname,
                             pax.name,
                             astra2iatci(DecodePerson(pax.pers_type.c_str())));

    iatci::FlightDetails ediFlight(firstEdiSeg.mark_flight.airline,
                                   Ticketing::FlightNum_t(firstEdiSeg.mark_flight.flt_no),
                                   firstEdiSeg.airp_dep,
                                   firstEdiSeg.airp_arv,
                                   BASIC::DateTimeToBoost(firstEdiSeg.mark_flight.scd).date());

    iatci::FlightDetails ownFlight(ownSeg.mark_flight.airline,
                                   Ticketing::FlightNum_t(ownSeg.mark_flight.flt_no),
                                   ownSeg.airp_dep,
                                   ownSeg.airp_arv,
                                   BASIC::DateTimeToBoost(ownSeg.mark_flight.scd).date());

    boost::optional<iatci::ServiceDetails> ediServices;
    if(pax.rems) {
        std::list<iatci::ServiceDetails::SsrInfo> lSsr;
        for(auto& rem: pax.rems->rems) {
            lSsr.push_back(iatci::ServiceDetails::SsrInfo(rem.rem_code,
                                                          "",
                                                          false,
                                                          rem.rem_text));
        }
        ediServices = iatci::ServiceDetails(lSsr);
    }

    boost::optional<iatci::SeatDetails> ediSeat;
    if(!pax.seat_no.empty()) {
        ediSeat = iatci::SeatDetails(pax.seat_no);
    }

    return iatci::CkiParams(ediOrg,
                            ediPax,
                            ediFlight,
                            ownFlight,
                            ediSeat,
                            boost::none,
                            boost::none,
                            boost::none,
                            ediServices);
}

static iatci::OriginatorDetails getOrigDetails(int grpId)
{
    PointInfo pointInfo = readPointInfo(grpId);
    return iatci::OriginatorDetails(pointInfo.m_airline,
                                    pointInfo.m_airport);
}

static iatci::CkxParams getCkxParams(xmlNodePtr reqNode)
{
    int firstOwnSegGrpId = getGrpId(reqNode, NULL, IatciInterface::Ckx);
    IatciPaxSeg ediPaxSeg = IatciPaxSeg::readFirst(firstOwnSegGrpId);
    return iatci::CkxParams(getOrigDetails(firstOwnSegGrpId),
                            ediPaxSeg.pax(),
                            ediPaxSeg.seg());
}

static iatci::BprParams getBprParams(int grpId)
{
    IatciPaxSeg ediPaxSeg = IatciPaxSeg::readFirst(grpId);
    return iatci::BprParams(getOrigDetails(grpId),
                            ediPaxSeg.pax(),
                            ediPaxSeg.seg());
}

static iatci::PlfParams getPlfParams(int grpId)
{
    IatciPaxSeg ediPaxSeg = IatciPaxSeg::readFirst(grpId);
    return iatci::PlfParams(getOrigDetails(grpId),
                            ediPaxSeg.pax(),
                            ediPaxSeg.seg());
}

static boost::optional<iatci::UpdatePaxDetails::UpdateDocInfo> getUpdDoc(const PaxChange& paxChange)
{
    if(!paxChange.docChange()) {
        return boost::none;
    }

    return iatci::UpdatePaxDetails::UpdateDocInfo(
                   iatci::UpdateDetails::Replace,
                   paxChange.docChange()->newEntity().m_type,
                   paxChange.docChange()->newEntity().m_country,
                   paxChange.docChange()->newEntity().m_num,
                   paxChange.docChange()->newEntity().m_surname,
                   paxChange.docChange()->newEntity().m_name,
                   paxChange.docChange()->newEntity().m_secName,
                   paxChange.docChange()->newEntity().m_gender,
                   paxChange.docChange()->newEntity().m_citizenship,
                   paxChange.docChange()->newEntity().m_birthDate,
                   paxChange.docChange()->newEntity().m_expiryDate);
}

static iatci::UpdatePaxDetails getUpdPax(const PaxChange& paxChange)
{
    return iatci::UpdatePaxDetails(
                   iatci::UpdateDetails::Replace,
                   paxChange.newPax().m_surname,
                   paxChange.newPax().m_name,
                   getUpdDoc(paxChange));
}

static boost::optional<iatci::UpdateServiceDetails> getUpdService(const PaxChange& paxChange)
{
    if(!paxChange.remChange()) {
        return boost::none;
    }

    iatci::UpdateServiceDetails updService(iatci::UpdateDetails::Replace); // Replace в конструкторе ни на что не влияет

    // удалённые
    for(auto& rem: paxChange.remChange()->removed()) {
        updService.addSsr(iatci::UpdateServiceDetails::UpdSsrInfo(
                                iatci::UpdateDetails::Cancel,
                                rem.m_remCode,
                                "",
                                false,
                                rem.m_remText));
    }

    // добавленные
    for(auto& rem: paxChange.remChange()->added()) {
        updService.addSsr(iatci::UpdateServiceDetails::UpdSsrInfo(
                                iatci::UpdateDetails::Add,
                                rem.m_remCode,
                                "",
                                false,
                                rem.m_remText));
    }

    // измененные
    for(auto& chng: paxChange.remChange()->modified()) {
        updService.addSsr(iatci::UpdateServiceDetails::UpdSsrInfo(
                                iatci::UpdateDetails::Replace,
                                chng.newEntity().m_remCode,
                                "",
                                false,
                                chng.newEntity().m_remText));
    }

    return updService;
}

static boost::optional<iatci::CkuParams> getCkuParams(xmlNodePtr reqNode)
{
    XmlCheckInTab firstOwnTab(NodeAsNode("segments/segment", reqNode));
    int firstOwnSegGrpId = firstOwnTab.seg().m_grpId;
    XMLDoc oldXmlDoc = iatci::createXmlDoc(iatci::IatciXmlDb::load(firstOwnSegGrpId));

    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    XmlCheckInTabs newIatciTabs(findNodeR(reqNode, "iatci_segments"));

    TabsDiff diff(oldIatciTabs, newIatciTabs);
    LogTrace(TRACE3) << "tabsDiff.size = " << diff.tabsDiff().size();

    for(size_t i = 0; i < diff.tabsDiff().size(); ++i)
    {
        TabDiff::Optional_t tabDiff = diff.at(i);
        if(tabDiff) {
            LogTrace(TRACE3) << "Tab diff detected!";
            PaxDiff paxDiff = tabDiff->paxDiff();
            LogTrace(TRACE3) << "Pax diff detected";
            LogTrace(TRACE3) << paxDiff.added().size() << " paxes were added";
            LogTrace(TRACE3) << paxDiff.removed().size() << " paxes were cancelled";
            LogTrace(TRACE3) << paxDiff.modified().size() << " paxes were changed";
            if(!paxDiff.paxChanges().empty())
            {
                const PaxChange& firstPaxChange = paxDiff.paxChanges().front();
                if(firstPaxChange.docChange()) {
                    LogTrace(TRACE3) << "Pax doc change detected";
                }
                if(firstPaxChange.remChange()) {
                    LogTrace(TRACE3) << "Pax rems change detected";
                }

                IatciPaxSeg firstEdiSeg = IatciPaxSeg::readFirst(firstOwnSegGrpId);
                return iatci::CkuParams(getOrigDetails(firstOwnSegGrpId),
                                        firstEdiSeg.pax(),
                                        firstEdiSeg.seg(),
                                        boost::none,
                                        getUpdPax(firstPaxChange),
                                        getUpdService(firstPaxChange));
            }
        }
    }

    tst();
    return boost::none;
}

static iatci::UpdateSeatDetails getSeatUpdate(xmlNodePtr reqNode)
{
    const std::string seatYname = NodeAsString("yname", reqNode);
    const std::string seatXname = NodeAsString("xname", reqNode);
    if(seatYname.empty() || seatXname.empty()) {
        throw AstraLocale::UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
    }
    const std::string seat = seatYname + seatXname;
    return iatci::UpdateSeatDetails(iatci::UpdateDetails::Replace,
                                    seat);
}

static iatci::CkuParams getSeatUpdateParams(xmlNodePtr reqNode, int magicId)
{
    ASSERT(magicId < 0);
    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    IatciPaxSeg iatciPaxSeg = IatciPaxSeg::read(magicTab.grpId(), magicTab.tabInd());
    iatci::UpdateSeatDetails updSeat = getSeatUpdate(reqNode);
    if(updSeat.seat() == getOldSeat(reqNode)) {
        throw AstraLocale::UserException("MSG.SEATS.SEAT_NO.PASSENGER_OWNER");
    }

    return iatci::CkuParams(getOrigDetails(magicTab.grpId()),
                            iatciPaxSeg.pax(),
                            iatciPaxSeg.seg(),
                            boost::none,
                            boost::none,
                            boost::none,
                            updSeat);
}

static iatci::SmfParams getSmfParams(int magicId)
{
    ASSERT(magicId < 0);
    if(magicId == -1) {
        throw AstraLocale::UserException("MSG.UNABLE_TO_SHOW_SEATMAP_BEFORE_CHECKIN"); // TODO #25409
    }
    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    IatciPaxSeg iatciPaxSeg = IatciPaxSeg::read(magicTab.grpId(), magicTab.tabInd());
    return iatci::SmfParams(getOrigDetails(magicTab.grpId()),
                            iatciPaxSeg.seg());
}

static iatci::SmfParams getSmfParams(xmlNodePtr reqNode)
{
    xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
    ASSERT(tripIdNode != NULL && !NodeIsNULL(tripIdNode));
    int magicId = NodeAsInteger(tripIdNode);
    return getSmfParams(magicId);
}

/////////////////////////////////////////////////////////////////////////////////////////

static void MagicUpdate(xmlNodePtr resNode, int grpId)
{
    xmlNodePtr segsNode = findNodeR(resNode, "segments");
    unsigned segInd = 0;
    for(xmlNodePtr segNode = segsNode->children; segNode != NULL; segNode = segNode->next)
    {
        iatci::MagicTab magicTab(grpId, ++segInd);
        xmlNodeSetContent(findNode(segNode, "point_dep"), magicTab.toNeg());
    }
}

static void SaveIatciXmlResToDb(const std::list<iatci::Result>& lRes,
                                xmlNodePtr iatciResNode, int grpId,
                                IatciInterface::RequestType reqType)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    MagicUpdate(iatciResNode, grpId);
    std::string xmlData = XMLTreeToText(iatciResNode->doc);

    switch(reqType)
    {
    case IatciInterface::Cki:
        tst();
        iatci::IatciXmlDb::add(grpId, xmlData);
        break;

    case IatciInterface::Cku:
    case IatciInterface::Plf:
        tst();
        iatci::IatciXmlDb::upd(grpId, xmlData);
        break;

    case IatciInterface::Ckx:
        tst();
        iatci::IatciXmlDb::del(grpId);
        break;

    default:
        break;
    }
}

static int SaveIatciPax(const std::list<iatci::Result>& lRes,
                        IatciInterface::RequestType reqType,
                        xmlNodePtr ediResNode, xmlNodePtr termReqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;

    int grpId = getGrpId(termReqNode, resNode, reqType);
    ASSERT(grpId > 0);

    // TODO Пока сохраним информацию для всей группы,
    // но, возможно, в будущем потребуется сохранять для каждого пассажира
    SaveIatciXmlResToDb(lRes, NodeAsNode("/iatci_result", ediResNode), grpId, reqType);

    return grpId;
}

//---------------------------------------------------------------------------------------

IatciInterface::RequestType IatciInterface::ClassifyCheckInRequest(xmlNodePtr reqNode)
{
    xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
    xmlNodePtr grpIdNode = findNode(segNode, "grp_id");
    if(grpIdNode == NULL) {
        LogTrace(TRACE3) << "Checkin request detected!";
        return IatciInterface::Cki;
    } else {
        xmlNodePtr paxesNode = findNode(segNode, "passengers");
        if(paxesNode != NULL) {
            xmlNodePtr paxNode = findNode(paxesNode, "pax");
            if(paxNode != NULL) {
                xmlNodePtr refuseNode = findNode(paxNode, "refuse");
                if(refuseNode != NULL && !NodeIsNULL(refuseNode)) {
                    LogTrace(TRACE3) << "Cancel request detected!";
                    return IatciInterface::Ckx;
                }
            }
        }

        LogTrace(TRACE3) << "Update request detected!";
        return IatciInterface::Cku;
    }

    TST();
    ASSERT(false); // throw always
}

bool IatciInterface::DispatchCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    RequestType reqType = ClassifyCheckInRequest(reqNode);
    switch(reqType)
    {
    case Cki:
        return InitialRequest(reqNode, ediResNode);
    case Cku:
        return UpdateRequest(reqNode, ediResNode);
    case Ckx:
        return CancelRequest(reqNode, ediResNode);
    default:
        break;
    }

    TST();
    ASSERT(false); // throw always

    return false;
}

// функция проверяет, будет ли послан iatci-запрос на регистрацию(checkin,update,cancel)
// причём запрос может быть не послан только в случае update, если изменения не затронули
// edifact-вкладки
bool IatciInterface::WillBeSentCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    RequestType reqType = ClassifyCheckInRequest(reqNode);
    if(reqType == Cku) {
        if(getCkuParams(reqNode)) {
            return true;
        } else {
            return false;
        }
    }

    return true;
}

bool IatciInterface::MayNeedSendIatci(xmlNodePtr reqNode)
{
    XmlCheckInTabs tabs(findNodeR(reqNode, "segments"));
    return tabs.containsEdiTab();
}

bool IatciInterface::InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
    edifact::SendCkiRequest(getCkiParams(reqNode),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);

    return true; /*req was sent*/
}

bool IatciInterface::UpdateRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    // запрос на Update может не затронуть edifact-сегменты:
    // в этом случае ckuParams будут отсутствовать
    boost::optional<iatci::CkuParams> ckuParams = getCkuParams(reqNode);
    if(ckuParams)
    {
        edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
        edifact::SendCkuRequest(ckuParams.get(),
                                getIatciPult(),
                                getIatciRequestContext(kickInfo),
                                kickInfo);

        return true; /*req was sent*/
    }

    return false; /*req was NOT sent*/
}

void IatciInterface::UpdateSeatRequest(xmlNodePtr reqNode, int magicId)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendCkuRequest(getSeatUpdateParams(reqNode, magicId),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);
}

bool IatciInterface::CancelRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
    edifact::SendCkxRequest(getCkxParams(reqNode),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);

    return true; /*req was sent*/
}

void IatciInterface::ReprintRequest(xmlNodePtr reqNode, int grpId)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendBprRequest(getBprParams(grpId),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);
}

void IatciInterface::PasslistRequest(xmlNodePtr reqNode, int grpId)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendPlfRequest(getPlfParams(grpId),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);
}

void IatciInterface::SeatmapRequest(xmlNodePtr reqNode, int magicId)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendSmfRequest(getSmfParams(magicId),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);
}

void IatciInterface::SeatmapRequest(xmlNodePtr reqNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendSmfRequest(getSmfParams(reqNode),
                            getIatciPult(),
                            getIatciRequestContext(kickInfo),
                            kickInfo);
}

void IatciInterface::CheckinKickHandler(int ctxtId,
                                        xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(CheckinKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Cki, ActSavePax);
    FuncOut(CheckinKickHandler);
}

void IatciInterface::UpdateKickHandler(int ctxtId,
                                       xmlNodePtr initialReqNode,
                                       xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    FuncIn(UpdateKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Cku,
                 isReseatReq(initialReqNode) ? ActReseatPax : ActSavePax);
    FuncOut(UpdateKickHandler);
}

void IatciInterface::CancelKickHandler(int ctxtId,
                                       xmlNodePtr initialReqNode,
                                       xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    FuncIn(CancelKickHandler);    
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Ckx, ActSavePax);
    FuncOut(CancelKickHandler);
}

void IatciInterface::ReprintKickHandler(int ctxtId,
                                        xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(ReprintKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Bpr, ActReprint);
    FuncOut(ReprintKickHandler);
}

void IatciInterface::PasslistKickHandler(int ctxtId,
                                         xmlNodePtr initialReqNode,
                                         xmlNodePtr resNode,
                                         const std::list<iatci::Result>& lRes)
{
    FuncIn(PasslistKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Plf, ActLoadPax);
    FuncOut(PasslistKickHandler);
}

void IatciInterface::SeatmapKickHandler(int ctxtId,
                                        xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(SeatmapKickHandler);
    ASSERT(lRes.size() == 1);
    if(isReseatReq(initialReqNode)) {
        SalonFormInterface::ReseatRemote(resNode,
                                         iatci::Seat::fromStr(ReqParams(initialReqNode).getStrParam("old_seat")),
                                         iatci::Seat::fromStr(getSeatUpdate(initialReqNode).seat()),
                                         lRes.front());
    } else {
        SalonFormInterface::ShowRemote(resNode,
                                       lRes.front());
    }
    FuncOut(SeatmapKickHandler);
}

void IatciInterface::SeatmapForPassengerKickHandler(int ctxtId,
                                                    xmlNodePtr initialReqNode,
                                                    xmlNodePtr resNode,
                                                    const std::list<iatci::Result>& lRes)
{
    FuncIn(SeatmapForPassengerKickHandler);
    FuncOut(SeatmapForPassengerKickHandler);
}

void IatciInterface::KickHandler(XMLRequestCtxt* ctxt,
                                 xmlNodePtr reqNode,
                                 xmlNodePtr resNode)
{
    using namespace edifact;
    FuncIn(KickHandler);
    pRemoteResults remRes = RemoteResults::readSingle();
    if(remRes)
    {
        LogTrace(TRACE3) << *remRes;

        int reqCtxtId = GetReqCtxtId(reqNode);
        TermReqCtxtWrapper termReqCtxt(reqCtxtId, true, "IatciInterface::KickHandler");
        if(remRes->status() == RemoteStatus::Timeout)
        {
            KickHandler_onTimeout(reqCtxtId, termReqCtxt.node(), resNode);
        }
        else
        {
            std::list<iatci::Result> lRes = iatci::loadCkiData(remRes->ediSession());

            if(remRes->status() == RemoteStatus::Success) {
                KickHandler_onSuccess(reqCtxtId, termReqCtxt.node(), resNode, lRes);
            } else {
                Ticketing::AstraMsg_t errCode = getInnerErrByErd(remRes->ediErrCode());
                if(!remRes->remark().empty()) {
                    AstraLocale::showProgError(remRes->remark());
                } else {
                    if(!remRes->ediErrCode().empty()) {
                        AstraLocale::showMessage(errCode);
                    } else {
                        AstraLocale::showProgError("Ошибка обработки в удалённой DCS"); // TODO #25409
                    }
                }
                KickHandler_onFailure(reqCtxtId, termReqCtxt.node(), resNode, lRes, errCode);
            }
        }
    }
    FuncOut(KickHandler);
}

void IatciInterface::KickHandler_onTimeout(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode)
{
    FuncIn(KickHandler_onTimeout);
    AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR"); // TODO #25409
    FuncOut(KickHandler_onTimeout);
}


void IatciInterface::KickHandler_onSuccess(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode,
                                           const std::list<iatci::Result>& lRes)
{
    FuncIn(KickHandler_onSuccess);
    ASSERT(!lRes.empty());
    switch(lRes.front().action())
    {
    case iatci::Result::Checkin:
        CheckinKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::Cancel:
        CancelKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::Update:
        UpdateKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::Reprint:
        ReprintKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::Passlist:
        PasslistKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::Seatmap:
        SeatmapKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case iatci::Result::SeatmapForPassenger:
        SeatmapForPassengerKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    }
    FuncOut(KickHandler_onSuccess);
}

void IatciInterface::KickHandler_onFailure(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode,
                                           const std::list<iatci::Result>& lRes,
                                           const Ticketing::AstraMsg_t& errCode)
{
    FuncIn(KickHandler_onFailure);
    ReqParams(initialReqNode).setBoolParam("after_kick", true);
    ASSERT(!lRes.empty());
    switch(lRes.front().action())
    {
    case iatci::Result::Checkin:
    case iatci::Result::Cancel:
    case iatci::Result::Update:
        // откат смены статуса, произошедшей ранее
        if(!isReseatReq(initialReqNode)) {
            RollbackChangeOfStatus(ctxtId);
        }
        break;

    case iatci::Result::Passlist:
        specialPlfErrorHandler(initialReqNode, errCode);
        CheckInInterface::LoadPax(initialReqNode, resNode);
        break;

    case iatci::Result::Reprint:
    case iatci::Result::Seatmap:
    case iatci::Result::SeatmapForPassenger:
        // ; NOP
        break;

    default:
        break;
    }
    FuncOut(KickHandler_onFailure);
}

int IatciInterface::GetReqCtxtId(xmlNodePtr kickReqNode) const
{
    ASSERT(GetNode("@req_ctxt_id", kickReqNode) != NULL);
    return NodeAsInteger("@req_ctxt_id", kickReqNode);
}

void IatciInterface::DoKickAction(int ctxtId,
                                  xmlNodePtr reqNode,
                                  xmlNodePtr resNode,
                                  const std::list<iatci::Result>& lRes,
                                  RequestType reqType,
                                  KickAction act)
{
    FuncIn(DoKickAction);
    ReqParams(reqNode).setBoolParam("after_kick", true);

    XMLDoc iatciResCtxt = iatci::createXmlDoc("<iatci_result/>");
    xmlNodePtr iatciResNode = NodeAsNode(std::string("/iatci_result").c_str(), iatciResCtxt.docPtr());
    xmlNodePtr segmentsNode = newChild(iatciResNode, "segments");
    for(const iatci::Result& res: lRes) {
        res.toXml(segmentsNode);
    }

    EdiResCtxtWrapper ediResCtxt(ctxtId, iatciResNode, "context", __FUNCTION__);

    switch(act)
    {
    case ActSavePax:
    {
        ASSERT(CheckInInterface::SavePax(reqNode, ediResCtxt.node(), resNode));
        int grpId = SaveIatciPax(lRes, reqType, iatciResNode, reqNode, resNode);
        CheckInInterface::LoadIatciPax(NULL, resNode, grpId, false);
    }
    break;

    case ActReseatPax:
    {
        ASSERT(lRes.size() == 1);
        ASSERT(lRes.front().seat());
        const std::string requestedSeat = getSeatUpdate(reqNode).seat();
        const std::string confirmedSeat = lRes.front().seat()->seat();
        if(requestedSeat != confirmedSeat) {
            LogError(STDLOG) << "Warning: remote DCS confirmed a place other than the requested! "
                             << "Requested: " << requestedSeat << "; "
                             << "Confirmed: " << confirmedSeat;
        }
        ReqParams(reqNode).setStrParam("old_seat", getOldSeat(reqNode));
        SaveIatciPax(lRes, reqType, iatciResNode, reqNode, resNode);
        SeatmapRequest(reqNode); // перепосылаем карту мест, чтобы сформировать ответ на смену места
    }
    break;

    case ActLoadPax:
    {
        SaveIatciPax(lRes, reqType, iatciResNode, reqNode, resNode);
        CheckInInterface::LoadPax(reqNode, resNode);
    }
    break;

    case ActReprint:
    {
        SaveIatciPax(lRes, reqType, iatciResNode, reqNode, resNode);
        PrintInterface::GetPrintDataBP(reqNode, resNode);
    }
    break;

    default:
        throw EXCEPTIONS::Exception("Can't be here!");
    }
    FuncOut(DoKickAction);
}

void IatciInterface::RollbackChangeOfStatus(int ctxtId)
{
    EdiResCtxtWrapper ediResCtxt(ctxtId, __FUNCTION__);
    ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(), false);
}
