#ifndef _REMARKS_H_
#define _REMARKS_H_

#include <string>
#include <set>
#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "astra_locale.h"

enum TRemCategory { remTKN, remDOC, remDOCO, remDOCA, remFQT, remASVC, remUnknown };

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
    retWEB,
    retKIOSK,
    retMOB
};

class TRemGrp : public std::set<std::string>
{
  public:
    bool exists (const std::string &rem) const { return find(rem) != end(); }
    void Load(TRemEventType rem_set_type, int point_id);
    void Load(TRemEventType rem_set_type, const std::string &airline);
};

namespace CheckIn
{

class TPaxRemBasic
{
  protected:
    std::string RemoveTrailingChars(const std::string &str, const std::string &chars) const;
    virtual std::string get_rem_text(bool inf_indicator,
                                     const std::string& lang,
                                     bool strictly_lat,
                                     bool translit_lat,
                                     bool language_lat) const=0;
  public:
    virtual bool empty() const=0;
    std::string rem_text(bool inf_indicator, const std::string& lang, TLangApplying fmt) const;
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
                             bool language_lat) const;
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
                bool inf_indicator, const std::string& lang, TLangApplying fmt);
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
    const TPaxRemItem& toDB(TQuery &Qry) const;
    TPaxRemItem& fromDB(TQuery &Qry);
    void calcPriority();
    std::string rem_code() const
    {
      return code;
    }
};

class TPaxFQTItem : public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string rem;
    std::string airline;
    std::string no;
    std::string extra;
    std::string tier_level;
    bool tier_level_confirm;
    TPaxFQTItem()
    {
      clear();
    };
    void clear()
    {
      rem.clear();
      airline.clear();
      no.clear();
      extra.clear();
      tier_level.clear();
      tier_level_confirm=false;
    };
    bool empty() const
    {
      return rem.empty() &&
             airline.empty() &&
             no.empty() &&
             extra.empty() &&
             tier_level.empty();
    };
    const TPaxFQTItem& toDB(TQuery &Qry) const;
    TPaxFQTItem& fromDB(TQuery &Qry);
    const TPaxFQTItem& toXML(xmlNodePtr node) const;
    TPaxFQTItem& fromXML(xmlNodePtr node);
    std::string rem_code() const
    {
      return rem;
    }
};

class TPaxASVCItem : public TPaxRemBasic
{
  protected:
    std::string get_rem_text(bool inf_indicator,
                             const std::string& lang,
                             bool strictly_lat,
                             bool translit_lat,
                             bool language_lat) const;
  public:
    std::string RFIC;
    std::string RFISC;
    std::string ssr_code;
    std::string service_name;
    std::string emd_type;
    std::string emd_no;
    int emd_coupon;
    std::string ssr_text;  //дополнительно, вычисляется из ремарок пассажира
    TPaxASVCItem()
    {
      clear();
    };
    void clear()
    {
      RFIC.clear();
      RFISC.clear();
      ssr_code.clear();
      service_name.clear();
      emd_type.clear();
      emd_no.clear();
      emd_coupon=ASTRA::NoExists;
      ssr_text.clear();
    };
    bool empty() const
    {
      return RFIC.empty() &&
             RFISC.empty() &&
             ssr_code.empty() &&
             service_name.empty() &&
             emd_type.empty() &&
             emd_no.empty() &&
             emd_coupon==ASTRA::NoExists &&
             ssr_text.empty();
    };
    bool operator < (const TPaxASVCItem &item) const
    {
      if (emd_no!=item.emd_no)
        return emd_no<item.emd_no;
      if (emd_coupon!=item.emd_coupon)
        return (item.emd_coupon==ASTRA::NoExists ||
                (emd_coupon!=ASTRA::NoExists && emd_coupon<item.emd_coupon));
      return false;
    };
    bool operator == (const TPaxASVCItem &item) const
    {
        return
            emd_no == item.emd_no and
            emd_coupon == item.emd_coupon;
    }
    const TPaxASVCItem& toXML(xmlNodePtr node) const;
    const TPaxASVCItem& toDB(TQuery &Qry) const;
    TPaxASVCItem& fromDB(TQuery &Qry);
    std::string rem_code() const
    {
      return "ASVC";
    }
    std::string no_str() const;
    void rcpt_service_types(std::set<ASTRA::TRcptServiceType> &service_types) const;
};

bool LoadPaxRem(int pax_id, std::vector<TPaxRemItem> &rems, bool withFQT=false);
bool LoadCrsPaxRem(int pax_id, std::vector<TPaxRemItem> &rems);
bool LoadPaxFQT(int pax_id, std::vector<TPaxFQTItem> &fqts);
bool LoadCrsPaxFQT(int pax_id, std::vector<TPaxFQTItem> &fqts);
bool LoadPaxASVC(int pax_id, std::vector<TPaxASVCItem> &asvc);
bool LoadCrsPaxASVC(int pax_id, std::vector<TPaxASVCItem> &asvc);

void SavePaxRem(int pax_id, const std::vector<TPaxRemItem> &rems);
void SyncFQTTierLevel(int pax_id, bool from_crs, std::vector<TPaxFQTItem> &fqts);
void SavePaxFQT(int pax_id, const std::vector<TPaxFQTItem> &fqts);

bool SyncPaxASVC(int id, bool is_grp_id);

void GetPaxRemDifference(const boost::optional<TRemGrp> &rem_grp,
                         const std::vector<TPaxRemItem> &prior_rems,
                         const std::vector<TPaxRemItem> &curr_rems,
                         std::multiset<TPaxRemItem> &added,
                         std::multiset<TPaxRemItem> &deleted);

void SyncPaxRemOrigin(const boost::optional<TRemGrp> &rem_grp,
                      const int &pax_id,
                      const std::vector<TPaxRemItem> &prior_rems,
                      const std::vector<TPaxRemItem> &curr_rems,
                      const int &user_id,
                      const std::string &desk);

void PaxRemAndASVCFromDB(int pax_id,
                         bool from_crs,
                         std::vector<TPaxRemItem> &rems_and_asvc);

void PaxFQTFromDB(int pax_id,
                  bool from_crs,
                  std::vector<TPaxFQTItem> &fqts);

void PaxRemAndASVCToXML(const std::vector<TPaxRemItem> &rems_and_asvc,
                        xmlNodePtr node);

void PaxFQTToXML(const std::vector<TPaxFQTItem> &fqts,
                 xmlNodePtr node);

};

CheckIn::TPaxRemItem getAPPSRem(const int pax_id, const std::string &lang );
std::string GetRemarkStr(const TRemGrp &rem_grp, const std::vector<CheckIn::TPaxRemItem> &rems, const std::string &term = " ");
std::string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &lang, const std::string &term = " ");
std::string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &term = " ");

#endif

