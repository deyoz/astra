#include <boost/crc.hpp>
#include <boost/optional.hpp>
#include "msg_queue.h"
#include "basic.h"
#include "exceptions.h"
#include "tlg/tlg.h"
#include "stl_utils.h"
#include "qrys.h"
#include "astra_consts.h"
#include "astra_context.h"


#define NICKNAME "VLAD"
#define NICKTRACE VLAD_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

namespace AstraMessages
{

const string MSG_PROCESS_SOCKDIR="MSG_PROCESS_SOCKDIR";
const string TYPE_IN="IN";
const string TYPE_OUT="OUT";
const string STATUS_PUT="PUT";
const string STATUS_PROC="PROC";
const int RELEASE_PROCESS_ATTEMPTS=3;

void TSetDetails::fromDB(const int &id)
{
  clear();
  {
    TCachedQuery Qry("SELECT dup_seq, handler, format FROM msg_set_details WHERE set_id=:id ORDER BY dup_seq",
                     QParams() << QParam("id", otInteger, id));
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
    {
      TSetDetailsId detailsId(id, Qry.get().FieldAsInteger("dup_seq"));

      dup.push_back(TPair());
      dup.back().fromDB(Qry.get(), TSetAdapter(detailsId));
    };
  };
  if (!dup.empty())
  {
    TCachedQuery Qry("SELECT msg_type FROM msg_sets WHERE id=:id",
                     QParams() << QParam("id", otInteger, id));
    Qry.get().Execute();
    if (Qry.get().Eof)
    {
      clear();
      return;
    };
    msg_type=Qry.get().FieldAsString("msg_type");
  };
}

TBagMessageSetDetails::TBagMessageSetDetails(const std::list<std::string> &handlers)
{
  TCachedQuery HQry("SELECT code AS handler, type, proc_attempt FROM msg_handlers WHERE code=:handler",
                    QParams() << QParam("handler", otString));
  TCachedQuery FQry("SELECT code AS format, binary_data FROM msg_formats WHERE code=:format",
                    QParams() << QParam("format", otString));
  msg_type=TYPE_OUT;
  for(list<string>::const_iterator h=handlers.begin(); h!=handlers.end(); ++h)
  {
    dup.push_back(TPair());

    THandler &handler=dup.back().handler;
    HQry.get().SetVariable("handler", *h);
    HQry.get().Execute();
    if (HQry.get().Eof) throw Exception("%s: handler '%s' not found in MSG_HANDLERS", __FUNCTION__, h->c_str());
    handler.fromDB(HQry.get(), TBasicAdapter(*h));
    if (handler.getType()!=THandler::BagMessage) throw Exception("%s: wrong type of handler '%s'", __FUNCTION__, h->c_str());

    TFormat &format=dup.back().format;
    string f="TYPEB_TEXT";
    FQry.get().SetVariable("format", f);
    FQry.get().Execute();
    if (FQry.get().Eof) throw Exception("%s: format '%s' not found in MSG_FORMATS", __FUNCTION__, f.c_str());
    format.fromDB(FQry.get(), TBasicAdapter(f));

  };
}

TProcess::TProcess(const string &p) : code(p), locked(false)
{
  if (code.empty() || code.size()>20)
    throw Exception("%s: wrong process name '%s'", __FUNCTION__, code.c_str());
  string sun_path=readStringFromTcl(MSG_PROCESS_SOCKDIR, "");
  if (sun_path.empty())
    throw Exception("%s: unknown socket dir", __FUNCTION__);
  bindLocalSocket(sun_path+"/"+lowerc(code)); //предполагается, что если два процесса запускается от одного имени, то это защита

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="UPDATE msg_processes SET busy=0 WHERE code=:process";
  Qry.CreateVariable("process", otString, code);
  Qry.Execute();
  if (Qry.RowsProcessed()==0)
    ProgError(STDLOG, "%s: process '%s' not found in MSG_PROCESSES", __FUNCTION__, code.c_str());
  //отпустить все сообщения которые были обработаны ранее
  Qry.SQLText="SELECT id FROM msg_queue WHERE process=:process";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    TQueue::releaseProcess(Qry.FieldAsInteger("id"));

  getHandlers(handlers);

  OraSession.Commit();
}

void TProcess::getHandlers(std::set<THandler> &handlers)
{
  handlers.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT handler, type, proc_attempt "
              "FROM msg_handler_processes, msg_handlers "
              "WHERE msg_handler_processes.handler=msg_handlers.code AND process=:process";
  Qry.CreateVariable("process", otString, code);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    if (!handlers.insert(THandler().fromDB(Qry, TBasicAdapter(Qry.FieldAsString("handler")))).second)
      ProgError(STDLOG, "%s: handler '%s' duplicated in MSG_HANDLER_PROCESSES for process '%s'",
                        __FUNCTION__, Qry.FieldAsString("handler"), code.c_str());
}

bool TProcess::lockHandler(const string &handler)
{
  TCachedQuery Qry("SELECT handler FROM msg_handler_params WHERE handler=:handler FOR UPDATE",
                   QParams() << QParam("handler", otString, handler));
  Qry.get().Execute();
  if (!Qry.get().Eof) locked=true;
  return !Qry.get().Eof;
}

void TProcess::unlockHandlers(bool rollback)
{
  if (!locked) return;
  rollback?OraSession.Rollback():OraSession.Commit();
}

boost::optional<TQueueId> TProcess::nextMsg()
{
  boost::optional<TQueueId> next;
  try
  {
    for(set<THandler>::const_iterator h=handlers.begin(); h!=handlers.end(); ++h)
    {
      if (!lockHandler(h->getCode()))
      {
        ProgError(STDLOG, "%s: handler '%s' is not locked", __FUNCTION__, h->getCode().c_str());
        continue;
      };
      if (boost::optional<TQueueId> nextForHandler=TQueue::next(h->getCode()))
      {
        if (!next || nextForHandler->put_order < next->put_order)
        {
          next=nextForHandler;
          next->id=nextForHandler->id;
          next->put_order=nextForHandler->put_order;
        }
      }
    };
    if (next) TQueue::assignProcess(next->id, code);
    unlockHandlers();
  }
  catch(...)
  {
    unlockHandlers(true);
    throw;
  }
  return next;
}

void TQueue::releaseProcess(int id)
{
  TCachedQuery Qry("BEGIN "
                   "  UPDATE msg_queue "
                   "  SET status=:status_put, proc_time_msec=NULL, process=NULL, "
                   "      release_attempt=release_attempt-1 "
                   "  WHERE id=:id RETURNING release_attempt INTO :release_attempt; "
                   "END;",
                   QParams() << QParam("id", otInteger, id)
                             << QParam("status_put", otString, STATUS_PUT)
                             << QParam("release_attempt", otInteger, FNull)
                  );
  Qry.get().Execute();
  if (Qry.get().VariableIsNULL("release_attempt"))
    ProgError(STDLOG, "%s: msg %d not found in MSG_QUEUE", __FUNCTION__, id);
  else
    if (Qry.get().GetVariableAsInteger("release_attempt")<=0)
    {
      if (!set_error(id, "Release attempt by process reached the limit")) return;
      TCachedQuery DelQry("DELETE FROM msg_queue WHERE id=:id",
                          QParams() << QParam("id", otInteger, id));
      DelQry.get().Execute();
    };
}

void TQueue::assignProcess(int id, const string &process)
{
  TCachedQuery Qry("UPDATE msg_queue "
                   "SET status=:status_proc, proc_time_msec=:proc_time_msec, process=:process "
                   "WHERE id=:id",
                   QParams() << QParam("id", otInteger, id)
                             << QParam("status_proc", otString, STATUS_PROC)
                             << QParam("proc_time_msec", otFloat, NowUTC())
                             << QParam("process", otString, process)
                  );
  Qry.get().Execute();
  if (Qry.get().RowsProcessed()==0)
    throw Exception("%s: msg %d not found in MSG_QUEUE", __FUNCTION__, id);
}

void TQueue::put(const TSetDetails &setDetails, const std::string &conseq_key, const std::string &content)
{
  if (setDetails.empty()) throw Exception("%s: empty setDetails", __FUNCTION__);
  if (content.empty()) throw Exception("%s: empty content", __FUNCTION__);

  TCachedQuery Qry("BEGIN "
                   "  INSERT INTO msgs(id, type, handler, format, time, binary_data, error) "
                   "  VALUES(tlgs_id.nextval, :type, :handler, :format, SYSTEM.UTCSYSDATE, :binary_data, NULL) RETURNING id INTO :id; "
                   "END;",
                   QParams() << QParam("type", otString, setDetails.msg_type)
                             << QParam("handler", otString)
                             << QParam("format", otString)
                             << QParam("binary_data", otInteger)
                             << QParam("id", otInteger));
  TCachedQuery QueueQry("INSERT INTO msg_queue(id, type, status, handler, format, conseq_crc, dup_key, priority, time, "
                        "  put_time_msec, put_order, proc_time_msec, proc_attempt, process, release_attempt) "
                        "SELECT id, type, :status, :handler, :format, :conseq_crc, :dup_key, 1, time, "
                        "  :put_time_msec, events__seq.nextval, NULL, :proc_attempt, NULL, :release_attempt "
                        "FROM msgs "
                        "WHERE id=:id",
                        QParams() << QParam("status", otString, STATUS_PUT)
                                  << QParam("handler", otString)
                                  << QParam("format", otString)
                                  << QParam("conseq_crc", otInteger)
                                  << QParam("dup_key", otInteger)
                                  << QParam("put_time_msec", otFloat)
                                  << QParam("proc_attempt", otInteger)
                                  << QParam("id", otInteger)
                                  << QParam("release_attempt", otInteger, RELEASE_PROCESS_ATTEMPTS));
  TCachedQuery ContentQry("INSERT INTO msg_content(id, page_no, data) VALUES(:id, :page_no, :data)",
                          QParams() << QParam("id", otInteger)
                                    << QParam("page_no", otInteger)
                                    << QParam("data", otString));

  int conseq_crc=ASTRA::NoExists;
  if (!conseq_key.empty())
  {
    boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
    crc32.reset();
    crc32.process_bytes( conseq_key.c_str(), conseq_key.size() );
    conseq_crc=crc32.checksum();
  };

  int dup_key=ASTRA::NoExists;
  for(std::list<TPair>::const_iterator i=setDetails.dup.begin(); i!=setDetails.dup.end(); ++i)
  {
    Qry.get().SetVariable("handler", i->handler.getCode());
    Qry.get().SetVariable("format", i->format.getCode());
    Qry.get().SetVariable("binary_data", i->format.getBinary());
    Qry.get().Execute();
    //пишем msg_queue
    int id=Qry.get().GetVariableAsInteger("id");
    if (i==setDetails.dup.begin() && setDetails.dup.size()>1) dup_key=id;
    //пишем параметры
    i->toDB(QueueQry.get(), TQueueAdapter(id));

    if (!conseq_key.empty()) //проверка conseq_crc!=NoExists не катит, так как MAX_VALUE может совпадать с checksum
      QueueQry.get().SetVariable("conseq_crc", conseq_crc);
    else
      QueueQry.get().SetVariable("conseq_crc", FNull);
    if (dup_key!=ASTRA::NoExists)
      QueueQry.get().SetVariable("dup_key", dup_key);
    else
      QueueQry.get().SetVariable("dup_key", FNull);
    QueueQry.get().SetVariable("put_time_msec", NowUTC());
    QueueQry.get().SetVariable("id", id);
    QueueQry.get().Execute();
    //пишем текст
    ContentQry.get().SetVariable("id", id);
    if (i->format.getBinary())
    {
      string hex;
      StringToHex(content, hex);
      longToDB(ContentQry.get(), "data", hex);
    }
    else
      longToDB(ContentQry.get(), "data", content);
  };
}

void TQueue::get(int id, boost::optional<TQueueMsg> &msg)
{
  msg=boost::none;
  TCachedQuery Qry("SELECT type, handler, format, binary_data FROM msgs WHERE id=:id",
                   QParams() << QParam("id", otInteger, id));
  Qry.get().Execute();
  if (Qry.get().Eof)
  {
    msg=boost::none;
    ProgError(STDLOG, "%s: msg %d not found in MSGS", __FUNCTION__, id);
    return;
  };
  msg=TQueueMsg();
  msg->handler.fromDB(Qry.get(), TQueueAdapter(id));
  msg->format.fromDB(Qry.get(), TQueueAdapter(id));
  msg->type=Qry.get().FieldAsString("type");

  TCachedQuery ContentQry("SELECT data FROM msg_content WHERE id=:id ORDER BY page_no",
                          QParams() << QParam("id", otInteger, id));

  ContentQry.get().Execute();
  if (ContentQry.get().Eof)
  {
    msg=boost::none;
    ProgError(STDLOG, "%s: msg %d not found in MSG_CONTENT", __FUNCTION__, id);
    return;
  }
  for(;!ContentQry.get().Eof;ContentQry.get().Next())
    msg->content.append(ContentQry.get().FieldAsString("data"));
}

boost::optional<TQueueId> TQueue::next(const string &handler)
{
  boost::optional<TQueueId> result;

  TCachedQuery Qry("SELECT id, put_order "
                   "FROM msg_queue a "
                   "WHERE handler=:handler AND "
                   "      (conseq_crc IS NULL OR "
                   "       NOT EXISTS(SELECT * FROM msg_queue "
                   "                  WHERE conseq_crc=a.conseq_crc AND put_order<a.put_order)) AND "
                   "      (dup_key IS NULL OR "
                   "       NOT EXISTS(SELECT * FROM msg_queue "
                   "                  WHERE dup_key=a.dup_key AND put_order<a.put_order)) AND "
                   "      process IS NULL "
                   "ORDER BY put_order",
                   QParams() << QParam("handler", otString, handler));

  Qry.get().Execute();
  if (!Qry.get().Eof)
    result=TQueueId(Qry.get().FieldAsInteger("id"), Qry.get().FieldAsFloat("put_order"));
  return result;
}

bool TQueue::set_error(int id, const std::string &error)
{
  TCachedQuery UpdQry("UPDATE msgs SET error=:error WHERE id=:id",
                      QParams() << QParam("id", otInteger, id)
                                << QParam("error", otString, error.substr(0,100)));
  UpdQry.get().Execute();
  if (UpdQry.get().RowsProcessed()==0)
  {
    ProgError(STDLOG, "%s: msg %d not found in MSGS", __FUNCTION__, id);
    return false;
  };
  return true;
}

void TQueue::complete_attempt(int id, const std::string &error)
{
  if (!set_error(id, error)) return;

  TCachedQuery DelQry("BEGIN "
                      "  IF :error IS NOT NULL THEN "
                      "    UPDATE msg_queue SET proc_attempt=proc_attempt-1, process=NULL WHERE id=:id RETURNING proc_attempt INTO :proc_attempt; "
                      "    IF :proc_attempt IS NOT NULL AND :proc_attempt<=0 THEN "
                      "      DELETE FROM msg_queue WHERE id=:id; "
                      "    END IF; "
                      "  ELSE "
                      "    DELETE FROM msg_queue WHERE id=:id RETURNING dup_key INTO :dup_key; "
                      "    DELETE FROM msg_queue WHERE dup_key=:dup_key; "
                      "  END IF; "
                      "END;",
                      QParams() << QParam("id", otInteger, id)
                                << QParam("error", otString, error)
                                << QParam("dup_key", otInteger, FNull)
                                << QParam("proc_attempt", otInteger, FNull));
  DelQry.get().Execute();
}

std::string paramValue(const THandler &handler, const std::string &name, const boost::optional<std::string> &defaultValue)
{
  boost::optional<TParam> param=handler.getParams().get(name);
  if (!param && !defaultValue) throw Exception("%s: parameter '%s' not found", __FUNCTION__, name.c_str());
  LogTrace(TRACE5) << __FUNCTION__ << ": " << handler.getCode() << "." << name << "='" << (param?param->value:*defaultValue) << "'";
  return (param?param->value:*defaultValue);
}

std::string paramValue(const TFormat &format, const std::string &name, const boost::optional<std::string> &defaultValue)
{
  boost::optional<TParam> param=format.getParams().get(name);
  if (!param && !defaultValue) throw Exception("%s: parameter '%s' not found", __FUNCTION__, name.c_str());
  LogTrace(TRACE5) << __FUNCTION__ << ": " << format.getCode() << "." << name << "='" << (param?param->value:*defaultValue) << "'";
  return (param?param->value:*defaultValue);
}


} //namespace AstraMessages

