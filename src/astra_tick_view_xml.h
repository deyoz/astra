#ifndef _ASTRA_TICK_VIEW_XML_H_
#define _ASTRA_TICK_VIEW_XML_H_

#include <string>
#include <libxml/tree.h>

#include "astra_tick_view.h"
#include "xmllibcpp.h"
#include "xml_tools.h"
#include "jxt_tools.h"

namespace Ticketing{
namespace TickView{
class XmlViewData : public ViewerData
{
  private:
    xmlNodePtr mainNode;
    xmlNodePtr currTickNode;
  public:
    virtual xmlNodePtr getNode()
    {
      return mainNode;
    }
    virtual unsigned short viewItem() const
    {
      return 0;
    }
    virtual bool isSingleTickView() const
    {
      return true;
    }
    void setCurrTickNode(xmlNodePtr node)
    {
      currTickNode=node;
    }
    xmlNodePtr getCurrTickNode()
    {
      return currTickNode;
    }
    XmlViewData(xmlNodePtr node) : mainNode(node),currTickNode(0) {}
};

class XmlViewDataList : public XmlViewData, public ViewerDataList
{
  private:
    int ItemNum;
    int currColumn;
    xmlNodePtr tabNode;
  public:
    virtual unsigned short viewItem() const { return ItemNum; }
    virtual bool isSingleTickView() const { return false; }
    virtual void setViewItemNum(int num) { ItemNum = num; }
    XmlViewDataList(xmlNodePtr node) : XmlViewData(node)
    {
      currColumn=0;
      tabNode=0;
    }
    virtual int getCurrColumn() const
    {
      return currColumn;
    }
    virtual void setCurrColumn(int colnum)
    {
      currColumn=colnum;
    }
    virtual xmlNodePtr getTabNode()
    {
      if(!tabNode)
      {
        tabNode=::getNode(XmlViewData::getNode(),"tab");
        xmlSetProp(tabNode,"refresh","true");
        xmlSetProp(tabNode,"columnData","true");
      }
      return tabNode;
    }
    virtual xmlNodePtr getRowNode()
    {
      xmlNodePtr tabNode=getTabNode();
      xmlNodePtr rowNode=findIChild(tabNode,viewItem());
      if(!rowNode)
      {
        rowNode=newChild(tabNode,"row");
        xmlSetProp(rowNode,"index",viewItem());
      }
      return rowNode;
    }
};

class ResContrInfoXmlView : public ResContrInfoViewer
{
  public:
    virtual void operator() (ViewerData &, const ResContrInfo&) const;
    virtual ~ResContrInfoXmlView(){}
};

class ResContrInfoXmlListView : public ResContrInfoViewer
{
  public:
    virtual void operator() (ViewerData &, const ResContrInfo&) const;
    virtual ~ResContrInfoXmlListView(){}
};

class OrigOfRequestXmlView : public OrigOfRequestViewer
{
    public :
  virtual void operator ( )(ViewerData &, const OrigOfRequest&) const;
  virtual ~OrigOfRequestXmlView(){}
};

class OrigOfRequestXmlListView : public OrigOfRequestViewer
{
    public :
  virtual void operator ( )(ViewerData &, const OrigOfRequest&) const;
  virtual ~OrigOfRequestXmlListView(){}
};

class PassengerXmlView : public PassengerViewer
{
    public:
  virtual void operator () (ViewerData &, const Passenger &) const;
  virtual ~PassengerXmlView() {}
};

class FormOfIdXmlView : public FormOfIdViewer
{
public:
    virtual void operator () (ViewerData &, const std::list<FormOfId>&) const;
    virtual ~FormOfIdXmlView() {}
};

class PassengerXmlListView : public PassengerViewer
{
    public:
  virtual void operator () (ViewerData &, const Passenger &) const;
  virtual ~PassengerXmlListView() {}
};

class TaxDetailsXmlView : public TaxDetailsViewer {
    public:
  virtual void operator () (ViewerData &, const list<TaxDetails> &) const;
  virtual ~TaxDetailsXmlView() {}
};

class TaxDetailsXmlListView : public TaxDetailsViewer {
    public:
  virtual void operator () (ViewerData &, const list<TaxDetails> &) const;
  virtual ~TaxDetailsXmlListView() {}
};

class MonetaryInfoXmlView : public MonetaryInfoViewer {
    public:
  virtual void operator () (ViewerData &, const list<MonetaryInfo> &) const;
  virtual ~MonetaryInfoXmlView() {}
};

class MonetaryInfoXmlListView : public MonetaryInfoViewer {
    public:
  virtual void operator () (ViewerData &, const list<MonetaryInfo> &) const;
  virtual ~MonetaryInfoXmlListView() {}
};

class FormOfPaymentXmlView : public FormOfPaymentViewer {
    public:
  virtual void operator () (ViewerData &, const list<FormOfPayment> &) const;
  virtual ~FormOfPaymentXmlView () {}
};

class FreeTextInfoXmlView : public FreeTextInfoViewer {
    public:
	virtual void operator () (ViewerData &, const list<FreeTextInfo> &) const;
	virtual ~FreeTextInfoXmlView () {}
};

class FormOfPaymentXmlListView : public FormOfPaymentViewer {
    public:
  virtual void operator () (ViewerData &, const list<FormOfPayment> &) const;
  virtual ~FormOfPaymentXmlListView () {}
};

class FrequentPassXmlView : public FrequentPassViewer {
    public:
	virtual void operator () (ViewerData &, const list<FrequentPass> &l) const{}
	virtual ~FrequentPassXmlView(){}
};

class CouponXmlView : public CouponViewer {
    FrequentPassXmlView FreqPassXmlView;
    public:
	virtual const FrequentPassViewer &freqPassView () const
	{
	    return FreqPassXmlView;
	}
	virtual void operator () (ViewerData &, const list<Coupon> &) const;
	virtual ~CouponXmlView() {}
};

class CouponXmlListView : public CouponViewer {
    FrequentPassXmlView FreqPassXmlView;
    public:
	virtual const FrequentPassViewer &freqPassView () const
	{
	    return FreqPassXmlView;
	}
	virtual void operator () (ViewerData &, const list<Coupon> &) const;
	virtual ~CouponXmlListView() {}
};

class TicketXmlView : public TicketViewer {
    public:
  virtual void operator () (ViewerData &, const list<Ticket> &, const CouponViewer &) const;
  virtual ~TicketXmlView() {}
};

class TicketXmlListView : public TicketViewer {
    public:
  virtual void operator ()
          (ViewerData &, const list<Ticket> &, const CouponViewer &) const;
  virtual ~TicketXmlListView() {}
};

class PnrXmlViewCommon
{
  private:
    OrigOfRequestXmlView OrigOfReqXmlView;
    ResContrInfoXmlView  ResContrXmlView;
    PassengerXmlView PassXmlView;
    FormOfPaymentXmlView FopXmlView;
    TicketXmlView TickXmlView;
    CouponXmlView CoupXmlView;
  public:
    virtual const OrigOfRequestXmlView & origOfReqView () const { return OrigOfReqXmlView; }
    virtual const PassengerXmlView & passengerView () const { return PassXmlView; }
    virtual const FormOfPaymentXmlView &formOfPaymentView () const { return FopXmlView; }
    virtual const TicketXmlView &ticketView () const { return TickXmlView; }
    virtual const CouponXmlView &couponView () const { return CoupXmlView; }
    virtual const ResContrInfoXmlView & resContrInfoView () const
    {
      return ResContrXmlView;
    }

