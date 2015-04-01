#include "astra_emd_view_xml.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "astra_locale.h"
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

using namespace AstraLocale;

string EmdXmlViewToText(const Emd &emd)
{
  XMLDoc doc("emd");
  xmlNodePtr node=NodeAsNode("/emd", doc.docPtr());
  EmdDisp::doDisplay(EmdXmlView(node, emd));
  ostringstream res;
  xmlNodePtr node2;
  //PNR
  node2=NodeAsNode("recloc",node)->children;
  res << "PNR" << ": "
      << NodeAsStringFast("regnum",node2) << "/" << NodeAsStringFast("awk",node2) << endl;
  //Продажа
  node2=NodeAsNode("origin",node)->children;
  res << getLocaleText("Продажа") << ": " << endl
      << "  " << getLocaleText("Дата") << ": " << NodeAsStringFast("date_of_issue",node2) << endl
      << "  " << getLocaleText("Адрес системы") << ": " << NodeAsStringFast("sys_addr",node2) << endl
      << "  " << getLocaleText("Агентство") << ": " << NodeAsStringFast("agn",node2) << " " << NodeAsStringFast("agency_name",node2,"") << endl
      << "  " << getLocaleText("Пункт продажи") << ": " << NodeAsStringFast("ppr",node2) << endl
      << "  " << getLocaleText("Город") << ": " << NodeAsStringFast("opr_flpoint",node2) << " " << NodeAsStringFast("city_name",node2,"") << endl
      << "  " << getLocaleText("Оператор") << ": " << NodeAsStringFast("authcode",node2) << endl
      << "  " << getLocaleText("Пульт") << ": " << NodeAsStringFast("pult",node2) << endl;
  //Пассажир
  node2=NodeAsNode("passenger",node)->children;
  res << getLocaleText("Пассажир") << ": " << endl
      << "  " << getLocaleText("Фамилия") << ": " << NodeAsStringFast("surname",node2) << endl
      << "  " << getLocaleText("Имя") << ": " << NodeAsStringFast("name",node2) << endl
      << "  " << getLocaleText("Категория") << ": " << NodeAsStringFast("kkp",node2) << endl
      << "  " << getLocaleText("Возр.") << ": " << NodeAsStringFast("age",node2) << endl;

  res << getLocaleText("Тип EMD") << ": " << NodeAsString("emd_type",node) << endl
      << "RFIC" << ": " << NodeAsString("rfic",node) << endl
      << getLocaleText("Купоны") << ": " << endl;

  const char* titles[] =
  {
    "№ куп.",
    "Дата",
    "Время",
    "Отпр.",
    "Назн.",
    "А/к",
    "Рейс",
    "Сумма",
    "RFISC",
    "Название услуги",
    "СтКуп.",
    "Ассоц.",
    "СтАссоц."
  };

  map<int, list<string> > coupons;
  map<int, list<string> >::iterator i=coupons.insert(make_pair(0, list<string>())).first;
  if (i==coupons.end()) throw EXCEPTIONS::Exception("%s: i==coupons.end()", __FUNCTION__);
  for(unsigned t=0; t<sizeof(titles)/sizeof(titles[0]); t++)
    i->second.push_back(getLocaleText(titles[t]));
  list<size_t> widths;
  for(list<string>::const_iterator f=i->second.begin(); f!=i->second.end(); ++f)
    widths.push_back(f->size());

  for(xmlNodePtr cNode=NodeAsNode("emd1/coupon", node)->children; cNode!=NULL; cNode=cNode->next)
  {
    node2=cNode->children;
    int num=NodeAsIntegerFast("num", node2);
    map<int, list<string> >::iterator i=coupons.insert(make_pair(num, list<string>())).first;
    if (i==coupons.end()) throw EXCEPTIONS::Exception("%s: i==coupons.end()", __FUNCTION__);
    i->second.push_back(NodeAsStringFast("num", node2));
    i->second.push_back(NodeAsStringFast("dep_date", node2));
    i->second.push_back(NodeAsStringFast("dep_time", node2));
    i->second.push_back(NodeAsStringFast("dep", node2));
    i->second.push_back(NodeAsStringFast("arr", node2));
    i->second.push_back(NodeAsStringFast("codea", node2));
    i->second.push_back(NodeAsStringFast("flight", node2));
    i->second.push_back(NodeAsStringFast("amount", node2));
    i->second.push_back(NodeAsStringFast("rfisc_code", node2));
    i->second.push_back(NodeAsStringFast("rfisc_desc", node2));
    i->second.push_back(NodeAsStringFast("coup_status", node2));
    string assoc=NodeAsStringFast("associated_doc_num", node2);
    RTrimString(assoc);
    assoc+="/";
    assoc+=NodeAsStringFast("associated_num", node2);
    i->second.push_back(assoc);
    i->second.push_back(NodeAsStringFast("association_status", node2));
  };

  for(map<int, list<string> >::const_iterator r=coupons.begin(); r!=coupons.end(); ++r)
  {
    list<string>::const_iterator f=r->second.begin();
    list<size_t>::iterator w=widths.begin();
    for(; f!=r->second.end() && w!=widths.end(); ++f, ++w)
      if (f->size()>*w) *w=f->size();
  };

  for(map<int, list<string> >::const_iterator r=coupons.begin(); r!=coupons.end(); ++r)
  {
    res << "  ";
    list<string>::const_iterator f=r->second.begin();
    list<size_t>::const_iterator w=widths.begin();
    for(; f!=r->second.end(); ++f)
    {
      if (w!=widths.end())
      {
        res << setw(*w) << left;
        ++w;
      };
      res << *f << " ";
    };
    res << endl;
  };

  //оплата
  node2=NodeAsNode("payment",node)->children;
  res << getLocaleText("Оплата") << ": " << endl
      << "  " << getLocaleText("Тариф") << ": " << NodeAsStringFast("fare",node2) << endl
      << "  " << getLocaleText("Сборы") << ": " << NodeAsStringFast("tax",node2) << endl
      << "  " << getLocaleText("Всего") << ": " << NodeAsStringFast("total",node2) << endl
      << "  " << getLocaleText("Оплата") << ": " << NodeAsStringFast("payment",node2) << endl;

  return res.str();
  //return XMLTreeToText(doc.docPtr());
}

}//namespace TickView
}//namespace Ticketing
