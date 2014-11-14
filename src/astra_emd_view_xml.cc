#include "astra_emd_view_xml.h"

#include <serverlib/dates_io.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing{
namespace TickView{

EmdXmlView::EmdXmlView(xmlNodePtr node, const Emd& emd)
    : m_viewData(node)
{
    m_emd.reset(new Emd(emd));
}

void EmdXmlView::viewOrigOfRequest() const
{
    Ticketing::TickView::OrigOfRequestXmlView orgXmlView;
    orgXmlView(m_viewData, m_emd->org());
}

void EmdXmlView::viewResContrInfo() const
{
    Ticketing::TickView::ResContrInfoXmlView rciXmlView;
    rciXmlView(m_viewData, m_emd->rci());
}

void EmdXmlView::viewPassenger() const
{
    Ticketing::TickView::PassengerXmlView passXmlView;
    passXmlView(m_viewData, m_emd->pass());
}

void EmdXmlView::viewFormsOfId() const
{
    Ticketing::TickView::FormOfIdXmlView foidXmlView;
    foidXmlView(m_viewData, m_emd->lFoid());
}

void EmdXmlView::viewTaxDetails() const
{
    Ticketing::TickView::TaxDetailsXmlView txdXmlView;
    txdXmlView(m_viewData, m_emd->lTax());
}

void EmdXmlView::viewFreeTextInfo() const
{
    Ticketing::TickView::FreeTextInfoXmlView iftXmlView;
    iftXmlView(m_viewData, m_emd->lIft());
}

void EmdXmlView::viewFormsOfPayment() const
{
    Ticketing::TickView::FormOfPaymentXmlView fopXmlView;
    fopXmlView(m_viewData, m_emd->lFop());
}

void EmdXmlView::viewMonetaryInfo() const
{
    Ticketing::TickView::MonetaryInfoXmlView monXmlView;
    monXmlView(m_viewData, m_emd->lMon());
}

void EmdXmlView::viewType() const
{
    xmlNodePtr node = m_viewData.getNode();
    xmlNewTextChild(node, NULL, "emd_type",
                    m_emd->type() == DocType::EmdA ? "A" : "S");
}

void EmdXmlView::viewRfic() const
{
    xmlNodePtr node = m_viewData.getNode();
    xmlNewTextChild(node, NULL, "rfic", m_emd->rfic()->code());
}

void EmdXmlView::viewEmdTickets() const
{
    xmlNodePtr mainNode = m_viewData.getNode();

    std::list<EmdTicket> lTick = m_emd->lTicket();

    int count = 0;
    for(std::list<EmdTicket>::const_iterator i = lTick.begin(); i != lTick.end(); ++i)
    {
        const EmdTicket& emdTick = (*i);

        std::string tmp1 = std::string("emd") + HelpCpp::string_cast(++count),
                    tmp2 = std::string("emd_inconnection") + HelpCpp::string_cast(count);
        xmlNodePtr emdTickNode = newChild(mainNode, tmp1.c_str());

        xmlNewTextChild(emdTickNode, NULL, "emd_num", emdTick.tickNum()); // номер билета EMD

        boost::optional<TicketNum_t> tickNumConnect;

        m_viewData.setCurrEmdNode(emdTickNode);
        viewEmdTicketCoupons(emdTick.lCpn());
    }
}

void EmdXmlView::viewEmdTicketCoupons(const std::list<EmdCoupon>& lCpn) const
{
    xmlNodePtr emdTickNode = m_viewData.getCurrEmdNode();
    if(!emdTickNode)
    {
        ProgError(STDLOG, "getCurrEmdNode returned NULL!");
        return;
    }

    xmlNodePtr coupNode = newChild(emdTickNode, "coupon");
    xmlSetProp(coupNode, "refresh", "true");

    int count = 0;
    for(std::list<EmdCoupon>::const_iterator i = lCpn.begin(); i != lCpn.end(); ++i)
    {
        const EmdCoupon &cpn = (*i);

        xmlNodePtr rowNode = newChild(coupNode, "row");
        xmlSetProp(rowNode, "index", count++);

        unsigned colNum = 0;
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "num", cpn.num()),
                   "index", colNum++);

        if(cpn.associatedNum()) {
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "associated_num", cpn.associatedNum()),
                       "index", colNum++);
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "associated_doc_num", cpn.associatedTickNum()),
                       "index", colNum++);
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "association_status", cpn.associated() ? "702" : "703"),
                       "index", colNum++);
        }

        if(cpn.haveItin()) {
            xmlSetProp(xmlNewTextChild(rowNode,NULL,"dep_date",
                   cpn.itin().date1().is_special()?"------":
                           HelpCpp::string_cast(cpn.itin().date1(), "%d%m%y",ENGLISH)),
                                                "index",colNum++);
        } else {
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "dep_date", " "),
                       "index",colNum++);
        }

        if(cpn.haveItin()) {
            xmlSetProp(xmlNewTextChild(rowNode,NULL,"dep_time",
                   cpn.itin().time1().is_special()?"----":
                           HelpCpp::string_cast(cpn.itin().time1(), "%H%M")),"index",colNum++);
        } else {
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "dep_time", " "),
                       "index",colNum++);
        }

        std::string depPointCode = cpn.haveItin() ? cpn.itin().depPointCode() : " ";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "dep", depPointCode),
                   "index", colNum++);

        std::string arrPointCode = cpn.haveItin() ? cpn.itin().arrPointCode() : " ";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "arr", arrPointCode),
                   "index", colNum++);

        std::string airlCode = cpn.haveItin() ? cpn.itin().airCode() : " ";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "codea", airlCode),
                   "index", colNum++);

        if(cpn.haveItin() && cpn.itin().flightnum()) {
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "flight", cpn.itin().flightnum()),
                       "index", colNum++);
        } else {
            xmlSetProp(xmlNewTextChild(rowNode, NULL, "flight", " "),
                       "index",colNum++);
        }

        std::string amountStr = cpn.amount() ? cpn.amount()->amStr() : " ";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "amount", amountStr),
                   "index", colNum++);

        std::string rfisc = cpn.rfisc() ? cpn.rfisc()->rfisc() : "---";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "rfisc_code", rfisc),
                   "index", colNum++);

        std::string rfiscDesc = cpn.rfisc() ? cpn.rfisc()->description() : " ";
        xmlSetProp(xmlNewTextChild(rowNode, NULL, "rfisc_desc", rfiscDesc),
                   "index", colNum++);

        xmlSetProp(xmlNewTextChild(rowNode, NULL, "sac", cpn.couponInfo().sac()),
                   "index", colNum++);

        xmlSetProp(xmlNewTextChild(rowNode, NULL, "coup_status", cpn.couponInfo().status()->dispCode()),
                   "index", colNum++);
    }
}

}//namespace TickView
}//namespace Ticketing
