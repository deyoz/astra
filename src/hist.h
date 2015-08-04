#ifndef HIST_H
#define HIST_H

#include <string>
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"

enum { OpenTimeShift, OpenUserShift, OpenDeskShift, CloseTimeShift, CloseUserShift, CloseDeskShift, InfoLength };

struct QryInfo {
  std::string fields, conditions, source;
};

struct HistTableInfo {
  std::string table_name, ident_fld, table_id, pseudo_name;
};

struct HistParams {
  std::vector<HistTableInfo> tables_info;
  std::vector<std::string> flds_no_show;
};

class THistCacheTable : public TCacheTable {

    QryInfo info;
    HistParams params;
    bool search_by_id;
    bool search_by_dates;

    void initFields();
    void makeSelectQuery(std::string& select_sql);
    bool parseSelectStmt(const std::string& select_sql);
    void prepareQry(std::string& select_sql);
    void eraseFlds();
    bool getHistInfo();
    std::string simpleQry();
    std::string multipleQry();
    std::string exclusionQry();

    bool sameBase(TTable::iterator previos, TTable::iterator next, const int num_cols) const;

  public:
    void Init(xmlNodePtr cacheNode);
    void DoAfter();
};

#endif // HIST_H
