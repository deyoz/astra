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
#include "qrys.h"
#include "astra_types.h"
#include "astra_misc.h"
#include "base_callbacks.h"
#include "db_tquery.h"

using BASIC::date_time::TDateTime;

const long int NO_FIELDS=0x0000;

enum TAPIType { apiDoc, apiDoco, apiDocaB, apiDocaR, apiDocaD, apiTkn, apiUnknown };
enum class PaxChanges { New, Cancel, last=Cancel};

std::ostream& operator << (std::ostream& os, const PaxChanges& value);

const int TEST_ID_BASE = 1000000000;
const int TEST_ID_LAST = TEST_ID_BASE+999999999;
const int EMPTY_ID = TEST_ID_LAST+1;

bool isTestPaxId(int id);
int getEmptyPaxId();
bool isEmptyPaxId(int id);

class PaxIdWithSegmentPair
{
  private:
    PaxId_t paxId;
    boost::optional<TPaxSegmentPair> paxSegment;
  public:
    explicit
    PaxIdWithSegmentPair(const PaxId_t& paxId_,
                         const boost::optional<TPaxSegmentPair>& paxSegment_) :
      paxId(paxId_), paxSegment(paxSegment_) {}
    PaxIdWithSegmentPair(const PaxId_t& paxId_,
                         const TPaxSegmentPair& paxSegment_) :
      paxId(paxId_), paxSegment(paxSegment_) {}
    const PaxId_t& operator() () const { return paxId; }
    const boost::optional<TPaxSegmentPair>& getSegmentPair() const { return paxSegment; }

    bool operator < (const PaxIdWithSegmentPair &item) const
    {
      return paxId<item.paxId;
    }
};

template <typename CategoryT>
class PaxEventCallbacks
{
  public:
    virtual ~PaxEventCallbacks() {}

    //ByPaxId
    virtual void initialize(TRACE_SIGNATURE,
                            const PaxOrigin& paxOrigin){}
    virtual void finalize(TRACE_SIGNATURE,
                          const PaxOrigin& paxOrigin){}
    virtual void onChange(TRACE_SIGNATURE,
                          const PaxOrigin& paxOrigin,
                          const PaxIdWithSegmentPair& paxId,
                          const std::set<CategoryT>& categories){}

    //ByCategory
    virtual void initializeForCategory(TRACE_SIGNATURE,
                                       const PaxOrigin& paxOrigin,
                                       const CategoryT& category){}
    virtual void finalizeForCategory(TRACE_SIGNATURE,
                                     const PaxOrigin& paxOrigin,
                                     const CategoryT& category){}
    virtual void onChange(TRACE_SIGNATURE,
                          const PaxOrigin& paxOrigin,
                          const CategoryT& category,
                          const std::set<PaxIdWithSegmentPair>& paxIds){}
};

template <typename CategoryT>
class PaxEventHolder
{
  private:
    static const auto categoryCount=typename std::underlying_type<CategoryT>::type(CategoryT::last)+1;
    std::vector<std::set<PaxIdWithSegmentPair>> paxIds;
    std::map<PaxIdWithSegmentPair, std::set<CategoryT> > paxCategories;
    PaxOrigin paxOrigin;
  public:
    PaxEventHolder(const PaxOrigin& paxOrigin_) : paxIds(categoryCount), paxOrigin(paxOrigin_)
    {}

    void clear()
    {
      for(auto& i : paxIds) i.clear();
      paxCategories.clear();
    }

    const std::set<PaxIdWithSegmentPair>& getPaxIds(const CategoryT& category) const
    {
      return paxIds[typename std::underlying_type<CategoryT>::type(category)];
    }

    void add(const CategoryT& category, const PaxIdWithSegmentPair& paxId)
    {
      paxIds[typename std::underlying_type<CategoryT>::type(category)].insert(paxId);
      auto i=paxCategories.find(paxId);
      if (i==paxCategories.end()) {
          i=paxCategories.emplace(paxId, std::set<CategoryT>()).first;
      }
      i->second.insert(category);
    }
    bool remove(const CategoryT& category, const PaxIdWithSegmentPair& paxId)
    {
      auto i=paxCategories.find(paxId);
      if (i!=paxCategories.end()) i->second.erase(category);
      return paxIds[typename std::underlying_type<CategoryT>::type(category)].erase(paxId);
    }
    bool remove(const PaxIdWithSegmentPair& paxId)
    {
      for(auto& paxIdsForCategory : paxIds)
        paxIdsForCategory.erase(paxId);
      return paxCategories.erase(paxId);
    }

