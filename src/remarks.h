#ifndef _REMARKS_H_
#define _REMARKS_H_

#include <string>
#include "astra_consts.h"
#include "oralib.h"

enum TRemCategory { remTKN, remDOC, remDOCO, remFQT, remUnknown };

TRemCategory getRemCategory( const std::string &rem_code, const std::string &rem_text );
bool isDisabledRemCategory( TRemCategory cat );
bool isDisabledRem( const std::string &rem_code, const std::string &rem_text );

enum TRemEventType {
    retALARM_SS,
    retPNL_SEL,
    retBRD_VIEW,
    retBRD_WARN,
    retRPT_SS,
    retRPT_PM,
    retCKIN_VEW,
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


std::string get_remarks(const TRemGrp &rem_grp, int pax_id, TQuery &Qry, const std::string &term = ", ");

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
    bool operator < (const TPaxRemItem &item) const
    {
      if (priority!=item.priority)
        return (item.priority==ASTRA::NoExists ||
                priority!=ASTRA::NoExists && item.priority<priority);
      if (code!=item.code)
        return code<item.code;
      return text<item.text;
    };
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
    TPaxFQTItem& fromDB(TQuery &Qry);
};

bool LoadPaxRem(int pax_id, std::vector<TPaxRemItem> &rems, TQuery& PaxRemQry);
bool LoadPaxFQT(int pax_id, std::vector<TPaxFQTItem> &fqts, TQuery& PaxFQTQry);

};

#endif

