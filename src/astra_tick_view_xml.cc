#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <string>
#include <sstream>
#include <libxml/tree.h>
#include "astra_ticket.h"
#include "astra_tick_view_xml.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/gettext.h"
#include "jxtlib/xml_tools.h"
#include "serverlib/date_cast.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "serverlib/test.h"

namespace Ticketing{
namespace TickView{

using namespace std;
// using namespace OciCpp;
//using namespace BaseTables;

/* Pnr data: single */
void ResContrInfoXmlView::operator()(ViewerData &Data, const ResContrInfo &Rci) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  xmlNodePtr reclocNode=newChild(mainNode,"recloc");
  xmlNewTextChild(reclocNode,NULL,"awk",Rci.ourAirlineCode());
  xmlNewTextChild(reclocNode,NULL,"regnum",Rci.airRecloc());

  xmlNodePtr origNode=newChild(VData.getNode(),"origin");
  xmlNewTextChild(origNode,NULL,"date_of_issue",
                  HelpCpp::string_cast(Rci.dateOfIssue(), "%d%b%y",
                                       (Language)currLang()));
}

/* Pnr data: list */
void ResContrInfoXmlListView::operator()(ViewerData &Data, const ResContrInfo &Rci) const
{
  XmlViewDataList &VData=dynamic_cast<XmlViewDataList &>(Data);
  xmlNodePtr rowNode=VData.getRowNode();

  xmlSetProp(xmlNewTextChild(rowNode,NULL,"regnum",Rci.airRecloc()),"index","1");
  xmlSetProp(xmlNewTextChild(rowNode,NULL,"int_regnum",Rci.ourRecloc()),"index","2");
}

/* Request origin data: single */
void OrigOfRequestXmlView::operator()(ViewerData &Data, const OrigOfRequest &Org) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  xmlNodePtr origNode=getNode(mainNode,"origin");
  string tmp = Org.locationCode();
  if(Org.airlineCode().size()) // ��������, �� ���ன ��襫 ����� (e.g., DT)
  {
    if(!tmp.empty())
      tmp+=" ";
    tmp+=Org.airlineCode();
  }
  xmlNewTextChild(origNode,NULL,"sys_addr",tmp);
  if(Org.agencyCode().size()) // ��� + ������⢮
  {
    xmlNewTextChild(origNode,NULL,"ppr",Org.pprNumber());
    xmlNewTextChild(origNode,NULL,"agn",Org.agencyCode());
  }
  else
  {
    newChild(origNode,"ppr");
    newChild(origNode,"agn");
  }
  if(Org.originLocationCode().size()) // flpoint ������, ��ࠢ��襣� �����
    xmlNewTextChild(origNode,NULL,"opr_flpoint",
                    Org.originLocationCode());
  else
    newChild(origNode,"opr_flpoint");
  //xmlNewTextChild(origNode,NULL,"type",Org.type()); // ��-� ��� ��� edifact
  // ��࠭� �� �।�⮨� ��������
  xmlNewTextChild(origNode,NULL,"authcode",Org.authCode()); // ����� ������
  xmlNewTextChild(origNode,NULL,"pult",Org.pult()); // ����
}

/* Request origin data: list */
void OrigOfRequestXmlListView::operator()(ViewerData &Data, const OrigOfRequest &Org) const
{
  return;
}

/* Form of payment data: single*/
void FormOfPaymentXmlView::operator ()(ViewerData &Data, const list<FormOfPayment> &lfop) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  string tmp;

  for(list<FormOfPayment>::const_iterator i=lfop.begin(); i!=lfop.end(); i++)
  {
    const FormOfPayment &Fop = (*i);
    if(!tmp.empty())
      tmp+=" ";
    tmp+=Fop.amValue().amStr() + " " + Fop.fopCode();

    if(Fop.vendor().size())
    {
      tmp+=string(" (")+Fop.vendor()+" "+
            Fop.accountNumMask()+
            (Fop.expDate().is_special()?"":
                ("/"+HelpCpp::string_cast(Fop.expDate(), "%m%y")))+")";
    }
    else
    {
    }
  }
  xmlNodePtr paymentNode=getNode(mainNode,"payment");
  xmlNewTextChild(paymentNode,NULL,"payment",tmp);
}