    bool exists(const std::initializer_list<CategoryT>& categoryList) const
    {
        return algo::any_of(categoryList, [&](const CategoryT & cat) {return !getPaxIds(cat).empty();});
    }

    bool empty() const
    {
      return paxCategories.empty();
    }

    void executeCallbacksByCategory(TRACE_SIGNATURE)
    {
        for(const auto & category_callbacks : callbacksSuite< PaxEventCallbacks<CategoryT> >()) {
            for(unsigned cat=0; cat<categoryCount; cat++)
            {
              CategoryT category=static_cast<CategoryT>(cat);
              category_callbacks->initializeForCategory(TRACE_VARIABLE, paxOrigin, category);
              if (!paxIds[cat].empty()) {
                  category_callbacks->onChange(TRACE_VARIABLE, paxOrigin, category, paxIds[cat]);
              }
              category_callbacks->finalizeForCategory(TRACE_VARIABLE, paxOrigin, category);
            }
        }
    }

    void executeCallbacksByPaxId(TRACE_SIGNATURE) const
    {
        for(const auto & category_callbacks : callbacksSuite< PaxEventCallbacks<CategoryT> >()) {
            category_callbacks->initialize(TRACE_VARIABLE, paxOrigin);

            for(const auto& i : paxCategories)
            {
              if (!i.second.empty()) {
                  category_callbacks->onChange(TRACE_VARIABLE, paxOrigin, i.first, i.second);
              }
            }

            category_callbacks->finalize(TRACE_VARIABLE, paxOrigin);
        }
    }
};

typedef PaxEventHolder<TRemCategory> ModifiedPaxRem;
typedef PaxEventHolder<PaxChanges> ModifiedPax;

void addPaxEvent(const PaxIdWithSegmentPair& paxId,
                 const bool cancelledOrMissingBefore,
                 const bool cancelledAfter,
                 ModifiedPax& modifiedPax);

void synchronizePaxEvents(const ModifiedPax& modifiedPax,
                          ModifiedPaxRem& modifiedPaxRem);

namespace CheckIn
{

class TSimplePaxItem;
std::optional<TPaxSegmentPair> paxSegment(const PaxId_t& pax_id);
std::optional<TPaxSegmentPair> crsSegment(const PaxId_t& pax_id);
std::optional<TPaxSegmentPair> ckinSegment(const GrpId_t& grp_id);
std::vector<TPaxSegmentPair> paxRouteSegments(const PaxId_t& pax_id);
std::vector<TPaxSegmentPair> crsRouteSegments(const PaxId_t& pax_id);
std::vector<int> routePoints(const PaxId_t& pax_id, PaxOrigin checkinType);
std::vector<std::string> routeAirps(const PaxId_t& pax_id, PaxOrigin checkinType);

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
                             bool language_lat,
                             TOutput output) const;
  public:
    std::string no;
    int coupon;
    std::string rem;
    bool confirm;
    TPaxTknItem()
    {
      clear();
    }
    TPaxTknItem(const std::string& _no, const int& _coupon)
    {
      clear();
      if (_no.empty()) return;
      no=_no;
      coupon=_coupon;
      rem=_coupon!=ASTRA::NoExists?"TKNE":"TKNA";
    }
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

    std::string no_str(std::string separator = "/") const
    {
      std::ostringstream s;
      s << no;
      if (coupon!=ASTRA::NoExists)
        s << separator << coupon;
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
    int checkedInETCount() const;

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    void addSearchPaxIds(const PaxOrigin& origin, std::set<int>& searchPaxIds) const { return; }
    bool finalPassengerCheck(const TSimplePaxItem& pax) const { return true; }
    bool suitable(const TPaxTknItem& tkn) const;
};

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn);
bool LoadPaxTkn(TDateTime part_key, int pax_id, TPaxTknItem &tkn);
bool LoadCrsPaxTkn(int pax_id, TPaxTknItem &tkn);

class TPaxDocCompoundType
{
  public:
    std::string type;
    std::string subtype;
  TPaxDocCompoundType()
  {
    clear();
  }
  void clear()
  {
    type.clear();
    subtype.clear();
  }
  bool empty() const
  {
    return type.empty() &&
           subtype.empty();
  }
  bool equalAttrs(const TPaxDocCompoundType &item) const
  {
    return type == item.type &&
           subtype == item.subtype;
  }

