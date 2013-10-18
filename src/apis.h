#ifndef _APIS_H_
#define _APIS_H_

#include <set>
#include <string>

const std::string BEFORE_TAKEOFF_30_US_ARRIVAL = "BEFORE_TAKEOFF_30_US_ARRIVAL";
const std::string BEFORE_TAKEOFF_60_US_ARRIVAL = "BEFORE_TAKEOFF_60_US_ARRIVAL";
const std::string BEFORE_TAKEOFF_70_US_ARRIVAL = "BEFORE_TAKEOFF_70_US_ARRIVAL";
const std::string TAKEOFF = "TAKEOFF";

class TCountriesUS : public std::set<std::string>
{
  public:
    TCountriesUS()
    {
      insert("Éì");
      insert("åè");
      insert("ûë");
    };
};

static TCountriesUS countriesUS;

void create_apis_file(int point_id);
void create_apis_nosir_help(const char *name);
int create_apis_nosir(int argc,char **argv);

#endif

