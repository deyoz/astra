#ifndef _REMARKS_H_
#define _REMARKS_H_

#include <string>
#include <set>
#include "astra_consts.h"
#include "astra_elems.h"
#include "oralib.h"
#include "xml_unit.h"
#include "astra_locale.h"

enum TRemCategory { remTKN, remDOC, remDOCO, remDOCA, remFQT, remASVC, remCREW, remUnknown };

TRemCategory getRemCategory( const std::string &rem_code, const std::string &rem_text );
bool isDisabledRemCategory( TRemCategory cat );
bool isDisabledRem(const std::string &rem_code, const std::string &rem_text);
bool IsReadonlyRem( const std::string &rem_code, const std::string &rem_text );

enum TRemEventType {
    retBP,
    retALARM_SS,
    retPNL_SEL,
    retBRD_VIEW,
    retBRD_WARN,
    retRPT_SS,
    retRPT_PM,
    retCKIN_VIEW,
    retTYPEB_PSM,
    retTYPEB_PIL,
    retSERVICE_STAT,
    retLIMITED_CAPAB_STAT,
    retSELF_CKIN_EXCHANGE,
    retWEB,
    retKIOSK,
    retMOB
};

class TRemGrp : public std::set<std::string>
{
  public:
    TRemGrp() {}
    TRemGrp(std::initializer_list<std::string> l) : std::set<std::string>(l) {}
    bool exists (const std::string &rem) const { return find(rem) != end(); }
    void Load(TRemEventType rem_set_type, int point_id);
    void Load(TRemEventType rem_set_type, const std::string &airline);
};

namespace CheckIn
{

class TPaxRemBasic
{
  public:
    enum TOutput {outputTlg, outputReport};
  protected:
    std::string RemoveTrailingChars(const std::string &str, const std::string &chars) const;
    virtual std::string get_rem_text(bool inf_indicator,
                                     const std::string& lang,
                                     bool strictly_lat,
                                     bool translit_lat,
                                     bool language_lat,
                                     TOutput output
                                     ) const=0;
  public:
    virtual bool empty() const=0;
    std::string rem_text(
            bool inf_indicator,
            const std::string& lang,
            TLangApplying fmt,
            TOutput output = outputTlg) const;
    std::string rem_text(bool inf_indicator) const;
    virtual std::string rem_code() const=0;
    int get_priority() const;
    virtual ~TPaxRemBasic() {}
};

class TPaxRemItem : public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:

    std::string code;
    std::string text;
    int priority;
    TPaxRemItem()
    {
      clear();
    };
    TPaxRemItem(const std::string &rem_code,
                const std::string &rem_text);
    TPaxRemItem(const TPaxRemBasic &basic,
                bool inf_indicator,
                const std::string& lang,
                TLangApplying fmt,
                TOutput output = outputTlg);
    TPaxRemItem(const TPaxRemBasic &basic,
                bool inf_indicator);
    void clear()
    {
      code.clear();
      text.clear();
      priority=ASTRA::NoExists;
    };
    bool empty() const
    {
      return code.empty() &&
             text.empty();
    };
    bool operator == (const TPaxRemItem &item) const
    {
      return code==item.code &&
             text==item.text;
    };
    bool operator < (const TPaxRemItem &item) const
    {
      if (priority!=item.priority)
        return (item.priority==ASTRA::NoExists ||
                (priority!=ASTRA::NoExists && priority<item.priority));
      if (code!=item.code)
        return code<item.code;
      return text<item.text;
    };
    const TPaxRemItem& toXML(xmlNodePtr node) const;
    TPaxRemItem& fromXML(xmlNodePtr node);
    TPaxRemItem& fromWebXML(xmlNodePtr node);
    const TPaxRemItem& toDB(TQuery &Qry) const;
    TPaxRemItem& fromDB(TQuery &Qry);
    void calcPriority();
    std::string rem_code() const
    {
      return code;
    }

    static std::pair<std::string, std::string> getWebXMLTagNames()
    {
      return std::make_pair("rems", "rem");
    }
};

class TPaxFQTCard
{
  public:
    std::string airline;
    std::string no;
    TPaxFQTCard()
    {
      clear();
    }
    void clear()
    {
      airline.clear();
      no.clear();
    }
    bool empty() const
    {
      return airline.empty() &&
             no.empty();
    }
    bool operator < (const TPaxFQTCard &item) const
    {
      if (no!=item.no)
        return no<item.no;
      return airline<item.airline;
    }
    bool operator == (const TPaxFQTCard &item) const
    {
      return airline==item.airline &&
             no==item.no;
    }
    std::string no_str(const std::string &lang) const;
};

