#include <string>
#include "apis.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_main.h"
#include "base_tables.h"
#include "apis_edi_file.h"
#include "misc.h"
#include "passenger.h"
#include "tlg/tlg.h"
#include "trip_tasks.h"
#include "file_queue.h"
#include "astra_service.h"
#include "apis_utils.h"
#include "date_time.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/slogger.h"

//#define ENDL "\r\n"


using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

namespace APIS
{

const set<string> &customsUS()
{
  static bool init=false;
  static set<string> depend;
  if (!init)
  {
    TQuery Qry(&OraSession);
    GetCustomsDependCountries("ЮС", depend, Qry);
    init=true;
  };
  return depend;
};

void GetCustomsDependCountries(const string &regul,
                               set<string> &depend,
                               TQuery &Qry)
{
  depend.clear();
  depend.insert(regul);
  const char *sql =
    "SELECT country_depend FROM apis_customs WHERE country_regul=:country_regul";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_regul",otString);
  };
  Qry.SetVariable("country_regul", regul);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    depend.insert(Qry.FieldAsString("country_depend"));
};

string GetCustomsRegulCountry(const string &depend,
                              TQuery &Qry)
{
  const char *sql =
    "SELECT country_regul FROM apis_customs WHERE country_depend=:country_depend";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_depend",otString);
  };
  Qry.SetVariable("country_depend", depend);
  Qry.Execute();
  if (!Qry.Eof)
    return Qry.FieldAsString("country_regul");
  else
    return depend;
};

};

const char* APIS_PARTY_INFO()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("APIS_PARTY_INFO","");
  return VAR.c_str();
};

//#define MAX_PAX_PER_EDI_PART 15
//#define MAX_LEN_OF_EDI_PART 3000

// class TAirlineOfficeInfo
// {
//   public:
//     string contact_name;
//     string phone;
//     string fax;
// };

void GetAirlineOfficeInfo(const string &airline,
                          const string &country,
                          const string &airp,
                          list<TAirlineOfficeInfo> &offices)
{
  offices.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airp, contact_name, phone, fax "
    "FROM airline_offices "
    "WHERE airline=:airline AND country=:country AND "
    "      (airp IS NULL OR airp=:airp) AND to_apis<>0 "
    "ORDER BY airp NULLS LAST";
  Qry.CreateVariable("airline", otString, airline);
  Qry.CreateVariable("country", otString, country);
  Qry.CreateVariable("airp", otString, airp);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TAirlineOfficeInfo info;
    info.contact_name=Qry.FieldAsString("contact_name");
    info.phone=Qry.FieldAsString("phone");
    info.fax=Qry.FieldAsString("fax");
    offices.push_back(info);
  };

  vector<string> strs;
  SeparateString(string(APIS_PARTY_INFO()), ':', strs);
  if (!strs.empty())
  {
    TAirlineOfficeInfo info;
    vector<string>::const_iterator i;
    i=strs.begin();
    if (i!=strs.end()) info.contact_name=(*i++);
    if (i!=strs.end()) info.phone=(*i++);
    if (i!=strs.end()) info.fax=(*i++);
    offices.push_back(info);
  };
};


string NormalizeDocNo(const string& str, bool try_keep_only_digits)
{
  string result;
  string max_num, curr_num;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i)
    if (IsDigitIsLetter(*i)) result+=*i;
  if (try_keep_only_digits)
  {
    for(string::const_iterator i=result.begin(); i!=result.end(); ++i)
    {
      if (IsDigit(*i)) curr_num+=*i;
      if (IsLetter(*i) && !curr_num.empty())
      {
        if (curr_num.size()>max_num.size()) max_num=curr_num;
        curr_num.clear();
      };
    };
    if (curr_num.size()>max_num.size()) max_num=curr_num;
  };

  return (max_num.size()<6)?result:max_num;
};

string HyphenToSpace(const string& str)
{
  string result;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i)
    result+=(*i=='-')?' ':*i;
  return result;
};

void create_apis_nosir_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<points.point_id>  ");
};

int create_apis_nosir(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  int point_id=ASTRA::NoExists;
  try
  {
    //проверяем параметры
    if (argc<2) throw EConvertError("wrong parameters");
    point_id = ToInt(argv[1]);
    Qry.Clear();
    Qry.SQLText="SELECT point_id FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof) throw EConvertError("point_id not found");
  }
  catch(EConvertError &E)
  {
    printf("Error: %s\n", E.what());
    if (argc>0)
    {
      puts("Usage:");
      create_apis_nosir_help(argv[0]);
      puts("Example:");
      printf("  %s 1234567\n",argv[0]);
    };
    return 1;
  };

  init_locale();
  create_apis_file(point_id, (argc>=3)?argv[2]:"");

  puts("create_apis successfully completed");

  send_apis_tr();
  return 0;
};

void create_apis_task(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;
  create_apis_file(task.point_id, task.params);
};

