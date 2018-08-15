#include "iatci.h"
#include "iatci_api.h"
#include "iatci_help.h"
#include "edi_utils.h"
#include "astra_api.h"
#include "date_time.h"
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
#include "tlg/IatciIfm.h"
#include "tlg/remote_system_context.h"
#include "tlg/edi_msg.h"

#include <serverlib/dates_oci.h>
#include <serverlib/cursctl.h>
#include <serverlib/savepoint.h>
#include <serverlib/dump_table.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/algo.h>

#include <boost/foreach.hpp>
#include <set>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace astra_api;
using namespace astra_api::xml_entities;
using namespace iatci;
using namespace BASIC::date_time;
using iatci::dcrcka::Result;


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
            return m_node;
        }

        xmlDocPtr docPtr() const
        {
            return m_doc.docPtr();
        }

    protected:
        void read(int ctxtId, const std::string& from)
        {
            AstraEdifact::getEdiResponseCtxt(ctxtId, true, from, m_doc, false);
            if(m_doc.docPtr() != NULL) {
                m_node = NodeAsNode("/context", m_doc.docPtr());
            }
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
            LogTrace(TRACE3) << "read first iatci paxseg for grpId: " << grpId;
            return read(grpId, 1);
        }

        static IatciPaxSeg read(int grpId, unsigned segInd)
        {
            LogTrace(TRACE3) << "read iatci paxseg for grpId: " << grpId
                             << " and segInd:" << segInd;
            XMLDoc loadedDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(grpId));
            XmlCheckInTabs loadedTabs(findNodeR(loadedDoc.docPtr()->children, "segments"));
            std::list<XmlSegment> lSeg = algo::transform< std::list<XmlSegment> >(loadedTabs.tabs(),
                [](const XmlCheckInTab& tab) { return tab.xmlSeg(); });
            std::vector<Result> lRes = LoadPaxXmlResult(lSeg).toIatci(Result::Passlist,
                                                                      Result::Ok);
            ASSERT(segInd > 0 && segInd <= lRes.size()); // TODO #21190 check pax_id
            Result res = lRes.at(segInd - 1);
            ASSERT(res.pax());
            return IatciPaxSeg(res.flight(), res.pax().get());
        }

        const iatci::FlightDetails& seg() const { return m_seg; }
        const iatci::PaxDetails&    pax() const { return m_pax; }
    };

    //-----------------------------------------------------------------------------------

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

    //-----------------------------------------------------------------------------------

    template<class EntityT>
    class CheckInSingleEntityDiff
    {
        boost::optional<EntityT> m_added;
        boost::optional<EntityT> m_removed;
        boost::optional<ModifiedEntity<EntityT> > m_modified;

    public:
        typedef boost::optional<CheckInSingleEntityDiff<EntityT> > Optional_t;

        CheckInSingleEntityDiff(const boost::optional<EntityT>& oldEntity,
                                const boost::optional<EntityT>& newEntity)
        {
            if(newEntity && oldEntity) {
                if(newEntity->isEmpty() && oldEntity->isEmpty()) {
                    ; // nop
                } else if(oldEntity->isEmpty()) {
                    m_added = newEntity;
                } else if(newEntity->isEmpty()) {
                    m_removed = oldEntity;
                } else { // if(!newEntity->isEmpty() && !oldEntity->isEmpty()) {
                    if(*oldEntity != *newEntity) {
                        m_modified = ModifiedEntity<EntityT>(*oldEntity, *newEntity);
                    }
                }
            }
        }

        const boost::optional<EntityT>&                     added() const { return m_added; }
        const boost::optional<EntityT>&                   removed() const { return m_removed; }
        const boost::optional<ModifiedEntity<EntityT> >& modified() const { return m_modified; }

        bool empty() const { return !m_added &&
                                    !m_removed &&
                                    !m_modified; }
    };

    //-----------------------------------------------------------------------------------

    template<class EntityT>
    class CheckInMultiEntityDiff
    {
        std::vector<EntityT> m_added;
        std::vector<EntityT> m_removed;
        std::vector<ModifiedEntity<EntityT> > m_modified;

    public:
        typedef boost::optional<CheckInMultiEntityDiff<EntityT> > Optional_t;

        CheckInMultiEntityDiff(const std::list<EntityT>& lOld, const std::list<EntityT>& lNew)
        {
            for(const EntityT& oldEntity: lOld) {
                const auto newEntityOpt = algo::find_opt_if<boost::optional>(lNew,
                    [&oldEntity](const EntityT& newEntity) { return newEntity.id() == oldEntity.id(); } );
                if(newEntityOpt) { // если найдена в новых и с изменениями -> помещаем в modified
                    if(*newEntityOpt != oldEntity) {
                        m_modified.push_back(ModifiedEntity<EntityT>(oldEntity, *newEntityOpt));
                    }
                } else { // если не найдена в новых -> помещаем в cancelled
                    m_removed.push_back(oldEntity);
                }
            }

            for(const EntityT& newEntity: lNew) {
                const auto oldEntityOpt = algo::find_opt_if<boost::optional>(lOld,
                    [&newEntity](const EntityT& oldEntity) { return oldEntity.id() == newEntity.id(); } );
                if(!oldEntityOpt) { // если в старых не найден -> помещаем в added
                    m_added.push_back(newEntity);
                }
            }
        }

        const std::vector<ModifiedEntity<EntityT> >& modified() const { return m_modified; }
        const std::vector<EntityT>&                     added() const { return m_added; }
        const std::vector<EntityT>&                   removed() const { return m_removed; }

        bool empty() const { return m_modified.empty() &&
                                    m_added.empty() &&
                                    m_removed.empty(); }
    };

    //-----------------------------------------------------------------------------------

    class PaxChange
    {
    public:
        typedef CheckInSingleEntityDiff<astra_entities::DocInfo>     DocChange_t;
        typedef CheckInSingleEntityDiff<astra_entities::VisaInfo>    VisaChange_t;

        typedef CheckInMultiEntityDiff<astra_entities::AddressInfo>  AddressChange_t;
        typedef CheckInMultiEntityDiff<astra_entities::Remark>       RemChange_t;
        typedef CheckInMultiEntityDiff<astra_entities::FqtRemark>    FqtRemChange_t;

        typedef DocChange_t::Optional_t     DocChangeOpt_t;
        typedef AddressChange_t::Optional_t AddressChangeOpt_t;
        typedef VisaChange_t::Optional_t    VisaChangeOpt_t;
        typedef RemChange_t::Optional_t     RemChangeOpt_t;
        typedef FqtRemChange_t::Optional_t  FqtRemChangeOpt_t;

    private:
        astra_entities::PaxInfo m_oldPax;
        astra_entities::PaxInfo m_newPax;

        DocChangeOpt_t          m_docChange;
        VisaChangeOpt_t         m_visaChange;

        AddressChangeOpt_t      m_addressChange;
        RemChangeOpt_t          m_remChange;
        FqtRemChangeOpt_t       m_fqtRemChange;
        bool                    m_persChange;

    public:
        PaxChange(const astra_entities::PaxInfo& oldPax,
                  const astra_entities::PaxInfo& newPax);

        int paxId() const { return m_oldPax.id(); }

        const astra_entities::PaxInfo& oldPax() const { return m_oldPax; }
        const astra_entities::PaxInfo& newPax() const { return m_newPax; }

        const DocChangeOpt_t&         docChange() const { return m_docChange; }
        const AddressChangeOpt_t& addressChange() const { return m_addressChange; }
        const VisaChangeOpt_t&       visaChange() const { return m_visaChange; }
        const RemChangeOpt_t&         remChange() const { return m_remChange; }
        const FqtRemChangeOpt_t&   fqtRemChange() const { return m_fqtRemChange; }
        bool                         persChange() const { return m_persChange; }

        bool empty() const { return !m_docChange &&
                                    !m_addressChange &&
                                    !m_visaChange &&
                                    !m_remChange &&
                                    !m_fqtRemChange &&
                                    !m_persChange; }
    };

    //

    PaxChange::PaxChange(const astra_entities::PaxInfo& oldPax,
                         const astra_entities::PaxInfo& newPax)
        : m_oldPax(oldPax), m_newPax(newPax), m_persChange(false)
    {
        // документ
        DocChange_t docChng(oldPax.m_doc, newPax.m_doc);
        if(!docChng.empty()) {
            m_docChange = docChng;
        }

        // виза
        VisaChange_t visaChng(oldPax.m_visa, newPax.m_visa);
        if(!visaChng.empty()) {
            m_visaChange = visaChng;
        }

        // адреса
        std::list<astra_entities::AddressInfo> lOldAddrs;
        if(oldPax.m_addrs) {
            lOldAddrs = oldPax.m_addrs->m_lAddrs;
        }
        std::list<astra_entities::AddressInfo> lNewAddrs;
        if(newPax.m_addrs) {
            lNewAddrs = newPax.m_addrs->m_lAddrs;

            AddressChange_t addrsChng(lOldAddrs, lNewAddrs);
            if(!addrsChng.empty()) {
                m_addressChange = addrsChng;
            }
        }

        // ремарки
        std::list<astra_entities::Remark> lOldRems;
        if(oldPax.m_rems) {
            lOldRems = oldPax.m_rems->m_lRems;
        }
        std::list<astra_entities::Remark> lNewRems;
        if(newPax.m_rems) {
            lNewRems = newPax.m_rems->m_lRems;
            RemChange_t remChng(lOldRems, lNewRems);
            if(!remChng.empty()) {
                m_remChange = remChng;
            }
        }
        
        // ремарки fqt
        std::list<astra_entities::FqtRemark> lOldFqtRems;
        if(oldPax.m_fqtRems) {
            lOldFqtRems = oldPax.m_fqtRems->m_lFqtRems;
        }
        std::list<astra_entities::FqtRemark> lNewFqtRems;
        if(newPax.m_fqtRems) {
            lNewFqtRems = newPax.m_fqtRems->m_lFqtRems;
            FqtRemChange_t fqtRemChng(lOldFqtRems, lNewFqtRems);
            if(!fqtRemChng.empty()) {
                m_fqtRemChange = fqtRemChng;
            }
        }
            
        // персональная информация
        if(oldPax.m_surname != newPax.m_surname || oldPax.m_name != newPax.m_name)
        {
            m_persChange = true;
        }
    }

    std::ostream& operator<<(std::ostream& os, const PaxChange& pc)
    {
        os << "Change of paxes with id:" << pc.newPax().id();
        if(pc.docChange()) {
            os << "\nChange of pax doc detected";
        }
        if(pc.addressChange()) {
            os << "\nChange of pax address detected";
        }
        if(pc.visaChange()) {
            os << "\nChange of pax visa detected";
        }
        if(pc.remChange()) {
            os << "\nChange of pax remarks detected";
        }
        if(pc.fqtRemChange()) {
            os << "\nChange of pax fqt remarks detected";
        }
        if(pc.persChange()) {
            os << "\nChange of pax personal detected";
        }
        return os;
    }

    //---------------------------------------------------------------------------------------

    class PaxDiff: public CheckInMultiEntityDiff<astra_entities::PaxInfo>
    {
    public:
        typedef CheckInMultiEntityDiff<astra_entities::PaxInfo> Base_t;
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
        for(const auto& re: modified()) {
            PaxChange paxChange(re.oldEntity(), re.newEntity());
            if(!paxChange.empty()) {
                m_paxChanges.push_back(paxChange);
            }
        }
    }

    const PaxDiff::PaxChanges_t& PaxDiff::paxChanges() const
    {
        return m_paxChanges;
    }

    std::ostream& operator<<(std::ostream& os, const PaxDiff& pd)
    {
        os << "\n" << pd.added().size()    << " pax(es) was(were) added"
           << "\n" << pd.removed().size()  << " pax(es) was(were) cancelled"
           << "\n" << pd.modified().size() << " pax(es) was(were) changed";
        return os;
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

    //-----------------------------------------------------------------------------------

    struct PointsPair
    {
        int m_pointDep;
        int m_pointArv;

        PointsPair(int pointDep, int pointArv)
            : m_pointDep(pointDep),
              m_pointArv(pointArv)
        {}

        static PointsPair readByGrpId(int grpId)
        {
            LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << " by grpId " << grpId;

            int pointDep = ASTRA::NoExists,
                pointArv = ASTRA::NoExists;
            OciCpp::CursCtl cur = make_curs(
                    "select POINT_DEP, POINT_ARV from PAX_GRP where GRP_ID=:grp_id");
            cur.bind(":grp_id", grpId)
               .def(pointDep)
               .def(pointArv)
               .EXfet();
            if(cur.err() == NO_DATA_FOUND) {
                LogWarning(STDLOG) << "Group " << grpId << " not found!";
                throw EXCEPTIONS::Exception("Group %d not found", grpId);
            }

            return PointsPair(pointDep, pointArv);
        }
    };

    //-----------------------------------------------------------------------------------

    struct PointInfo
    {
        std::string            m_airline;
        Ticketing::FlightNum_t m_flNum;
        boost::gregorian::date m_scdOut;
        std::string            m_airport;

        PointInfo()
        {}

        PointInfo(const std::string& airline,
                  const Ticketing::FlightNum_t& flNum,
                  const boost::gregorian::date& scdOut,
                  const std::string& airport)
            : m_airline(airline),
              m_flNum(flNum),
              m_scdOut(scdOut),
              m_airport(airport)
        {}

        static PointInfo readByPointId(int pointId)
        {
            LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << " by pointId " << pointId;

            std::string airl, airp;
            int flNum = 0;
            boost::gregorian::date scdOut;

            OciCpp::CursCtl cur = make_curs(
                    "select AIRLINE, FLT_NO, SCD_OUT, AIRP from POINTS where POINT_ID=:point_id");
            cur.bind(":point_id", pointId)
               .defNull(airl, "")
               .defNull(flNum, 0)
               .defNull(scdOut, boost::gregorian::date())
               .def(airp)
               .EXfet();
            if(cur.err() == NO_DATA_FOUND) {
                LogWarning(STDLOG) << "Point " << pointId << " not found!";
                throw EXCEPTIONS::Exception("Point %d not found", pointId);
            }

            return PointInfo(airl,
                             Ticketing::getFlightNum(flNum),
                             BASIC::date_time::UTCToLocal(scdOut, AirpTZRegion(airp)),
                             airp);
        }
    };

    //-----------------------------------------------------------------------------------

    class IatciUpdater
    {
    public:
        IatciUpdater(xmlNodePtr iatciNode, const std::string& iatciRootSegsNodeName,
                     xmlNodePtr reqNode,   const std::string& reqRootSegsNodeName);

        void updatePaxDoc(const iatci::PaxDetails& pax);
        void updatePaxAddresses(const iatci::PaxDetails& pax);
        void updatePaxVisa(const iatci::PaxDetails& pax);
        void updatePaxRems(const iatci::PaxDetails& pax);
        void updatePaxFqtRems(const iatci::PaxDetails& pax);
        void updateBagPoolNums(const iatci::PaxDetails& pax);
        void updatePersonal(const iatci::PaxDetails& pax,
                            const iatci::UpdatePaxDetails& updPax);
        void updatePaxSeat(const iatci::UpdateSeatDetails& updSeat);

        xmlNodePtr iatciNode() const { return m_iatciNode; }
        
        bool tryUpdatePaxDoc(const iatci::PaxDetails& pax);
        bool tryUpdatePaxVisa(const iatci::PaxDetails& pax);
        bool tryUpdatePaxAddresses(const iatci::PaxDetails& pax);

    protected:
        std::list<xmlNodePtr> findPaxNodes(xmlNodePtr parentNode,
                                           const iatci::PaxDetails& pax) const;

        void copyNodeToIatci(const iatci::PaxDetails& pax,
                             const std::string& nodeName);

        void updateNode(xmlNodePtr parentNode,
                        const std::string& newVal,
                        const std::string& nodeName);
        void updateSeatNode(int tripId, int paxId, const std::string& seat);

    private:
        xmlNodePtr        m_iatciNode;
        xmlNodePtr        m_reqNode;
        std::string       m_iatciRootSegsNodeName;
        std::string       m_reqRootSegsNodeName;
    };

    //

    IatciUpdater::IatciUpdater(xmlNodePtr iatciNode,
                               const std::string& iatciRootSegsNodeName,
                               xmlNodePtr reqNode,
                               const std::string& reqRootSegsNodeName)
        : m_iatciNode(iatciNode),
          m_reqNode(reqNode),
          m_iatciRootSegsNodeName(iatciRootSegsNodeName),
          m_reqRootSegsNodeName(reqRootSegsNodeName)
    {
        ASSERT(m_iatciNode != NULL);
        ASSERT(m_reqNode != NULL);
    }

    void IatciUpdater::updatePaxDoc(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "document");
    }

    void IatciUpdater::updatePaxRems(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "rems");
    }

    void IatciUpdater::updatePaxAddresses(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "addresses");
    }

    void IatciUpdater::updatePaxVisa(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "doco");
    }

    void IatciUpdater::updatePaxFqtRems(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "fqt_rems");
    }

    void IatciUpdater::updateBagPoolNums(const iatci::PaxDetails& pax)
    {
        copyNodeToIatci(pax, "bag_pool_num");
    }

    void IatciUpdater::updatePersonal(const iatci::PaxDetails& pax,
                                      const iatci::UpdatePaxDetails& updPax)
    {
        xmlNodePtr iatciSegsNode = findNodeR(m_iatciNode, "segments");
        ASSERT(iatciSegsNode != NULL);

        std::list<xmlNodePtr> iatciPaxNodes = findPaxNodes(iatciSegsNode, pax);
        ASSERT(!iatciPaxNodes.empty());
        for(const auto& iatciPaxNode: iatciPaxNodes) {
            updateNode(iatciPaxNode, "surname", updPax.surname());
            updateNode(iatciPaxNode, "name",    updPax.name());
        }
    }

    void IatciUpdater::updatePaxSeat(const iatci::UpdateSeatDetails& updSeat)
    {
        int tripId = NodeAsInteger("trip_id", m_reqNode, ASTRA::NoExists);
        ASSERT(tripId != ASTRA::NoExists);

        int paxId = NodeAsInteger("pax_id", m_reqNode, ASTRA::NoExists);
        ASSERT(paxId != ASTRA::NoExists);

        updateSeatNode(tripId, paxId, updSeat.seat());
    }
    
    bool IatciUpdater::tryUpdatePaxDoc(const iatci::PaxDetails& pax)
    {
        xmlNodePtr reqSegsNode = findNodeR(m_reqNode, m_reqRootSegsNodeName);
        if(reqSegsNode == NULL) {
            tst();
            return false;
        }
        std::list<xmlNodePtr> paxNodes = findPaxNodes(reqSegsNode, pax);
        if(paxNodes.empty()) {
            return false;
        }
        XmlPax xmlPax = XmlEntityReader::readPax(paxNodes.front());
        if(xmlPax.doc && !xmlPax.doc->toDoc().isEmpty()) {
            updatePaxDoc(pax);
            return true;
        }
        
        return false;
    }

    bool IatciUpdater::tryUpdatePaxVisa(const iatci::PaxDetails& pax)
    {
        xmlNodePtr reqSegsNode = findNodeR(m_reqNode, m_reqRootSegsNodeName);
        if(reqSegsNode == NULL) {
            tst();
            return false;
        }
        std::list<xmlNodePtr> paxNodes = findPaxNodes(reqSegsNode, pax);
        if(paxNodes.empty()) {
            return false;
        }
        XmlPax xmlPax = XmlEntityReader::readPax(paxNodes.front());
        if(xmlPax.visa && !xmlPax.visa->toVisa().isEmpty()) {
            updatePaxVisa(pax);
            return true;
        }

        return false;
    }

    bool IatciUpdater::tryUpdatePaxAddresses(const iatci::PaxDetails& pax)
    {
        xmlNodePtr reqSegsNode = findNodeR(m_reqNode, m_reqRootSegsNodeName);
        if(reqSegsNode == NULL) {
            tst();
            return false;
        }
        std::list<xmlNodePtr> paxNodes = findPaxNodes(reqSegsNode, pax);
        if(paxNodes.empty()) {
            return false;
        }
        XmlPax xmlPax = XmlEntityReader::readPax(paxNodes.front());
        if(xmlPax.addrs) {
            updatePaxAddresses(pax);
            return true;
        }

        return false;
    }

    std::list<xmlNodePtr> IatciUpdater::findPaxNodes(xmlNodePtr parentNode,
                                                     const iatci::PaxDetails& pax) const
    {
        std::list<xmlNodePtr> paxNodes;
        for(xmlNodePtr segNode = parentNode->children;
            segNode != NULL; segNode = segNode->next)
        {
            xmlNodePtr paxesNode = findNode(segNode, "passengers");
            for(xmlNodePtr paxNode = paxesNode->children;
                paxNode != NULL; paxNode = paxNode->next)
            {
                XmlPax xmlPax = XmlEntityReader::readPax(paxNode);
                LogTrace(TRACE3) << "compare paxes: " << xmlPax.surname << "/" << xmlPax.name
                                 << " and " << pax.surname() << "/" << pax.name();
                // возможно стоит сравнивать ещё что-то (например, тип пакса)
                if(xmlPax.surname == pax.surname() &&
                   xmlPax.name    == pax.name())
                {
                    paxNodes.push_back(paxNode);
                }
            }
        }
        return paxNodes;
    }

    void IatciUpdater::copyNodeToIatci(const iatci::PaxDetails& pax,
                                       const std::string& nodeName)
    {
        xmlNodePtr iatciSegsNode = findNodeR(m_iatciNode, m_iatciRootSegsNodeName);
        ASSERT(iatciSegsNode != NULL);
        xmlNodePtr reqSegsNode = findNodeR(m_reqNode, m_reqRootSegsNodeName);
        if(reqSegsNode == NULL) {
            tst();
            return;
        }
        std::list<xmlNodePtr> reqPaxNodes = findPaxNodes(reqSegsNode, pax);
        if(reqPaxNodes.empty()) {
            tst();
            return;
        }

        xmlNodePtr reqPaxNode = reqPaxNodes.front();
        xmlNodePtr srcNode = findNode(reqPaxNode, nodeName);
        ASSERT(srcNode);

        std::list<xmlNodePtr> iatciPaxNodes = findPaxNodes(iatciSegsNode, pax);
        ASSERT(!iatciPaxNodes.empty());
        for(const auto& iatciPaxNode: iatciPaxNodes)
        {
            xmlNodePtr destNode = findNode(iatciPaxNode, nodeName);
            ASSERT(destNode);
            RemoveChildNodes(destNode);
            RemoveNode(destNode);
            CopyNode(iatciPaxNode, srcNode);
        }
    }

    void IatciUpdater::updateNode(xmlNodePtr parentNode,
                                  const std::string& newVal,
                                  const std::string& nodeName)
    {
        xmlNodePtr node = findNode(parentNode, nodeName);
        ASSERT(node != NULL);
        std::string oldVal = NodeAsString(node);
        LogTrace(TRACE3) << "replace value of '" << nodeName << "' node from '"
                         << oldVal << "' to '" << newVal << "'";
        xmlNodeSetContent(node, newVal);
    }

    void IatciUpdater::updateSeatNode(int tripId, int paxId,
                                      const std::string& seat)
    {
        xmlNodePtr iatciSegsNode = findNodeR(m_iatciNode, "segments");
        ASSERT(iatciSegsNode != NULL);
        for(xmlNodePtr segNode = iatciSegsNode->children;
            segNode != NULL; segNode = segNode->next)
        {
            int pointDep = NodeAsInteger("point_dep", segNode);
            if(pointDep == tripId) {
                xmlNodePtr paxesNode = findNode(segNode, "passengers");
                for(xmlNodePtr paxNode = paxesNode->children;
                    paxNode != NULL; paxNode = paxNode->next)
                {
                    int pId = NodeAsInteger("pax_id", paxNode);
                    if(pId == paxId) {
                        xmlNodePtr seatNode = findNode(paxNode, "seat_no");
                        ASSERT(seatNode != NULL);
                        std::string oldSeat = NodeAsString(seatNode);
                        LogTrace(TRACE5) << "Replace seat " << oldSeat << " with " << seat;
                        xmlNodeSetContent(seatNode, seat);
                        return;
                    }
                }
            }
        }

        TST();
        throw EXCEPTIONS::Exception("Logic error!");
    }

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

