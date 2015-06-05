#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include <string>
#include <map>
#include <boost/optional.hpp>

#include "oralib.h"
#include "qrys.h"
#include "stl_utils.h"
#include "astra_consts.h"

namespace AstraMessages
{

const std::string PARAM_TYPE_HANDLER="HANDLER";
const std::string PARAM_TYPE_FORMAT="FORMAT";

class TBasicAdapter
{
  private:
    std::string code;
  public:
    TCachedQuery selectQuery(const std::string &type) const
    {
      if (type==PARAM_TYPE_HANDLER)
      {
        TCachedQuery Qry("SELECT * FROM msg_handler_params WHERE handler=:handler",
                         QParams() << QParam("handler", otString, code));
        return Qry;
      }
      else
      {
        TCachedQuery Qry("SELECT * FROM msg_format_params WHERE format=:format",
                         QParams() << QParam("format", otString, code));
        return Qry;
      };
    }
    TCachedQuery deleteQuery(const std::string &type) const
    {
      if (type==PARAM_TYPE_HANDLER)
      {
        TCachedQuery Qry("DELETE FROM msg_handler_params WHERE handler=:handler",
                         QParams() << QParam("handler", otString, code));
        return Qry;
      }
      else
      {
        TCachedQuery Qry("DELETE FROM msg_format_params WHERE format=:format",
                         QParams() << QParam("format", otString, code));
        return Qry;
      };
    }
    TCachedQuery insertQuery(const std::string &type) const
    {
      if (type==PARAM_TYPE_HANDLER)
      {
        TCachedQuery Qry("INSERT INTO msg_handler_params(handler, name, value, editable) "
                         "VALUES(:handler, :name, :value, :editable)",
                         QParams() << QParam("handler", otString, code)
                                   << QParam("name", otString)
                                   << QParam("value", otString)
                                   << QParam("editable", otInteger));
        return Qry;
      }
      else
      {
        TCachedQuery Qry("INSERT INTO msg_format_params(format, name, value, editable) "
                         "VALUES(:format, :name, :value, :editable)",
                         QParams() << QParam("format", otString, code)
                                   << QParam("name", otString)
                                   << QParam("value", otString)
                                   << QParam("editable", otInteger));
        return Qry;
      };
    }

    TBasicAdapter(const std::string &p) : code(p) {}
};

class TQueueAdapter
{
  private:
    int id;
  public:
    TCachedQuery selectQuery(const std::string &type) const
    {
      TCachedQuery Qry("SELECT name, value, 0 AS editable FROM msg_params WHERE id=:id AND type=:type",
                       QParams() << QParam("id", otInteger, id)
                                 << QParam("type", otString, type));
      return Qry;
    }
    TCachedQuery deleteQuery(const std::string &type) const
    {
      TCachedQuery Qry("DELETE FROM msg_params WHERE id=:id AND type=:type",
                       QParams() << QParam("id", otInteger, id)
                                 << QParam("type", otString, type));
      return Qry;
    }
    TCachedQuery insertQuery(const std::string &type) const
    {
      TCachedQuery Qry("INSERT INTO msg_params(id, type, name, value) VALUES(:id, :type, :name, :value)",
                       QParams() << QParam("id", otInteger, id)
                                 << QParam("type", otString, type)
                                 << QParam("name", otString)
                                 << QParam("value", otString));
      return Qry;
    }

    TQueueAdapter(const int &p) : id(p) {}
};

class TSetDetailsId
{
  public:
    int set_id, dup_seq;
    TSetDetailsId(const int &id, const int &seq) : set_id(id), dup_seq(seq) {}
};

class TSetAdapter
{
  private:
    TSetDetailsId id;
  public:
    TCachedQuery selectQuery(const std::string &type) const
    {
      TCachedQuery Qry("SELECT name, value, 0 AS editable FROM msg_set_params "
                       "WHERE set_id=:set_id AND dup_seq=:dup_seq AND type=:type",
                       QParams() << QParam("set_id", otInteger, id.set_id)
                                 << QParam("dup_seq", otInteger, id.dup_seq)
                                 << QParam("type", otString, type));
      return Qry;
    }

