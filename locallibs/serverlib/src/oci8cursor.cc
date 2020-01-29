#include <cstring>
#include <sstream>
#include <algorithm>
#include "cursctl.h"
#include "oci8cursor.h"
#include "dates_oci.h"
#define NICKNAME "MIKHAIL"
#include "slogger.h"

namespace OciCpp {

const char* status2str(int status)
{
  return 
    status==OCI_SUCCESS?           "OCI_SUCCESS":
    status==OCI_NEED_DATA?         "OCI_NEED_DATA":
    status==OCI_NO_DATA?           "OCI_NO_DATA":
    status==OCI_SUCCESS_WITH_INFO? "OCI_SUCCESS_WITH_INFO":
    status==OCI_ERROR?             "OCI_ERROR":
    status==OCI_INVALID_HANDLE?    "OCI_INVALID_HANDLE":
    status==OCI_STILL_EXECUTING?   "OCI_STILL_EXECUTING":
    status==OCI_CONTINUE?          "OCI_CONTINUE":
                                   "\?\?\?";
}






class TOci8NativeLobDef : public TOci8Native
{
  public:
    enum bufferType_t
    {
        bufString,
        bufCharVector,
        bufUCharVector,
    };
  private:
    OCILobLocator* lob_loc;
    ub2 data_type;
    bufferType_t bufType;
    typedef std::string StringLob_buf_t;
    typedef std::vector<char> CharVecLob_buf_t;
    typedef std::vector<unsigned char> UCharVecLob_buf_t;
    class buf_vec_proxy_ptr
    {
        CharVecLob_buf_t * ch;
        UCharVecLob_buf_t * uch;
    public:
        buf_vec_proxy_ptr(void * p, bufferType_t t)
            : ch((t == bufCharVector) ? reinterpret_cast<CharVecLob_buf_t *>(p) :nullptr)
            , uch((t == bufUCharVector) ? reinterpret_cast<UCharVecLob_buf_t *>(p) :nullptr)
        {}
        UCharVecLob_buf_t::size_type size() const
        {
            if(ch) return ch->size();
            return uch->size();
        }
        void resize(UCharVecLob_buf_t::size_type count)
        {
            if(ch) ch->resize(count);
            else uch->resize(count);
        }
        void * ptr_at(UCharVecLob_buf_t::size_type pos)
        {
            if(ch) return &(ch->at(pos));
            return &(uch->at(pos)); 
        }
    };

  public:
    TOci8NativeLobDef(Oci8Session& os, ub2 data_type_, bufferType_t bt)
        :data_type(data_type_), bufType(bt)
    {
        if(data_type_ != SQLT_BLOB && data_type_ != SQLT_CLOB)
        {
            throw ociexception("TOci8NativeLobDef: Invalid Oracle datatype! Only SQLT_BLOB and  SQLT_CLOB is valid ");
        }
        if (!(lob_loc = createLobLocator(os)))
        {
            throw ociexception("Lob locator creation failed");
        }
    }
    virtual dvoid* data() {
            return &lob_loc;
    }
    virtual ub2 type() const { return data_type; }
    virtual sb4 size() const { return 0; } // 0 size for LOBs

    virtual void get_data(Oci8Session& os,void* data, sb2 indicator) const;
    virtual void get_data(Oci8Session& os,void* data, const dvoid* buf) const { throw ociexception("TOci8NativeLobDef: get_data(os,data,buf)"); }
    virtual void set_data(Oci8Session& os,dvoid* buf,const void* data) const { throw ociexception("TOci8NativeLobDef: set_data(os,buf,data)"); }

    virtual ~TOci8NativeLobDef() {
        OCIDescriptorFree((dvoid*)lob_loc, OCI_DTYPE_LOB);
    };
};

int Curs8Ctl::throw_ocierror(const char* n, const char* f, int l, const char* freetext)
{
    constexpr OCIStmt* stmt = nullptr;
  return throw_ocierror(n, f, l, stmt, freetext);
}

int Curs8Ctl::throw_ocierror(const char* n, const char* f, int l, OCIStmt* stmthp, const char* freetext)
{
  sb4 errcode = 0;

  char errbuf[1024]={0};
  bool throw_exception = false;
  switch (os.status)
  {
      case OCI_SUCCESS:
          //LogTrace(TRACE5)<<"throw_ocierror("<<f<<':'<<l<<") : os.status="
          //  <<status2str(os.status)<<"("<<os.status<<")";
          break;
      case OCI_NEED_DATA:
          strcpy(errbuf, "ERROR - OCI_NEED_DATA");
          throw_exception = true;
          break;
      case OCI_NO_DATA:
          strcpy(errbuf, "Error - OCI_NO_DATA");
      case OCI_SUCCESS_WITH_INFO:
      case OCI_ERROR:
          OCIErrorGet((dvoid*)os.errhp, 1, 0, &errcode, reinterpret_cast<OraText*>(errbuf), sizeof(errbuf)-1, OCI_HTYPE_ERROR);
          if (no_throw.find(errcode) == no_throw.end())
          {
              throw_exception = true;
          }
          else
          {
            LogTrace(getTraceLev(TRACE5),nick,file,line)
                <<__func__<<'('<<f<<':'<<l<<") : os.status="
                <<status2str(os.status)<<'('<<os.status<<") no_throw: errcode="
                <<errcode;
          }
          break;
      case OCI_INVALID_HANDLE:
          strcpy(errbuf, "ERROR - OCI_INVALID_HANDLE");
          throw_exception = true;
          errcode = OCI_INVALID_HANDLE;
          break;
      case OCI_STILL_EXECUTING:
          strcpy(errbuf, "ERROR - OCI_STILL_EXECUTING");
          throw_exception = true;
          errcode = OCI_STILL_EXECUTING;
          break;
      case OCI_CONTINUE:
          strcpy(errbuf, "ERROR - OCI_CONTINUE");
          throw_exception = true;
          errcode = OCI_CONTINUE;
          break;
      default:
          strcpy(errbuf, "ERROR - default");
          throw_exception = true;
          break;
  }
  if(not throw_exception)
      return errcode;
  {
      LogTrace(TRACE1)<<"throw_ocierror("<<f<<':'<<l<<") : os.status="
        <<status2str(os.status)<<"("<<os.status<<")";
      LogError(nick,file,line) << '[' << os.getConnectString() << "] " << errbuf;
      LogError(nick,file,line) << query;
      LogTrace(getTraceLev(TRACE1),n,f,l) << "Oci errcode: "<< errcode;
      LogTrace(getTraceLev(TRACE1),n,f,l) << "Other text: "<< freetext;

      ub2 pos=0;
      if (stmthp!=NULL)
      {
        ub4 sizep = sizeof(pos);
        if (!OCIAttrGet(stmthp, OCI_HTYPE_STMT, &pos, &sizep, OCI_ATTR_PARSE_ERROR_OFFSET, os.errhp))
          pos=0;
      }
      else
      {
        LogTrace(getTraceLev(TRACE1),n,f,l)<<"No statement handler";
      }
      
      if (pos>0 && pos<query.size())
      {
        LogTrace(getTraceLev(TRACE1),n,f,l)<<"pos: "<<pos;

        size_t beg=pos>40?pos-40:0;
        size_t sep=pos>40?39:pos;
        
        LogTrace(getTraceLev(TRACE1),n,f,l)<<"Query: "<<query.substr(beg,40);
        LogTrace(getTraceLev(TRACE1),n,f,l)<<"       "<<std::string(sep,'-')<<"^"<<std::string(40,'-');
      }
      else
      {
        LogTrace(getTraceLev(TRACE1),n,f,l)<<"No query position info (pos="<<pos<<" query.size()="<<query.size()<<")";
      }
                      
      throw ociexception(errbuf, errcode, freetext);
  }
}

class TOci8NativeStringDef : public TOci8Native
{
    std::vector<char> buffer;
  public:
    TOci8NativeStringDef() : buffer(4096)
    {}

