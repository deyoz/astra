#pragma once

#include "astra_emd.h"
#include "astra_tick_view_xml.h"


namespace Ticketing{
namespace TickView{

class EmdXmlViewData: public Ticketing::TickView::XmlViewData
{
public:
    EmdXmlViewData(xmlNodePtr node)
        : XmlViewData(node)
    {}

    void setCurrEmdNode(xmlNodePtr node) { setCurrTickNode(node); }
    xmlNodePtr getCurrEmdNode() { return getCurrTickNode(); }

    virtual ~EmdXmlViewData() {}
};

//-----------------------------------------------------------------------------

class EmdXmlView
{
    friend class EmdDisp;

public:
    EmdXmlView(xmlNodePtr node, const Emd& emd);

    EmdXmlViewData& viewData() const { return m_viewData; }

protected:
    void viewOrigOfRequest() const;
    void viewResContrInfo() const;
    void viewPassenger() const;
    void viewFormsOfId() const;
    void viewTaxDetails() const;
    void viewFreeTextInfo() const;
    void viewFormsOfPayment() const;
    void viewMonetaryInfo() const;
    void viewType() const;
    void viewRfic() const;
    void viewEmdTickets() const;
    void viewEmdTicketCoupons(const std::list<EmdCoupon>& lCpn) const;

private:
    mutable EmdXmlViewData m_viewData;
    boost::shared_ptr<Emd> m_emd;
};

//-----------------------------------------------------------------------------

struct EmdDisp
{
    static void doDisplay(const EmdXmlView& viewer)
    {
        viewer.viewPassenger();
        viewer.viewResContrInfo();
        viewer.viewOrigOfRequest();
        viewer.viewFormsOfId();
        viewer.viewMonetaryInfo();
        viewer.viewFormsOfPayment();
        viewer.viewTaxDetails();
        viewer.viewFreeTextInfo();
        viewer.viewRfic();
        viewer.viewType();
        viewer.viewEmdTickets();
    }
};

}//namespace TickView
}//namespace Ticketing