  const TPaxDocCompoundType& toXML(xmlNodePtr node) const;
  const TPaxDocCompoundType& toWebXML(xmlNodePtr node,
                                      const boost::optional<AstraLocale::OutputLang>& lang) const;
  TPaxDocCompoundType& fromXML(xmlNodePtr node);
  TPaxDocCompoundType& fromWebXML(xmlNodePtr node);
  TPaxDocCompoundType& fromMeridianXML(xmlNodePtr node);
  const TPaxDocCompoundType& toDB(TQuery &Qry) const;
  TPaxDocCompoundType& fromDB(DB::TQuery &Qry);
};

class TPaxDocItem : public TPaxAPIItem, public TPaxRemBasic, public TPaxDocCompoundType
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:
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
    }
    void clear()
    {
      TPaxDocCompoundType::clear();
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
    }
    bool empty() const
    {
      return TPaxDocCompoundType::empty() &&
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
    }
    bool equalAttrs(const TPaxDocItem &item) const
    {
      return TPaxDocCompoundType::equalAttrs(item) &&
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
    }
    bool equal(const TPaxDocItem &item) const
    {
      return equalAttrs(item) &&
             scanned_attrs == item.scanned_attrs;
    }
    const TPaxDocItem& toXML(xmlNodePtr node) const;
    const TPaxDocItem& toWebXML(xmlNodePtr node,
                                const boost::optional<AstraLocale::OutputLang>& lang) const;
    TPaxDocItem& fromXML(xmlNodePtr node);
    TPaxDocItem& fromWebXML(xmlNodePtr node);
    TPaxDocItem& fromMeridianXML(xmlNodePtr node);
    const TPaxDocItem& toDB(TQuery &Qry) const;
    TPaxDocItem& fromDB(DB::TQuery &Qry);

    long int getEqualAttrsFieldsMask(const TPaxDocItem &item) const;
    long int getNotEmptyFieldsMask() const;
    TAPIType apiType() const { return apiDoc; }
    std::string rem_code() const
    {
      return "DOCS";
    }
    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const;
    std::string full_name() const;
    bool isNationalRussianPassport() const { return type=="P" && subtype=="N" && issue_country=="RUS"; }
    std::string getSurnameWithInitials() const;

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    void addSearchPaxIds(const PaxOrigin& origin, std::set<int>& searchPaxIds) const;
    bool finalPassengerCheck(const TSimplePaxItem& pax) const { return true; }
    bool suitable(const TPaxDocItem& doc) const;

    static boost::optional<TPaxDocItem> get(const PaxOrigin& origin, const PaxId_t& paxId);
};

class TScannedPaxDocItem : public TPaxDocItem
{
  public:
    std::string extra;
    TScannedPaxDocItem()
    {
      clear();
    }
    void clear()
    {
      TPaxDocItem::clear();
      extra.clear();
    }

    TScannedPaxDocItem& fromXML(xmlNodePtr node);
    std::string getTrueNo() const;

    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    void addSearchPaxIds(const PaxOrigin& origin, std::set<int>& searchPaxIds) const;
    bool finalPassengerCheck(const TSimplePaxItem& pax) const { return true; }
    bool suitable(const TPaxDocItem& doc) const;
};

const std::string DOCO_PSEUDO_TYPE="-";

class TPaxDocoItem : public TPaxAPIItem, public TPaxRemBasic, public TPaxDocCompoundType
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:
    std::string birth_place;
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
    }
    void clear()
    {
      TPaxDocCompoundType::clear();
      birth_place.clear();
      no.clear();
      issue_place.clear();
      issue_date=ASTRA::NoExists;
      expiry_date=ASTRA::NoExists;
      applic_country.clear();
      scanned_attrs=NO_FIELDS;
      doco_confirm=false;
    }
    bool empty() const
    {
      return TPaxDocCompoundType::empty() &&
             birth_place.empty() &&
             no.empty() &&
             issue_place.empty() &&
             issue_date==ASTRA::NoExists &&
             expiry_date==ASTRA::NoExists &&
             applic_country.empty();
    }
    bool equalAttrs(const TPaxDocoItem &item) const
    {
      return TPaxDocCompoundType::equalAttrs(item) &&
             birth_place == item.birth_place &&
             no == item.no &&
             issue_place == item.issue_place &&
             issue_date == item.issue_date &&
             expiry_date == item.expiry_date &&
             applic_country == item.applic_country;
    }
    bool equal(const TPaxDocoItem &item) const
    {
      return equalAttrs(item) &&
             scanned_attrs == item.scanned_attrs;
    }
    const TPaxDocoItem& toXML(xmlNodePtr node) const;
    const TPaxDocoItem& toWebXML(xmlNodePtr node,
                                 const boost::optional<AstraLocale::OutputLang>& lang) const;
    TPaxDocoItem& fromXML(xmlNodePtr node);
    TPaxDocoItem& fromWebXML(xmlNodePtr node);
    TPaxDocoItem& fromMeridianXML(xmlNodePtr node);
    const TPaxDocoItem& toDB(TQuery &Qry) const;
    TPaxDocoItem& fromDB(DB::TQuery &Qry);

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

    static boost::optional<TPaxDocoItem> get(const PaxOrigin& origin, const PaxId_t& paxId);
};