static bool isReseatReq(xmlNodePtr reqNode)
{
    return findNodeR(reqNode, "Reseat") != NULL;
}

static bool isCheckinRequest(xmlNodePtr reqNode)
{
    return findNodeR(reqNode, "TCkinSavePax") != NULL;
}

static std::string getOldSeat(xmlNodePtr reqNode)
{
    xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
    ASSERT(tripIdNode != NULL && !NodeIsNULL(tripIdNode));
    int magicId = NodeAsInteger(tripIdNode);
    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(magicTab.grpId()));
    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    const XmlCheckInTab& oldIatciTab = oldIatciTabs.tabs().at(magicTab.tabInd() - 1);
    ASSERT(!oldIatciTab.lPax().empty());
    std::string oldSeat = oldIatciTab.lPax().front().m_seatNo;
    LogTrace(TRACE3) << "old seat: " << oldSeat;
    return oldSeat;
}

static PointsPair readPointsPair(int grpId)
{
    TCkinRoute tckinRoute;
    if(tckinRoute.GetRouteAfter(grpId, crtNotCurrent, crtOnlyDependent)) {
        // если на стороне Астры делалась локальная(не iatci) сквозная регистрация
        ASSERT(!tckinRoute.empty());
        return PointsPair(tckinRoute.back().point_dep,
                          tckinRoute.back().point_arv);
    } else {
        // если один единственный сегмент
        return PointsPair::readByGrpId(grpId);
    }
}