    virtual sb4 size() const override { return buffer.size(); }
    virtual ub2 type() const override { return SQLT_STR; }
    virtual dvoid* data() override { return buffer.data(); }

    virtual void get_data(Oci8Session& os,void* data,const dvoid* buf) const override
    {
        throw ociexception("TOci8NativeStringDef: get_data(os,data,buf)");
    }

    virtual void get_data(Oci8Session& os,void* data, sb2 indicator) const override
    {
        if (indicator < 0  )
        {
            reinterpret_cast<std::string*>(data)->clear();
        }
        else
        {
            reinterpret_cast<std::string*>(data)->assign(buffer.data());
        }
    }
    virtual void set_data(Oci8Session& os,dvoid* buf,const void* data) const override
    {
        throw ociexception("TOci8NativeStringDef: set_data(os,buf,data)");
    }

    virtual ~TOci8NativeStringDef() {};
};

int Curs8Ctl::throw_ocierror(const char* n, const char* f, int l, int errcode, const char* freetext)
{
  if (os.status)
  {
      LogTrace(TRACE5)<<"throw_ocierror("<<f<<':'<<l<<") : os.status="
        <<status2str(os.status)<<"("<<os.status<<") errcode="<<errcode;
  }
  bool throw_exception = false;
  if (errcode!=0 && no_throw.find(errcode) == no_throw.end())
  {
      throw_exception = true;
  }
  else
  {
    LogTrace(getTraceLev(TRACE5),nick,file,line)<<"no_throw: errcode="
      <<errcode;
  }
  if (throw_exception)
  {
      std::ostringstream oss;
      const char* errtext=NULL;
      if (errcode==CERR_NULL)
          errtext="ORA-01405: fetched column value is NULL\n";
      else if (errcode==CERR_TRUNC)
          errtext="ORA-01406: fetched column value was truncated\n";
      else
      {        
          oss<<"Oracle error "<<errcode<<std::endl;
          errtext=oss.str().c_str();
      }

      LogTrace(TRACE1)<<"throw_ocierror("<<f<<':'<<l<<") : os.status="
        <<status2str(os.status)<<"("<<os.status<<") errcode="<<errcode;
      
      LogError(nick,file,line) << '[' << os.getConnectString() << "] " << errtext;
      LogError(nick,file,line) << query;
      LogTrace(getTraceLev(TRACE1),nick,file,line) << "Oci errcode : "<< errcode;
      LogTrace(getTraceLev(TRACE1),nick,file,line) << "Other text: "<< freetext;
      throw ociexception(errtext, errcode, freetext);
  }
  return errcode;
}

void Curs8Ctl::on_create() // common part of constructor
{
  os.status = OCIStmtPrepare2(os.svchp, &stmthp, os.errhp,
                              reinterpret_cast<const OraText*>(query.c_str()), query.size(), // query
                              reinterpret_cast<const OraText*>(query.c_str()), query.size(), // cache key
                              OCI_NTV_SYNTAX, OCI_DEFAULT);

  if(os.status != OCI_SUCCESS and os.status != OCI_SUCCESS_WITH_INFO)
  {
      throw_ocierror(STDLOG,stmthp,"Parse error");
  }
  else if(os.status == OCI_SUCCESS_WITH_INFO)
  {
      os.status = OCI_SUCCESS;
  }
  noThrowError(NO_DATA_FOUND);
}

Curs8Ctl::Curs8Ctl(const std::string& q, Oci8Session* s)
    : nick(""), file(""), line(0), query(q),
      os(s ? *s : Oci8Session::instance(STDLOG)),
      no_cache(false)
{
  on_create();
}

Curs8Ctl::Curs8Ctl(const char* n, const char* f, int l, const std::string& q, Oci8Session* s)
    : nick(n), file(f), line(l), query(q), os(s ? *s : Oci8Session::instance(n,f,l)),
      no_cache(false)
{
  on_create();
}

Curs8Ctl::Curs8Ctl(const char* n, const char* f, int l, const std::string& q, Oci8Session* s,bool no_cache_)
    : nick(n), file(f), line(l), query(q), os(s ? *s : Oci8Session::instance(n,f,l)),
      no_cache(no_cache_)
{
  on_create();
}

Curs8Ctl::Curs8Ctl(Curs8Ctl&& old)
    :  nick(old.nick), file(old.file), line(old.line),
    query(std::move(old.query)),
    rows(old.rows),
    sts(std::move(old.sts)),
    def_vector(std::move(old.def_vector)),
    indicatores(std::move(old.indicatores)),
    natives(std::move(old.natives)),
    native_bind_buffers(std::move(old.native_bind_buffers)),
    no_throw(std::move(old.no_throw)),
    os(old.os),
    stmthp(old.stmthp),
    locators(std::move(old.locators)),
    no_cache(old.no_cache)
{
    // old one doesn't own handle anymore
    old.stmthp = nullptr;
}




Curs8Ctl::~Curs8Ctl()
{
  for(TOci8Native const* native:  natives)
  {
    delete native;
  }

  for(std::list<OCILobLocator*>::iterator it=locators.begin();it!=locators.end();++it)
  {
      if(not *it)
          continue;
      boolean is_temporary=false;
      OCILobIsTemporary(os.envhp,os.errhp,*it,&is_temporary);
      if(is_temporary)
          OCILobFreeTemporary(os.svchp,os.errhp,*it);
      OCIDescriptorFree(*it, OCI_DTYPE_LOB);
  }
  if (stmthp)
  {
      if((os.status = OCIStmtRelease(stmthp, os.errhp,
                      reinterpret_cast<const OraText*>(query.c_str()), query.size(), // cache key
                      (no_cache)?OCI_STRLS_CACHE_DELETE:OCI_DEFAULT))) try {
          throw_ocierror(STDLOG,"OCIStmtRelease error");
      } catch(const ociexception& e) {
          LogError(STDLOG)<<"OCIStmtRelease() :: "<<e.what();
      }
  }
}

namespace {

sword bind_internal(OCIStmt* stmt, OCIError* err, const char* name,  dvoid* data, sb4 len, ub2 type, ub4 skip, bool with_skip)
{
  OCIBind* bin=NULL;
  sword res=OCIBindByName(stmt, &bin, err, (const text*)name, -1,
                             data, len, type, 0, 0, 0, 0, 0, OCI_DEFAULT);
  if(!res)
    res=OCIBindArrayOfStruct(bin, err, with_skip?skip:0, 0, 0, 0);

  return res;
}

}

Curs8Ctl& Curs8Ctl::lbindArr(const char* name, const char* data, size_t len, ub4 skip,ub2* alenp, ub4 alskip, ub2 type)
{
  LogTrace(TRACE5)<<"lbindArr type="<<type;
  
  if (static_cast<size_t>(len)>=sizeof(boost::posix_time::ptime) && type==SQLT_DAT)
    throw_ocierror(STDLOG,"boost::posix_time::ptime unsupported");
  
  OCIBind* bin=NULL;
  sword res=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                             (dvoid*)data, len, type, 0, alenp, 0, 0, 0, OCI_DEFAULT);
  if(!res)
    res=OCIBindArrayOfStruct(bin, os.errhp, skip, 0, alskip, 0);

