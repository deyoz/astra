#ifndef _PASSENGER_H_
#define _PASSENGER_H_

#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "baggage.h"
#include "baggage_wt.h"
#include "remarks.h"
#include "payment_base.h"
#include <boost/optional.hpp>
#include "date_time.h"

using BASIC::date_time::TDateTime;

const long int NO_FIELDS=0x0000;

enum TAPIType { apiDoc, apiDoco, apiDocaB, apiDocaR, apiDocaD, apiTkn, apiUnknown };

namespace CheckIn
{

class TPaxGrpCategory
{
  public:
    enum Enum
    {
      Passenges,
      Crew,
      UnnacompBag,
      Unknown
    };

    static const std::list< std::pair<Enum, std::string> >& view_pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Passenges,   "Passenges"));
        l.push_back(std::make_pair(Crew,        "Crew"));
        l.push_back(std::make_pair(UnnacompBag, "UnnacompBag"));
        l.push_back(std::make_pair(Unknown,     "Unknown"));
      }
      return l;
    }
};

class TPaxGrpCategoriesView : public ASTRA::PairList<TPaxGrpCategory::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TPaxGrpCategoriesView"; }
  public:
    TPaxGrpCategoriesView() : ASTRA::PairList<TPaxGrpCategory::Enum, std::string>(TPaxGrpCategory::view_pairs(),
                                                                                  boost::none,
                                                                                  boost::none) {}
};

const TPaxGrpCategoriesView& PaxGrpCategories();

class TPaxAPIItem
{
  public:
    virtual long int getNotEmptyFieldsMask() const=0;
    virtual TAPIType apiType() const=0;
    virtual ~TPaxAPIItem() {}
};

class TPaxTknItem : public TPaxAPIItem, public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string no;
    int coupon;
    std::string rem;
    bool confirm;
    TPaxTknItem()
    {
      clear();
    };
    void clear()
    {
      no.clear();
      coupon=ASTRA::NoExists;
      rem.clear();
      confirm=false;
    };
    bool empty() const
    {
      return no.empty() &&
             coupon==ASTRA::NoExists &&
             rem.empty();
    };
    bool operator == (const TPaxTknItem &item) const
    {
      return  no == item.no &&
              coupon == item.coupon &&
              rem == item.rem &&
              confirm == item.confirm;
    };
    bool equalAttrs(const TPaxTknItem &item) const
    {
      return  no == item.no &&
              coupon == item.coupon &&
              rem == item.rem;
    }
    const TPaxTknItem& toXML(xmlNodePtr node) const;
    TPaxTknItem& fromXML(xmlNodePtr node);
    const TPaxTknItem& toDB(TQuery &Qry) const;
    TPaxTknItem& fromDB(TQuery &Qry);

    long int getNotEmptyFieldsMask() const;
    TAPIType apiType() const { return apiTkn; }
    bool validET() const { return rem=="TKNE" && !no.empty() && coupon!=ASTRA::NoExists; }

    std::string no_str() const
    {
      std::ostringstream s;
      s << no;
      if (coupon!=ASTRA::NoExists)
        s << "/" << coupon;
      return s.str();
    };
    std::string rem_code() const
    {
      return rem;
    }
    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const
    {
      return no_str();
    }
};

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn);
bool LoadPaxTkn(TDateTime part_key, int pax_id, TPaxTknItem &tkn);
bool LoadCrsPaxTkn(int pax_id, TPaxTknItem &tkn);