static PointInfo readDepPointInfo(int grpId)
{
    return PointInfo::readByPointId(readPointsPair(grpId).m_pointDep);
}

static iatci::OriginatorDetails makeOrg(int grpId)
{
    PointInfo pointInfo = readDepPointInfo(grpId);
    return iatci::OriginatorDetails(pointInfo.m_airline,
                                    pointInfo.m_airport);
}

static iatci::FlightDetails makeOwnFlight(int pointDep, int pointArv)
{
    PointInfo depPointInfo = PointInfo::readByPointId(pointDep);
    PointInfo arvPointInfo = PointInfo::readByPointId(pointArv);
    return iatci::FlightDetails(depPointInfo.m_airline,
                                depPointInfo.m_flNum,
                                depPointInfo.m_airport,
                                arvPointInfo.m_airport,
                                depPointInfo.m_scdOut);
}

static iatci::FlightDetails makeOwnFlight(int grpId)
{
    PointsPair pointsPair = readPointsPair(grpId);
    return makeOwnFlight(pointsPair.m_pointDep,
                         pointsPair.m_pointArv);
}

static std::string getIatciRequestContext(const edifact::KickInfo& kickInfo = edifact::KickInfo())
{
    XMLDoc xmlCtxt = ASTRA::createXmlDoc("<context/>");
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
        if(grpIdNode != NULL && !isempty(grpIdNode)) {
            grpId = NodeAsInteger(grpIdNode);
        } else {
            xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
            if(tripIdNode != NULL && !isempty(tripIdNode)) {
                int magicId = NodeAsInteger(tripIdNode);
                ASSERT(magicId < 0);
                iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
                grpId = magicTab.grpId();
            } else {
                xmlNodePtr pointIdNode = findNodeR(reqNode, "point_id");
                ASSERT((pointIdNode != NULL && !isempty(pointIdNode)));
                int pointId = NodeAsInteger(pointIdNode);
                xmlNodePtr regNoNode = findNodeR(reqNode, "reg_no");
                if(regNoNode != NULL && !isempty(regNoNode)) {
                    int regNo = NodeAsInteger(regNoNode);
                    grpId = astra_api::findGrpIdByRegNo(pointId, regNo);
                } else {
                    xmlNodePtr paxIdNode = findNodeR(reqNode, "pax_id");
                    if(paxIdNode != NULL && !isempty(paxIdNode)) {
                        int paxId = NodeAsInteger(paxIdNode);
                        grpId = astra_api::findGrpIdByPaxId(pointId, paxId);
                    }
                }
            }
        }
    }

    int lastGrpId = iatci::getLastTCkinGrpId(grpId);
    LogTrace(TRACE3) << "grpId: " << grpId << "; lastGrpId: " << lastGrpId;
    ASSERT(grpId > 0 && lastGrpId > 0);
    return lastGrpId;
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

static boost::optional<iatci::UpdatePaxDetails> getUpdPersonal(const PaxChange& paxChange)
{
    return boost::none; // TODO
}

static boost::optional<iatci::UpdateServiceDetails> getUpdService(const PaxChange& paxChange)
{
    boost::optional<iatci::UpdateServiceDetails> updService;

    if(paxChange.remChange())
    {
        if(!updService) {
            updService = iatci::UpdateServiceDetails();
        }

        /* ремарки */

        // удалённые
        for(auto& rem: paxChange.remChange()->removed()) {
            updService->addSsr(iatci::makeUpdSsr(rem,
                                                 iatci::UpdateDetails::Cancel));
        }

        // добавленные
        for(auto& rem: paxChange.remChange()->added()) {
            updService->addSsr(iatci::makeUpdSsr(rem,
                                                 iatci::UpdateDetails::Add));
        }

        // измененные
        for(auto& chng: paxChange.remChange()->modified()) {
            updService->addSsr(iatci::makeUpdSsr(chng.newEntity(),
                                                 iatci::UpdateDetails::Replace));
        }
    }


    //

    if(paxChange.fqtRemChange())
    {
        if(!updService) {
            updService = iatci::UpdateServiceDetails();
        }

        /* ремарки fqt */

        // удалённые
        for(auto& rem: paxChange.fqtRemChange()->removed()) {
            updService->addSsr(iatci::makeUpdSsrFqt(rem,
                                                    iatci::UpdateDetails::Cancel));
        }

        // добавленные
        for(auto& rem: paxChange.fqtRemChange()->added()) {
            updService->addSsr(iatci::makeUpdSsrFqt(rem,
                                                    iatci::UpdateDetails::Add));
        }

        // измененные
        for(auto& chng: paxChange.fqtRemChange()->modified()) {
            updService->addSsr(iatci::makeUpdSsrFqt(chng.newEntity(),
                                                    iatci::UpdateDetails::Replace));
        }
    }

    return updService;
}

static boost::optional<iatci::UpdateDocDetails> getUpdDoc(const PaxChange& paxChange)
{
    if(!paxChange.docChange()) {
        return boost::none;
    }

    // знаем, что может быть один документ на пассажира

    // удаленный
    if(paxChange.docChange()->removed()) {
        return iatci::makeUpdDoc(*paxChange.docChange()->removed(),
                                 iatci::UpdateDetails::Cancel);
    }

    // добавленный
    if(paxChange.docChange()->added()) {
        return iatci::makeUpdDoc(*paxChange.docChange()->added(),
                                 iatci::UpdateDetails::Add);
    }

    // изменённый
    if(paxChange.docChange()->modified()) {
        return iatci::makeUpdDoc(paxChange.docChange()->modified()->newEntity(),
                                 iatci::UpdateDetails::Replace);
    }

    ASSERT(false); // не должны сюда попадать
    return boost::none;
}

static boost::optional<iatci::UpdateAddressDetails> getUpdAddress(const PaxChange& paxChange)
{
    if(!paxChange.addressChange()) {
        return boost::none;
    }

    if(paxChange.addressChange()->empty()) {
        return boost::none;
    }

    UpdateAddressDetails result(UpdateDetails::Replace);

    // добавленные адреса
    for(const auto& addedAddr: paxChange.addressChange()->added()) {
        result.addAddr(iatci::makeAddrInfo(addedAddr));
    }

    // измененные адреса
    for(const auto& modifiedAddr: paxChange.addressChange()->modified()) {
        result.addAddr(iatci::makeAddrInfo(modifiedAddr.newEntity()));
    }

    return result;
}

static boost::optional<iatci::UpdateVisaDetails> getUpdVisa(const PaxChange& paxChange)
{
    if(!paxChange.visaChange()) {
        return boost::none;
    }

    // знаем, что может быть одна виза на пассажира

    // удаленная виза
    if(paxChange.visaChange()->removed()) {
        return iatci::makeUpdVisa(*paxChange.visaChange()->removed(),
                                  iatci::UpdateDetails::Cancel);
    }


    // добавленная виза
    if(paxChange.visaChange()->added()) {
        return iatci::makeUpdVisa(*paxChange.visaChange()->added(),
                                  iatci::UpdateDetails::Add);
    }

    // изменённая виза
    if(paxChange.visaChange()->modified()) {
        return iatci::makeUpdVisa(paxChange.visaChange()->modified()->newEntity(),
                                  iatci::UpdateDetails::Replace);
    }

    ASSERT(false); // не должны сюда попадать
    return boost::none;
}

