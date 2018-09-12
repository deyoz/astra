#ifndef _APIS_H_
#define _APIS_H_

#include <set>
#include <string>
#include "oralib.h"
#include "astra_consts.h"
#include "trip_tasks.h"
#include "apis_creator.h"

namespace APIS
{

const std::set<std::string> &customsUS();

void GetCustomsDependCountries(const std::string &regul,
                               std::set<std::string> &depend,
                               TQuery &Qry);
std::string GetCustomsRegulCountry(const std::string &depend,
                                   TQuery &Qry);

};

void create_apis_task(const TTripTaskKey &task);

void create_apis_nosir_help(const char *name);
int create_apis_nosir(int argc,char **argv);

#endif

