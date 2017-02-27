#ifndef _ASTRA_CONTEXT_H_
#define _ASTRA_CONTEXT_H_

#include "date_time.h"
#include "astra_consts.h"
#include "oralib.h"

using BASIC::date_time::TDateTime;

namespace AstraContext
{

int SetContext(const std::string &name,
               const int id,
               const std::string &value);

int SetContext(const std::string &name,
               const std::string &value);

TDateTime GetContext(const std::string &name,
                            const int id,
                            std::string &value);

void ClearContext(const std::string &name,
                  const TDateTime time_create = ASTRA::NoExists);

void ClearContext(const std::string &name,
                  const int id);

}; /* namespace AstraContext */



#endif /*_ASTRA_CONTEXT_H_*/


