#include "code_convert.h"

#include "astra_main.h"
#include "astra_misc.h"

enum EDirection
{
  toInternal,
  toExternal,
};

void InitQry(TQuery& Qry, EDirection direction, const string& table_name, const string& system_name)
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
  TQuery Qry(&OraSession);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otString, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsInteger("result");
  else return ASTRA::NoExists;
}

// int, string
template <> string ConvertCode(const int& source, EDirection direction, const string& table_name, const string& system_name)
{
  TQuery Qry(&OraSession);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otInteger, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsString("result");
  else return "";
}

// string, string
template <> string ConvertCode(const string& source, EDirection direction, const string& table_name, const string& system_name)
{
  TQuery Qry(&OraSession);
  InitQry(Qry, direction, table_name, system_name);
  Qry.CreateVariable("source", otString, source);
  Qry.Execute();
  if (!Qry.Eof) return Qry.FieldAsString("result");
  else return "";
}

// airline
string AirlineToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "convert_airlines", system_name);
}

string AirlineToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "convert_airlines", system_name);
}

// airport
string AirportToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "convert_airps", system_name);
}

string AirportToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "convert_airps", system_name);
}

// craft
string CraftToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "convert_crafts", system_name);
}

string CraftToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "convert_crafts", system_name);
}

// litera
string LiteraToInternal(const string& external, const string& system_name)
{
  return ConvertCode<string, string>(external, toInternal, "convert_liters", system_name);
}

string LiteraToExternal(const string& internal, const string& system_name)
{
  return ConvertCode<string, string>(internal, toExternal, "convert_liters", system_name);
}