static edifact::KickInfo getIatciKickInfo(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    if(reqNode != NULL)
    {
        int termReqCtxtId = AstraContext::SetContext("TERM_REQUEST", XMLTreeToText(reqNode->doc));
        if(ediResNode != NULL) {
            AstraEdifact::addToEdiResponseCtxt(termReqCtxtId, ediResNode->children, "");
        }
        return AstraEdifact::createKickInfo(termReqCtxtId, "IactiInterface");
    }

    return edifact::KickInfo();
}

static std::list<astra_entities::PaxInfo> filterNotInfants(const std::list<astra_entities::PaxInfo>& lPax)
{
    std::list<astra_entities::PaxInfo> lNotInfants;
    for(const auto& pax: lPax) {
        if(!pax.isInfant()) {
            lNotInfants.push_back(pax);
        }
    }
    return lNotInfants;
}

static std::list<astra_entities::PaxInfo> filterInfants(const std::list<astra_entities::PaxInfo>& lPax)
{
    std::list<astra_entities::PaxInfo> lInfants;
    for(const auto& pax: lPax) {
        if(pax.isInfant()) {
            lInfants.push_back(pax);
        }
    }
    return lInfants;
}

static boost::optional<astra_entities::PaxInfo> findInfant(const std::list<astra_entities::PaxInfo>& lInfants,
                                                           const astra_entities::Remark& ssrInft)
{
    for(const auto& infant: lInfants) {
        if(ssrInft.containsText(infant.fullName())) {
            return infant;
        }
    }
    return boost::none;
}

static boost::optional<astra_entities::PaxInfo> findPax(const std::list<astra_entities::PaxInfo>& lPax,
                                                        int paxId)
{
    for(const auto& pax: lPax) {
        if(pax.id() == paxId) {
            return pax;
        }
    }

    return boost::none;
}

static boost::optional<astra_entities::PaxInfo> findPax(const std::list<astra_entities::PaxInfo>& lPax,
                                                        const std::string& fullName)
{
    for(const auto& pax: lPax) {
        if(pax.fullName() == fullName) {
            return pax;
        }
    }

    return boost::none;
}

static boost::optional<astra_entities::PaxInfo> findInfantByParentId(const std::list<astra_entities::PaxInfo>& lInfants,
                                                                     int parentId)
{
    for(const auto& inft: lInfants) {
        if(inft.iatciParentId() == parentId) {
            return inft;
        }
    }

    return boost::none;
}

static void checkInfants(const std::list<astra_entities::PaxInfo>& lReqInfants,
                         const std::list<astra_entities::PaxInfo>& lReqNonInfants,
                         const std::list<astra_entities::PaxInfo>& lPax)
{
    for(const auto& reqInft: lReqInfants) {
        auto inft = findPax(lPax, reqInft.id());
        ASSERT(inft && inft->iatciParentId());
        if(!findPax(lReqNonInfants, inft->iatciParentId())) {
            throw AstraLocale::UserException("MSG.CHECKIN.UNABLE_TO_CANCEL_INFANT_WITHOUT_PARENT");
        }
    }
}

static iatci::CkiParams getCkiParams(xmlNodePtr reqNode)
{
    XmlCheckInTabs ownTabs(findNodeR(reqNode, "segments"));
    ASSERT(!ownTabs.empty());

    const XmlCheckInTab& lastOwnTab = ownTabs.tabs().back();

    const astra_entities::SegmentInfo lastOwnSeg = lastOwnTab.seg();

    XmlCheckInTabs ediTabs(findNodeR(reqNode, "iatci_segments"));
    ASSERT(ediTabs.size() == 1);

    const XmlCheckInTab& firstEdiTab = ediTabs.tabs().front();
    ASSERT(!firstEdiTab.lPax().empty());

    const std::list<astra_entities::PaxInfo> lPaxOwn = lastOwnTab.lPax();
    const std::list<astra_entities::PaxInfo> lPax = firstEdiTab.lPax();
    const std::list<astra_entities::PaxInfo> lNonInfants = filterNotInfants(lPax);
    const std::list<astra_entities::PaxInfo> lInfants = filterInfants(lPax);

    std::list<iatci::dcqcki::PaxGroup> lPaxGrp;
    std::set<int> processedInfantIds;
    for(const auto& pax: lNonInfants) {
        // поищем SSR INFT у пассажира из non-edi вкладки
        boost::optional<astra_entities::PaxInfo> ownPax = findPax(lPaxOwn, pax.fullName());
        ASSERT(ownPax);
        boost::optional<astra_entities::Remark> ssrInft = ownPax->ssrInft();

        boost::optional<astra_entities::PaxInfo> inft;

        boost::optional<iatci::PaxDetails> infant;
        boost::optional<iatci::DocDetails> infantDoc;
        boost::optional<iatci::AddressDetails> infantAddress;
        boost::optional<iatci::VisaDetails> infantVisa;
        if(ssrInft) {
            inft = findInfant(lInfants, *ssrInft);
            if(inft) {
                if(!algo::contains(processedInfantIds, inft->id())) {
                    processedInfantIds.insert(inft->id());
                    infant        = iatci::makeQryPax(*inft);
                    infantDoc     = iatci::makeDoc(*inft);
                    infantAddress = iatci::makeAddress(*inft);
                    infantVisa    = iatci::makeVisa(*inft);
                } else {
                    LogTrace(TRACE1) << "Skip infant already processed: " << inft->fullName()
                                     << " (" << inft->id() << ")";
                    inft = boost::none;
                }
            }
        }
        lPaxGrp.push_back(iatci::dcqcki::PaxGroup(iatci::makeQryPax(pax, inft),
                                                  iatci::makeReserv(pax),
                                                  iatci::makeSeat(pax),
                                                  iatci::makeBaggage(pax),
                                                  iatci::makeService(pax, inft),
                                                  iatci::makeDoc(pax),
                                                  iatci::makeAddress(pax),
                                                  iatci::makeVisa(pax),
                                                  infant,
                                                  infantDoc,
                                                  infantAddress,
                                                  infantVisa));
    }

    return iatci::CkiParams(iatci::makeOrg(lastOwnSeg),
                            iatci::makeCascade(),
                            iatci::dcqcki::FlightGroup(iatci::makeFlight(firstEdiTab.seg()),
                                                       makeOwnFlight(lastOwnSeg.m_pointDep, lastOwnSeg.m_pointArv),
                                                       lPaxGrp));
}

static void checkLatOnly(const std::string& field)
{
    if(StrUtils::containsRus(field)) {
        LogWarning(STDLOG) << "Field containing rus characters is found: "
                           << "'" << field << "'";
        throw AstraLocale::UserException("MSG.UNEXPECTED_RUS_FIELD");
    }
}

static void checkLatOnly(const boost::optional<iatci::PaxDetails>& pax)
{
    if(pax) {
        checkLatOnly(pax->surname());
        checkLatOnly(pax->name());
        checkLatOnly(pax->qryRef());
        checkLatOnly(pax->respRef());
    }
}

static void checkLatOnly(const boost::optional<iatci::UpdatePaxDetails>& updPax)
{
    if(updPax) {
        checkLatOnly(updPax->surname());
        checkLatOnly(updPax->name());
        checkLatOnly(updPax->qryRef());
    }
}

static void checkLatOnly(const boost::optional<iatci::ReservationDetails>& reserv)
{
    if(reserv) {
        checkLatOnly(reserv->rbd());
    }
}

static void checkLatOnly(const boost::optional<iatci::DocDetails>& doc)
{
    if(doc) {
        checkLatOnly(doc->docType());
        checkLatOnly(doc->issueCountry());
        checkLatOnly(doc->no());
        checkLatOnly(doc->surname());
        checkLatOnly(doc->name());
        checkLatOnly(doc->secondName());
        checkLatOnly(doc->gender());
        checkLatOnly(doc->nationality());
    }
}

static void checkLatOnly(const boost::optional<iatci::UpdateDocDetails>& updDoc)
{
    if(updDoc) {
        DocDetails dd = updDoc.get();
        checkLatOnly(dd);
    }
}

static void checkLatOnly(const ServiceDetails::SsrInfo& ssr)
{
    checkLatOnly(ssr.ssrCode());
    checkLatOnly(ssr.ssrText());
    checkLatOnly(ssr.freeText());
}

static void checkLatOnly(const boost::optional<ServiceDetails>& service)
{
    if(service) {
        checkLatOnly(service->osi());
        for(auto ssr: service->lSsr()) {
            checkLatOnly(ssr);
        }
    }
}

static void checkLatOnly(const boost::optional<UpdateServiceDetails>& updService)
{
    if(updService) {
        for(auto ssr: updService->lSsr()) {
            checkLatOnly(ssr);
        }
    }
}

static void checkLatOnly(const boost::optional<AddressDetails>& address)
{
    if(address) {
        for(auto addr: address->lAddr())
        {
            checkLatOnly(addr.type());
            checkLatOnly(addr.country());
            checkLatOnly(addr.address());
            checkLatOnly(addr.city());
            checkLatOnly(addr.region());
            checkLatOnly(addr.postalCode());
        }
    }
}

static void checkLatOnly(const boost::optional<UpdateAddressDetails>& updAddress)
{
    if(updAddress) {
        AddressDetails ad = updAddress.get();
        checkLatOnly(ad);
    }
}

static void checkLatOnly(const boost::optional<VisaDetails>& visa)
{
    if(visa) {
        checkLatOnly(visa->visaType());
        checkLatOnly(visa->issueCountry());
        checkLatOnly(visa->no());
        checkLatOnly(visa->placeOfIssue());
    }
}

static void checkLatOnly(const boost::optional<UpdateVisaDetails>& updVisa)
{
    if(updVisa) {
        VisaDetails vd = updVisa.get();
        checkLatOnly(vd);
    }
}

static void checkLatOnly(const boost::optional<SeatDetails>& seat)
{
    if(seat) {
        checkLatOnly(seat->seat());
    }
}

static void checkLatOnly(const boost::optional<UpdateSeatDetails>& updSeat)
{
    if(updSeat) {
        SeatDetails sd = updSeat.get();
        checkLatOnly(sd);
    }
}

static void checkLatOnly(const iatci::PaxGroup& paxGrp)
{
    checkLatOnly(paxGrp.pax());
    checkLatOnly(paxGrp.reserv());
    checkLatOnly(paxGrp.service());
    checkLatOnly(paxGrp.doc());
    checkLatOnly(paxGrp.address());
    checkLatOnly(paxGrp.visa());
    checkLatOnly(paxGrp.infant());
    checkLatOnly(paxGrp.infantDoc());
    checkLatOnly(paxGrp.infantAddress());
    checkLatOnly(paxGrp.infantVisa());
}

