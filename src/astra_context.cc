#include "astra_context.h"
#include "astra_consts.h"
#include "exceptions.h"
#include "oralib.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

void longToDB(TQuery &Qry, const std::string &column_name, const std::string &src, bool nullable, int len)
{
  if (!src.empty())
  {
    std::string::const_iterator ib,ie;
    ib=src.begin();
    for(int page_no=1;ib<src.end();page_no++)
    {
      ie=ib+len;
      if (ie>src.end()) ie=src.end();
      Qry.SetVariable("page_no", page_no);
      Qry.SetVariable(column_name.c_str(), std::string(ib,ie));
      Qry.Execute();
      ib=ie;
    };
  }
  else
  {
    if (nullable)
    {
      Qry.SetVariable("page_no", 1);
      Qry.SetVariable(column_name.c_str(), FNull);
      Qry.Execute();
    };
  }
}

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
    Qry.CreateVariable("time_create",otDate,BASIC::NowUTC());
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

BASIC::TDateTime GetContext(const std::string &name,
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
}

void ClearContext(const std::string &name,
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
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "UPDATE context SET name=:dest_name, id=:dest_id "
        "WHERE name=:name AND id=:id";
    Qry.CreateVariable("name", otString, srcName);
    Qry.CreateVariable("id", otInteger, srcId);
    Qry.CreateVariable("dest_name", otString, destName);
    Qry.CreateVariable("dest_id", otInteger, destId);
    Qry.Execute();
}

} /* namespace AstraContext */
