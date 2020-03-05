#ifndef RFISC_PRICE_H
#define RFISC_PRICE_H
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_elems.h"
#include "rfisc.h"
#include "sirena_exchange.h"
#include "date_time.h"

struct PriceDoc
{
  std::string doc_id;
  std::string doc_type;
  bool unpoundable;
  PriceDoc( const std::string& _doc_id, const std::string& _doc_type, bool _unpoundable) {
    doc_id = _doc_id;
    doc_type = _doc_type;
    unpoundable = _unpoundable;
  }
  PriceDoc(){
    clear();
  }
  void clear() {
    doc_id.clear();
    doc_type.clear();
    unpoundable = false;
  }
};

class SvcFromSirena
{
  public:
    std::string svc_id;
    PriceDoc doc;
    std::string pass_id;
    std::string seg_id;
    std::string status_svc;
    std::string status_direct;
    std::string name;
    float price;
    std::string currency;
    std::string ticket_cpn;
    std::string ticknum;
    BASIC::date_time::TDateTime time_change;
    SvcFromSirena() {
      clear();
    }
    void clear() {
      svc_id.clear();
      doc.clear();
      pass_id.clear();
      seg_id.clear();
      status_svc.clear();
      status_direct.clear();
      name.clear();
      price = ASTRA::NoExists;
      currency.clear();
      ticket_cpn.clear();
      ticknum.clear();
      time_change = ASTRA::NoExists;
    }
    void fromContextXML(xmlNodePtr node);
    void fromXML(xmlNodePtr node);
    void toContextXML(xmlNodePtr node) const;
    void toXML(xmlNodePtr node) const;
    void toDB(TQuery& Qry) const;
    void fromDB(TQuery& Qry,const std::string& lang="");
    bool valid() const;
    bool only_for_cost() const;
    std::string toString() const;
};

typedef  std::map<std::string,SvcFromSirena> SVCS;

class TPriceServiceItem : public TPaxSegRFISCKey
{
  public:
    std::string pax_name;
  private:
    SVCS svcs; // std::string svc_id;
  public:
     enum EnumSVCS
    {
      only_for_cost,
      only_for_pay,
      all
    };
    TPriceServiceItem() { clear(); }
    TPriceServiceItem(const TPaxSegRFISCKey& _item, const std::string &_pax_name, std::map<std::string,SvcFromSirena> _svcs) :
      TPaxSegRFISCKey(_item), pax_name(_pax_name), svcs(_svcs) {}
    void clear()
    {
      TPaxSegRFISCKey::clear();
      pax_name.clear();
      svcs.clear();
    }
    std::string name_view(const std::string& lang="") const;

    void addSVCS(const std::string &code, const SvcFromSirena &val ) {
      svcs.emplace( code, val );
    }
    void eraseSVC(const std::string &code) {
      svcs.erase(code);
    }

    bool findSVC( const std::string& code, SVCS::iterator& f ) {
      f = svcs.find( code );
      return ( f != svcs.end() );
    }

    void getSVCS( SVCS& _svcs, EnumSVCS style  ) const {
      _svcs.clear();
      for ( const auto svc : svcs ) {
        switch( style ) {
          case only_for_cost:
            if ( !svc.second.only_for_cost() ) {
              continue;
            }
            break;
          case only_for_pay:
            if ( svc.second.only_for_cost() ) {
              continue;
            }
            break;
          case all:
            break;
        }
        _svcs.emplace( svc.first,svc.second );
      }
    }

    void getSVCS( SVCS& _svcs, const std::string &status_direct ) const {
      _svcs.clear();
      for ( const auto svc : svcs ) {
        if ( svc.second.status_direct == status_direct ) {
          _svcs.emplace( svc.first,svc.second );
        }
      }
    }

    void changeStatus( const std::string& from, const std::string& to );

    const TPriceServiceItem& toXML(xmlNodePtr node, const std::string& svc_idx) const;
    const TPriceServiceItem& toContextXML(xmlNodePtr node) const;
    TPriceServiceItem& fromXML(xmlNodePtr node);
    TPriceServiceItem& fromContextXML(xmlNodePtr node);
    const TPriceServiceItem& toDB(TQuery &Qry, const std::string& svc_id) const;
    TPriceServiceItem& fromDB(TQuery &Qry,const std::string& lang="");
    std::string traceStr() const;
};