class TPaxDocaItem : public TPaxAPIItem, public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:
    std::string type;
    std::string country;
    std::string address;
    std::string city;
    std::string region;
    std::string postal_code;
    long int scanned_attrs; //всегда NO_FIELDS
    TPaxDocaItem()
    {
      clear();
    };
    TPaxDocaItem(const TAPIType& apiType);
    void clear()
    {
      type.clear();
      country.clear();
      address.clear();
      city.clear();
      region.clear();
      postal_code.clear();
      scanned_attrs=NO_FIELDS;
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
    bool suitableForDB() const
    {
      return apiType()!=apiUnknown && !emptyWithoutType();
    }

    bool emptyWithoutType() const
    {
      return country.empty() &&
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
    bool equalAttrs(const TPaxDocaItem &item) const
    {
      return operator ==(item);
    }
    const TPaxDocaItem& toXML(xmlNodePtr node) const;
    TPaxDocaItem& fromXML(xmlNodePtr node);
    TPaxDocaItem& fromMeridianXML(xmlNodePtr node);
    const TPaxDocaItem& toDB(TQuery &Qry) const;
    TPaxDocaItem& fromDB(DB::TQuery &Qry);

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

class TComplexClass
{
  public:
    std::string subcl;
    std::string cl;
    int cl_grp;
    TComplexClass()
    {
      clear();
    }
    void clear()
    {
      subcl.clear();
      cl.clear();
      cl_grp=ASTRA::NoExists;
    }

    const TComplexClass& toDB(TQuery &Qry, const std::string& fieldPrefix) const;
    TComplexClass& fromDB(TQuery &Qry, const std::string& fieldPrefix);
    const TComplexClass& toXML(xmlNodePtr node, const std::string& fieldPrefix) const;
};



class TSimplePaxItem
{
  public:
    int id; //crs
    int grp_id;
    int pnr_id; //crs
    std::string surname; //crs
    std::string name; //crs
    ASTRA::TPerson pers_type; //crs
    ASTRA::TCrewType::Enum crew_type;
    bool is_jmp;
    std::string seat_no;
    std::string seat_type; //crs
    int seats; //crs
    std::string refuse;
    bool pr_brd;
    bool pr_exam;
    std::string wl_type;
    int reg_no;
    std::string subcl;
    TComplexClass cabin;
    int bag_pool_num;
    int tid;
    TPaxTknItem tkn;
    bool TknExists;
    ASTRA::TGender::Enum gender;
    TSimplePaxItem()
    {
      clear();
    }
    void clear()
    {
      id=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      pnr_id=ASTRA::NoExists;
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      crew_type=ASTRA::TCrewType::Unknown;
      is_jmp=false;
      seat_no.clear();
      seat_type.clear();
      seats=ASTRA::NoExists;
      refuse.clear();
      pr_brd=false;
      pr_exam=false;
      wl_type.clear();
      reg_no=ASTRA::NoExists;
      subcl.clear();
      cabin.clear();
      bag_pool_num=ASTRA::NoExists;
      tid=ASTRA::NoExists;
      tkn.clear();
      TknExists=false;
      gender=ASTRA::TGender::Unknown;
    }

    static ASTRA::TGender::Enum genderFromDB(TQuery &Qry);
    static ASTRA::TTrickyGender::Enum getTrickyGender(ASTRA::TPerson pers_type, ASTRA::TGender::Enum gender);
    static const std::string& origClassFromCrsSQL();
    static const std::string& origSubclassFromCrsSQL();
    static const std::string& cabinClassFromCrsSQL();
    static const std::string& cabinSubclassFromCrsSQL();

    const TSimplePaxItem& toEmulXML(xmlNodePtr node, bool PaxUpdatesPending) const;
    TSimplePaxItem& fromDB(TQuery &Qry);
    TSimplePaxItem& fromDBCrs(TQuery &Qry, bool withTkn);
    bool getByPaxId(int pax_id, TDateTime part_key = ASTRA::NoExists);
    std::string full_name() const;
    bool isCBBG() const;
    bool api_doc_applied() const;
    bool upward_within_bag_pool(const TSimplePaxItem& pax) const;
    bool HaveBaggage() const { return bag_pool_num != ASTRA::NoExists; }
    ASTRA::TTrickyGender::Enum getTrickyGender() const { return getTrickyGender(pers_type, gender); }
    static void UpdTid(int pax_id);
    bool allowToBoarding() const
    {
      return refuse.empty() && !pr_brd;
    }

    bool allowToBagCheckIn() const
    {
      return refuse.empty();
    }
    bool allowToBagRevoke() const
    {
      return true;
    }

    bool isTest() const { return isTestPaxId(id); }
    int paxId() const { return id; }

    std::string checkInStatus() const;

    std::string getCabinClass() const;
    std::string getCabinSubclass() const;
    bool cabinClassToDB() const;
    bool hasCabinSeatNumber() const
    {
      return seats>0 && !is_jmp;
    }
    std::string getSeatNo(const std::string& fmt) const;

    bool getBaggageInHoldTotals(TBagTotals& totals) const;
    boost::optional<WeightConcept::TPaxNormComplex> getRegularNorm() const;
    void getBaggageListForSBDO(TRFISCListWithProps &list) const;
    void getBaggageListForSBDO(TBagTypeList& list) const;

    PaxOrigin origin() const
    {
      if (isTest())
        return paxTest;
      else if (grp_id==ASTRA::NoExists)
        return paxPnl;
      else
        return paxCheckIn;
    }

    static boost::optional<TComplexClass> getCrsClass(const PaxId_t& paxId, bool onlyIfClassChange);
    static boost::optional<TBagQuantity> getCrsBagNorm(const PaxId_t& paxId);

    template <typename T>
    static boost::optional<TBagQuantity> getCrsBagNorm(TQuery &Qry, const T& quantityField, const T& unitField)
    {
      if (!Qry.FieldIsNULL(quantityField) && !Qry.FieldIsNULL(unitField))
        return TBagQuantity(Qry.FieldAsInteger(quantityField),
                            std::string(Qry.FieldAsString(unitField)));
      return {};
    }
};

template <class T>
bool infantsMoreThanAdults(const T& container)
{
  int adult_count=0, without_seat_count=0;
  for(const auto& pax : container)
  {
    if (pax.pers_type==ASTRA::adult) adult_count++;
    if (pax.pers_type==ASTRA::baby && pax.seats==0) without_seat_count++;
  }
  return without_seat_count>adult_count;
}

class TSimplePaxList : public std::list<TSimplePaxItem>
{
  public:
    bool infantsMoreThanAdults() const;
};

class TDocaMap : public std::map<TAPIType, TPaxDocaItem>
{
  public:
    boost::optional<TPaxDocaItem> get(const TAPIType& type) const;
    void get(const TAPIType& type, TPaxDocaItem& doca) const;
    const TDocaMap& toXML(xmlNodePtr node) const;
    TDocaMap& fromXML(xmlNodePtr node);

    static TDocaMap get(const PaxOrigin& origin, const PaxId_t& paxId);
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

    TPaxItem()
    {
      clear();
    }
    void clear()
    {
      TSimplePaxItem::clear();
      doc.clear();
      doco.clear();
      doca_map.clear();
      PaxUpdatesPending=false;
      DocExists=false;
      DocoExists=false;
      DocaExists=false;
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
      doca_map.clear();
    };

    const TAPISItem& toXML(xmlNodePtr node) const;
    TAPISItem& fromDB(int pax_id);
};

int is_female(const std::string &pax_doc_gender, const std::string &pax_name);

class TPaxListItem
{
  public:
    CheckIn::TPaxItem pax;
    bool dont_check_payment;
    std::string crs_seat_no; //crs
    boost::optional<PaxId_t> generatedPaxId; //заполняется только при первоначальной регистрации (new_checkin) и только для NOREC
    boost::optional<PaxId_t> originalCrsPaxId; //заполняется на промежуточных транзитных сегментах для всех, кто не NOREC на исходном сегменте
    bool remsExists;
    std::multiset<CheckIn::TPaxRemItem> rems;
    std::set<CheckIn::TPaxFQTItem> fqts;
    boost::optional< std::list<WeightConcept::TPaxNormItem> > norms;
    xmlNodePtr node;

    TPaxListItem() { clear(); }
    TPaxListItem(const TSimplePaxItem& _pax)
    {
      clear();
      static_cast<TSimplePaxItem&>(pax)=_pax;
    }

    void clear()
    {
      pax.clear();
      dont_check_payment=false;
      crs_seat_no.clear();
      generatedPaxId=boost::none;
      originalCrsPaxId=boost::none;
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

    TPaxListItem& fromXML(xmlNodePtr paxNode, bool trfer_confirm);

    void addFQT(const CheckIn::TPaxFQTItem &fqt);
    void checkFQTTierLevel();
    void checkImportantRems(bool new_checkin, ASTRA::TPaxStatus grp_status);
    int getExistingPaxIdOrSwear() const;
};

class TPaxList : public std::list<CheckIn::TPaxListItem>
{
  public:
    int getBagPoolMainPaxId(int bag_pool_num) const;
};

class TSimplePnrItem
{
  public:
    int id;
    std::string airp_arv;
    std::string cl;
    std::string cabin_cl;
    std::string status;

    TSimplePnrItem() { clear(); }

    void clear()
    {
      id=ASTRA::NoExists;
      airp_arv.clear();
      cl.clear();
      cabin_cl.clear();
      status.clear();
    }

    TSimplePnrItem& fromDB(TQuery &Qry);
    bool getByPaxId(int pax_id);

    int pnrId() const { return id; }
};

class TSimplePaxGrpItem
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
    bool trfer_confirm;
    bool is_mark_norms;
    ASTRA::TClientType client_type;
    int tid;
    bool baggage_pc;      //!!!потом удалить

    TSimplePaxGrpItem() { clear(); }

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
      trfer_confirm=false;
      is_mark_norms=false;
      client_type = ASTRA::ctTypeNum;
      tid=ASTRA::NoExists;

      baggage_pc=false;
    }

    TPaxGrpCategory::Enum grpCategory() const;
    bool is_unaccomp() const
    {
      return grpCategory()==TPaxGrpCategory::UnnacompBag;
    }
    TSimplePaxGrpItem& fromDB(TQuery &Qry);
    const TSimplePaxGrpItem& toXML(xmlNodePtr node) const;
    const TSimplePaxGrpItem& toEmulXML(xmlNodePtr emulReqNode, xmlNodePtr emulSegNode) const;
    bool getByGrpId(int grp_id);
    bool getByPaxId(int pax_id);

    bool allowToBagCheckIn() const { return trfer_confirm; }

    ASTRA::TCompLayerType getCheckInLayerType() const;

    TPaxSegmentPair getSegmentPair() const
    {
      return TPaxSegmentPair(point_dep, airp_arv);
    }
};

std::set<int> loadInfIdSet(int pax_id, bool lock);
std::set<int> loadSeatIdSet(int pax_id, bool lock);

class TPaxGrpItem : public TSimplePaxGrpItem
{
  private:
    TSimplePaxGrpItem& fromDB(TQuery &Qry);
  public:
    bool pc, wt;
    bool rfisc_used;

    boost::optional< std::list<WeightConcept::TPaxNormItem> > norms;
    boost::optional< WeightConcept::TPaidBagList > paid;
    boost::optional< TGroupBagItem > group_bag;
    boost::optional< TGrpServiceList > svc;
    boost::optional< TGrpServiceAutoList > svc_auto;
    boost::optional< TServicePaymentList > payment;

    TPaxGrpItem() { clear(); }

    void clear()
    {
      TSimplePaxGrpItem::clear();

      pc=false;
      wt=false;
      rfisc_used=false;

      norms=boost::none;
      paid=boost::none;
      group_bag=boost::none;
      svc=boost::none;
      svc_auto=boost::none;
      payment=boost::none;
    }

    const TPaxGrpItem& toXML(xmlNodePtr node) const;
    bool fromXML(xmlNodePtr node);
    TPaxGrpItem& fromXMLadditional(xmlNodePtr node, xmlNodePtr firstSegNode, bool is_unaccomp);
    const TPaxGrpItem& toDB(TQuery &Qry) const;
    TPaxGrpItem& fromDBWithBagConcepts(TQuery &Qry);
    bool getByGrpIdWithBagConcepts(int grp_id);
    void SyncServiceAuto(const TTripInfo &flt);
    void checkInfantsCount(const CheckIn::TSimplePaxList &prior_paxs,
                           const CheckIn::TSimplePaxList &curr_paxs) const;

    static void UpdTid(int grp_id);
    static void setRollbackGuaranteedTo(int grp_id, bool value);
    static bool allPassengersRefused(int grp_id);

    TBagConcept::Enum getBagAllowanceType() const
    {
      if ( (!pc && !wt) || (pc && wt) ) return TBagConcept::Unknown;
      return pc? TBagConcept::Piece: TBagConcept::Weight;
    }
};

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc);
bool LoadPaxDoc(TDateTime part_key, int pax_id, TPaxDocItem &doc);
std::string GetPaxDocStr(TDateTime part_key,
                         int pax_id,
                         bool with_issue_country=false,
                         const std::string &lang="");

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc);
bool LoadPaxDoco(TDateTime part_key, int pax_id, TPaxDocoItem &doc);