  if(res)
  {
      std::ostringstream os;
      os<<"Ошибка lbindArr: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}


Curs8Ctl& Curs8Ctl::bind(const char* name, dvoid* data, sb4 len, ub2 type)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",common): data="<<std::hex<<data<<std::dec<<" len="<<len<<" type="<<type;
  
  if (static_cast<size_t>(len)>=sizeof(boost::posix_time::ptime) && type==SQLT_DAT) 
    return bindN__<TOci8NativeDate_PTIME>(name,data);
  
  if((os.status=bind_internal(stmthp, os.errhp, name, data, len, type, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind common: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const char& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",char): data="<<(int)data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind char: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const char& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",char): data="<<(int)data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind char array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind int array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const unsigned int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",unsigned int&): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned int&: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const unsigned int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",unsigned int&): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned int& array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const long long int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long long int): data="<<data;

  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind long long int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
    
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const boost::posix_time::ptime& data)
{
  return bindN__<TOci8NativeDate_PTIME>(name,&data);
}

// allow on ORA instant client 11.02
Curs8Ctl& Curs8Ctl::bindArr(const char* name, const long long int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind long long int array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const unsigned long long int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",unsigned long long int): data="<<data;  

  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned long long int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
    
  return *this;
      
}

// allow on ORA instant client 11.02
Curs8Ctl& Curs8Ctl::bindArr(const char* name, const unsigned long long int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",unsigned long long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned long long int array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindVec_internal__(TOci8Native const& native,size_t size,
  const char* name, const dvoid* data,size_t skip, bool with_skip)
{
  //LogTrace(TRACE5)<<"bindVec("<<name<<") with_skip="<<std::boolalpha<<with_skip;

  if (size==0)
    throw_ocierror(STDLOG,"Invalid size");
  
  sb4 native_size=native.size();
  
  native_bind_buffers.push_back( std::vector<char>(native_size*size) );
  
  char* buf_ptr=&(native_bind_buffers.back()[0]);
  for(size_t i=0; i<size; ++i)
  {
    native.set_data(os,buf_ptr+(i*native_size),(char*)data+skip*i);
    if( os.status )
      throw_ocierror(STDLOG,"Ошибка native.set_data");
  }
  
  if((os.status=bind_internal(stmthp, os.errhp, name, buf_ptr, native_size, native.type(), native_size, with_skip) ))
  {
      std::ostringstream os;
      os<<"Ошибка bindVec: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const long int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind long int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const long int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_INT, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind long int array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const unsigned long int& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned long int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const unsigned long int& data, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",long int): data="<<data;
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)&data, sizeof(data), SQLT_UIN, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind unsigned long int array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindOut(const char* name, int& data)
{
  //LogTrace(TRACE5)<<"bindOut("<<name<<",int): data="<<data;
  OCIBind* bin=NULL;
  if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                             (dvoid*)&data, sizeof(data), SQLT_INT, 0, 0, 0, 0, 0, OCI_DEFAULT)) )
  {
      std::ostringstream os;
      os<<"Ошибка bind int: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const char* data, size_t len)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",char*,len): data='"<<std::string(data,len).c_str()<<"'";
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)data, len, SQLT_STR, 0, false) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind char*,len: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bindArr(const char* name, const char* data, size_t len, ub4 skip)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",char*,len): data='"<<std::string(data,len).c_str()<<"'";
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)data, len, SQLT_STR, skip, true) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind char*,len array: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const char* data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",char*): data='"<<data<<"'";
  if((os.status=bind_internal(stmthp, os.errhp, name, (dvoid*)data, strlen(data)+1, SQLT_STR, 0, false) ))
      throw_ocierror(STDLOG,"Ошибка bind char*");
  return *this;
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const std::string& data)
{
  if(data.empty())
      return bind(name,"");
  else
  {
      //LogTrace(TRACE5)<<"bind("<<name<<",string): data='"<<data<<"'";
      OCIBind* bin=NULL;
      char last_symb = *data.rbegin();
      if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                                 (dvoid*)data.c_str(), last_symb ? data.size()+1 : data.size(),
                                 SQLT_STR, 0, 0, 0, 0, 0, (ub4) OCI_DEFAULT)) )
          throw_ocierror(STDLOG,"Ошибка bind std::string");
      return *this;
  }
}

