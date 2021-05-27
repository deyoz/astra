#include "astra_context.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "db_tquery.h"
#include "PgOraConfig.h"

#include "date_time.h"
using namespace BASIC::date_time;

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"


namespace AstraContext
{

int GetContextId(const int id)
{
    if (id == ASTRA::NoExists) {
        return PgOra::getSeqNextVal("context__seq");
    }
    return id;
}

int SetContext(const std::string &name,
               const int id,
               const std::string &value)
{
  try
  {
    if (name.empty())
      throw EXCEPTIONS::Exception("AstraContext::SetContext: empty context name");
    if (name.size()>20)
      throw EXCEPTIONS::Exception("AstraContext::SetContext: context name %s too long",name.c_str());

    const int contextId = GetContextId(id);
    DB::TQuery Qry(PgOra::getRWSession("CONTEXT"), STDLOG);
    Qry.SQLText=
        "INSERT INTO context(name,id,page_no,time_create,value) "
        "  VALUES(:name,:id,:page_no,:time_create,:value)";
    Qry.CreateVariable("name",otString,name);
    Qry.CreateVariable("id",otInteger, contextId);
    Qry.DeclareVariable("page_no",otInteger);
    Qry.CreateVariable("time_create",otDate, NowUTC());
    Qry.DeclareVariable("value",otString);

    longToDB(Qry, "value", value, true);

    return contextId;
  }
  catch(std::exception &e)
  {
    ProgTrace(TRACE5, "AstraContext::SetContext: name=%s id=%d", name.c_str(), id);
    throw;
  }
};

int SetContext(const std::string &name,
               const std::string &value)
{
  return SetContext(name,ASTRA::NoExists,value);
};

TDateTime GetContext(const std::string &name,
                            const int id,
                            std::string &value)
{
  value.clear();
  DB::TQuery Qry(PgOra::getROSession("CONTEXT"), STDLOG);
  Qry.SQLText=
    "SELECT value,time_create FROM context "
    "WHERE name=:name AND id=:id ORDER BY page_no";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  TDateTime time_create=ASTRA::NoExists;
  for(;!Qry.Eof;Qry.Next())
  {
    value.append(Qry.FieldAsString("value"));
    time_create=Qry.FieldAsDateTime("time_create");
  };
  return time_create;
};

void ClearContext(const std::string &name,
                  const TDateTime time_create)
{
  DB::TQuery Qry(PgOra::getRWSession("CONTEXT"), STDLOG);
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

void ClearContext(const std::string &name,
                  const int id)
{
  DB::TQuery Qry(PgOra::getRWSession("CONTEXT"), STDLOG);
  Qry.SQLText=
    "DELETE FROM context WHERE name=:name AND id=:id";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
};

} /* namespace AstraContext */

