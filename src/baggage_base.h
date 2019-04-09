#ifndef _BAGGAGE_BASE_H_
#define _BAGGAGE_BASE_H_

#include <list>

#include <etick/tick_data.h>
#include "astra_consts.h"
#include "astra_misc.h"
#include "oralib.h"
#include "xml_unit.h"
#include "date_time.h"

using BASIC::date_time::TDateTime;

class TServiceCategory
{
  public:
    enum Enum
    {
      Other=0,
      BaggageInHold=1,
      BaggageInCabinOrCarryOn=2,
      BaggageAndCarryOn=3,
      BaggageInHoldWithOrigInfo=4,
      BaggageInCabinOrCarryOnWithOrigInfo=5,
      BaggageAndCarryOnWithOrigInfo=6
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Other,                               "Other"));
        l.push_back(std::make_pair(BaggageInHold,                       "BaggageInHold"));
        l.push_back(std::make_pair(BaggageInCabinOrCarryOn,             "BaggageInCabinOrCarryOn"));
        l.push_back(std::make_pair(BaggageAndCarryOn,                   "BaggageAndCarryOn"));
        l.push_back(std::make_pair(BaggageInHoldWithOrigInfo,           "BaggageInHoldWithOrigInfo"));
        l.push_back(std::make_pair(BaggageInCabinOrCarryOnWithOrigInfo, "BaggageInCabinOrCarryOnWithOrigInfo"));
        l.push_back(std::make_pair(BaggageAndCarryOnWithOrigInfo,       "BaggageAndCarryOnWithOrigInfo"));
      }
      return l;
    }
};

class TServiceCategories : public ASTRA::PairList<TServiceCategory::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TServiceCategories"; }
  public:
    TServiceCategories() : ASTRA::PairList<TServiceCategory::Enum, std::string>(TServiceCategory::pairs(),
                                                                                    boost::none,
                                                                                    boost::none) {}
};

const TServiceCategories& ServiceCategories();

class TBagConcept
{
  public:
    enum Enum
    {
      Unknown,
      Piece,
      Weight,
      No
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Unknown, "unknown"));
        l.push_back(std::make_pair(Piece,   "piece"));
        l.push_back(std::make_pair(Weight,  "weight"));
        l.push_back(std::make_pair(No,      "no"));
      }
      return l;
    }
};

class TBagConcepts : public ASTRA::PairList<TBagConcept::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TBagConcepts"; }
  public:
    TBagConcepts() : ASTRA::PairList<TBagConcept::Enum, std::string>(TBagConcept::pairs(),
                                                                     boost::none,
                                                                     boost::none) {}
};

const TBagConcepts& BagConcepts();

class TBagUnit
{
  private:
    Ticketing::Baggage::Baggage_t unit;
  public:
    TBagUnit() : unit(Ticketing::Baggage::Nil) {}
    TBagUnit(const Ticketing::Baggage::Baggage_t &value) : unit(value) {}
    TBagUnit(const std::string &value) { set(value); }
    void clear()
    {
      unit=Ticketing::Baggage::Nil;
    }
    bool empty() const
    {
      return unit==Ticketing::Baggage::Nil;
    }
    void set(const Ticketing::Baggage::Baggage_t &value);
    void set(const std::string &value);
    Ticketing::Baggage::Baggage_t get() const;
    std::string get_db_form() const;
    std::string get_lexeme_form() const;
};

class TBagQuantity
{
  private:
    int quantity;
    TBagUnit unit;
  public:
    TBagQuantity(const int& _quantity, const TBagUnit& _unit) :
      quantity(_quantity), unit(_unit) {}
    std::string view(const AstraLocale::OutputLang &lang, const bool &unitRequired=true) const;
    Ticketing::Baggage::Baggage_t getUnit() const { return unit.get(); }
    int getQuantity() const { return quantity; }
    bool empty() const { return unit.empty(); }
    bool zero() const { return quantity==0; }

    bool operator == (const TBagQuantity &item) const
    {
      return quantity==item.quantity &&
             unit.get()==item.unit.get();
    }

    TBagQuantity& operator += (const TBagQuantity &item);
};

class TBagPieces : public TBagQuantity
{
  public:
    TBagPieces(const int& _quantity) :
      TBagQuantity(_quantity, Ticketing::Baggage::NumPieces) {}
};

class TBagKilos : public TBagQuantity
{
  public:
    TBagKilos(const int& _quantity) :
      TBagQuantity(_quantity, Ticketing::Baggage::WeightKilo) {}
};

class TBagTotals
{
  public:
    int amount;
    int weight;

    TBagTotals() { clear(); }
    TBagTotals(int _amount, int _weight) : amount(_amount), weight(_weight) {}

    void clear()
    {
      amount=0;
      weight=0;
    }
};