Curs8Ctl& Curs8Ctl::bind(const char* name, const oracle_datetime& data)
{
  //LogTrace(TRACE5)<<"bind("<<name<<",date): data="<<data<<".";
  OCIBind* bin=NULL;
  if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                             (dvoid*)data.value, sizeof(data.value), SQLT_DAT, 0, 0, 0, 0, 0, OCI_DEFAULT) ))
  {
      std::ostringstream os;
      os<<"Ошибка bind date: '"<<name<<"'";
      throw_ocierror(STDLOG,os.str().c_str());
  }
  return *this;
}

sb4 get_data_callaback(dvoid* ctx, OCIDefine* dfnhp, ub4 iter, dvoid** buf, ub4** alen, ub1* piece, dvoid** ind, ub2** rc)
{
  return 0;
}

sword def_internal_arr(OCIStmt* stmt, OCIError* err, std::list<select8_type>& sts, ub4 position, dvoid* data, sb4 len, ub2 type, ub4 skip)
{
  OCIDefine* defnp;

  if (sts.size()+1==position)
    sts.push_back(select8_type(type,0,(dvoid*)data,len,NULL,NULL));
  else if (sts.size()+1<position)
  {
     LogTrace(TRACE5)<<" sts.size()="<<sts.size()<<" position="<<position;
     throw ociexception("Ошибка - неверная последовательность и/или position");
  }
  
  sword res = OCIDefineByPos(stmt, &defnp, err, position, data, len, type, 0, 0, 0, OCI_DEFAULT);
    
  if (!res)  
    res=OCIDefineArrayOfStruct(defnp, err, skip, 0, 0, 0);
  return res;
}

sword def_internal(OCIStmt* stmt, OCIError* err, 
  std::list<select8_type>& sts, dvoid* data, sb4 len, ub2 type,
  TOci8Native* native=NULL)
{
  OCIDefine* defnp;
  
  sts.push_back(select8_type(type,0,data,len,native,NULL));

  sword res = 0;
  
  if(native!=NULL)
  {
      bool need_indicator = (sts.back().type == SQLT_STR || sts.back().type == SQLT_BLOB || sts.back().type == SQLT_CLOB);
      res = OCIDefineByPos(stmt, &defnp, err, sts.size(), native->data(), native->size(), sts.back().type,
                          need_indicator ? &sts.back().indicator : 0, 0, 0, OCI_DEFAULT);
  }
  else
  {
    res = OCIDefineByPos(stmt, &defnp, err, sts.size(), sts.back().data, len, sts.back().type,
                          sts.back().type == SQLT_STR ? &sts.back().indicator : 0, 0, 0, OCI_DEFAULT);
  }
  if (!res)  
    res=OCIDefineArrayOfStruct(defnp, err, 0, 0, 0, 0);
  return res;
}


Curs8Ctl& Curs8Ctl::def(char* data, size_t len)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)data, len, SQLT_STR) ))
      throw_ocierror(STDLOG,"Ошибка define char*");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, char* data, size_t len, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)data, len, SQLT_STR, skip) ))
      throw_ocierror(STDLOG,"Ошибка define char* array");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(char& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_INT) ))
      throw_ocierror(STDLOG,"Ошибка define char");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, char& data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_INT, skip) ))
      throw_ocierror(STDLOG,"Ошибка define char");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(unsigned int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_UIN) ))
      throw_ocierror(STDLOG,"Ошибка define size_t");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, unsigned int& data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_UIN, skip) ))
      throw_ocierror(STDLOG,"Ошибка define size_t array");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_INT) ))
      throw_ocierror(STDLOG,"Ошибка define int");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, int& data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_INT, skip) ))
      throw_ocierror(STDLOG,"Ошибка define int");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(unsigned long int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_UIN) ))
      throw_ocierror(STDLOG,"Ошибка define unsigned long");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, unsigned long int & data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_UIN, skip) ))
      throw_ocierror(STDLOG,"Ошибка define unsigned long array");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(long int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_INT) ))
      throw_ocierror(STDLOG,"Ошибка define long");
  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, long int & data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_INT, skip) ))
      throw_ocierror(STDLOG,"Ошибка define long array");

  return *this;
}

Curs8Ctl& Curs8Ctl::def(boost::posix_time::ptime& data)
{
    TOci8Native *native=new TOci8NativeDateDef_PTIME();
    natives.push_back(native);

    if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), native->type(), native) ))
        throw_ocierror(STDLOG,"Ошибка define boost::posix_time::ptime");

    return *this;
}

Curs8Ctl& Curs8Ctl::def(std::string& data)
{
    TOci8Native *native=new TOci8NativeStringDef();
    natives.push_back(native);

    if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), native->type(), native) ))
        throw_ocierror(STDLOG,"Ошибка define std::string");

    return *this;
}