void ConvertDoca(const TDocaMap& doca_map,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD);

bool LoadPaxDoca(int pax_id, TDocaMap &doca_map);
bool LoadPaxDoca(TDateTime part_key, int pax_id, TDocaMap &doca_map);

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc);
bool LoadCrsPaxDoco(int pax_id, TPaxDocoItem &doc);
bool LoadCrsPaxVisa(int pax_id, TPaxDocoItem &doc); //вызывает LoadCrsPaxDoco, потом проверяет, что type=V, иначе вернет false
bool LoadCrsPaxDoca(int pax_id, TDocaMap &doca_map);

void SavePaxDoc(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const TPaxDocItem& doc,
                ModifiedPaxRem& modifiedPaxRem);
void SavePaxDoc(const PaxIdWithSegmentPair& paxId,
                const TPaxDocItem& doc,
                const TPaxDocItem& priorDoc,
                ModifiedPaxRem& modifiedPaxRem);
void SavePaxDoco(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const TPaxDocoItem& doc,
                 ModifiedPaxRem& modifiedPaxRem);
void SavePaxDoco(const PaxIdWithSegmentPair& paxId,
                 const TPaxDocoItem& doc,
                 const TPaxDocoItem& priorDoc,
                 ModifiedPaxRem& modifiedPaxRem);