void FreeTextInfoXmlView::operator () (ViewerData &Data, const list<FreeTextInfo> &lift) const
{
    XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
    xmlNodePtr mainNode=VData.getNode();

    for(list<FreeTextInfo>::const_iterator i=lift.begin(); i!=lift.end(); i++)
    {
    	const FreeTextInfo &Ift = (*i);
    	if(Ift.fTType()->type() == FreeTextType::FareCalc) {
    	    // ��ப� ����� ���
    	    ProgTrace(TRACE3, "Ift fare calc xml view");
    	    xmlNodePtr paymentNode=getNode(mainNode,"payment");
    	    xmlNewTextChild(paymentNode,NULL,"fare_calc", Ift.fullText());
    	} else if(Ift.fTType()->type() == FreeTextType::AgnAirName){
    	    //������ ��� ��த�/�����⢠
    	    xmlNodePtr origNode=getNode(mainNode,"origin");
    	    xmlNewTextChild(origNode,NULL,"city_name", Ift.text(0));
    	    if(Ift.text(0).size() < 25) {
      	    	//�᫨ �������� ����, 祬 �⢥���� ���� � ����䥩�
          		setElemProp(mainNode->parent, "city_name" ,"col",
    			    HelpCpp::string_cast(Ift.text(0).size()).c_str());
    	    }
    	    std::string agn_name;
    	    if(Ift.numParts() > 1){
    	       agn_name = Ift.text(1);
    	    }
    	    xmlNewTextChild(origNode,NULL,"agency_name", agn_name);
      }
    }
}

/* Form of payment data: list */
void FormOfPaymentXmlListView::operator ()(ViewerData &Data, const list<FormOfPayment> &lfop) const
{
  return;
}

/* Fare data: single */
void MonetaryInfoXmlView::operator () (ViewerData &Data, const list<MonetaryInfo> &lmon) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  string fare, total;
  for(list<MonetaryInfo>::const_iterator i=lmon.begin(); i!= lmon.end(); ++i)
  {
    const MonetaryInfo &Mon = (*i);
    if(Mon.code()->codeInt() == AmountCode::Total) // �ᥣ�
    {
      if(!total.empty())
        total+=" ";
      total+=Mon.amValue().amStr()+Mon.currCode();
    }
    else // ��㣨� ����
    {
      if(!fare.empty())
        fare+=" ";
      fare+=Mon.amValue().amStr() + Mon.currCode();
    }
  }
  xmlNodePtr paymentNode=getNode(mainNode,"payment");
  xmlNewTextChild(paymentNode,NULL,"fare",fare);
  xmlNewTextChild(paymentNode,NULL,"total",total);
}

/* Fare data: list */
void MonetaryInfoXmlListView::operator () (ViewerData &Data, const list<MonetaryInfo> &lmon) const
{
  return;
  XmlViewDataList &VData=dynamic_cast<XmlViewDataList &>(Data);
  xmlNodePtr rowNode=VData.getRowNode();

  string total;
  for(list<MonetaryInfo>::const_iterator i=lmon.begin(); i!= lmon.end(); ++i)
  {
    const MonetaryInfo &Mon = (*i);
    if(Mon.code()->codeInt() == AmountCode::Total) // �ᥣ�
    {
      if(!total.empty())
        total+=" ";
      total+=Mon.amValue().amStr()+Mon.currCode();
    }
  }

  xmlSetProp(xmlNewTextChild(rowNode,NULL,"total",total),"index","4");
}

/* Taxes data: single */
void TaxDetailsXmlView::operator () (ViewerData &Data, const list<TaxDetails> &ltxd) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  string tmp;
  for(list<TaxDetails>::const_iterator i=ltxd.begin(); i!= ltxd.end(); i++)
  {
    const TaxDetails &Tax = (*i);
    if(!tmp.empty())
      tmp+=" ";
    tmp+=Tax.type()+Tax.amValue().amStr();
  }
  xmlNodePtr paymentNode=getNode(mainNode,"payment");
  xmlNewTextChild(paymentNode,NULL,"tax",tmp);
}

/* Taxes data: list */
void TaxDetailsXmlListView::operator () (ViewerData &Data, const list<TaxDetails> &ltxd) const
{
  return;
}