Curs8Ctl& Curs8Ctl::def(long long int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_INT) ))
      throw_ocierror(STDLOG,"Ошибка define long long int");
    
  return *this;
}

Curs8Ctl& Curs8Ctl::def(unsigned long long int& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)&data, sizeof(data), SQLT_INT) ))
      throw_ocierror(STDLOG,"Ошибка define long long int");
  
  return *this;
}

/*
Curs8Ctl& Curs8Ctl::defArr(ub4 position, long long int& data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_INT, skip) ))
      throw_ocierror(STDLOG,"Ошибка define long long int array");

  return *this;
}

Curs8Ctl& Curs8Ctl::defArr(ub4 position, unsigned long long int& data, ub4 skip)
{
  if(( os.status = def_internal_arr(stmthp, os.errhp, sts, position, (dvoid*)&data, sizeof(data), SQLT_UIN, skip) ))
      throw_ocierror(STDLOG,"Ошибка define unsigned long long int array");

  return *this;
}

*/

Curs8Ctl& Curs8Ctl::def(oracle_datetime& data)
{
  if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)data.value, sizeof(data.value), SQLT_DAT) ))
      throw_ocierror(STDLOG,"Ошибка define date");
  return *this;
}

Curs8Ctl& Curs8Ctl::bindlob(const char* name, OCILobLocator* clob, sb4 data_type)
{
  if(clob)
  {
      locators.push_back(clob);
      OCIBind* bin=NULL;
      if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                                 (dvoid*)&locators.back(), (sb4)-1, data_type, 0, 0, 0, 0, 0, OCI_DEFAULT)) )
          throw_ocierror(STDLOG,"Ошибка bind clob");
      return *this;
  }
  else
      throw ociexception("lob creation failed");
  return *this;

}

Curs8Ctl& Curs8Ctl::bindBlob(const char* name, const std::string& data)
{
    return bindblob(name, data);
}

Curs8Ctl& Curs8Ctl::bindBlob(const char* name, const std::vector<char>& data)
{
    return bindblob(name, data);
}

Curs8Ctl& Curs8Ctl::bindBlob(const char* name, const std::vector<uint8_t>& data)
{
    return bindblob(name, data);
}

Curs8Ctl& Curs8Ctl::bindClob(const char* name, const std::string& data)
{
    return bindclob(name, data);
}

Curs8Ctl& Curs8Ctl::bindClob(const char* name, const std::vector<char>& data)
{
    return bindclob(name, data);
}

Curs8Ctl& Curs8Ctl::bindClob(const char* name, const std::vector<uint8_t>& data)
{
    return bindclob(name, data);
}


template<typename T>
Curs8Ctl& Curs8Ctl::bindblob(const char* name, const T& data)
{
    //LogTrace(TRACE5)<<"bind(" << name << ",blob): data.size()=" << data.size();
    if(data.size() < 1024)
    {
        OCIBind* bin=NULL;
        if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                                    data.empty() ? nullptr : (dvoid*)data.data(), (sb4)data.size(),
                                    SQLT_LBI, 0, 0, 0, 0, 0, OCI_DEFAULT)) )
            throw_ocierror(STDLOG,"Ошибка bind SQLT_LBI to blob");
        return *this;
    }
    else
    {
        return bindlob(name, createLob(os, data, OCI_TEMP_BLOB), SQLT_BLOB);
    }
}

template<typename T>
Curs8Ctl& Curs8Ctl::bindclob(const char* name, const T& data)
{
    //LogTrace(TRACE5)<<"bind(" << name << ",clob): data.size()=" << data.size();
    if(data.empty())
    {
        OCIBind* bin=NULL;
        if((os.status=OCIBindByName(stmthp, &bin, os.errhp, (const text*)name, -1,
                                    nullptr, 0, SQLT_STR, 0, 0, 0, 0, 0, OCI_DEFAULT)) )
            throw_ocierror(STDLOG,"Ошибка bind null to clob as SQLT_STR");
        return *this;
    }
    else
        return bindlob(name, createLob(os, data, OCI_TEMP_CLOB), SQLT_CLOB);
}

struct CheckNulls_and_Native
{
    Oci8Session& os;
    CheckNulls_and_Native(Oci8Session& os_) : os(os_) {};
    
    void operator()(select8_type& st) const
    {
        if(st.def_vector!=NULL)
          return;
        if (st.native != NULL)
        {
          st.native->get_data(os,st.data, st.indicator);
          if( os.status )
            throw ociexception("Ошибка native->get_data");
        }
        else if(st.indicator < 0 and st.type == SQLT_STR and st.data)
        {
            ((char*)st.data)[0] = '\0';
        }
    }
};