void SavePaxDoca(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const TDocaMap& doca,
                 ModifiedPaxRem& modifiedPaxRem);
void SavePaxDoca(const PaxIdWithSegmentPair& paxId,
                 const TDocaMap& doca,
                 const TDocaMap& priorDoca,
                 ModifiedPaxRem& modifiedPaxRem);
void SavePaxFQT(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::set<TPaxFQTItem>& fqts,
                ModifiedPaxRem& modifiedPaxRem);
void SavePaxFQT(const PaxIdWithSegmentPair& paxId,
                const std::set<TPaxFQTItem>& fqts,
                const std::set<TPaxFQTItem>& prior,
                ModifiedPaxRem& modifiedPaxRem);
void SavePaxRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::multiset<TPaxRemItem>& rems,
                ModifiedPaxRem& modifiedPaxRem);
void SavePaxRem(const PaxIdWithSegmentPair& paxId,
                const std::multiset<TPaxRemItem>& rems,
                const std::multiset<TPaxRemItem>& prior,
                ModifiedPaxRem& modifiedPaxRem);

std::string PaxDocGenderNormalize(const std::string &pax_doc_gender);

template<class T>
void CalcGrpEMDProps(const T &prior,
                     const boost::optional< T > &curr,
                     CheckIn::TGrpEMDProps &diff,
                     CheckIn::TGrpEMDProps &props)
{
  diff.clear();
  props.clear();
  if (!curr) return;  //ничего не изменялось
  CheckIn::TGrpEMDProps props1, props2;
  for(const auto& i : prior)
    if (i.isEMD()) props1.emplace(i);
  for(const auto& i : curr.get())
    if (i.isEMD()) props2.emplace(i);

  //в различия попадают и добавленные, и удаленные
  set_symmetric_difference(props1.begin(), props1.end(),
                           props2.begin(), props2.end(),
                           inserter(diff, diff.end()));
  //конструируем props только для удаленных
  set_difference(props1.begin(), props1.end(),
                 props2.begin(), props2.end(),
                 inserter(props, props.end()));
}

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

