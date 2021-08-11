#include "code_convert.h"

#include "db_tquery.h"
#include "PgOraConfig.h"
#include "astra_main.h"
#include "astra_misc.h"

#define NICKNAME "ROMAN"
#include "serverlib/slogger.h"

enum EDirection
{
  toInternal,
  toExternal,
};

void InitQry(DB::TQuery& Qry, EDirection direction, const string& table_name, const string& system_name)
{
  if (direction == toInternal)
    Qry.SQLText = string("SELECT code_internal AS result FROM ") + table_name + string(" ") +
                  string("WHERE system_name = :system_name AND code_external = :source ORDER BY result");
  else
    Qry.SQLText = string("SELECT code_external AS result FROM ") + table_name + string(" ") +
                  string("WHERE system_name = :system_name AND code_internal = :source ORDER BY result");
  Qry.CreateVariable("system_name", otString, system_name);
}

template <typename SOURCE, typename RESULT>
RESULT ConvertCode(const SOURCE& source, EDirection direction, const string& table_name, const string& system_name)
{ return RESULT(); }

// string, int
template <> int ConvertCode(const string& source, EDirection direction, const string& table_name, const string& system_name)
{
  DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otString, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsInteger("result");
  else return ASTRA::NoExists;
}

// int, string
template <> string ConvertCode(const int& source, EDirection direction, const string& table_name, const string& system_name)
{
  DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otInteger, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsString("result");
  else return "";
}

// string, string
template <> string ConvertCode(const string& source, EDirection direction, const string& table_name, const string& system_name)
{
  DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otString, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsString("result");
  else return "";
}

// airline
string AirlineToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "CONVERT_AIRLINES", system_name);
}

string AirlineToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "CONVERT_AIRLINES", system_name);
}

// airport
string AirportToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "CONVERT_AIRPS", system_name);
}

string AirportToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "CONVERT_AIRPS", system_name);
}

// craft
string CraftToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "CONVERT_CRAFTS", system_name);
}

string CraftToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "CONVERT_CRAFTS", system_name);
}

// litera
string LiteraToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "CONVERT_LITERS", system_name);
}

string LiteraToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "CONVERT_LITERS", system_name);
}

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /*HAVE_CONFIG_H*/

#ifdef XP_TESTING
#include "xp_testing.h"
START_TEST(check_convert)
{
  fail_unless(LiteraToInternal("CODE", "DDD") == "");
  fail_unless(LiteraToExternal("CODE", "DDD") == "");

  fail_unless(CraftToInternal("CODE", "DDD") == "");
  fail_unless(CraftToExternal("CODE", "DDD") == "");

  fail_unless(AirportToInternal("CODE", "DDD") == "");
  fail_unless(AirportToExternal("CODE", "DDD") == "");

  fail_unless(AirlineToInternal("CODE", "DDD") == "");
  fail_unless(AirlineToExternal("CODE", "DDD") == "");

  make_db_curs("insert into convert_airlines (CODE_EXTERNAL, CODE_INTERNAL, SYSTEM_NAME) values ('SOCHI', 'AER', 'IKAO')", PgOra::getRWSession("CONVERT_AIRLINES")).exec();

  fail_unless(AirlineToInternal("SOCHI", "IKAO") == "AER");
  fail_unless(AirlineToExternal("AER", "IKAO") == "SOCHI");
}
END_TEST;

#define SUITENAME "convert_code"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
  ADD_TEST(check_convert);
}
TCASEFINISH;
#undef SUITENAME

#endif