class TPaxFQTItem : public TPaxRemBasic, public TPaxFQTCard
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:
    std::string rem;
    std::string extra;
    std::string tier_level;
    bool tier_level_confirm;
    TPaxFQTItem()
    {
      clear();
    }
    void clear()
    {
      TPaxFQTCard::clear();
      rem.clear();
      extra.clear();
      tier_level.clear();
      tier_level_confirm=false;
    }
    bool empty() const
    {
      return TPaxFQTCard::empty() &&
             rem.empty() &&
             extra.empty() &&
             tier_level.empty();
    }
    const TPaxFQTItem& toDB(TQuery &Qry) const;
    TPaxFQTItem& fromDB(TQuery &Qry);
    const TPaxFQTItem& toXML(xmlNodePtr node,
                             const boost::optional<AstraLocale::OutputLang>& lang=boost::none) const;
    TPaxFQTItem& fromXML(xmlNodePtr node);
    TPaxFQTItem& fromWebXML(xmlNodePtr node);
    std::string rem_code() const
    {
      return rem;
    }

    bool operator < (const TPaxFQTItem &item) const
    {
      if (!(TPaxFQTCard::operator ==(item)))
        return TPaxFQTCard::operator <(item);
      return rem<item.rem;
    }

    bool operator == (const TPaxFQTItem &item) const
    {
      return TPaxFQTCard::operator ==(item) &&
             rem==item.rem &&
             extra==item.extra &&
             tier_level==item.tier_level &&
             tier_level_confirm==item.tier_level_confirm;
    }

    std::string logStr(const std::string &lang=AstraLocale::LANG_EN) const;
    bool copyIfBetter(const TPaxFQTItem &item)
    {
      if (!item.tier_level.empty())
      {
        if (tier_level.empty() ||
            (!tier_level.empty() && !tier_level_confirm && item.tier_level_confirm))
        {
          tier_level=item.tier_level;
          tier_level_confirm=item.tier_level_confirm;
          return true;
        }
      }
      return false;
    }

    static std::pair<std::string, std::string> getWebXMLTagNames()
    {
      return std::make_pair("fqt_rems", "fqt_rem");
    }
};

class TServiceBasic
{
  public:
    std::string RFIC;
    std::string RFISC;
    int service_quantity;
    std::string ssr_code;
    std::string service_name;
    std::string emd_type;

    TServiceBasic() { clear(); }
    void clear()
    {
      RFIC.clear();
      RFISC.clear();
      service_quantity=ASTRA::NoExists;
      ssr_code.clear();
      service_name.clear();
      emd_type.clear();
    }
    bool empty() const
    {
      return RFIC.empty() &&
             RFISC.empty() &&
             service_quantity==ASTRA::NoExists &&
             ssr_code.empty() &&
             service_name.empty() &&
             emd_type.empty();
    }

    const TServiceBasic& toDB(TQuery &Qry) const;
    TServiceBasic& fromDB(TQuery &Qry);

    void rcpt_service_types(std::set<ASTRA::TRcptServiceType> &service_types) const;
    bool service_quantity_valid() const { return service_quantity!=ASTRA::NoExists && service_quantity>0; }
};

class TPaxASVCItem : public TPaxRemBasic, public TServiceBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat,
                             TOutput output) const;
  public:
    std::string emd_no;
    int emd_coupon;
    std::string ssr_text;  //дополнительно, вычисляется из ремарок пассажира

    TPaxASVCItem() { clear(); }
    void clear()
    {
      TServiceBasic::clear();
      emd_no.clear();
      emd_coupon=ASTRA::NoExists;
      ssr_text.clear();
    }
    bool empty() const
    {
      return TServiceBasic::empty() &&
             emd_no.empty() &&
             emd_coupon==ASTRA::NoExists &&
             ssr_text.empty();
    }
    bool operator < (const TPaxASVCItem &item) const
    {
      if (emd_no!=item.emd_no)
        return emd_no<item.emd_no;
      if (emd_coupon!=item.emd_coupon)
        return (item.emd_coupon==ASTRA::NoExists ||
                (emd_coupon!=ASTRA::NoExists && emd_coupon<item.emd_coupon));
      return false;
    }
    bool operator == (const TPaxASVCItem &item) const
    {
        return
            emd_no == item.emd_no and
            emd_coupon == item.emd_coupon;
    }
    bool sameDoc(const TPaxASVCItem &item) const
    {
      return operator ==(item);
    }
    const TPaxASVCItem& toXML(xmlNodePtr node) const;
    const TPaxASVCItem& toDB(TQuery &Qry) const;
    TPaxASVCItem& fromDB(TQuery &Qry);
    std::string rem_code() const
    {
      return "ASVC";
    }
    std::string no_str() const;
};