class TPaxDocItem : public TPaxAPIItem, public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string type;
    std::string issue_country;
    std::string no;
    std::string nationality;
    TDateTime birth_date;
    std::string gender;
    TDateTime expiry_date;
    std::string surname;
    std::string first_name;
    std::string second_name;
    bool pr_multi;
    std::string type_rcpt;
    long int scanned_attrs;
    TPaxDocItem()
    {
      clear();
    };
    void clear()
    {
      type.clear();
      issue_country.clear();
      no.clear();
      nationality.clear();
      birth_date=ASTRA::NoExists;
      gender.clear();
      expiry_date=ASTRA::NoExists;
      surname.clear();
      first_name.clear();
      second_name.clear();
      pr_multi=false;
      type_rcpt.clear();
      scanned_attrs=NO_FIELDS;
    };
    bool empty() const
    {
      return type.empty() &&
             issue_country.empty() &&
             no.empty() &&
             nationality.empty() &&
             birth_date==ASTRA::NoExists &&
             gender.empty() &&
             expiry_date==ASTRA::NoExists &&
             surname.empty() &&
             first_name.empty() &&
             second_name.empty() &&
             pr_multi==false &&
             type_rcpt.empty();
    };
    bool equalAttrs(const TPaxDocItem &item) const
    {
      return type == item.type &&
             issue_country == item.issue_country &&
             no == item.no &&
             nationality == item.nationality &&
             birth_date == item.birth_date &&
             gender == item.gender &&
             expiry_date == item.expiry_date &&
             surname == item.surname &&
             first_name == item.first_name &&
             second_name == item.second_name &&
             pr_multi == item.pr_multi &&
             type_rcpt == item.type_rcpt;
    };
    bool equal(const TPaxDocItem &item) const
    {
      return equalAttrs(item) &&
             scanned_attrs == item.scanned_attrs;
    };
    const TPaxDocItem& toXML(xmlNodePtr node) const;
    TPaxDocItem& fromXML(xmlNodePtr node);
    const TPaxDocItem& toDB(TQuery &Qry) const;
    TPaxDocItem& fromDB(TQuery &Qry);

    long int getEqualAttrsFieldsMask(const TPaxDocItem &item) const;
    long int getNotEmptyFieldsMask() const;
    TAPIType apiType() const { return apiDoc; }
    std::string rem_code() const
    {
      return "DOCS";
    }
    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const;
    std::string full_name() const;
};

const std::string DOCO_PSEUDO_TYPE="-";

class TPaxDocoItem : public TPaxAPIItem, public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string birth_place;
    std::string type;
    std::string no;
    std::string issue_place;
    TDateTime issue_date;
    TDateTime expiry_date;
    std::string applic_country;
    long int scanned_attrs;
    bool doco_confirm;
    TPaxDocoItem()
    {
      clear();
    };
    void clear()
    {
      birth_place.clear();
      type.clear();
      no.clear();
      issue_place.clear();
      issue_date=ASTRA::NoExists;
      expiry_date=ASTRA::NoExists;
      applic_country.clear();
      scanned_attrs=NO_FIELDS;
      doco_confirm=false;
    };
    bool empty() const
    {
      return birth_place.empty() &&
             type.empty() &&
             no.empty() &&
             issue_place.empty() &&
             issue_date==ASTRA::NoExists &&
             expiry_date==ASTRA::NoExists &&
             applic_country.empty();
    };
    bool equalAttrs(const TPaxDocoItem &item) const
    {
      return birth_place == item.birth_place &&
             type == item.type &&
             no == item.no &&
             issue_place == item.issue_place &&
             issue_date == item.issue_date &&
             expiry_date == item.expiry_date &&
             applic_country == item.applic_country;
    };
    bool equal(const TPaxDocoItem &item) const
    {
      return equalAttrs(item) &&
             scanned_attrs == item.scanned_attrs;
    };
    const TPaxDocoItem& toXML(xmlNodePtr node) const;
    TPaxDocoItem& fromXML(xmlNodePtr node);
    const TPaxDocoItem& toDB(TQuery &Qry) const;
    TPaxDocoItem& fromDB(TQuery &Qry);

    bool needPseudoType() const;

    long int getEqualAttrsFieldsMask(const TPaxDocoItem &item) const;
    void ReplacePunctSymbols();
    long int getNotEmptyFieldsMask() const;
    TAPIType apiType() const { return apiDoco; }
    std::string rem_code() const
    {
      return "DOCO";
    }
    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const;
};

