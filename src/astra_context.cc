#include "astra_context.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"

#include "date_time.h"
using namespace BASIC::date_time;

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

namespace AstraContext
{

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

    TQuery Qry(&OraSession);
    Qry.SQLText=
        "BEGIN "
        "  IF :id IS NULL THEN "
        "    SELECT context__seq.nextval INTO :id FROM dual; "
        "  END IF; "
        "  INSERT INTO context(name,id,page_no,time_create,value) "
        "  VALUES(:name,:id,:page_no,:time_create,:value); "
        "END;";
    Qry.CreateVariable("name",otString,name);
    if (id!=ASTRA::NoExists)
      Qry.CreateVariable("id",otInteger,id);
    else
      Qry.CreateVariable("id",otInteger,FNull);
    Qry.DeclareVariable("page_no",otInteger);
    Qry.CreateVariable("time_create",otDate, NowUTC());
    Qry.DeclareVariable("value",otString);

    longToDB(Qry, "value", value, true);

    if (!Qry.VariableIsNULL("id"))
      return Qry.GetVariableAsInteger("id");
    else
      return ASTRA::NoExists;
  }
  catch(std::exception &e)
  {
    ProgTrace(TRACE5, "AstraContext::SetContext: name=%s id=%d", name.c_str(), id);
    throw;
  }
}

int SetContext(const std::string &name,
               const std::string &value)
{
  return SetContext(name,ASTRA::NoExists,value);
}

TDateTime GetContext(const std::string &name,
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
  TDateTime time_create=ASTRA::NoExists;
  for(;!Qry.Eof;Qry.Next())
  {
    value.append(Qry.FieldAsString("value"));
    time_create=Qry.FieldAsDateTime("time_create");
  };
  return time_create;
}

void ClearContext(const std::string &name,
                  const TDateTime time_create)
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
}

void ClearContext(const std::string &name,
                  const int id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "DELETE FROM context WHERE name=:name AND id=:id";
  Qry.CreateVariable("name",otString,name);
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
}

void MoveContext(const std::string& srcName, int srcId,
                 const std::string& destName, int destId)
{
    CopyContext(srcName, srcId, destName, destId);
    ClearContext(srcName, srcId);
}

void CopyContext(const std::string& srcName, int srcId,
                 const std::string& destName, int destId)
{
    std::string value;
    GetContext(srcName, srcId, value);
    if(value.empty()) {
        LogWarning(STDLOG) << "Context " << srcName << "[" << srcId << "] is empty!";
    }

    ClearContext(destName, destId);


    LogTrace(TRACE3) << "Copied context:\n" << value;

    SetContext(destName, destId, value);
}

} /* namespace AstraContext */