    TCachedQuery deleteQuery(const std::string &type) const
    {
      TCachedQuery Qry("DELETE FROM msg_set_params "
                       "WHERE set_id=:set_id AND dup_seq=:dup_seq AND type=:type",
                       QParams() << QParam("set_id", otInteger, id.set_id)
                                 << QParam("dup_seq", otInteger, id.dup_seq)
                                 << QParam("type", otString, type));
      return Qry;
    }

    TCachedQuery insertQuery(const std::string &type) const
    {
      TCachedQuery Qry("INSERT INTO msg_set_params(set_id, dup_seq, type, name, value) "
                       "VALUES(:set_id, :dup_seq, :type, :name, :value)",
                       QParams() << QParam("set_id", otInteger, id.set_id)
                                 << QParam("dup_seq", otInteger, id.dup_seq)
                                 << QParam("type", otString, type)
                                 << QParam("name", otString)
                                 << QParam("value", otString));
      return Qry;
    }

    TSetAdapter(const TSetDetailsId &p) : id(p) {}
};

class TParam
{
  public:
    std::string name, value;
    bool editable;
    TParam() : editable(false) {}
    void clear()
    {
      name.clear();
      value.clear();
      editable=false;
    }
    TParam& fromDB(TQuery &Qry)
    {
      clear();
      name=Qry.FieldAsString("name");
      value=Qry.FieldAsString("value");
      editable=Qry.FieldAsInteger("editable")!=0;
      return *this;
    }
    const TParam& toDB(TQuery &Qry) const
    {
      Qry.SetVariable("name", name);
      Qry.SetVariable("value", value);
      if (Qry.GetVariableIndex("editable")>=0)
        Qry.SetVariable("editable", (int)editable);
      return *this;
    }
};

class TParams
{
  private:
    std::map<std::string, TParam> params;
  protected:
    virtual std::string type() const =0;
  public:
    void clear()
    {
      params.clear();
    }
    boost::optional<TParam> get(const std::string &name) const
    {
      std::map<std::string, TParam>::const_iterator i=params.find(lowerc(name));
      if (i!=params.end())
        return i->second;
      else
        return boost::none;
    }
    void set(const boost::optional<TParam> &param)
    {
      if (!param) return;
      params.insert(make_pair(lowerc(param.get().name), param.get()));
    }
    template<typename T>
    void fromDB(const T &adapter)
    {
      TCachedQuery Qry=adapter.selectQuery(type());
      Qry.get().Execute();
      for(;!Qry.get().Eof;Qry.get().Next())
        set(TParam().fromDB(Qry.get()));
    }
    template<typename T>
    void toDB(const T &adapter) const
    {
      TCachedQuery DelQry=adapter.deleteQuery(type());
      DelQry.get().Execute();
      TCachedQuery InsQry=adapter.insertQuery(type());
      for(std::map<std::string, TParam>::const_iterator i=params.begin(); i!=params.end(); ++i)
      {
        i->second.toDB(InsQry.get());
        InsQry.get().Execute();
      };
    }

