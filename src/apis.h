#ifndef _APIS_H_
#define _APIS_H_

#include <set>
#include <string>

class TCountriesUS : public std::set<std::string>
{
  public:
    TCountriesUS()
    {
      insert("ƒ“");
      insert("Œ");
      insert("‘");
    };
};

static TCountriesUS countriesUS;

void create_apis_file(int point_id);
void create_apis_nosir_help(const char *name);
int create_apis_nosir(int argc,char **argv);

#endif