int Curs8Ctl::EXfetVec(size_t a_load_by)
{
  tst();
  int errcode=exec();
  if (errcode!=0)
    return errcode;

  tst();
  size_t load_by=a_load_by<1?1:a_load_by;
  std::vector<ub2> errcodes( def_vector.size() * load_by , 0);

  std::vector< std::vector<char> > native_bind_buffers;
  for(select8_type const& t: sts)
  {
    if (t.def_vector!=NULL && t.native!=NULL)
      native_bind_buffers.push_back( std::vector<char>( t.native->size() * load_by ) );
  }

  size_t size=0;
  for(bool key=true; key;)
  {
    for(TDefVector const& d: def_vector)
    {
      d.resize(d.ptr,size+load_by);
      if (d.indicator!=NULL)
        d.indicator->resize(size+load_by,0);
    }

    {
      size_t natives_ind=0;
      size_t pos=0;
      size_t vec_pos=0;
      size_t inds_count=0;
      size_t al_count=0;
      for(select8_type const& t: sts)
      {
        ++pos;
        if (t.def_vector==NULL)
          continue;
        TDefVector const& d=*t.def_vector;
  
        OCIStmt* stmt=stmthp;
        OCIError* err=os.errhp;    
        ub2 type=t.type;
        sb4 len=t.size;
  
        std::vector<sb2> *indicator=d.indicator;
        
        OCIDefine* defnp=NULL;
  //      LogTrace(TRACE5)<<"pos="<<pos<<" len="<<len<<" size="<<size
  //        <<" d.offset="<<d.offset<<" addr="<<std::hex<<d.addr(d.ptr,size);
  //      LogTrace(TRACE5)<<indicator;
        sword res = 0;
        
        if (t.def_ind==NULL)
        {
          if (t.native!=NULL)
          {
            char* ptr=&(native_bind_buffers[natives_ind][0]);
            res = OCIDefineByPos(stmt, &defnp, err, pos, ptr, t.native->size(), t.native->type(),
                                  (indicator!=NULL ? &(*indicator)[size] : 0), 0, &errcodes[0]+vec_pos*load_by, OCI_DEFAULT);
            if (!res)  
              res=OCIDefineArrayOfStruct(defnp, err, t.native->size(), (indicator!=NULL ? sizeof((*indicator)[0]) : 0), 0, sizeof(errcodes[0]));
            natives_ind++;
          }
          else
          {
            res = OCIDefineByPos(stmt, &defnp, err, pos, (char*)d.addr(d.ptr,size)+d.offset, len, type,
                                (indicator!=NULL ? &(*indicator)[size] : 0), 0, &errcodes[0]+vec_pos*load_by, OCI_DEFAULT);
            if (!res)  
              res=OCIDefineArrayOfStruct(defnp, err, d.skip, (indicator!=NULL ? sizeof((*indicator)[0]) : 0), 0, sizeof(errcodes[0]));
          }
        }
        else
        {
          ++inds_count;
          
          TDefVector const& i=*t.def_ind;
          TDefVector const* al=t.def_al;
          
          if (al!=NULL)
            ++al_count;

          if (t.native!=NULL)
          {
            char* ptr=&(native_bind_buffers[natives_ind][0]);
            res = OCIDefineByPos(stmt, &defnp, err, pos, ptr, t.native->size(), t.native->type(),
                                (char*)i.addr(i.ptr,size)+i.offset,
                                (al==NULL)?NULL:(ub2*)((char*)al->addr(al->ptr,size)+al->offset), 
                                &errcodes[0]+vec_pos*load_by, OCI_DEFAULT);
            if (!res)  
              res=OCIDefineArrayOfStruct(defnp, err, t.native->size(), i.skip,(al==NULL)?0:al->skip, sizeof(errcodes[0]));
            natives_ind++;
          }
          else
          {
            res = OCIDefineByPos(stmt, &defnp, err, pos, (char*)d.addr(d.ptr,size)+d.offset, len, type,
                                (char*)i.addr(i.ptr,size)+i.offset, 
                                (al==NULL)?NULL:(ub2*)((char*)al->addr(al->ptr,size)+al->offset), 
                                &errcodes[0]+vec_pos*load_by, OCI_DEFAULT);
            if (!res)  
              res=OCIDefineArrayOfStruct(defnp, err, d.skip, i.skip,(al==NULL)?0:al->skip, sizeof(errcodes[0]));
          }
        }
  
        if (res)
          errcode = throw_ocierror(STDLOG,"def array");
        
        if (errcode!=0)
          return errcode;
        
        ++vec_pos;
      }
  
      if (def_vector.size()!=vec_pos+inds_count+al_count)
      {
        LogError(STDLOG)<<"def_vector.size()="<<def_vector.size()<<" vec_pos="
          <<vec_pos<<" inds_count="<<inds_count<<" al_count="<<al_count;
        throw ociexception("Internal error");
      }
    }    
    
    errcode=this->fen(load_by);
    LogTrace(TRACE5)<<"errcode="<<errcode<<" rows="<<rows;

    if (errcode==NO_DATA_FOUND)
    {
      key=false;
      errcode=0;
    }
    if (errcode!=0)
      return errcode;

    size_t readed=rows;
    
    {
      size_t natives_ind=0;
      size_t vec_pos=0;
      for(select8_type const& t: sts)
      {
        if (t.def_vector==NULL)
          continue;
        TDefVector const& d=*t.def_vector;
        size_t col_ind=vec_pos*load_by;
        for(size_t row=0; row<readed-size; ++row)
        {
          int err=errcodes[row+col_ind];
          
          // NULL supported by local & external indicator 
          if ( err==CERR_NULL /* ORA-01405: fetched column value is NULL */
            && (d.indicator!=NULL || t.def_ind!=NULL))
            continue;

          // TRUNC supported by local(???) & external indicator
          if (err==CERR_TRUNC /* ORA-01406: fetched column value was truncated */
            && (d.indicator!=NULL || t.def_ind!=NULL))
            continue;

            
          if (err!=0)
          {
            LogTrace(TRACE1)<<"vec_pos="<<vec_pos<<" row="<<row<<" : "<<err;
            return errcode=throw_ocierror(STDLOG, err, "Colunm error during vector fetch");
          }

          if (t.native!=NULL)
          {
            char* ptr=&(native_bind_buffers[natives_ind][0]);
            
            t.native->get_data(os,(char*)d.addr(d.ptr,size)+d.offset+ d.skip*row,ptr+t.native->size()*row);
            if( os.status )
              throw ociexception("Ошибка native->get_data (vec fetch)");
          }
        }

        if (t.native!=NULL)
          natives_ind++;

        ++vec_pos;
      }
    }
    
    LogTrace(TRACE5)<<"size="<<size<<" readed="<<readed<<" key="<<std::boolalpha<<key<<" load_by="<<load_by;
    size+=load_by;

    if (readed<size)
    {  
      for(TDefVector const& d: def_vector)
      {
        d.resize(d.ptr,readed);
        if (d.indicator!=NULL)
          d.indicator->resize(readed);
      }
    }
  }

  // Check indicators    
  for(select8_type const& t: sts)
  {
    if (t.def_vector==NULL || t.type!=SQLT_STR)
      continue;
    std::vector<sb2> *indicator=t.def_vector->indicator;
    
    if (indicator==NULL)
      continue;

    for(size_t i=0; i<indicator->size(); ++i)
    {
      if((*indicator)[i] < 0)
      {
        TDefVector const& d=*t.def_vector;
        ((char*)(d.addr(d.ptr,i)))[0] = '\0';
      }
    }
  }

  LogTrace(TRACE5)<<"errcode="<<errcode<<" rows="<<rows;
  
  if (rows==0 && errcode==0)
    errcode=NO_DATA_FOUND;

  return errcode;
}