static void checkCkiPaxGroup(const iatci::dcqcki::PaxGroup& paxGrp)
{
    checkLatOnly(paxGrp);
    checkLatOnly(paxGrp.seat());
}

static void checkCkiParams(const iatci::CkiParams& params)
{
    for(auto paxGrp: params.fltGroup().paxGroups()) {
        checkCkiPaxGroup(paxGrp);
    }
}

static void checkCkuPaxGroup(const iatci::dcqcku::PaxGroup& paxGrp)
{
    checkLatOnly(paxGrp);
    checkLatOnly(paxGrp.updPax());
    checkLatOnly(paxGrp.updSeat());
    checkLatOnly(paxGrp.updService());
    checkLatOnly(paxGrp.updDoc());
    checkLatOnly(paxGrp.updAddress());
    checkLatOnly(paxGrp.updVisa());
    checkLatOnly(paxGrp.updInfant());
    checkLatOnly(paxGrp.updInfantDoc());
    checkLatOnly(paxGrp.updInfantAddress());
    checkLatOnly(paxGrp.updInfantVisa());
}

static void checkCkuParams(const iatci::CkuParams& params)
{
    for(auto paxGrp: params.fltGroup().paxGroups()) {
        checkCkuPaxGroup(paxGrp);
    }
}

static iatci::CkxParams getCkxParams(xmlNodePtr reqNode)
{
    int ownGrpId = getGrpId(reqNode, NULL, IatciInterface::Ckx);
    XMLDoc xmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(ownGrpId));

    XmlCheckInTabs oldIatciTabs(findNodeR(xmlDoc.docPtr()->children, "segments"));
    XmlCheckInTabs reqIatciTabs(findNodeR(reqNode, "iatci_segments"));

    const XmlCheckInTab& firstOldTab = oldIatciTabs.tabs().front();
    const XmlCheckInTab& firstReqTab = reqIatciTabs.tabs().front();

    // что имеем
    const std::list<astra_entities::PaxInfo> lPax = firstOldTab.lPax();
    const std::list<astra_entities::PaxInfo> lNonInfants = filterNotInfants(lPax);
    const std::list<astra_entities::PaxInfo> lInfants = filterInfants(lPax);

    // что в запросе
    const std::list<astra_entities::PaxInfo> lReqPax = firstReqTab.lPax();
    const std::list<astra_entities::PaxInfo> lReqNonInfants = filterNotInfants(lReqPax);
    const std::list<astra_entities::PaxInfo> lReqInfants = filterInfants(lReqPax);

    checkInfants(lReqInfants, lReqNonInfants, lPax);

    std::list<iatci::dcqckx::PaxGroup> lPaxGrp;
    for(const auto& pax: lNonInfants) {
        if(firstReqTab.getPaxById(pax.id())) {
            boost::optional<iatci::PaxDetails> infant;
            auto inft = findInfantByParentId(lInfants, pax.id());
            if(inft) {
                infant = iatci::makeQryPax(*inft);
            }
            lPaxGrp.push_back(iatci::dcqckx::PaxGroup(iatci::makeQryPax(pax, inft),
                                                      boost::none,
                                                      boost::none,
                                                      boost::none,
                                                      boost::none,
                                                      infant));
        }
    }

    return iatci::CkxParams(makeOrg(ownGrpId),
                            makeCascade(),
                            iatci::dcqckx::FlightGroup(iatci::makeFlight(firstReqTab.seg()),
                                                       makeOwnFlight(ownGrpId),
                                                       lPaxGrp));
}

static boost::optional<PaxChange> findPaxChange(const std::vector<PaxChange>& allChanges,
                                                int paxId)
{
    for(const auto& change: allChanges) {
        if(change.paxId() == paxId) {
            return change;
        }
    }

    return boost::none;
}

static boost::optional<XmlBag> findBag(const std::list<XmlBag>& bags, int bagNum)
{
    for(const auto& bag: bags) {
        if(bag.num == bagNum) {
            return bag;
        }
    }

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

static iatci::CkuParams getSeatUpdateParams(xmlNodePtr reqNode)
{
    int magicId = NodeAsInteger("trip_id", reqNode);
    ASSERT(magicId < 0);
    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    int paxId = NodeAsInteger("pax_id", reqNode);
    LogTrace(TRACE3) << "Iatci reseat: grpId: " << magicTab.grpId()
                     << "; tabInd: " << magicTab.tabInd()
                     << "; paxId: " << paxId;

    int ownGrpId = magicTab.grpId();
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(ownGrpId));

    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    ASSERT(magicTab.tabInd() <= oldIatciTabs.size() && magicTab.tabInd() > 0);
    const XmlCheckInTab& oldIatciTab = oldIatciTabs.tabs().at(magicTab.tabInd() - 1);
    auto paxOpt = oldIatciTab.getPaxById(paxId);
    ASSERT(paxOpt);
    std::string oldSeat = paxOpt->seatNo();
    LogTrace(TRACE3) << "old seat: " << oldSeat;

    iatci::UpdateSeatDetails updSeat = getSeatUpdate(reqNode);
    if(updSeat.seat() == oldSeat) {
        throw AstraLocale::UserException("MSG.SEATS.SEAT_NO.PASSENGER_OWNER");
    }

    boost::optional<astra_entities::PaxInfo> inft = findInfantByParentId(oldIatciTab.lPax(),
                                                                         paxOpt->id());
    boost::optional<iatci::PaxDetails> infant;
    if(inft) {
        infant = iatci::makeQryPax(*inft);
    }

    std::list<iatci::dcqcku::PaxGroup> lPaxGrp;
    lPaxGrp.push_back(iatci::dcqcku::PaxGroup(iatci::makeQryPax(*paxOpt, inft),
                                              boost::none, // Reserv
                                              boost::none, // Baggage
                                              boost::none, // Service
                                              infant,
                                              boost::none, // Update personal
                                              updSeat,     // Update seat
                                              boost::none, // Update baggage
                                              boost::none, // Update service
                                              boost::none, // Update doc
                                              boost::none, // Update address
                                              boost::none  // Update visa
                                              ));

    return iatci::CkuParams(makeOrg(ownGrpId),
                            boost::none,
                            iatci::dcqcku::FlightGroup(iatci::makeFlight(oldIatciTabs.tabs().front().xmlSeg()),
                                                       makeOwnFlight(ownGrpId),
                                                       lPaxGrp));
}

static bool ediTabsContainsDestination(const XmlCheckInTabs& tabs, const std::string& dest)
{
    LogTrace(TRACE3) << "check destination for " << dest;
    for(const auto tab: tabs.ediTabs()) {
        if(BaseTables::Port(tab.seg().m_airpArv)->ida() == BaseTables::Port(dest)->ida()) {
            return true;
        }
    }

    return false;
}

static boost::optional<iatci::CkuParams> getUpdateBaggageParams(xmlNodePtr reqNode)
{
    int ownGrpId = getGrpId(reqNode, NULL, IatciInterface::Cku);
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(ownGrpId));

    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    xmlNodePtr iatciSegsNode = findNodeR(reqNode, "iatci_segments");
    boost::optional<XmlCheckInTabs> reqIatciTabs; // must be!
    if(iatciSegsNode) {
        reqIatciTabs = XmlCheckInTabs(iatciSegsNode);
    } else {
        reqIatciTabs = oldIatciTabs;
    }
    ASSERT(reqIatciTabs);

    XmlCheckInTabs ownTabs(findNodeR(reqNode, "segments"));
    ASSERT(!ownTabs.empty());
    const XmlCheckInTab& firstOwnTab = ownTabs.tabs().front();

    const XmlCheckInTab& firstOldTab = oldIatciTabs.tabs().front();
    const XmlCheckInTab& firstReqTab = reqIatciTabs->tabs().front();

    std::list<XmlBag> reqBags = XmlEntityReader::readBags(findNodeR(reqNode, "bags"));

    std::list<XmlBagTag> oldBagTags;
    xmlNodePtr tagsNode = findNodeR(reqNode, "tags");
    if(tagsNode) {
        oldBagTags = XmlEntityReader::readBagTags(tagsNode);
    }

    std::list<XmlBagTag> reqBagTags;
    xmlNodePtr reqBagsNode = findNodeR(reqNode, "generated_tags");
    if(reqBagsNode) {
        reqBagTags = XmlEntityReader::readBagTags(reqBagsNode);
    }

    const auto allBagTags = algo::combine(oldBagTags, reqBagTags);

    // что имеем
    const std::list<astra_entities::PaxInfo> lPax = firstOldTab.lPax();

    // что в запросе
    const std::list<astra_entities::PaxInfo> lReqPax = firstReqTab.lPax();

    std::list<iatci::dcqcku::PaxGroup> lPaxGrp;

    astra_api::astra_entities::Baggage baggage;
    for(const auto& reqBag: reqBags) {
        baggage.addPool(astra_api::astra_entities::BagPool(reqBag.bag_pool_num,
                                                           reqBag.amount,
                                                           reqBag.weight),
                        reqBag.pr_cabin);
    }

    const std::set<int> bagPoolNums = baggage.poolNums();

    std::list<astra_entities::PaxInfo> lModPax;
    for(const auto& oldPax: lPax) {
        astra_entities::PaxInfo modPax = oldPax;
        boost::optional<astra_entities::PaxInfo> reqPax = findPax(lReqPax, oldPax.id());
        if(reqPax) {
            modPax.setBagPoolNum(reqPax->bagPoolNum());
        }
        lModPax.push_back(modPax);
    }

    const std::list<astra_entities::PaxInfo> lModNonInfants = filterNotInfants(lModPax);
    const std::list<astra_entities::PaxInfo> lModInfants = filterInfants(lModPax);

    std::string firstSegOperAirline = firstOwnTab.xmlSeg().seg_info.airline;
    ASSERT(!firstSegOperAirline.empty());

    std::set<int> processedPaxIds;
    for(const auto& pax: lModPax) {
        if(algo::contains(processedPaxIds, pax.id())) {
            LogTrace(TRACE1) << "Skip pax already processed: " << pax.fullName()
                             << " (" << pax.id() << ")";
            continue;
        }

        int poolNumAdult = 0, poolNumInft = 0, poolNum = 0;
        boost::optional<astra_entities::PaxInfo> adult, inft;
        if(pax.isInfant()) {
            inft = pax;
            adult = findPax(lModNonInfants, pax.iatciParentId());
            poolNumInft = inft->bagPoolNum();
            if(adult) {
                poolNumAdult = adult->bagPoolNum();
            }
            poolNum = poolNumInft;
        } else {
            inft = findInfantByParentId(lModInfants, pax.id());
            adult = pax;
            poolNumAdult = adult->bagPoolNum();
            if(inft) {
                poolNumInft = inft->bagPoolNum();
            }
            poolNum = poolNumAdult;
        }

        astra_api::astra_entities::BagPool total(poolNum),
                                       handTotal(poolNum);       

        if(poolNumInft) {
            ASSERT(algo::contains(bagPoolNums, poolNumInft));
            total += baggage.totalByPoolNum(poolNumInft);
            handTotal += baggage.totalByHandPoolNum(poolNumInft);
        }

        if(poolNumAdult) {
            ASSERT(algo::contains(bagPoolNums, poolNumAdult));
            total += baggage.totalByPoolNum(poolNumAdult);
            handTotal += baggage.totalByHandPoolNum(poolNumAdult);
        }

        boost::optional<iatci::PaxDetails> infant;
        processedPaxIds.insert(adult->id());
        if(inft) {
            processedPaxIds.insert(inft->id());
            infant = iatci::makeQryPax(*inft);
        }

        std::list<astra_entities::BaggageTag> bagTags;
        for(const XmlBagTag& xmlBagTag: allBagTags) {
            auto xmlBag = findBag(reqBags, xmlBagTag.bag_num);
            if(xmlBag) {
                bool saveTagFlag = false;
                if(poolNumInft && xmlBag->bag_pool_num == poolNumInft) {
                    saveTagFlag = true;
                }
                if(poolNumAdult && xmlBag->bag_pool_num == poolNumAdult) {
                    saveTagFlag = true;
                }

                if(saveTagFlag) {
                    bagTags.push_back(astra_entities::BaggageTag(firstSegOperAirline,
                                                                 xmlBagTag.no,
                                                                 1,
                                                                 xmlBag->airp_arv_final));
                }
            } else {
                LogError(STDLOG) << "Bag not found by num " << xmlBagTag.bag_num;
            }
        }

        bool skipUpdate = false;
        if(!bagTags.empty()) {
            skipUpdate = !ediTabsContainsDestination(oldIatciTabs,
                                                     bagTags.front().destination());
        }

        if(!skipUpdate)
        {
            lPaxGrp.push_back(iatci::dcqcku::PaxGroup(iatci::makeQryPax(adult.get(), inft),
                                                      boost::none, // Reserv
                                                      boost::none, // Baggage
                                                      boost::none, // Service
                                                      infant,
                                                      boost::none,
                                                      boost::none,
                                                      iatci::makeUpdBaggage(total,
                                                                            handTotal,
                                                                            bagTags),
                                                      boost::none,
                                                      boost::none,
                                                      boost::none,
                                                      boost::none));
        }
    }


    if(lPaxGrp.empty()) {
        LogTrace(TRACE0) << "Non-iatci pax bag change!";
        return boost::none;
    }

    return iatci::CkuParams(makeOrg(ownGrpId),
                            boost::none,
                            iatci::dcqcku::FlightGroup(iatci::makeFlight(oldIatciTabs.tabs().front().xmlSeg()),
                                                       makeOwnFlight(ownGrpId),
                                                       lPaxGrp));
}

