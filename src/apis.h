#ifndef _APIS_H_
#define _APIS_H_

#include <set>
#include <string>
#include "oralib.h"
#include "astra_consts.h"

namespace APIS
{

const std::set<std::string> &customsUS();

void GetCustomsDependCountries(const std::string &regul,
                               std::set<std::string> &depend,
                               TQuery &Qry);
std::string GetCustomsRegulCountry(const std::string &depend,
                                   TQuery &Qry);

bool isValidGender(const std::string &fmt, const std::string &pax_doc_gender, const std::string &pax_name);
bool isValidDocType(const std::string &fmt, const ASTRA::TPaxStatus &status, const std::string &doc_type);

};

void create_apis_task(int point_id, const std::string& task_name, const std::string &params);
bool create_apis_file(int point_id, const std::string& task_name);
void create_apis_nosir_help(const char *name);
int create_apis_nosir(int argc,char **argv);

#endif