void GetTCkinPassengers(int pax_id, std::map<int, TSimplePaxItem> &paxs);
void GetTCkinTickets(int pax_id, std::map<int, TCkinPaxTknItem> &tkns);
void GetTCkinTicketsBefore(int pax_id, std::map<int, TCkinPaxTknItem> &tkns);
void GetTCkinTicketsAfter(int pax_id, std::map<int, TCkinPaxTknItem> &tkns);

std::string isFemaleStr( int is_female );

std::string paxDocCountryToWebXML(const std::string &code, const boost::optional<AstraLocale::OutputLang>& lang);

} //namespace CheckIn

namespace Sirena
{

void PaxBrandsNormsToStream(const TTrferRoute &trfer, const CheckIn::TPaxItem &pax, std::ostringstream &s);

} //namespace Sirena

std::string convert_pnr_addr(const std::string &value, bool pr_lat);

class TPnrAddrs;

class TPnrAddrInfo
{
  public:
    enum Format { AddrOnly,
                  AddrAndAirline };

    std::string airline, addr;

    TPnrAddrInfo() {}

    TPnrAddrInfo(const std::string& _airline,
                 const std::string& _addr) :
      airline(_airline), addr(_addr) {}

    void clear()
    {
      airline.clear();
      addr.clear();
    }