    virtual ~PnrXmlViewCommon(){}
};

class PnrXmlListViewCommon
{
  private:
    OrigOfRequestXmlListView OrigOfReqXmlListView;
    ResContrInfoXmlListView  ResContrXmlListView;
    PassengerXmlListView PassXmlListView;
    FormOfPaymentXmlListView FopXmlListView;
    TicketXmlListView TickXmlListView;
    CouponXmlListView CoupXmlListView;
  public:
    virtual const OrigOfRequestXmlListView & origOfReqView () const { return OrigOfReqXmlListView; }
    virtual const PassengerXmlListView & passengerView () const { return PassXmlListView; }
    virtual const FormOfPaymentXmlListView &formOfPaymentView () const { return FopXmlListView; }
    virtual const TicketXmlListView &ticketView () const { return TickXmlListView; }
    virtual const CouponXmlListView &couponView () const { return CoupXmlListView; }
    virtual const ResContrInfoXmlListView & resContrInfoView () const
    {
      return ResContrXmlListView;
    }

    virtual ~PnrXmlListViewCommon(){}
};

class PnrXmlView : public PnrViewer, public PnrXmlViewCommon
{
  private:
    TaxDetailsXmlView TaxXmlView;
    MonetaryInfoXmlView MonXmlView;
    FreeTextInfoXmlView IftXmlView;
    FormOfIdXmlView     FoidView;