static void checkDocAndVisaUpdate(const boost::optional<UpdateDocDetails>& updDoc,
                                  const boost::optional<UpdateVisaDetails>& updVisa)
{
    if(updDoc && updVisa) {
       throw AstraLocale::UserException("MSG.UPD_DOC_AND_VISA_SEPARATELY");
    }
}

static boost::optional<iatci::CkuParams> getCkuParams(xmlNodePtr reqNode)
{
    if(findNodeR(reqNode, "bags")) {
        return getUpdateBaggageParams(reqNode);
    }

    int ownGrpId = getGrpId(reqNode, NULL, IatciInterface::Cku);
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(ownGrpId));

    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    xmlNodePtr iatciSegsNode = findNodeR(reqNode, "iatci_segments");

    boost::optional<XmlCheckInTabs> reqIatciTabs; // optional only for compilation - must be!
    if(iatciSegsNode) {
        reqIatciTabs = XmlCheckInTabs(iatciSegsNode);
    } else {
        reqIatciTabs = oldIatciTabs;
    }
    ASSERT(reqIatciTabs);

    const XmlCheckInTab& firstOldTab = oldIatciTabs.tabs().front();

    TabsDiff diff(oldIatciTabs, *reqIatciTabs);
    if(diff.tabsDiff().empty()) {
        LogTrace(TRACE1) << "Empty iatci tabs diff";
        return boost::none;
    }

    if(!diff.tabsDiff().at(0)) {
        LogTrace(TRACE1) << "No changes at first iatci tab";
        return boost::none;
    }

    // что имеем
    const std::list<astra_entities::PaxInfo> lPax = firstOldTab.lPax();
    const std::list<astra_entities::PaxInfo> lNonInfants = filterNotInfants(lPax);
    const std::list<astra_entities::PaxInfo> lInfants = filterInfants(lPax);

    const auto firstTabDiff = diff.tabsDiff().at(0);

    PaxDiff paxDiff = firstTabDiff->paxDiff();
    LogTrace(TRACE3) << "Pax diff:" << paxDiff;

    std::list<iatci::dcqcku::PaxGroup> lPaxGrp;

    std::set<int> processedPaxIds;
    const PaxDiff::PaxChanges_t& allChanges = paxDiff.paxChanges();
    for(const auto& paxChange: allChanges) {
        const astra_entities::PaxInfo& oldPax = paxChange.oldPax();
        LogTrace(TRACE3) << "process diff of pax: " << oldPax.fullName();
        if(algo::contains(processedPaxIds, paxChange.paxId())) {
            LogTrace(TRACE1) << "Skip pax already processed: " << oldPax.fullName();
            continue;
        }

        boost::optional<PaxChange> adultChange, inftChange;

        boost::optional<astra_entities::PaxInfo> adult, inft;

        LogTrace(TRACE3) << paxChange;
        if(oldPax.isInfant()) {
            inft = oldPax;
            adult = findPax(lNonInfants, oldPax.iatciParentId());
            inftChange = paxChange;
        } else {
            inft = findInfantByParentId(lInfants, oldPax.id());
            adult = oldPax;
            adultChange = paxChange;
        }

        ASSERT(adult);
        adultChange = findPaxChange(allChanges, adult->id());
        if(inft) {
            inftChange = findPaxChange(allChanges, inft->id());
        }

        boost::optional<iatci::PaxDetails> infant;
        processedPaxIds.insert(adult->id());
        if(inft) {
            processedPaxIds.insert(inft->id());
            infant = iatci::makeQryPax(*inft);
        }

        boost::optional<iatci::UpdatePaxDetails>     adultUpdPersonal, inftUpdPersonal;
        boost::optional<iatci::UpdateServiceDetails> adultUpdService;
        boost::optional<iatci::UpdateDocDetails>     adultUpdDoc, inftUpdDoc;
        boost::optional<iatci::UpdateAddressDetails> adultUpdAddress, inftUpdAddress;
        boost::optional<iatci::UpdateVisaDetails>    adultUpdVisa, inftUpdVisa;

        if(adultChange) {
            adultUpdPersonal = getUpdPersonal(*adultChange);
            adultUpdService  = getUpdService(*adultChange);
            adultUpdDoc      = getUpdDoc(*adultChange);
            adultUpdAddress  = getUpdAddress(*adultChange);
            adultUpdVisa     = getUpdVisa(*adultChange);
        }

        if(inftChange) {
            inftUpdPersonal = getUpdPersonal(*inftChange);
            inftUpdDoc      = getUpdDoc(*inftChange);
            inftUpdAddress  = getUpdAddress(*inftChange);
            inftUpdVisa     = getUpdVisa(*inftChange);
        }

        checkDocAndVisaUpdate(adultUpdDoc, adultUpdVisa);
        checkDocAndVisaUpdate(inftUpdDoc, inftUpdVisa);

        lPaxGrp.push_back(iatci::dcqcku::PaxGroup(iatci::makeQryPax(adult.get(), inft),
                                                  boost::none, // Reserv
                                                  boost::none, // Baggage
                                                  boost::none, // Service
                                                  infant,
                                                  adultUpdPersonal,
                                                  boost::none, // Seat
                                                  boost::none, // Baggage
                                                  adultUpdService, // SSRs
                                                  adultUpdDoc, // Doc
                                                  adultUpdAddress, // Address
                                                  adultUpdVisa, // Visa
                                                  inftUpdPersonal,
                                                  inftUpdDoc,
                                                  inftUpdAddress,
                                                  inftUpdVisa));
    }

    if(lPaxGrp.empty()) {
        LogTrace(TRACE0) << "Non-iatci pax change!";
        return boost::none;
    }

    return iatci::CkuParams(makeOrg(ownGrpId),
                            boost::none,
                            iatci::dcqcku::FlightGroup(iatci::makeFlight(oldIatciTabs.tabs().front().xmlSeg()),
                                                       makeOwnFlight(ownGrpId),
                                                       lPaxGrp));
}

static iatci::BprParams getBprParams(xmlNodePtr reqNode)
{
    int ownGrpId = NodeAsInteger("grp_id", reqNode);
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(ownGrpId));
    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    ASSERT(!oldIatciTabs.tabs().empty());

    const XmlCheckInTab& firstEdiTab = oldIatciTabs.tabs().front();

    const std::list<astra_entities::PaxInfo> lPax = firstEdiTab.lPax();
    const std::list<astra_entities::PaxInfo> lNonInfants = filterNotInfants(lPax);
    const std::list<astra_entities::PaxInfo> lInfants = filterInfants(lPax);

    std::list<iatci::dcqbpr::PaxGroup> lPaxGrp;
    for(const auto& pax: lNonInfants) {
        boost::optional<iatci::PaxDetails> infant;
        auto inft = findInfantByParentId(lInfants, pax.id());
        if(inft) {
            infant = iatci::makeQryPax(*inft);
        }
        lPaxGrp.push_back(iatci::dcqbpr::PaxGroup(iatci::makeQryPax(pax),
                                                  boost::none, // reserv
                                                  boost::none, // baggage
                                                  boost::none, // service
                                                  infant));
    }

    return iatci::BprParams(makeOrg(ownGrpId),
                            boost::none,
                            iatci::dcqbpr::FlightGroup(iatci::makeFlight(firstEdiTab.xmlSeg()),
                                                       makeOwnFlight(ownGrpId),
                                                       lPaxGrp));
}

static iatci::PlfParams getPlfParams(int grpId)
{
    IatciPaxSeg ediPaxSeg = IatciPaxSeg::readFirst(grpId);
    return iatci::PlfParams(makeOrg(grpId),
                            ediPaxSeg.pax(),
                            ediPaxSeg.seg());
}