    bool operator == (const TPnrAddrInfo &item) const
    {
      return airline==item.airline &&
             convert_pnr_addr(addr, true)==convert_pnr_addr(item.addr, true);
    }

    std::string str(TPnrAddrInfo::Format format,
                    const boost::optional<AstraLocale::OutputLang>& lang = boost::none) const
    {
      std::ostringstream s;
      if (lang)
      {
        s << convert_pnr_addr(addr, lang->isLatin());
        if (format == AddrAndAirline)
          s << "/" << airlineToPrefferedCode(airline, lang.get());
      }
      else
      {
        s << addr;
        if (format == AddrAndAirline)
          s << "/" << airline;
      }
      return s.str();
    }

    const TPnrAddrInfo& toXML(xmlNodePtr addrParentNode,
                              const boost::optional<AstraLocale::OutputLang>& lang=boost::none) const;

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    void addSearchPaxIds(const PaxOrigin& origin, std::set<int>& searchPaxIds) const { return; }
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const { return true; }
    bool suitable(const TPnrAddrInfo& pnr) const;
    bool suitable(const TPnrAddrs& pnrs) const;
};

class TPnrAddrs : public std::vector<TPnrAddrInfo>
{
  public:
    std::string getByPnrId(int pnr_id, std::string &airline);
    std::string getByPaxId(int pax_id, std::string &airline);

    std::string getByPnrId(int pnr_id);
    std::string getByPaxId(int pax_id);

    void getByPnrIdFast(int pnr_id);
    void getByPaxIdFast(int pax_id);

    std::string firstAddrByPnrId(int pnr_id, TPnrAddrInfo::Format format)
    {
      getByPnrId(pnr_id);
      return empty()?"":front().str(format);
    }
    std::string firstAddrByPaxId(int pax_id, TPnrAddrInfo::Format format)
    {
      getByPaxId(pax_id);
      return empty()?"":front().str(format);
    }
    bool equalPnrExists(const TPnrAddrs& pnrAddrs) const
    {
      for(const TPnrAddrInfo& addr : pnrAddrs)
        if (find(begin(), end(), addr)!=end()) return true;
      return false;
    }
    const TPnrAddrs &toXML(xmlNodePtr addrsParentNode) const;
    const TPnrAddrs &toWebXML(xmlNodePtr addrsParentNode,
                              const boost::optional<AstraLocale::OutputLang>& lang=boost::none) const;
    const TPnrAddrs &toSirenaXML(xmlNodePtr addrParentNode,
                                 const AstraLocale::OutputLang& lang) const;

    std::string str(TPnrAddrInfo::Format format,
                    const boost::optional<AstraLocale::OutputLang>& lang = boost::none) const
    {
      std::ostringstream s;
      std::string separator;
      for (const auto& info : *this)
      {
        s << separator << info.str(format, lang);
        separator = ",";
      }
      return s.str();
    }

    const std::string traceStr() const
    {
      std::ostringstream s;
      for(const TPnrAddrInfo& addr : *this)
        s << addr.str(TPnrAddrInfo::AddrAndAirline) << " ";
      return s.str();
    }
};

typedef ASTRA::Cache<int/*grp_id*/, CheckIn::TSimplePaxGrpItem> PaxGrpCache;

#endif