    virtual ~TParams() {}
};

class THandlerParams : public TParams
{
  protected:
    std::string type() const
    {
      return PARAM_TYPE_HANDLER;
    }
};

class TFormatParams : public TParams
{
  protected:
    std::string type() const
    {
      return PARAM_TYPE_FORMAT;
    }
};

class THandler
{
  public:
    enum Type {BagMessage, Unknown};
  private:
    std::string code;
    Type type;
    boost::optional<int> proc_attempt;
    THandlerParams params;
    Type DecodeType(const std::string& type_) const
    {
      if (type_=="BAG_MESSAGE") return BagMessage;
      return Unknown;
    }
  public:
    void clear()
    {
      code.clear();
      type=Unknown;
      params.clear();
      proc_attempt=boost::none;
    }
    bool operator < (const THandler &item) const
    {
      return code < item.code;
    }
    template<typename T>
    THandler& fromDB(TQuery &Qry, const T &adapter)
    {
      clear();
      code=Qry.FieldAsString("handler");
      if (Qry.GetVariableIndex("proc_attempt")>=0)
      {
        if (!Qry.FieldIsNULL("proc_attempt"))
          proc_attempt=Qry.FieldAsInteger("proc_attempt");
      };
      if (Qry.GetFieldIndex("type")>=0)
        type=DecodeType(Qry.FieldAsString("type"));
      params.fromDB<T>(adapter);
      return *this;
    }
    template<typename T>
    const THandler& toDB(TQuery &Qry, const T &adapter) const
    {
      Qry.SetVariable("handler", code);
      if (Qry.GetVariableIndex("proc_attempt")>=0)
        proc_attempt?Qry.SetVariable("proc_attempt", proc_attempt.get()):
                     Qry.SetVariable("proc_attempt", FNull);
      params.toDB<T>(adapter);
      return *this;
    }
    std::string getCode() const { return code; }
    boost::optional<int> getProcAttempt() const { return proc_attempt; }
    const THandlerParams& getParams() const { return params; }
    Type getType() const { return type; }
};

class TFormat
{
  private:
    std::string code;
    bool binary;
    TFormatParams params;
  public:
    void clear()
    {
      code.clear();
      params.clear();
      binary=false;
    }
    template<typename T>
    TFormat& fromDB(TQuery &Qry, const T &adapter)
    {
      clear();
      code=Qry.FieldAsString("format");
      binary=Qry.FieldAsInteger("binary_data")!=0;
      params.fromDB<T>(adapter);
      return *this;
    }
    template<typename T>
    const TFormat& toDB(TQuery &Qry, const T &adapter) const
    {
      Qry.SetVariable("format", code);
      if (Qry.GetVariableIndex("binary_data")>=0)
        Qry.SetVariable("binary", (int)binary);
      params.toDB<T>(adapter);
      return *this;
    }
    std::string getCode() const { return code; }
    bool getBinary() const { return binary; }
    const TFormatParams& getParams() const { return params; }
};


class TPair
{
  public:
    THandler handler;
    TFormat format;
    void clear()
    {
      handler.clear();
      format.clear();
    }
    template<typename T>
    TPair& fromDB(TQuery &Qry, const T &adapter)
    {
      clear();
      handler.fromDB<T>(Qry, adapter);
      format.fromDB<T>(Qry, adapter);
      return *this;
    }
    template<typename T>
    const TPair& toDB(TQuery &Qry, const T &adapter) const
    {
      handler.toDB<T>(Qry, adapter);
      format.toDB<T>(Qry, adapter);
      return *this;
    }
};

class TSetDetails
{
  public:
    std::list<TPair> dup;
    std::string msg_type;
    void clear()
    {
      dup.clear();
      msg_type.clear();
    }
    bool empty() const
    {
      return dup.empty();
    }
    void fromDB(const int &id);
};

class TBagMessageSetDetails : public TSetDetails
{
  public:
    TBagMessageSetDetails(const std::list<std::string> &handlers);
};

class TQueueId
{
  public:
    int id;
    double put_order;
    TQueueId(int p1, double p2) : id(p1), put_order(p2) {}
};

class TQueueMsg
{
  public:
    THandler handler;
    TFormat format;
    std::string type;
    std::string content;
};

class TProcess
{
  private:
    std::string code;
    std::set<THandler> handlers;
    void getHandlers(std::set<THandler> &handlers);
    bool locked;
  public:
    TProcess(const std::string &p);
    bool lockHandler(const std::string &handler);
    void unlockHandlers(bool rollback=false);
    const std::set<THandler>& getHandlers() const {return handlers;}
    boost::optional<TQueueId> nextMsg();
};

class TQueue
{
  public:
    static void releaseProcess(int id);
    static void assignProcess(int id, const std::string &process);
    static void put(const TSetDetails &setDetails, const std::string &conseq_key, const std::string &content);
    static void get(int id, boost::optional<TQueueMsg> &msg);
    static boost::optional<TQueueId> next(const std::string &handler);
    static void complete_attempt(int id, const std::string &error="");
};

std::string paramValue(const THandler &handler, const std::string &name, const boost::optional<std::string> &defaultValue=boost::none);

} //namespace AstraMessages

#endif /*_MSG_QUEUE_H_*/
