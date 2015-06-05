#ifndef _ASTRA_CONTEXT_H_
#define _ASTRA_CONTEXT_H_

#include "basic.h"
#include "astra_consts.h"
#include "oralib.h"

void longToDB(TQuery &Qry, const std::string &column_name, const std::string &src, int len=4000);

namespace AstraContext
{

int SetContext(const std::string name,
               const int id,
               const std::string &value);

int SetContext(const std::string name,
               const std::string &value);

BASIC::TDateTime GetContext(const std::string name,
                            const int id,
                            std::string &value);

void ClearContext(const std::string name,
                  const BASIC::TDateTime time_create = ASTRA::NoExists);

void ClearContext(const std::string name,
                  const int id);

}; /* namespace AstraContext */



#endif /*_ASTRA_CONTEXT_H_*/