typedef std::multiset<TPaxRemItem> PaxRems;
bool LoadPaxRem(int pax_id, std::multiset<TPaxRemItem> &rems);
bool LoadCrsPaxRem(int pax_id, std::multiset<TPaxRemItem> &rems);
bool LoadPaxFQT(int pax_id, std::set<TPaxFQTItem> &fqts);
bool LoadCrsPaxFQT(int pax_id, std::set<TPaxFQTItem> &fqts);

void SavePaxRem(int pax_id, const std::multiset<TPaxRemItem> &rems);
void SavePaxFQT(int pax_id, const std::set<TPaxFQTItem> &fqts);

typedef std::map<TPaxFQTCard, TPaxFQTItem> TPaxFQTCards;
void GetPaxFQTCards(const std::set<TPaxFQTItem> &fqts, TPaxFQTCards &cards);
void SyncFQTTierLevel(int pax_id, bool from_crs, std::set<TPaxFQTItem> &fqts);

bool needTryCheckinServicesAuto(int id, bool is_grp_id);
void setSyncEmdsFlag(int id, bool is_grp_id, bool flag);
bool SyncPaxASVC(int pax_id);
bool DeletePaxASVC(int pax_id);
bool AddPaxASVC(int id, bool is_grp_id);
bool LoadPaxASVC(int pax_id, std::vector<TPaxASVCItem> &asvc);
bool LoadCrsPaxASVC(int pax_id, std::vector<TPaxASVCItem> &asvc);

void GetPaxRemDifference(const boost::optional<TRemGrp> &rem_grp,
                         const PaxRems &prior_rems,
                         const PaxRems &curr_rems,
                         PaxRems &added,
                         PaxRems &deleted,
                         std::list<std::pair<TPaxRemItem, TPaxRemItem>> &modified);

void GetPaxRemDifference(const boost::optional<TRemGrp> &rem_grp,
                         const PaxRems &prior_rems,
                         const PaxRems &curr_rems,
                         PaxRems &added,
                         PaxRems &deleted);

void SyncPaxRemOrigin(const boost::optional<TRemGrp> &rem_grp,
                      const int &pax_id,
                      const std::multiset<TPaxRemItem> &prior_rems,
                      const std::multiset<TPaxRemItem> &curr_rems,
                      const int &user_id,
                      const std::string &desk);

void PaxRemAndASVCFromDB(int pax_id,
                         bool from_crs,
                         const boost::optional<std::set<TPaxFQTItem>>& add_fqts,
                         std::multiset<TPaxRemItem> &rems_and_asvc);

void PaxFQTFromDB(int pax_id,
                  bool from_crs,
                  std::set<TPaxFQTItem> &fqts);

void PaxRemAndASVCToXML(const std::multiset<TPaxRemItem> &rems_and_asvc,
                        xmlNodePtr node);

void PaxFQTToXML(const std::set<TPaxFQTItem> &fqts,
                 xmlNodePtr node);

};

CheckIn::TPaxRemItem getAPPSRem(const int pax_id, const std::string &lang );
void GetRemarks(int pax_id, const std::string &lang, std::multiset<CheckIn::TPaxRemItem> &rems);
std::string GetRemarkStr(const TRemGrp &rem_grp, const std::multiset<CheckIn::TPaxRemItem> &rems, const std::string &term = " ");
std::string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &lang, const std::string &term = " ");
std::string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &term = " ");
std::string GetRemarkMSGText(int pax_id, const std::string &rem_msg);


CheckIn::TPaxRemItem CalcCrewRem(const ASTRA::TPaxStatus grp_status,
                                 const ASTRA::TCrewType::Enum crew_type);
CheckIn::TPaxRemItem CalcJmpRem(const ASTRA::TPaxStatus grp_status,
                                const bool is_jmp);

#endif

