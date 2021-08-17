#ifndef _TYPEB_ORIGINATORS_H_
#define _TYPEB_ORIGINATORS_H_

#include "astra_consts.h"
#include "date_time.h"
#include "hist.h"

using BASIC::date_time::TDateTime;

namespace TypeB {

void checkSitaAddr(const std::string &str,
                   const std::string &cacheTable,
                   const std::string &cacheField,
                   const std::string &lang);

void modifyOriginator(const RowId_t &id,
                      const TDateTime lastDate,
                      const std::string &lang,
                      const RowId_t &tid = RowId_t(ASTRA::NoExists));

void deleteOriginator(const RowId_t &id,
                      const RowId_t &tid = RowId_t(ASTRA::NoExists));

void addOriginator(RowId_t &id,
                   const std::string &airline,
                   const std::string &airpDep,
                   const std::string &tlgType,
                   const TDateTime firstDate,
                   const TDateTime lastDate,
                   const std::string &addr,
                   const std::string &doubleSign,
                   const std::string &descr,
                   const RowId_t &tid,
                   const std::string &lang);
} // namespace TypeB

#endif
