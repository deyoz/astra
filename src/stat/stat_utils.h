#ifndef _STAT_UTILS_H_
#define _STAT_UTILS_H_

#include <libxml/tree.h>
#include <string>
#include <map>
#include "stat_common.h"

namespace STAT {
    bool bad_client_img_version();
    xmlNodePtr set_variables(xmlNodePtr resNode, std::string lang = "");
    xmlNodePtr getVariablesNode(xmlNodePtr resNode);
}

// TODO переименовать здесь и всюду bool full в что-то типа override_MAX_STAT_ROWS()
template <class keyClass, class rowClass, class cmpClass>
void AddStatRow(const keyClass &key, const rowClass &row, std::map<keyClass, rowClass, cmpClass> &stat, bool full)
{
  typename std::map< keyClass, rowClass, cmpClass >::iterator i = stat.find(key);
  if (i!=stat.end())
    i->second+=row;
  else
  {
    if ((not full) and (stat.size() > (size_t)MAX_STAT_ROWS()))
      throw AstraLocale::MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", AstraLocale::LParams() << AstraLocale::LParam("num", MAX_STAT_ROWS()));
    if (full or stat.size()<=(size_t)MAX_STAT_ROWS())
      stat.insert(std::make_pair(key,row));
  };
};


#endif