int Curs8Ctl::EXfet(ub4 mode)
{
  int errcode = 0;
  if((os.status = OCIStmtExecute(os.svchp, stmthp, os.errhp, 1, 0, 0, 0, mode)))
      errcode = throw_ocierror(STDLOG,"OCIStmtExecute");

  ub4 sizep = sizeof(rows);
  if((os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rows, &sizep, OCI_ATTR_ROWS_FETCHED, os.errhp)))
      throw_ocierror(STDLOG,"Ошибка получения атрибута");

  if (rows>0)
    for_each(sts.begin(), sts.end(), CheckNulls_and_Native(os));

  return errcode;
}

int Curs8Ctl::exec(ub4 mode)
{
  int errcode = 0;

  ub4 type = 0;
  ub4 sizet = sizeof(type);
  if(( os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &type, &sizet, OCI_ATTR_STMT_TYPE, os.errhp) ))
      throw_ocierror(STDLOG,"OCIAttrGet(OCI_ATTR_STMT_TYPE)");
//  if(type == OCI_STMT_SELECT and mode == OCI_COMMIT_ON_SUCCESS

  if(( os.status = OCIStmtExecute(os.svchp, stmthp, os.errhp, type == OCI_STMT_SELECT ? 0 : 1, 0, 0, 0, mode) ))
      errcode = throw_ocierror(STDLOG,"OCIStmtExecute");

  ub4 sizep = sizeof(rows);
  if(( os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rows, &sizep, OCI_ATTR_ROWS_FETCHED, os.errhp) ))
      throw_ocierror(STDLOG,"OCIAttrGet(OCI_ATTR_ROWS_FETCHED)");

  if(type == OCI_STMT_SELECT && rows>0)
      for_each(sts.begin(), sts.end(), CheckNulls_and_Native(os));

  return errcode;
}

int Curs8Ctl::execn(ub4 iters,ub4 mode,ub4 rowoff)
{
  int errcode = 0;

  ub4 type = 0;
  ub4 sizet = sizeof(type);
  LogTrace(TRACE5)<<"execn: iters="<<iters<<" rowoff="<<rowoff;
  
  if(( os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &type, &sizet, OCI_ATTR_STMT_TYPE, os.errhp) ))
      throw_ocierror(STDLOG,"OCIAttrGet(OCI_ATTR_STMT_TYPE)");
  if(type == OCI_STMT_SELECT && rowoff>0)
    throw ociexception("Cannot set rowoff for SELECT statement");

  if(( os.status = OCIStmtExecute(os.svchp, stmthp, os.errhp, type == OCI_STMT_SELECT ? 0 : iters, type == OCI_STMT_SELECT ? 0 : rowoff , 0, 0, mode) ))
  {
    errcode = throw_ocierror(STDLOG,stmthp,"OCIStmtExecute");
  }

  ub4 sizep = sizeof(rows);
  if(( os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rows, &sizep, OCI_ATTR_ROW_COUNT, os.errhp) ))
      throw_ocierror(STDLOG,"OCIAttrGet(OCI_ATTR_ROW_COUNT)");
    
  if(type == OCI_STMT_SELECT && rows>0)
      for_each(sts.begin(), sts.end(), CheckNulls_and_Native(os));

  return errcode;
}

int Curs8Ctl::fen(ub4 nrows)
{
  int errcode = 0;

  if((os.status = OCIStmtFetch(stmthp, os.errhp, nrows, OCI_FETCH_NEXT, OCI_DEFAULT)))
  {
    errcode = throw_ocierror(STDLOG,"fen failed");
  }
    
  ub4 sizep = sizeof(rows);
  if((os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rows, &sizep, OCI_ATTR_ROW_COUNT, os.errhp)))
      throw_ocierror(STDLOG,"Ошибка получения атрибута");

  if(rows>0)
  {
    sb4 rows_now=0;
    sizep = sizeof(rows_now);
    if(( os.status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rows_now, &sizep, OCI_ATTR_ROWS_FETCHED, os.errhp) ))
        throw_ocierror(STDLOG,"OCIAttrGet(OCI_ATTR_ROWS_FETCHED)");
    
    if (rows_now>0)  
        for_each(sts.begin(), sts.end(), CheckNulls_and_Native(os));
  }

  return errcode;
}

Curs8Ctl& Curs8Ctl::noThrowError(int err)
{
  no_throw.insert(err);
  return *this;
}

Curs8Ctl& Curs8Ctl::throwError(int err)
{
  no_throw.erase(err);
  return *this;
}

Curs8Ctl& Curs8Ctl::defClob(std::string& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_CLOB, TOci8NativeLobDef::bufString );
}

Curs8Ctl& Curs8Ctl::defClob(std::vector<char>& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_CLOB, TOci8NativeLobDef::bufCharVector );
}

Curs8Ctl& Curs8Ctl::defClob(std::vector<unsigned char>& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_CLOB, TOci8NativeLobDef::bufUCharVector );
}

Curs8Ctl& Curs8Ctl::defBlob(std::string& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_BLOB, TOci8NativeLobDef::bufString );
}

Curs8Ctl& Curs8Ctl::defBlob(std::vector<char>& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_BLOB, TOci8NativeLobDef::bufCharVector );
}

Curs8Ctl& Curs8Ctl::defBlob(std::vector<unsigned char>& data)
{
    return deflob(reinterpret_cast<dvoid*>(&data), SQLT_BLOB, TOci8NativeLobDef::bufUCharVector );
}


template <typename T>
Curs8Ctl& Curs8Ctl::deflob(dvoid* data, ub2 data_type, T buftype)
{
    TOci8Native * native = new TOci8NativeLobDef(os, data_type, buftype);
    natives.push_back(native);
    if(( os.status = def_internal(stmthp, os.errhp, sts, (dvoid*)data, sizeof(data), native->type(), native) ))
        throw_ocierror(STDLOG,"Ошибка define LobLocator");
    return *this;
}