class TPaxDocaItem : public TPaxAPIItem, public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string type;
    std::string country;
    std::string address;
    std::string city;
    std::string region;
    std::string postal_code;
    TPaxDocaItem()
    {
      clear();
    };
    void clear()
    {
      type.clear();
      country.clear();
      address.clear();
      city.clear();
      region.clear();
      postal_code.clear();
    };
    bool empty() const
    {
      return type.empty() &&
             country.empty() &&
             address.empty() &&
             city.empty() &&
             region.empty() &&
             postal_code.empty();
    };
    bool operator < (const TPaxDocaItem &item) const
    {
        if(type != item.type)
            return type < item.type;
        if(country != item.country)
            return country < item.country;
        if(address != item.address)
            return address < item.address;
        if(city != item.city)
            return city < item.city;
        if(region != item.region)
            return region < item.region;
        return postal_code < item.postal_code;
    }

    bool operator == (const TPaxDocaItem &item) const
    {
      return type == item.type &&
             country == item.country &&
             address == item.address &&
             city == item.city &&
             region == item.region &&
             postal_code == item.postal_code;
    };
    const TPaxDocaItem& toXML(xmlNodePtr node) const;
    TPaxDocaItem& fromXML(xmlNodePtr node);
    const TPaxDocaItem& toDB(TQuery &Qry) const;
    TPaxDocaItem& fromDB(TQuery &Qry);

    long int getEqualAttrsFieldsMask(const TPaxDocaItem &item) const;
    void ReplacePunctSymbols();
    long int getNotEmptyFieldsMask() const;
    TAPIType apiType() const;
    std::string rem_code() const
    {
      return "DOCA";
    }
    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const;
};

class TSimplePaxItem
{
  public:
    int id;
    std::string surname;
    std::string name;
    ASTRA::TPerson pers_type;
    ASTRA::TCrewType::Enum crew_type;
    std::string seat_no;
    std::string seat_type;
    int seats;
    std::string refuse;
    bool pr_brd;
    bool pr_exam;
    std::string wl_type;
    int reg_no;
    std::string subcl;
    int bag_pool_num;
    int tid;
    TPaxTknItem tkn;
    bool TknExists;
    TSimplePaxItem()
    {
      clear();
    }
    void clear()
    {
      id=ASTRA::NoExists;
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      crew_type=ASTRA::TCrewType::Unknown;
      seat_no.clear();
      seat_type.clear();
      seats=ASTRA::NoExists;
      refuse.clear();
      pr_brd=false;
      pr_exam=false;
      wl_type.clear();
      reg_no=ASTRA::NoExists;
      subcl.clear();
      bag_pool_num=ASTRA::NoExists;
      tid=ASTRA::NoExists;
      tkn.clear();
      TknExists=false;
    }

    TSimplePaxItem& fromDB(TQuery &Qry);
    std::string full_name() const;
    bool api_doc_applied() const;
    bool upward_within_bag_pool(const TSimplePaxItem& pax) const;
};

class TDocaMap : public std::map<TAPIType, TPaxDocaItem>
{
  public:
    TDocaMap() { Clear(); }
    void Clear()
    {
      typedef std::map<TAPIType, TPaxDocaItem> Base;
      Base::clear();
      Base::operator[](apiDocaB).type = "B";
      Base::operator[](apiDocaR).type = "R";
      Base::operator[](apiDocaD).type = "D";
    }
    std::string ToString() const
    {
      std::string result = "TDocaMap types: ";
      for (TDocaMap::const_iterator i = this->begin(); i != this->end(); ++i)
        result += "[" + i->second.type + "]";
      return result;
    }
};

