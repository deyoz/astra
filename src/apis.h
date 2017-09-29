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

#if !USE_NEW_CREATE_APIS

// старая функция, а новая в apis_creator.h
#if APIS_TEST
bool create_apis_file(int point_id, const string& task_name, TApisTestMap* test_map = nullptr);
#else
bool create_apis_file(int point_id, const string& task_name);
#endif

#endif

void create_apis_nosir_help(const char *name);
int create_apis_nosir(int argc,char **argv);

#endif

