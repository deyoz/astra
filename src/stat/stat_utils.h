#ifndef _STAT_UTILS_H_
#define _STAT_UTILS_H_

#include <libxml/tree.h>
#include <string>
#include <map>
#include "stat_common.h"

#define WITHOUT_TOTAL_WHEN_PROBLEM false

namespace STAT {
    bool bad_client_img_version();
    xmlNodePtr set_variables(xmlNodePtr resNode, std::string lang = "");
    xmlNodePtr getVariablesNode(xmlNodePtr resNode);
}

template <class keyClass, class rowClass, class cmpClass>
void AddStatRow(const TStatOverflow &overflow, const keyClass &key, const rowClass &row, std::map<keyClass, rowClass, cmpClass> &stat)
{
  typename std::map< keyClass, rowClass, cmpClass >::iterator i = stat.find(key);
  if (i!=stat.end())
    i->second+=row;
  else
  {
      overflow.check(stat.size());
      stat.insert(std::make_pair(key,row));
  };
};

struct TStatPlaces {
    private:
        std::string result;
    public:
        void set(std::string aval, bool pr_locale);
        std::string get() const;
};

enum TScreenState {ssNone,ssLog,ssPaxList,ssFltLog,ssFltTaskLog,ssSystemLog,ssPaxSrc};

class TPointsRow
{
  public:
    BASIC::date_time::TDateTime part_key, real_out_client;
    std::string airline, suffix, name;
    int point_id, flt_no, move_id, point_num;
    bool operator == (const TPointsRow &item) const;
} ;

void GetFltCBoxList(TScreenState scr, BASIC::date_time::TDateTime first_date, BASIC::date_time::TDateTime last_date, bool pr_show_del, std::vector<TPointsRow> &points);

struct TPeriods {
    typedef std::list<std::pair<BASIC::date_time::TDateTime, BASIC::date_time::TDateTime> > TItems;
    TItems items;
    void get(BASIC::date_time::TDateTime FirstDate, BASIC::date_time::TDateTime LastDate);
    void dump();
    void dump(TItems::iterator i);
};

int nosir_months(int argc,char **argv);

#endif
