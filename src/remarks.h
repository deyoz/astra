#ifndef _REMARKS_H_
#define _REMARKS_H_

#include <string>
#include <set>
#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"

enum TRemCategory { remTKN, remDOC, remDOCO, remDOCA, remFQT, remUnknown };

TRemCategory getRemCategory( const std::string &rem_code, const std::string &rem_text );
bool isDisabledRemCategory( TRemCategory cat );
bool isDisabledRem( const std::string &rem_code, const std::string &rem_text );

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
    retTYPEB_PIL
};

struct TRemGrp:private std::vector<std::string> {
    private:
        bool any;
    public:
        TRemGrp(): any(false) {};
        bool exists (const std::string &rem) const { return any or find(begin(), end(), rem) != end(); }
        void Load(TRemEventType rem_set_type, int point_id);
        void Load(TRemEventType rem_set_type, const std::string &airline);
        void Clear() { clear(); any = false; };
};

namespace CheckIn
{

class TPaxRemItem
{
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
};

class TPaxFQTItem
{
  public:
    std::string rem;
    std::string airline;
    std::string no;
    std::string extra;
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
    };
    bool empty() const
    {
      return rem.empty() &&
             airline.empty() &&
             no.empty() &&
             extra.empty();
    };
    const TPaxFQTItem& toDB(TQuery &Qry) const;
    TPaxFQTItem& fromDB(TQuery &Qry);
};

bool LoadPaxRem(int pax_id, bool withFQTcat, std::vector<TPaxRemItem> &rems);
bool LoadCrsPaxRem(int pax_id, std::vector<TPaxRemItem> &rems);
bool LoadPaxFQT(int pax_id, std::vector<TPaxFQTItem> &fqts);

void SavePaxRem(int pax_id, const std::vector<TPaxRemItem> &rems);
void SavePaxFQT(int pax_id, const std::vector<TPaxFQTItem> &fqts);

};

std::string GetRemarkStr(const TRemGrp &rem_grp, const std::vector<CheckIn::TPaxRemItem> &rems, const std::string &term = " ");
std::string GetRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &term = " ");
std::string GetCrsRemarkStr(const TRemGrp &rem_grp, int pax_id, const std::string &term = " ");

#endif

