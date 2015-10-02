#ifndef _PASSENGER_H_
#define _PASSENGER_H_

#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "baggage.h"

const long int NO_FIELDS=0x0000;

namespace CheckIn
{

class TPaxTknItem
{
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
    const TPaxTknItem& toXML(xmlNodePtr node) const;
    TPaxTknItem& fromXML(xmlNodePtr node);
    const TPaxTknItem& toDB(TQuery &Qry) const;
    TPaxTknItem& fromDB(TQuery &Qry);
    
    long int getNotEmptyFieldsMask() const;
};

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn);
bool LoadPaxTkn(BASIC::TDateTime part_key, int pax_id, TPaxTknItem &tkn);
bool LoadCrsPaxTkn(int pax_id, TPaxTknItem &tkn);

class TPaxDocItem
{
  public:
    std::string type;
    std::string issue_country;
    std::string no;
    std::string nationality;
    BASIC::TDateTime birth_date;
    std::string gender;
    BASIC::TDateTime expiry_date;
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
    
    long int getNotEmptyFieldsMask() const;
    long int getEqualAttrsFieldsMask(const TPaxDocItem &item) const;
};

const std::string DOCO_PSEUDO_TYPE="-";

class TPaxDocoItem
{  
  public:
    std::string birth_place;
    std::string type;
    std::string no;
    std::string issue_place;
    BASIC::TDateTime issue_date;
    BASIC::TDateTime expiry_date;
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
    
    long int getNotEmptyFieldsMask() const;
    long int getEqualAttrsFieldsMask(const TPaxDocoItem &item) const;
    void ReplacePunctSymbols();
};

class TPaxDocaItem
{
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

    long int getNotEmptyFieldsMask() const;
    long int getEqualAttrsFieldsMask(const TPaxDocaItem &item) const;
    void ReplacePunctSymbols();
};

class TPaxItem
{
  public:
    int id;
    std::string surname;
    std::string name;
    ASTRA::TPerson pers_type;
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
    TPaxDocItem doc;
    TPaxDocoItem doco;
    std::list<TPaxDocaItem> doca;
    bool PaxUpdatesPending;
    bool TknExists;
    bool DocExists;
    bool DocoExists;
    bool DocaExists;    
    TPaxItem()
    {
      clear();
    };
    void clear()
    {
      id=ASTRA::NoExists;
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
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
      doc.clear();
      doco.clear();
      doca.clear();
      PaxUpdatesPending=false;
      TknExists=false;
      DocExists=false;
      DocoExists=false;
      DocaExists=false;      
    };

    const TPaxItem& toXML(xmlNodePtr node) const;
    TPaxItem& fromXML(xmlNodePtr node);
    const TPaxItem& toDB(TQuery &Qry) const;
    TPaxItem& fromDB(TQuery &Qry);
    int is_female() const;
    std::string full_name() const;
};

class TAPISItem
{
  public:
    TPaxDocItem doc;
    TPaxDocoItem doco;
    std::list<TPaxDocaItem> doca;
    TAPISItem()
    {
      clear();
    };
    void clear()
    {
      doc.clear();
      doco.clear();
      doca.clear();
    };

    const TAPISItem& toXML(xmlNodePtr node) const;
    TAPISItem& fromDB(int pax_id);
};

int is_female(const std::string &pax_doc_gender, const std::string &pax_name);

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
    int excess;
    int hall;
    std::string bag_refuse;
    int tid;
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
      excess=ASTRA::NoExists;
      hall=ASTRA::NoExists;
      bag_refuse.clear();
      tid=ASTRA::NoExists;
    };

    const TPaxGrpItem& toXML(xmlNodePtr node) const;
    TPaxGrpItem& fromXML(xmlNodePtr node);
    TPaxGrpItem& fromXMLadditional(xmlNodePtr node);
    const TPaxGrpItem& toDB(TQuery &Qry) const;
    TPaxGrpItem& fromDB(TQuery &Qry);
};

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc);
bool LoadPaxDoc(BASIC::TDateTime part_key, int pax_id, TPaxDocItem &doc);
std::string GetPaxDocStr(BASIC::TDateTime part_key,
                         int pax_id,
                         bool with_issue_country=false,
                         const std::string &lang="");

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc);
bool LoadPaxDoco(BASIC::TDateTime part_key, int pax_id, TPaxDocoItem &doc);
enum TDocaType
{
  docaDestination,
  docaResidence,
  docaBirth
};

void ConvertDoca(const std::list<TPaxDocaItem> &doca,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD);

bool LoadPaxDoca(int pax_id, std::list<TPaxDocaItem> &doca);
bool LoadPaxDoca(int pax_id, TDocaType type, TPaxDocaItem &doca);
bool LoadPaxDoca(BASIC::TDateTime part_key, int pax_id, std::list<TPaxDocaItem> &doca);
bool LoadPaxDoca(BASIC::TDateTime part_key, int pax_id, TDocaType type, TPaxDocaItem &doca);

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc, bool without_inf_indicator=false);
bool LoadCrsPaxVisa(int pax_id, TPaxDocoItem &doc);
bool LoadCrsPaxDoca(int pax_id, std::list<TPaxDocaItem> &doca);

void SavePaxDoc(int pax_id, const TPaxDocItem &doc, TQuery& PaxDocQry);
void SavePaxDoco(int pax_id, const TPaxDocoItem &doc, TQuery& PaxDocQry);
void SavePaxDoca(int pax_id, const std::list<TPaxDocaItem> &doca, TQuery& PaxDocaQry, bool new_checkin);

bool LoadPaxNorms(int pax_id, std::vector< std::pair<TPaxNormItem, TNormItem> > &norms);
bool LoadGrpNorms(int grp_id, std::vector< std::pair<TPaxNormItem, TNormItem> > &norms);
void LoadNorms(xmlNodePtr node, bool pr_unaccomp);
void SaveNorms(xmlNodePtr node, bool pr_unaccomp);

}; //namespace CheckIn

#endif