namespace Sirena
{

class TLocaleTextItem
{
  public:
    std::string lang, text;
};

class TLocaleTextMap : public std::map<std::string/*lang*/, TLocaleTextItem>
{
  public:
    TLocaleTextMap& fromSirenaXML(xmlNodePtr node);
    TLocaleTextMap& add(const TLocaleTextMap &_map);
};

class TSimplePaxNormItem : public TLocaleTextMap
{
  public:
    bool carry_on;
    TBagConcept::Enum concept;
    std::string airline;
    TSimplePaxNormItem& fromSirenaXML(xmlNodePtr node);
    void fromSirenaXMLAdv(xmlNodePtr node, bool carry_on);

    const TSimplePaxNormItem& toDB(TQuery &Qry) const;
    TSimplePaxNormItem& fromDB(TQuery &Qry);

    TSimplePaxNormItem()
    {
      clear();
    }
    void clear()
    {
      carry_on=false;
      concept=TBagConcept::Unknown;
      airline.clear();
      TLocaleTextMap::clear();
    }
};

class TSimplePaxBrandItem : public TLocaleTextMap
{
  public:
    void fromSirenaXMLAdv(xmlNodePtr node);
};

class TPaxSegKey
{
  public:
    int pax_id, trfer_num;
    TPaxSegKey()
    {
      clear();
    }
    TPaxSegKey(int _pax_id, int _trfer_num)
    {
      pax_id=_pax_id;
      trfer_num=_trfer_num;
    }
    void clear()
    {
      pax_id=ASTRA::NoExists;
      trfer_num=ASTRA::NoExists;
    }
    bool operator == (const TPaxSegKey &key) const
    {
      return pax_id==key.pax_id &&
             trfer_num==key.trfer_num;
    }
    bool operator < (const TPaxSegKey &key) const
    {
      if (pax_id!=key.pax_id)
        return pax_id<key.pax_id;
      return trfer_num<key.trfer_num;
    }

    const TPaxSegKey& toSirenaXML(xmlNodePtr node) const;
    TPaxSegKey& fromSirenaXML(xmlNodePtr node);

    const TPaxSegKey& toDB(TQuery &Qry) const;
    TPaxSegKey& fromDB(TQuery &Qry);
    const TPaxSegKey& toXML(xmlNodePtr node) const;
    TPaxSegKey& fromXML(xmlNodePtr node);
};

class TPaxNormListKey : public TPaxSegKey
{
  public:
    bool carry_on;
    TPaxNormListKey()
    {
      clear();
    }
    TPaxNormListKey(const TPaxSegKey &key, bool _carry_on) : TPaxSegKey(key.pax_id, key.trfer_num), carry_on(_carry_on) {}

    void clear()
    {
      TPaxSegKey::clear();
      carry_on=false;
    }
    bool operator < (const TPaxNormListKey &key) const
    {
      if (!(TPaxSegKey::operator ==(key)))
        return TPaxSegKey::operator <(key);
      return carry_on<key.carry_on;
    }

    TPaxNormListKey& fromSirenaXML(xmlNodePtr node);

    const TPaxNormListKey& toDB(TQuery &Qry) const;
    TPaxNormListKey& fromDB(TQuery &Qry);
};

typedef std::map<TPaxNormListKey, TSimplePaxNormItem> TPaxNormList;

class TPaxNormItem : public TSimplePaxNormItem, public TPaxSegKey
{
  public:
    void clear()
    {
      TSimplePaxNormItem::clear();
      TPaxSegKey::clear();
    }
    const TPaxNormItem& toDB(TQuery &Qry) const
    {
      TSimplePaxNormItem::toDB(Qry);
      TPaxSegKey::toDB(Qry);
      return *this;
    }

    TPaxNormItem& fromDB(TQuery &Qry)
    {
      TSimplePaxNormItem::fromDB(Qry);
      TPaxSegKey::fromDB(Qry);
      return *this;
    }
};

typedef std::map<TPaxSegKey, TSimplePaxBrandItem> TPaxBrandList;

class TPaxBrandItem : public TSimplePaxBrandItem, public TPaxSegKey
{
  public:
    void clear()
    {
      TSimplePaxBrandItem::clear();
      TPaxSegKey::clear();
    }
};

void PaxNormsFromDB(int pax_id, TPaxNormList &norms);
void PaxBrandsFromDB(int pax_id, TPaxBrandList &brands);
void PaxNormsToDB(const TCkinGrpIds &tckin_grp_ids, const std::list<TPaxNormItem> &norms);
void PaxBrandsToDB(const TCkinGrpIds &tckin_grp_ids, const std::list<TPaxBrandItem> &norms);

} //namespace Sirena

std::string BagTypeFromXML(const std::string& bag_type);
std::string BagTypeFromDB(TQuery &Qry);
void BagTypeToDB(TQuery &Qry, const std::string& bag_type, const std::string &where);

int get_max_tckin_num(int grp_id);

#endif