/* Passenger data: single */
void PassengerXmlView::operator () (ViewerData &Data, const Passenger &Pass) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  xmlNodePtr passNode=newChild(mainNode,"passenger");

  xmlNewTextChild(passNode,NULL,"surname",Pass.surname()); // 䠬���� ���ᠦ��
  xmlNewTextChild(passNode,NULL,"kkp",Pass.typeCode()); // ⨯ (��⥣���) ���ᠦ��
  if(Pass.age())
    xmlNewTextChild(passNode,NULL,"age",Pass.age()); // ������ ���ᠦ��
  else
    newChild(passNode,"age");
  xmlNewTextChild(passNode,NULL,"name",Pass.name()); // ��� ���ᠦ��
}

void FormOfIdXmlView::operator ( )(ViewerData &Data, const std::list<FormOfId> &lFoid) const
{
    XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
    xmlNodePtr mainNode=VData.getNode();

    std::string FOID_str;
    bool first=true;

    for(list<FormOfId>::const_iterator iter=lFoid.begin();
        iter!=lFoid.end(); iter ++)
    {
        if(!first)
            FOID_str += "; ";
        FOID_str += iter->type()->code();
        FOID_str += "/";
        FOID_str += iter->number();
        FOID_str += "/";
        FOID_str += iter->owner();
        first=false;
    }

    xmlNewTextChild(mainNode,NULL,"foid", FOID_str);
    if(!lFoid.empty())
    {
        setElemProp(mainNode->parent, "foid", "tip",
                    lFoid.front().type()->description((Language)currLang()));
    }
}

/* Passenger data: list */
void PassengerXmlListView::operator () (ViewerData &Data, const Passenger &Pass) const
{
  XmlViewDataList &VData=dynamic_cast<XmlViewDataList &>(Data);
  xmlNodePtr rowNode=VData.getRowNode();

  xmlSetProp(xmlNewTextChild(rowNode,NULL,"surname",Pass.surname()),"index","3");
  xmlSetProp(xmlNewTextChild(rowNode,NULL,"name",Pass.name()),"index","4");
}

/* Ticket data: single */
void TicketXmlView::operator ()
	(ViewerData &Data, const list<Ticket> &lTick, const CouponViewer &cpnView) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  xmlNodePtr mainNode=VData.getNode();

  int count=0;
  int t=0;
  for(list<Ticket>::const_iterator i=lTick.begin(); i!=lTick.end();++i,++t)
  {
    const Ticket &Tick = (*i);
    if(i->actCode() == TickStatAction::oldtick)
    {
        continue;
    }
    string tmp=string("ticket")+HelpCpp::string_cast(++count);
    xmlNodePtr tickNode=newChild(mainNode,tmp.c_str());
    setElemProp(mainNode->parent,(string("gr_")+tmp+"_data").c_str(),"visible","true");

    xmlNewTextChild(tickNode,NULL,"tick_num",Tick.ticknum()); // ����� �����

    VData.setCurrTickNode(tickNode);
    cpnView(Data, Tick.getCoupon()); //������ �㯮�� // ��� ���� !!!
  }
  for(count++;count<5;++count)
  {
    setElemProp(mainNode->parent,
      (string("gr_ticket")+HelpCpp::string_cast(count)+"_data").c_str(),"visible","false");
  }
}

namespace
{
string getTickNums(const list<Ticket> &lTick)
{
  string res;
  if(!lTick.empty())
  {
    res=lTick.begin()->ticknum();
    if(lTick.size()>1)
    {
      string last_tick=lTick.rbegin()->ticknum();
      res+=string("-")+last_tick.substr(last_tick.size()-3,3);
    }
  }
  return res;
}

};
/* Ticket data: list */
void TicketXmlListView::operator ()
	(ViewerData &Data, const list<Ticket> &lTick, const CouponViewer &cpnView) const
{
  XmlViewDataList &VData=dynamic_cast<XmlViewDataList &>(Data);
  xmlNodePtr rowNode=VData.getRowNode();

  string tick_nums=getTickNums(lTick);
  xmlSetProp(xmlNewTextChild(rowNode,NULL,"tick",tick_nums),"index","0");
}

