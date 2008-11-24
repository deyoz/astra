#include "astra_context.h"
#include "astra_consts.h"
#include "exceptions.h"
#include "oralib.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

namespace AstraContext
{

void SetContext(const std::string name,
                const int id,
                const std::string &value)
{
  if (name.empty())
    throw EXCEPTIONS::Exception("AstraContext::SetContext: empty context name");
  if (name.size()>20)
    throw EXCEPTIONS::Exception("AstraContext::SetContext: context name too long");

  TQuery Qry(&OraSession);
  Qry.SQLText=
    "INSERT INTO context(name,id,page_no,time_create,value) "
    "VALUES(:name,:id,:page_no,:time_create,:value) ";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.DeclareVariable("page_no",otInteger);
  Qry.CreateVariable("time_create",otDate,BASIC::NowUTC());
  Qry.DeclareVariable("value",otString);

  std::string::const_iterator ib,ie;
  ib=value.begin();
  for(int page_no=1;ib<value.end();page_no++)
  {
    Qry.SetVariable("page_no",page_no);
    ie=ib+4000;
    if (ie>value.end()) ie=value.end();
    Qry.SetVariable("value",std::string(ib,ie));
    Qry.Execute();
    ib=ie;
  };
};

int SetContext(const std::string name,
                const std::string &value)
{
  return -1;
};

BASIC::TDateTime GetContext(const std::string name,
                            const int id,
                            std::string &value)
{
  value.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT value,time_create FROM context "
    "WHERE name=:name AND id=:id ORDER BY page_no";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  BASIC::TDateTime time_create=ASTRA::NoExists;
  for(;!Qry.Eof;Qry.Next())
  {
    value.append(Qry.FieldAsString("value"));
    time_create=Qry.FieldAsDateTime("time_create");
  };
  return time_create;
};

void ClearContext(const std::string name,
                  const BASIC::TDateTime time_create)
{
  TQuery Qry(&OraSession);
  if (time_create != ASTRA::NoExists)
  {
    Qry.SQLText=
      "DELETE FROM context WHERE name=:name AND time_create<=:time_create";
    Qry.CreateVariable("time_create",otDate,time_create);

  }
  else
  {
    Qry.SQLText=
      "DELETE FROM context WHERE name=:name";
  };
  Qry.CreateVariable("name",otString,name);
  Qry.Execute();
};

void ClearContext(const std::string name,
                  const int id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "DELETE FROM context WHERE name=:name AND id=:id";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
};

} /* namespace AstraContext */