static iatci::SmfParams getSmfParams(int magicId)
{
    ASSERT(magicId < 0);
    if(magicId == -1) {
        throw AstraLocale::UserException("MSG.UNABLE_TO_SHOW_SEATMAP_BEFORE_CHECKIN");
    }

    iatci::MagicTab magicTab = iatci::MagicTab::fromNeg(magicId);
    XMLDoc oldXmlDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(magicTab.grpId()));
    XmlCheckInTabs oldIatciTabs(findNodeR(oldXmlDoc.docPtr()->children, "segments"));
    const XmlCheckInTab& oldIatciTab = oldIatciTabs.tabs().at(magicTab.tabInd() - 1);

    return iatci::SmfParams(makeOrg(magicTab.grpId()),
                            iatci::makeSeatReq(oldIatciTab.xmlSeg()),
                            iatci::makeFlight(oldIatciTab.xmlSeg()));
}

static iatci::SmfParams getSmfParams(xmlNodePtr reqNode)
{
    xmlNodePtr tripIdNode = findNodeR(reqNode, "trip_id");
    ASSERT(tripIdNode != NULL && !NodeIsNULL(tripIdNode));
    int magicId = NodeAsInteger(tripIdNode);
    return getSmfParams(magicId);
}

/////////////////////////////////////////////////////////////////////////////////////////

static void MagicUpdate(xmlNodePtr resNode, int grpId, IatciInterface::RequestType reqType)
{
    xmlNodePtr segsNode = findNodeR(resNode, "segments");
    unsigned segInd = 0;
    for(xmlNodePtr segNode = segsNode->children; segNode != NULL; segNode = segNode->next)
    {
        iatci::MagicTab magicTab(grpId, ++segInd);
        xmlNodeSetContent(findNode(segNode, "point_dep"), magicTab.toNeg());

        xmlNodePtr paxesNode = findNodeR(segNode, "passengers");
        if(paxesNode != NULL && reqType == IatciInterface::Bpr) {
            for(xmlNodePtr paxNode = paxesNode->children; paxNode != NULL; paxNode = paxNode->next) {
                xmlNodePtr bpPrintNode = findNode(paxNode, "pr_bp_print");
                if(bpPrintNode != NULL) {
                    xmlNodeSetContent(bpPrintNode, 1);
                }
            }
        }
    }
}

static void SaveIatciCkiXmlRes(xmlNodePtr iatciResNode, int grpId)
{
    LogTrace(TRACE3) << __FUNCTION__;
    iatci::IatciXmlDb::add(grpId, XMLTreeToText(iatciResNode->doc));
}

static void SaveIatciCkuPlfBprXmlRes(xmlNodePtr iatciResNode, int grpId)
{
    LogTrace(TRACE3) << __FUNCTION__;
    iatci::IatciXmlDb::upd(grpId, XMLTreeToText(iatciResNode->doc));
}

static void SaveIatciCkxXmlRes(xmlNodePtr iatciResNode, int grpId)
{
    LogTrace(TRACE3) << __FUNCTION__;

    XMLDoc loadedDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(grpId));
    xmlNodePtr loadedSegsNode = findNodeR(loadedDoc.docPtr()->children, "segments");

    // считаем, что список отменяемых пассажиров один и тот же на разных сегментах
    xmlNodePtr paxes2removeNode = findNodeR(iatciResNode, "passengers");
    std::list<XmlPax> paxes2remove = XmlEntityReader::readPaxes(paxes2removeNode);

    xmlNodePtr firstNewPaxesNode = NULL;

    for(xmlNodePtr segNode = loadedSegsNode->children;
        segNode != NULL; segNode = segNode->next)
    {
        xmlNodePtr paxesNode = findNodeR(segNode, "passengers");
        xmlNodePtr newPaxesNode = newChild(segNode, "new_passengers");
        for(xmlNodePtr paxNode = paxesNode->children;
            paxNode != NULL; paxNode = paxNode->next)
        {
            XmlPax p1 = XmlEntityReader::readPax(paxNode);
            bool found2remove = algo::find_opt_if<boost::optional>(paxes2remove,
                [&p1](const XmlPax& p2) { return p1.pax_id == p2.pax_id; });

            if(!found2remove) {
                CopyNode(newPaxesNode, paxNode);
            }
        }

        RemoveNode(paxesNode);
        RenameNode(newPaxesNode, "passengers");

        if(!firstNewPaxesNode) {
            firstNewPaxesNode = newPaxesNode;
        }
    }

    // всех пассажиров удалили - удаляем совсем данные iatci
    if(XmlEntityReader::readPaxes(firstNewPaxesNode).empty()) {
        iatci::IatciXmlDb::del(grpId);
    } else {
        iatci::IatciXmlDb::upd(grpId, XMLTreeToText(loadedSegsNode->doc));
    }
}

static void SaveIatciXmlResByReqType(xmlNodePtr iatciResNode, int grpId,
                                     IatciInterface::RequestType reqType)
{
    switch(reqType)
    {
    case IatciInterface::Cki:
        SaveIatciCkiXmlRes(iatciResNode, grpId);
        break;

    case IatciInterface::Cku:
    case IatciInterface::Plf:
    case IatciInterface::Bpr:        
        SaveIatciCkuPlfBprXmlRes(iatciResNode, grpId);
        break;

    case IatciInterface::Ckx:
        SaveIatciCkxXmlRes(iatciResNode, grpId);
        break;

    default:
        break;
    }
}

static void SaveIatciXmlRes(xmlNodePtr iatciResNode, xmlNodePtr termReqNode, int grpId,
                            IatciInterface::RequestType reqType)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    MagicUpdate(iatciResNode, grpId, reqType);
    SaveIatciXmlResByReqType(iatciResNode, grpId, reqType);
}

static void SaveIatciGrp(int grpId, IatciInterface::RequestType reqType,
                         xmlNodePtr ediResNode, xmlNodePtr termReqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId:" << grpId;
    xmlNodePtr iatciResNode = NodeAsNode("/iatci_result", ediResNode);
    SaveIatciXmlRes(iatciResNode, termReqNode, grpId, reqType);
}

static void UpdateIatciGrp(int grpId, IatciInterface::RequestType reqType,
                           xmlNodePtr ediResNode, xmlNodePtr termReqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId:" << grpId;
    XMLDoc loadedDoc = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(grpId));

    boost::optional<iatci::CkuParams> ckuParams;
    if(isReseatReq(termReqNode)) {
        ckuParams = getSeatUpdateParams(termReqNode);
    } else {
        ckuParams = getCkuParams(termReqNode);
    }
    ASSERT(ckuParams);

    IatciUpdater updater(loadedDoc.docPtr()->children, "segments",
                         termReqNode, "iatci_segments");

    dcqcku::FlightGroup fltGrp = ckuParams->fltGroup();
    for(const dcqcku::PaxGroup& paxGrp: fltGrp.paxGroups()) {
        if(paxGrp.updBaggage()) {
            updater.updateBagPoolNums(paxGrp.pax());
            if(paxGrp.infant()) {
                updater.updateBagPoolNums(paxGrp.infant().get());
            }
        }
        if(paxGrp.updDoc()) {
            updater.updatePaxDoc(paxGrp.pax());
        }
        if(paxGrp.updAddress()) {
            updater.updatePaxAddresses(paxGrp.pax());
        }
        if(paxGrp.updVisa()) {
            updater.updatePaxVisa(paxGrp.pax());
        }
        if(paxGrp.updService()) {
            if(paxGrp.updService()->containsNonFqt()) {
                updater.updatePaxRems(paxGrp.pax());
            }
            if(paxGrp.updService()->containsFqt()) {
                updater.updatePaxFqtRems(paxGrp.pax());
            }
        }
        if(paxGrp.updPax()) {
            updater.updatePersonal(paxGrp.pax(), paxGrp.updPax().get());
        }
        if(paxGrp.updSeat()) {
            updater.updatePaxSeat(paxGrp.updSeat().get());
        }
        if(paxGrp.updInfant()) {
            ASSERT(paxGrp.infant());
            updater.updatePersonal(paxGrp.infant().get(), paxGrp.updInfant().get());
        }
        if(paxGrp.updInfantDoc()) {
            ASSERT(paxGrp.infant());
            updater.updatePaxDoc(paxGrp.infant().get());
        }
        if(paxGrp.updInfantAddress()) {
            ASSERT(paxGrp.infant());
            updater.updatePaxAddresses(paxGrp.infant().get());
        }
        if(paxGrp.updInfantVisa()) {
            ASSERT(paxGrp.infant());
            updater.updatePaxVisa(paxGrp.infant().get());
        }
    }

    SaveIatciGrp(grpId, reqType, updater.iatciNode(), termReqNode, resNode);
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
    xmlNodePtr segsNode = findNodeR(reqNode, "segments");
    if(!segsNode) return false;

    XmlCheckInTabs tabs(segsNode);
    if(tabs.empty()) return false;
    if(tabs.containsEdiTab()) return true;

    return IatciXmlDb::exists(tabs.tabs().back().seg().m_grpId);
}

bool IatciInterface::InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
    auto ckiParams = getCkiParams(reqNode);
    // пока проверяем без какого-либо условия
    checkCkiParams(ckiParams);
    edifact::SendCkiRequest(ckiParams,
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
        checkCkuParams(*ckuParams);
        edifact::SendCkuRequest(ckuParams.get(),
                                getIatciPult(),
                                getIatciRequestContext(kickInfo),
                                kickInfo);

        return true; /*req was sent*/
    }

    return false; /*req was NOT sent*/
}

void IatciInterface::UpdateSeatRequest(xmlNodePtr reqNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendCkuRequest(getSeatUpdateParams(reqNode),
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

void IatciInterface::ReprintRequest(xmlNodePtr reqNode)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edifact::SendBprRequest(getBprParams(reqNode),
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
                                        const std::list<Result>& lRes)
{
    FuncIn(CheckinKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Cki, ActSavePax);
    FuncOut(CheckinKickHandler);
}

void IatciInterface::UpdateKickHandler(int ctxtId,
                                       xmlNodePtr initialReqNode,
                                       xmlNodePtr resNode,
                                       const std::list<Result>& lRes)
{
    FuncIn(UpdateKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Cku,
                 isReseatReq(initialReqNode) ? ActReseatPax : ActSavePax);
    FuncOut(UpdateKickHandler);
}

void IatciInterface::CancelKickHandler(int ctxtId,
                                       xmlNodePtr initialReqNode,
                                       xmlNodePtr resNode,
                                       const std::list<Result>& lRes)
{
    FuncIn(CancelKickHandler);    
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Ckx, ActSavePax);
    FuncOut(CancelKickHandler);
}

void IatciInterface::ReprintKickHandler(int ctxtId,
                                        xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode,
                                        const std::list<Result>& lRes)
{
    FuncIn(ReprintKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Bpr, ActReprint);
    FuncOut(ReprintKickHandler);
}

void IatciInterface::PasslistKickHandler(int ctxtId,
                                         xmlNodePtr initialReqNode,
                                         xmlNodePtr resNode,
                                         const std::list<Result>& lRes)
{
    FuncIn(PasslistKickHandler);
    DoKickAction(ctxtId, initialReqNode, resNode, lRes, Plf, ActLoadPax);
    FuncOut(PasslistKickHandler);
}

void IatciInterface::SeatmapKickHandler(int ctxtId,
                                        xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode,
                                        const std::list<Result>& lRes)
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
                                                    const std::list<Result>& lRes)
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
            std::list<Result> lRes = iatci::loadCkiData(remRes->ediSession());
            if(remRes->status() == RemoteStatus::Success) {
                KickHandler_onSuccess(reqCtxtId, termReqCtxt.node(), resNode, lRes);
            } else {
                const Ticketing::AstraMsg_t RemDcsErr = "MSG.ERROR_IN_REMOTE_DCS";
                Ticketing::AstraMsg_t errCode = getInnerErrByErd(remRes->ediErrCode(),
                                                                 RemDcsErr);
                if(!remRes->remark().empty()) {
                    AstraLocale::showProgError(remRes->remark());
                } else {
                    if(!remRes->ediErrCode().empty()) {
                        AstraLocale::showProgError(errCode);
                    } else {
                        AstraLocale::showProgError(RemDcsErr);
                    }
                }
                KickHandler_onFailure(reqCtxtId, termReqCtxt.node(), resNode, lRes, errCode);
            }
        }
    }
    FuncOut(KickHandler);
}