class SvcEmdPayDoc {
  private:
  //formpay = IN - платежное поручение
  //formpay=CA (латиницей) -  наличные
    std::string formpay;
    std::string type;
  public:
    SvcEmdPayDoc() {
      clear();
    }
    SvcEmdPayDoc( const std::string& _formpay,
                  const std::string& _type ): formpay(_formpay),type(_type){}
    void toXML(xmlNodePtr node) const;
    void fromXML(xmlNodePtr reqNode);
    void clear() {
      formpay.clear();
      type.clear();
    }
    bool empty() const {
      return formpay.empty();
    }
    SvcEmdPayDoc getSvcEmdPayDoc() {
      SvcEmdPayDoc _paydoc = *this;
      return _paydoc;
    }
    void setSvcEmdPayDoc( const SvcEmdPayDoc& _paydoc ) {
      SvcEmdPayDoc::operator = (_paydoc);
    }
};


class SvcEmdTimeout
{
  private:
    int timeout;
    TDateTime utc_deadline;
  public:
    void clear() {
       utc_deadline = ASTRA::NoExists;
       timeout = ASTRA::NoExists;
    }
    void fromXML(xmlNodePtr node);
    void toXML(xmlNodePtr node) const;
    SvcEmdTimeout() {
      clear();
    }
    SvcEmdTimeout(int _timeout,TDateTime _utc_deadline):timeout(_timeout),utc_deadline(_utc_deadline){}
    SvcEmdTimeout getSvcEmdTimeout() {
      SvcEmdTimeout _timeout = *this;
      return _timeout;
    }
    void setSvcEmdTimeout( const SvcEmdTimeout& _timeout ) {
      SvcEmdTimeout::operator = (_timeout);
    }
};

class SvcEmdRegnum
{
  private:
    std::string regnum;
    int version;
  public:
    enum Enum
    {
      propStyle,
      nodeStyle,
      noneStyle
    };

    void clear() {
       regnum.clear();
       version = ASTRA::NoExists;
    }
    void fromXML(xmlNodePtr node,SvcEmdRegnum::Enum style);
    void toXML(xmlNodePtr node,SvcEmdRegnum::Enum style) const;
    SvcEmdRegnum() {
      clear();
    }
    SvcEmdRegnum(const std::string& _regnum, int _version):regnum(_regnum),version(_version){}
    SvcEmdRegnum(const std::string& _regnum):regnum(_regnum),version(ASTRA::NoExists){}
    bool empty() const {
      return regnum.empty();
    }
    SvcEmdRegnum getSvcEmdRegnum() {
      SvcEmdRegnum _regnum = *this;
      return _regnum;
    }
    void setSvcEmdRegnum( const SvcEmdRegnum& _regnum ) {
      SvcEmdRegnum::operator = (_regnum);
    }
    std::string getRegnum() {
      return regnum;
    }

    std::string toString() {
      return regnum;
    }

};

class SvcEmdCost
{
  private:
    std::string cost;
    std::string currency;
    TElemFmt fmt;
  public:
    void clear() {
       cost.clear();
       currency.clear();
       fmt = efmtUnknown;
    }
    void fromXML(xmlNodePtr node);
    void toXML(xmlNodePtr node) const;
    SvcEmdCost() {
      clear();
    }
    SvcEmdCost(std::string& _cost, const std::string _currency, TElemFmt _fmt ):cost(_cost),currency(_currency),fmt(_fmt){}
    SvcEmdCost(std::string& _cost, const std::string _currency ):cost(_cost),currency(_currency), fmt(efmtUnknown){}
    SvcEmdCost getSvcEmdCost() const {
      SvcEmdCost _cost = *this;
      return _cost;
    }
    void setSvcEmdCost( const SvcEmdCost& _cost ) {
      SvcEmdCost::operator = (_cost);
    }
    std::string getCost() {
      return cost;
    }
    std::string getCurrency() {
      return currency;
    }
};

class SvcEmdSvcsReq
{
  private:
    std::vector<std::string> svcs;
  public:
    void clear() {
       svcs.clear();
    }
    void toXML(xmlNodePtr node) const;
    SvcEmdSvcsReq() {
      clear();
    }
    SvcEmdSvcsReq(std::vector<std::string>_svcs):svcs(_svcs){}

};

class SvcValue {
  public:
    std::string id;
    std::string name;
    std::string pass_id;
    std::string rfic;
    std::string rfisc;
    std::string seg_id;
    std::string ticket_cpn;
    std::string ticknum;
    std::string service_type;
    std::string status;
    void fromXML( xmlNodePtr node );
    SvcValue() {}
};