    mutable XmlViewData ViewData;
  public:
    PnrXmlView(xmlNodePtr node):ViewData(node){}

    virtual const OrigOfRequestXmlView & origOfReqView () const
    {
      return PnrXmlViewCommon::origOfReqView();
    }
    virtual const PassengerXmlView & passengerView () const
    {
      return PnrXmlViewCommon::passengerView();
    }
    virtual const FormOfIdViewer &formOfIdView () const
    {
        return FoidView;
    }
    virtual const FormOfPaymentXmlView &formOfPaymentView () const
    {
      return PnrXmlViewCommon::formOfPaymentView();
    }
    virtual const TicketXmlView &ticketView () const
    {
      return PnrXmlViewCommon::ticketView();
    }
    virtual const CouponXmlView &couponView () const
    {
      return PnrXmlViewCommon::couponView();
    }

    virtual const ResContrInfoXmlView & resContrInfoView () const
    {
      return PnrXmlViewCommon::resContrInfoView();
    }
    virtual const TaxDetailsXmlView & taxDetailsView () const
    {
      return TaxXmlView;
    }
    virtual const MonetaryInfoXmlView &monetaryInfoView () const
    {
      return MonXmlView;
    }
    virtual const FreeTextInfoXmlView &freeTextInfoView() const
    {
	return IftXmlView;
    }

    virtual XmlViewData &viewData () const
    {
      return ViewData;
    }
    virtual ~PnrXmlView(){};
};

class PnrXmlListView : public PnrListViewer, public PnrXmlListViewCommon
{
  private:
    mutable XmlViewDataList ViewData;
  public:
    PnrXmlListView(xmlNodePtr node) : ViewData(node) {}
    virtual const OrigOfRequestXmlListView & origOfReqView () const
    {
      return PnrXmlListViewCommon::origOfReqView();
    }
    virtual const PassengerXmlListView & passengerView () const
    {
      return PnrXmlListViewCommon::passengerView();
    }
    virtual const FormOfPaymentXmlListView &formOfPaymentView () const
    {
      return PnrXmlListViewCommon::formOfPaymentView();
    }
    virtual const TicketXmlListView &ticketView () const
    {
      return PnrXmlListViewCommon::ticketView();
    }
    virtual const CouponXmlListView &couponView () const
    {
      return PnrXmlListViewCommon::couponView();
    }
    virtual const ResContrInfoXmlListView & resContrInfoView () const
    {
      return PnrXmlListViewCommon::resContrInfoView();
    }

    virtual ViewerDataList &viewData () const
    {
      return ViewData;
    }

    virtual ~PnrXmlListView(){}
};
}
}

#endif /*_ASTRA_TICK_VIEW_XML_H_*/