class TPaxItem : public TSimplePaxItem
{
  public:
    TPaxDocItem doc;
    TPaxDocoItem doco;
    TDocaMap doca_map;
    bool PaxUpdatesPending;
    bool DocExists;
    bool DocoExists;
    bool DocaExists;
    bool dont_check_payment;
    TPaxItem()
    {
      clear();
    }
    void clear()
    {
      TSimplePaxItem::clear();
      doc.clear();
      doco.clear();
      doca_map.Clear();
      PaxUpdatesPending=false;
      DocExists=false;
      DocoExists=false;
      DocaExists=false;
      dont_check_payment=false;
    }

    const TPaxItem& toXML(xmlNodePtr node) const;
    TPaxItem& fromXML(xmlNodePtr node);
    const TPaxItem& toDB(TQuery &Qry) const;
    TPaxItem& fromDB(TQuery &Qry);
    int is_female() const;
};

class TAPISItem
{
  public:
    TPaxDocItem doc;
    TPaxDocoItem doco;
    TDocaMap doca_map;
    TAPISItem()
    {
      clear();
    };
    void clear()
    {
      doc.clear();
      doco.clear();
      doca_map.Clear();
    };

    const TAPISItem& toXML(xmlNodePtr node) const;
    TAPISItem& fromDB(int pax_id);
};

int is_female(const std::string &pax_doc_gender, const std::string &pax_name);

class TPaxListItem
{
  public:
    CheckIn::TPaxItem pax;
    int generated_pax_id; //заполняется только при первоначальной регистрации (new_checkin) и только для NOREC
    bool remsExists;
    std::multiset<CheckIn::TPaxRemItem> rems;
    std::set<CheckIn::TPaxFQTItem> fqts;
    boost::optional< std::list<WeightConcept::TPaxNormItem> > norms;
    xmlNodePtr node;

    TPaxListItem() { clear(); }

    void clear()
    {
      pax.clear();
      generated_pax_id=ASTRA::NoExists;
      remsExists=false;
      rems.clear();
      fqts.clear();
      norms=boost::none;
      node=NULL;
    }

    bool trferAttachable() const
    {
      return pax.seats!=ASTRA::NoExists &&
          (pax.pers_type==ASTRA::adult || pax.pers_type==ASTRA::child || pax.pers_type==ASTRA::baby) &&
          pax.refuse!=ASTRA::refuseAgentError;
    }

    TPaxListItem& fromXML(xmlNodePtr paxNode);

    void addFQT(const CheckIn::TPaxFQTItem &fqt);
    void checkFQTTierLevel();
    void checkCrewType(bool new_checkin, ASTRA::TPaxStatus grp_status);
};

class TPaxList : public std::list<CheckIn::TPaxListItem>
{
  public:
    int getBagPoolMainPaxId(int bag_pool_num) const;
};

class TPaxGrpItem
{
  public:
    int id;
    int point_dep;
    int point_arv;
    std::string airp_dep;
    std::string airp_arv;
    std::string cl;
    ASTRA::TPaxStatus status;
    int hall;
    std::string bag_refuse;
    bool pc, wt;
    bool rfisc_used;
    bool trfer_confirm;
    bool is_mark_norms;
    ASTRA::TClientType client_type;
    int tid;
    int bag_types_id;     //!!!потом удалить
    bool baggage_pc;      //!!!потом удалить
    bool need_upgrade_db; //!!!потом удалить

    boost::optional< std::list<WeightConcept::TPaxNormItem> > norms;
    boost::optional< WeightConcept::TPaidBagList > paid;
    boost::optional< TGroupBagItem > group_bag;
    boost::optional< TGrpServiceList > svc;
    boost::optional< TServicePaymentList > payment;
    TPaxGrpItem()
    {
      clear();
    };
    void clear()
    {
      id=ASTRA::NoExists;
      point_dep=ASTRA::NoExists;
      point_arv=ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();
      cl.clear();
      status=ASTRA::psCheckin;
      hall=ASTRA::NoExists;
      bag_refuse.clear();
      pc=false;
      wt=false;
      rfisc_used=false;
      trfer_confirm=false;
      is_mark_norms=false;
      client_type = ASTRA::ctTypeNum;
      tid=ASTRA::NoExists;

      bag_types_id=ASTRA::NoExists;
      baggage_pc=false;
      need_upgrade_db=false;

      norms=boost::none;
      paid=boost::none;
      group_bag=boost::none;
      svc=boost::none;
      payment=boost::none;
    };