void IatciInterface::FallbackMessage(xmlNodePtr initialReqNode)
{
    if(isCheckinRequest(initialReqNode)) 
    {
        RequestType reqType = ClassifyCheckInRequest(initialReqNode);
        switch(reqType) 
        {
        case Cki:
        {
            CkiParams ckiParams = getCkiParams(initialReqNode);
            IfmMessage ifm(IfmFlights(ckiParams.fltGroup().outboundFlight(),
                                      ckiParams.fltGroup().inboundFlight()),
                           IfmAction(IfmAction::Del),
                           IfmPaxes(ckiParams.fltGroup().paxGroups()));
            ifm.send();    
        }
        break;

        case Cku:
        {
            ; // TODO
        }   
        break;

        case Ckx:
        {
            CkxParams ckxParams = getCkxParams(initialReqNode);
            IfmMessage ifm(IfmFlights(ckxParams.fltGroup().outboundFlight(),
                                      ckxParams.fltGroup().inboundFlight()),
                           IfmAction(IfmAction::Del),
                           IfmPaxes(ckxParams.fltGroup().paxGroups()));
            ifm.send();                            
        } 
        break;

        default:
            ;
        }
    }
}

void IatciInterface::KickHandler_onTimeout(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode)
{
    FuncIn(KickHandler_onTimeout);
    AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR");
    RollbackChangeOfStatus(initialReqNode, ctxtId); // откат ранее смененного статуса в СЭБ(если менялся)
    FallbackMessage(initialReqNode); // send IFM
    FuncOut(KickHandler_onTimeout);
}

void IatciInterface::KickHandler_onSuccess(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode,
                                           const std::list<Result>& lRes)
{
    FuncIn(KickHandler_onSuccess);
    ASSERT(!lRes.empty());
    switch(lRes.front().action())
    {
    case Result::Checkin:
        CheckinKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::Cancel:
        CancelKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::Update:
        UpdateKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::Reprint:
        ReprintKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::Passlist:
        PasslistKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::Seatmap:
        SeatmapKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    case Result::SeatmapForPassenger:
        SeatmapForPassengerKickHandler(ctxtId, initialReqNode, resNode, lRes);
        break;
    }
    FuncOut(KickHandler_onSuccess);
}

void IatciInterface::KickHandler_onFailure(int ctxtId,
                                           xmlNodePtr initialReqNode,
                                           xmlNodePtr resNode,
                                           const std::list<Result>& lRes,
                                           const Ticketing::AstraMsg_t& errCode)
{
    FuncIn(KickHandler_onFailure);
    ReqParams(initialReqNode).setBoolParam("after_kick", true);
    ASSERT(!lRes.empty());
    switch(lRes.front().action())
    {
    case Result::Checkin:
    {
        RollbackChangeOfStatus(initialReqNode, ctxtId);
    }
    break;
    
    case Result::Cancel:
    {
        RollbackChangeOfStatus(initialReqNode, ctxtId);
        FallbackMessage(initialReqNode);
    }
    break;

    case Result::Update:
    {
        FallbackMessage(initialReqNode);
    }
    break;

    case Result::Passlist:
    {
        specialPlfErrorHandler(initialReqNode, errCode);
        CheckInInterface::LoadPax(initialReqNode, resNode);
    }
    break;

    case Result::Reprint:
    case Result::Seatmap:
    case Result::SeatmapForPassenger:
    {
        // ; NOP
    }
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

static std::list<Ticketing::TicketNum_t> getTickNumsOnFirstSeg(xmlNodePtr node)
{
    std::list<Ticketing::TicketNum_t> tickNums;
    XmlCheckInTabs tabs(findNodeR(node, "segments"));
    ASSERT(tabs.size() > 0);
    const XmlCheckInTab& firstTab = tabs.tabs().front();
    std::list<astra_entities::PaxInfo> lPax = firstTab.lPax();
    for(const auto& pax: lPax) {
        tickNums.push_back(pax.tickNum());
    }
    return tickNums;
}

static IatciViewXmlParams getIatciViewXmlParams(xmlNodePtr node)
{
    std::list<Ticketing::TicketNum_t> tickNumOrder = getTickNumsOnFirstSeg(node);
    return IatciViewXmlParams(tickNumOrder);
}

static IatciViewXmlParams getIatciViewXmlParams(int grpId)
{
    XMLDoc oldXml = ASTRA::createXmlDoc(iatci::IatciXmlDb::load(grpId));
    return getIatciViewXmlParams(oldXml.docPtr()->children);
}

static void normalizeIatciPaxDocs(xmlNodePtr iatciResNode,
                                  xmlNodePtr reqNode,
                                  const std::list<Result>& lRes)
{
    IatciUpdater updateByOwnSeg(iatciResNode, "segments", reqNode, "segments");
    IatciUpdater updateByIatciSeg(iatciResNode, "segments", reqNode, "iatci_segments");
    for(const Result& res: lRes) {
        for(const iatci::dcrcka::PaxGroup& pxg: res.paxGroups()) {
            if(!pxg.doc()) {
                if(!updateByIatciSeg.tryUpdatePaxDoc(pxg.pax())) {
                    updateByOwnSeg.updatePaxDoc(pxg.pax());
                }
                if(!updateByIatciSeg.tryUpdatePaxVisa(pxg.pax())) {
                    updateByOwnSeg.updatePaxVisa(pxg.pax());
                }
                if(!updateByIatciSeg.tryUpdatePaxAddresses(pxg.pax())) {
                    updateByOwnSeg.updatePaxAddresses(pxg.pax());
                }
            }
        }
    }
}

void IatciInterface::DoKickAction(int ctxtId,
                                  xmlNodePtr reqNode,
                                  xmlNodePtr resNode,
                                  const std::list<Result>& lRes,
                                  RequestType reqType,
                                  KickAction act)
{
    FuncIn(DoKickAction);
    ReqParams(reqNode).setBoolParam("after_kick", true);        

    XMLDoc iatciResCtxt = ASTRA::createXmlDoc("<iatci_result/>");
    xmlNodePtr iatciResNode = NodeAsNode(std::string("/iatci_result").c_str(), iatciResCtxt.docPtr());
    xmlNodePtr segmentsNode = newChild(iatciResNode, "segments");
    newChild(iatciResNode, "segments_for_log");

    EdiResCtxtWrapper ediResCtxt(ctxtId, iatciResNode, "context", __FUNCTION__);
    xmlNodePtr logSegmentsNode = findNodeR(ediResCtxt.node(), "segments_for_log");
    ASSERT(logSegmentsNode);

    switch(act)
    {
    case ActSavePax:
    {
        if(reqType == Cki) {
            // сохраняем edifact-сегменты для записи в журнал.
            iatci::iatci2xml(logSegmentsNode, lRes, IatciViewXmlParams());
        }

        ASSERT(CheckInInterface::SavePax(reqNode, ediResCtxt.node(), resNode));
        int grpId = getGrpId(reqNode, resNode, reqType);

        if(reqType == Ckx) {
            // ответ на отмену, не содержащий данных, дополним информацией о пассажирах
            CopyNode(segmentsNode, findNodeR(reqNode, "iatci_segments"));
        } else if(reqType == Cki) {
            iatci::iatci2xml(segmentsNode, lRes, getIatciViewXmlParams(resNode));
        }

        if(reqType == Cku) {
            UpdateIatciGrp(grpId, reqType, iatciResNode, reqNode, resNode);
        } else {
            SaveIatciGrp(grpId, reqType, iatciResNode, reqNode, resNode);
        }
        CheckInInterface::LoadIatciPax(NULL, resNode, grpId, false);
    }
    break;

    case ActReseatPax:
    {
        ASSERT(lRes.size() == 1);
        ASSERT(!lRes.front().paxGroups().empty());
        ASSERT(lRes.front().paxGroups().front().seat());
        const std::string requestedSeat = getSeatUpdate(reqNode).seat();
        const std::string confirmedSeat = lRes.front().paxGroups().front().seat()->seat();
        if(requestedSeat != confirmedSeat) {
            LogError(STDLOG) << "Warning: remote DCS confirmed a place other than the requested! "
                             << "Requested: " << requestedSeat << "; "
                             << "Confirmed: " << confirmedSeat;
        }
        int grpId = getGrpId(reqNode, resNode, reqType);
        ReqParams(reqNode).setStrParam("old_seat", getOldSeat(reqNode));        
        UpdateIatciGrp(grpId, reqType, iatciResNode, reqNode, resNode);
        SeatmapRequest(reqNode); // перепосылаем карту мест, чтобы сформировать ответ на смену места
    }
    break;

    case ActLoadPax:
    {
        int grpId = getGrpId(reqNode, resNode, reqType);
        iatci::iatci2xml(segmentsNode, lRes, getIatciViewXmlParams(grpId));
        SaveIatciGrp(grpId, reqType, iatciResNode, reqNode, resNode);
        CheckInInterface::LoadPax(reqNode, resNode);
    }
    break;

    case ActReprint:
    {
        int grpId = getGrpId(reqNode, resNode, reqType);
        iatci::iatci2xml(segmentsNode, lRes, getIatciViewXmlParams(grpId));
        SaveIatciGrp(grpId, reqType, iatciResNode, reqNode, resNode);
        PrintInterface::GetPrintDataBP(reqNode, resNode);
    }
    break;

    default:
        throw EXCEPTIONS::Exception("Can't be here!");
    }

    FuncOut(DoKickAction);
}

void IatciInterface::RollbackChangeOfStatus(xmlNodePtr initialReqNode, int ctxtId)
{
    if(!isCheckinRequest(initialReqNode)) {
        return;
    }
    RequestType reqType = ClassifyCheckInRequest(initialReqNode);
    if(reqType == Cki || reqType == Ckx)
    {
        EdiResCtxtWrapper ediResCtxt(ctxtId, __FUNCTION__);
        ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(), false);
    }
}