/* Coupons data: single */
void CouponXmlView::operator () (ViewerData &Data, const list<Coupon> &lcpn) const
{
  XmlViewData &VData=dynamic_cast<XmlViewData &>(Data);
  //xmlNodePtr tickNode=VData.getNode();
  xmlNodePtr tickNode=VData.getCurrTickNode();
  if(!tickNode)
  {
    ProgError(STDLOG,"getCurrTickNode returned NULL!");
    return;
  }

  xmlNodePtr coupNode=newChild(tickNode,"coupon");
  xmlSetProp(coupNode,"refresh","true");

  int count=0;
  for(list<Coupon>::const_iterator i=lcpn.begin();i!=lcpn.end();++i)
  {
    const Coupon &cpn = (*i);
    const Itin &itin = cpn.itin();
    xmlNodePtr rowNode=newChild(coupNode,"row");
    xmlSetProp(rowNode,"index",count++);

    int col_num=0;
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"num",i->couponInfo().num()),"index",col_num++); // ����� ᥣ����

    // ��� �뫥�
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"dep_date",
               itin.date1().is_special()?ItinStatus::Open:
               HelpCpp::string_cast(itin.date1(), "%d%m%y",ENGLISH)),
                "index",col_num++);
    // �६� �뫥�
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"dep_time",
               itin.time1().is_special()?"----":
               HelpCpp::string_cast(itin.time1(), "%H%M")),"index",col_num++);

    xmlSetProp(xmlNewTextChild(rowNode,NULL,"dep",
               itin.depPointCode()),"index",col_num++); // ��㤠
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"arr",
               itin.arrPointCode()),"index",col_num++); // �㤠
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"codea",
               itin.airCode()),"index",col_num++); // ��������
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"codea_oper",
               itin.airCodeOper().empty()?
                  itin.airCode():itin.airCodeOper()),
                               "index",col_num++); // 䠪��᪨� ��ॢ��稪
    // ����� ३�
    if(itin.flightnum()){
        xmlSetProp(xmlNewTextChild(rowNode,NULL,"flight",
                   itin.flightnum()),"index",col_num++);
        xmlSetProp(xmlNewTextChild(rowNode,NULL,"flight_oper", // 䠪��᪨� ३�
                   (!itin.flightnumOper())?
                     itin.flightnum():itin.flightnumOper()),
                                     "index",col_num++);
    } else {
        xmlSetProp(xmlNewTextChild(rowNode,NULL,"flight",
                   ItinStatus::Open),"index",col_num++);
        xmlSetProp(xmlNewTextChild(rowNode,NULL,"flight_oper",
                   ItinStatus::Open),"index",col_num++);
    }

    xmlSetProp(xmlNewTextChild(rowNode,NULL,"cls",
               itin.classCodeStr((Language)currLang())),"index",col_num++); // ����� �஭�஢����
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"seg_status",
               itin.rpistat()->code()),"index",col_num++); // ����� ᥣ����
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"fare_basis",itin.fareBasis()),"index",col_num++); // ����

    xmlSetProp(xmlNewTextChild(rowNode,NULL,"valid_before",
           (itin.validDates().first.is_special()?
                   "":HelpCpp::string_cast(itin.validDates().first, "%d%b%y",
                                           (Language)currLang()))),
                "index",col_num++); // ����
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"valid_after",
           (itin.validDates().second.is_special()?
                   "":HelpCpp::string_cast(itin.validDates().second, "%d%b%y", (Language)currLang()))),
		       "index",col_num++); // ����

    xmlSetProp(xmlNewTextChild(rowNode,NULL,"sac",cpn.couponInfo().sac()),"index",col_num++); // ��� ���ਧ�樨 (Settlement)

    ostringstream ebd;
    if(itin.luggage().haveLuggage()){
        ebd<<itin.luggage()->quantity()<<AstraLocale::getLocaleText(itin.luggage()->code());
    } else {
        ebd<<AstraLocale::getLocaleText("���");
    }
    xmlSetProp(xmlNewTextChild(rowNode,NULL,"lugg_norm",ebd.str()),"index",col_num++); // ��ଠ ������

    xmlSetProp(xmlNewTextChild(rowNode,NULL,"coup_status",cpn.couponInfo().status()->dispCode()),"index",col_num++); // ����� �㯮��
  }
}

/* Coupons data: list */
void CouponXmlListView::operator () (ViewerData &Data, const list<Coupon> &lcpn) const
{
  return;
}

} //namespace TickView
} //namespace Ticketing