class SvcEmdSvcsAns: public std::vector<SvcValue>
{
  public:
    void clear() {
      std::vector<SvcValue>::clear();
    }
    void fromXML( xmlNodePtr node );
    void getSvcValue( const std::string &id, SvcValue &vsvc  );
    SvcEmdSvcsAns getSvcEmdSvcsAns() {
      SvcEmdSvcsAns _svcs = *this;
      return _svcs;
    }
};

struct PointGrpPaxs
{
  int point_id;
  int grp_id;
  std::map<int,int> paxs; // first - crs_pax_id, second - pax_key_id - ид. пасс по сквозному маршруту взятому с первого сегмента
  PointGrpPaxs(int vpoint_id,int vgrp_id) {
    point_id = vpoint_id;
    grp_id = vgrp_id;
  }
};

class SegsPaxs
{
  private:
    std::map<int,PointGrpPaxs> segs; //key=segno
  public:
    void fromDB(int grp_id, int point_dep);
    void getPaxs(std::map<int,PointGrpPaxs>& _items) {
      _items = segs;
    }
    bool checkTrferNum( int trfer_num ) {
      return ( segs.find(trfer_num) != segs.end() );
    }
};

class PaxsNames
{
  private:
    TQuery Qry;
    std::map<int,std::string> items;
  public:
    PaxsNames():Qry(&OraSession) {
      Qry.SQLText =
        "SELECT pax.* "
        "FROM pax "
        "WHERE pax_id=:pax_id ";
      Qry.DeclareVariable( "pax_id", otInteger );
    }
    std::string getPaxName( int pax_id );
};

class TPriceRFISCList: public std::map<TPaxSegRFISCKey, TPriceServiceItem>, public SvcEmdRegnum, public SvcEmdPayDoc, public SvcEmdCost, public SvcEmdTimeout
{
  public:
   static const std::string STATUS_DIRECT_ORDER;
   static const std::string STATUS_DIRECT_SELECTED;
   static const std::string STATUS_DIRECT_ISSUE_QUERY;
   static const std::string STATUS_DIRECT_ISSUE_ANSWER;
   static const std::string STATUS_DIRECT_ISSUE_CONFIRM;
   static const std::string STATUS_DIRECT_REFUND;
   static const std::string STATUS_DIRECT_PAID;
   static const std::string STATUS_DIRECT_ONLY_FOR_COST;
  private:
    std::string surname;
    BASIC::date_time::TDateTime time_create;
    std::string mps_order_id;
    std::string error_code, error_message;
  public:
    void clear() {
      SvcEmdRegnum::clear();
      SvcEmdPayDoc::clear();
      SvcEmdCost::clear();
      SvcEmdTimeout::clear();
      std::map<TPaxSegRFISCKey, TPriceServiceItem>::clear();
      surname.clear();
      mps_order_id.clear();
      error_code.clear();
      error_message.clear();
    }
    void setSurname( const std::string& _surname ) {
      surname=_surname;
    }
    std::string getSurname() {
      return surname;
    }
    void setMPSOrderId( const std::string& _mps_order_id ) {
      mps_order_id = _mps_order_id;
    }
    std::string getMPSOrderId() {
      return mps_order_id;
    }
    void setServices( const TPaidRFISCList& list );
    void setError( const SirenaExchange::TExchange& ex ) {
      error_code = ex.error_code;
      error_message = ex.error_message;
    }
    std::string getErrorMessage() {
      return error_message;
    }

    std::string getErrorCode() {
      return error_code;
    }

    TPriceRFISCList();
    bool notInit() {
      return SvcEmdRegnum::empty();
    }
    void Lock(int grp_id);
    void fromContextDB(int grp_id);
    void toContextDB(int grp_id, bool pr_only_del=false) const;
    void toDB(int grp_id) const;
    void fromDB(int grp_id);
    void fromXML(xmlNodePtr node);
    void fromContextXML(xmlNodePtr node);
    void toXML(xmlNodePtr node) const;
    void toContextXML(xmlNodePtr node, bool checkFormPay=true) const;
    bool synchFromSirena(const TPriceRFISCList& list,bool only_delete=false);
    void synchFromSirena(const SvcEmdSvcsAns& svcs);
    bool filterFromTerminal(const TPriceRFISCList& list);
    void setStatusDirect( const std::string &from, const std::string &to );
    bool haveStatusDirect( const std::string& statusDirect, std::vector<std::string> &svcs);
    bool haveStatusDirect( const std::string& statusDirect );
    bool getNextIssueQueryGrpEMD( std::vector<std::string> &svcs);
    bool terminalChoiceAny();
    float getTotalCost();
    std::string getTotalCurrency();
};



#endif // RFISC_PRICE_H