void TOci8NativeLobDef::get_data(Oci8Session& os, void* data, sb2 indicator) const
{
    //Reading LOB
    if (indicator < 0 )
    {
        LogTrace(TRACE5) << "Null value fetched, clearing output variable";
        switch (bufType)
        {
        case bufString:
            reinterpret_cast<StringLob_buf_t*>(data)->clear();
            break;
        case bufCharVector:
            reinterpret_cast<CharVecLob_buf_t*>(data)->clear();
            break;
        case bufUCharVector:
            reinterpret_cast<UCharVecLob_buf_t*>(data)->clear();
            break;
        default:
            throw ociexception("Неверный тип LOB буфера");
        }
        return;
    }

    bool done=false;
    sword err = 0;
    CharVecLob_buf_t internal_buf;
    std::unique_ptr<buf_vec_proxy_ptr> buf;
    switch (bufType)
    {
        case bufString:
            buf.reset(new buf_vec_proxy_ptr(&internal_buf, bufCharVector));
            break;
        case bufCharVector:
        case bufUCharVector:
            LogTrace(TRACE5) << "Vector type, using external buffer ";
            buf.reset(new buf_vec_proxy_ptr(data, bufType));
            break;
        default:
            throw ociexception("Неверный тип LOB буфера");
    }


    size_t bufoffset = 0;
    ub4 amt=0;
    ub4 offset=1;
    ub4 buflen = 0;
    ub4 LobLen = 0;
    ub4 ChunkSize = 0;


    if ((err= OCILobGetChunkSize(os.svchp,os.errhp, lob_loc, &ChunkSize)))
    {
      if(os.checkerr(STDLOG,err))
      {
          throw ociexception("Ошибка OCILobGetChunkSize");
      }
    }

    if ((err= OCILobGetLength(os.svchp,os.errhp, lob_loc, &LobLen)))
    {
      if(os.checkerr(STDLOG,err))
      {
         throw ociexception("Ошибка OCILobGetLength");
      }
    }

   // only for test
   //LobLen /= 2;
   //LobLen = 0;


    ProgTrace(TRACE5, "Lob length = %i  Lob chunk = %i ", LobLen, ChunkSize );
    // small overhead here to avoid reallocations and correct zero length Lob reading
    buf->resize(LobLen + ChunkSize);
    LogTrace(TRACE5) << "buffer size " << buf->size();

    if ((err=OCILobOpen(os.svchp,os.errhp, lob_loc ,OCI_LOB_READONLY)))
    {
        if(os.checkerr(STDLOG,err))
        {
            throw ociexception("Ошибка OCILobOpen");
        }
    }


    while(!done)
    {
      if (buf->size() < (bufoffset + ChunkSize))
      {
          buf->resize(buf->size() + ChunkSize);
      }
      buflen = buf->size() - bufoffset;

      // amt and offset values used by OCI only at first call
      sword retval=OCILobRead(os.svchp, os.errhp, lob_loc, &amt, offset,
        (dvoid *)buf->ptr_at(bufoffset), buflen, (dvoid *)0,
        (sb4 (*)(dvoid *, const dvoid *, ub4, ub1)) 0,
        (ub2) 0, (ub1) SQLCS_IMPLICIT);
      switch (retval)
      {
        case OCI_SUCCESS:
          /* Only one piece since amtp == buf */
          /* Process the data in buf. amt will give the amount of data just read
             in buf. This is in bytes for BLOBs and in characters for fixed
             width CLOBS and in bytes for variable width CLOBs */
        //WARNING!!!
        //This code correct for CLOB case only
        //while single-byte or multibyte variable-width client charset used.
        //More safe way is to use OCILobRead2 from OCI10
          ProgTrace(TRACE4, "Lob Read completed ");
          done=true;
        case OCI_NEED_DATA:
          ProgTrace(TRACE4, "amt=%i", amt);
          bufoffset += amt;
          break;
        case OCI_ERROR:
          if(os.checkerr(STDLOG, retval))
          {
             throw ociexception("Ошибка OCILobRead");
          }
        default:
          ProgTrace(TRACE1,"Unexpected ERROR: OCILobRead() LOB.");
          done=true;
          break;
      }
    }

    // Closing the LOB is mandatory if you have opened it 
    if ((err=OCILobClose(os.svchp, os.errhp, lob_loc)))
    {
        if(os.checkerr(STDLOG, err))
        {
            throw ociexception("Ошибка OCILobClose");
        }
    }

    if (bufType == bufString)
    {
        reinterpret_cast<StringLob_buf_t*>(data)->assign(reinterpret_cast<char *>(buf->ptr_at(0)), bufoffset);
    }
    else
    {
        buf->resize(bufoffset);
    }
}

bool check_vec_size(size_t size,const dvoid* data,size_t skip,
  void const* vec_ptr_4_check, size_t vec_size_4_check, size_t data_size_4_check)
{
  if (size==0)
  {
    LogTrace(TRACE1)<<"size="<<size;
    return false;
  }
  
  if (static_cast<const char*>(data)+(size-1)*skip+data_size_4_check>=
    static_cast<const char*>(vec_ptr_4_check)+vec_size_4_check*skip)
  {
    LogTrace(TRACE1)<<"size="<<size<<" skip="<<skip<<" vec_size_4_check="<<vec_size_4_check
      <<" data_size_4_check="<<data_size_4_check
      <<" data-vec_ptr_4_check="<<(static_cast<const char*>(data)-static_cast<const char*>(vec_ptr_4_check));
    LogTrace(TRACE1)<<"Max   bind ptr="<<(static_cast<const char*>(data)+size*skip+data_size_4_check);
    LogTrace(TRACE1)<<"Max vector ptr="<<(static_cast<const char*>(vec_ptr_4_check)+vec_size_4_check*skip);
    return false;
  }
  return true;
}

} // namespace OciCpp