    const TPaxGrpItem& toXML(xmlNodePtr node) const;
    bool fromXML(xmlNodePtr node);
    TPaxGrpItem& fromXMLadditional(xmlNodePtr node, xmlNodePtr firstSegNode, bool is_unaccomp);
    const TPaxGrpItem& toDB(TQuery &Qry) const;
    TPaxGrpItem& fromDB(TQuery &Qry);
    bool fromDB(int grp_id);
    TPaxGrpCategory::Enum grpCategory() const;
    bool is_unaccomp() const
    {
      return grpCategory()==TPaxGrpCategory::UnnacompBag;
    }
};

class TPnrAddrItem
{
  public:
    std::string airline, addr;
    TPnrAddrItem()
    {
      clear();
    }
    void clear()
    {
      airline.clear();
      addr.clear();
    }
    TPnrAddrItem& fromDB(TQuery &Qry);
};

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc);
bool LoadPaxDoc(TDateTime part_key, int pax_id, TPaxDocItem &doc);
std::string GetPaxDocStr(TDateTime part_key,
                         int pax_id,
                         bool with_issue_country=false,
                         const std::string &lang="");

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc);
bool LoadPaxDoco(TDateTime part_key, int pax_id, TPaxDocoItem &doc);
enum TDocaType
{
  docaDestination,
  docaResidence,
  docaBirth
};

void ConvertDoca(TDocaMap doca_map,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD);

bool LoadPaxDoca(int pax_id, TDocaMap &doca_map);
bool LoadPaxDoca(int pax_id, TDocaType type, TPaxDocaItem &doca);
bool LoadPaxDoca(TDateTime part_key, int pax_id, TDocaMap &doca_map);
bool LoadPaxDoca(TDateTime part_key, int pax_id, TDocaType type, TPaxDocaItem &doca);

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc, bool without_inf_indicator=false);
bool LoadCrsPaxVisa(int pax_id, TPaxDocoItem &doc);
bool LoadCrsPaxDoca(int pax_id, TDocaMap &doca_map);

void SavePaxDoc(int pax_id, const TPaxDocItem &doc, TQuery& PaxDocQry);
void SavePaxDoco(int pax_id, const TPaxDocoItem &doc, TQuery& PaxDocQry);
void SavePaxDoca(int pax_id, const TDocaMap &doca_map, TQuery& PaxDocaQry, bool new_checkin);

std::string PaxDocGenderNormalize(const std::string &pax_doc_gender);

bool LoadCrsPaxPNRs(int pax_id, std::list<TPnrAddrItem> &pnrs);

void CalcPaidBagEMDProps(const CheckIn::TServicePaymentList &prior_payment,
                         const boost::optional< CheckIn::TServicePaymentList > &curr_payment,
                         CheckIn::TPaidBagEMDProps &diff,
                         CheckIn::TPaidBagEMDProps &props);

class TCkinPaxTknItem : public TPaxTknItem
{
  public:
    int grp_id;
    int pax_id;
    TCkinPaxTknItem()
    {
      clear();
    }
    void clear()
    {
      TPaxTknItem::clear();
      grp_id=ASTRA::NoExists;
      pax_id=ASTRA::NoExists;
    }

    TCkinPaxTknItem& fromDB(TQuery &Qry);
};

void GetTCkinTickets(int pax_id, std::map<int, TCkinPaxTknItem> &tkns);

std::string isFemaleStr( int is_female );

} //namespace CheckIn

namespace Sirena
{

void PaxBrandsNormsToStream(const TTrferRoute &trfer, const CheckIn::TPaxItem &pax, std::ostringstream &s);

} //namespace Sirena


#endif


