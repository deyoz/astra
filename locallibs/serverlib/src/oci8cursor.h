#ifndef __OCI8CURSOR_H_
#define __OCI8CURSOR_H_

#include <set>
#include <list>
#include <string>
#include "oci8.h"
#include "dates_oci.h"

namespace OciCpp {

namespace VectorFunc {
  
typedef void Tresize_func(void* ptr,size_t newlen);
typedef void* Taddr_func(void* ptr,size_t ind);

template <typename T> void resize(void* ptr,size_t newlen)
{
  (static_cast < std::vector<T> * > (ptr))->resize(newlen); 
}

template <typename T> void* addr(void* ptr,size_t ind)
{
  return &(*(static_cast < std::vector<T> * > (ptr)))[ind]; 
}

} // VectorFunc

class TOci8Native
{
  private:
  public:
    virtual sb4 size() const =0;
    virtual ub2 type() const =0;
    
    virtual dvoid* data() { throw ociexception("TOci8Native: data()"); }
    virtual void get_data(Oci8Session& os,void* data, sb2 indicator) const { throw ociexception("TOci8Native: get_data(os,data,ind)"); }

    virtual void get_data(Oci8Session& os,void* data,const dvoid* buf) const =0;
    virtual void set_data(Oci8Session& os,dvoid* buf,const void* data) const =0;
    virtual ~TOci8Native() {};
};

template <typename T,uword SGN>
class TOci8NativeNumber : public TOci8Native
{
  public:
    typedef OCINumber TN;
  private:
    using TOci8Native::data;
    using TOci8Native::get_data;
  public:
    virtual sb4 size() const { return sizeof(TN); }
    virtual ub2 type() const { return SQLT_VNU; }
    
    virtual void get_data(Oci8Session& os,void* data,const dvoid* buf) const { os.status=OCINumberToInt(os.errhp, (const TN*)buf, sizeof(T), SGN, data); }
    virtual void set_data(Oci8Session& os,dvoid* buf,const void* data) const { os.status=OCINumberFromInt(os.errhp, data, sizeof(T), SGN, (TN*)buf); }
    
    virtual ~TOci8NativeNumber<T,SGN>() {};
};

template <typename T,uword SGN>
class TOci8NativeNumberDef : public TOci8NativeNumber<T,SGN>
{
  private:
    typename TOci8NativeNumber<T,SGN>::TN number;
  public:
    virtual dvoid* data() { return &number; }
    virtual void get_data(Oci8Session& os,void* data,sb2 indicator) const { TOci8NativeNumber<T,SGN>::get_data(os,data,&number); }
    
 //   TOci8NativeNumber_LLI_def() : number(OCINumber()) {}
    virtual ~TOci8NativeNumberDef<T,SGN>() {};
};

typedef TOci8NativeNumber<long long int,OCI_NUMBER_SIGNED> TOci8NativeNumber_LLI;
typedef TOci8NativeNumberDef<long long int,OCI_NUMBER_SIGNED> TOci8NativeNumberDef_LLI;
typedef TOci8NativeNumber<unsigned long long int,OCI_NUMBER_UNSIGNED> TOci8NativeNumber_ULLI;
typedef TOci8NativeNumberDef<unsigned long long int,OCI_NUMBER_UNSIGNED> TOci8NativeNumberDef_ULLI;

class TOci8NativeDate_PTIME : public TOci8Native
{
  public:
    typedef oracle_datetime TN;
  private:
    using TOci8Native::data;
  public:
    virtual sb4 size() const { return sizeof(TN); }
    virtual ub2 type() const { return SQLT_DAT; }
    
    virtual void get_data(Oci8Session& os,void* data,const dvoid* buf) const { *((boost::posix_time::ptime*)data)=from_oracle_time(*((const TN*)buf)); }
    virtual void set_data(Oci8Session& os,dvoid* buf,const void* data) const { *((TN*)buf)=to_oracle_datetime(*((boost::posix_time::ptime const*)data)); }
    
    virtual ~TOci8NativeDate_PTIME() {};
};

class TOci8NativeDateDef_PTIME : public TOci8NativeDate_PTIME
{
  private:
    TOci8NativeDate_PTIME::TN date;
    using TOci8Native::get_data;
  public:
    virtual dvoid* data() { return &date; }
    virtual void get_data(Oci8Session& os,void* data, sb2 indicator) const { TOci8NativeDate_PTIME::get_data(os,data,&date); }

    virtual ~TOci8NativeDateDef_PTIME() {};
};




struct TDefVector
{
    void* ptr;
    size_t offset;
    size_t skip;

    std::vector<sb2> *indicator;    

    VectorFunc::Taddr_func *addr;
    VectorFunc::Tresize_func *resize;

    TDefVector (void *ptr_,size_t offset_,size_t skip_,std::vector<sb2> *indicator_,
        VectorFunc::Taddr_func *addr_,VectorFunc::Tresize_func *resize_) :
        ptr(ptr_),offset(offset_),skip(skip_),indicator(indicator_),addr(addr_),
        resize(resize_) {}
};

struct select8_type
{
    ub2 type;
    sb2 indicator;
    dvoid* data;
    sb4 size;
    TOci8Native const* native;
    TDefVector *def_vector;
    TDefVector *def_ind;
    TDefVector *def_al;
    select8_type(ub2 type_,sb2 indicator_,dvoid* data_,sb4 size_,
        TOci8Native const* native_,TDefVector *def_vector_) : 
        type(type_),indicator(indicator_),data(data_),size(size_),native(native_),
        def_vector(def_vector_),def_ind(NULL),def_al(NULL) {}
    select8_type(ub2 type_,sb2 indicator_,dvoid* data_,sb4 size_,
        TOci8Native const* native_,TDefVector *def_vector_,TDefVector *def_ind_) : 
        type(type_),indicator(indicator_),data(data_),size(size_),native(native_),
        def_vector(def_vector_),def_ind(def_ind_),def_al(NULL) {}
    select8_type(ub2 type_,sb2 indicator_,dvoid* data_,sb4 size_,
        TOci8Native const* native_,TDefVector *def_vector_,TDefVector *def_ind_,TDefVector *def_al_) : 
        type(type_),indicator(indicator_),data(data_),size(size_),native(native_),
        def_vector(def_vector_),def_ind(def_ind_),def_al(def_al_) {}
};

namespace {
  
 
template <typename T> struct oci_char_buf
{
};

template <int L> struct oci_char_buf< char [L] > {
    static ub2 type() { return SQLT_STR; }   
    static void* addr(char (*a)[L]){return a;}
    static int size(char (*a)[L]) { return L; }
};

template <int L> struct oci_char_buf< const char [L] > {
    static ub2 type() { return SQLT_STR; }   
    static const void* addr(const char (*a)[L]) {return a;}
    static int size(const char (*a)[L]) { return L; }
};

} // namespace

class Curs8Ctl
{
    const char* nick;
    const char* file;
    const int line;

    const std::string query;
    sb4 rows;
    std::list<select8_type> sts;
    std::list<TDefVector> def_vector;
    std::list< std::vector<sb2> > indicatores;
    std::list<TOci8Native*> natives;
    std::list< std::vector<char> > native_bind_buffers;
    std::set<int> no_throw;
    Oci8Session& os;
    OCIStmt* stmthp;
    std::list<OCILobLocator*> locators;
    bool no_cache;
    int throw_ocierror(const char *nick, const char *file, int line, OCIStmt* stmthp, const char* freetext="");
    int throw_ocierror(const char *nick, const char *file, int line, const char* freetext="");
    int throw_ocierror(const char *nick, const char *file, int line, int errcode, const char* freetext="");
    template<typename T>
    Curs8Ctl& bindclob(const char* name, const T& data);
    template<typename T>
    Curs8Ctl& bindblob(const char* name, const T& data);

    Curs8Ctl& bindlob(const char* name, OCILobLocator* clob, sb4 data_type);

    template <typename T>
    Curs8Ctl& deflob(dvoid* databuf, ub2 data_type, T buftype);
    
    template <typename T,typename I,typename L>  Curs8Ctl& defVec__(std::vector<T> &v, dvoid* data,sb4 len,ub2 type,
      std::vector<I> *vi, dvoid* ind,std::vector<L> *vl, ub2* al);

    template <typename T,class N> Curs8Ctl& defVecN__(std::vector<T> &vec, dvoid* data, sb4 len);

    Curs8Ctl& bindVec_internal__(TOci8Native const& native,size_t size,const char* name, const dvoid* data,size_t skip, bool with_skip);
    template <class N> Curs8Ctl& bindVecN__(size_t size,const char* name, 
      const dvoid* data,size_t skip,
      // for check:
      void const* vec_ptr_4_check, size_t vec_size_4_check, size_t data_size_4_check  
      );
    template <class N> Curs8Ctl& bindN__(const char* name, const dvoid* data);
    
    void on_create(); // common part of constructor

  public:
    Curs8Ctl(const Curs8Ctl&) = delete;
    Curs8Ctl& operator= (const Curs8Ctl&) = delete;
    Curs8Ctl(Curs8Ctl&&);

    Curs8Ctl(const std::string& query, Oci8Session* s=0);
    Curs8Ctl(const char* n, const char* f, int l, const std::string& q, Oci8Session* s=0);
    Curs8Ctl(const char* n, const char* f, int l, const std::string& q, Oci8Session* s, bool no_cache_);
    ~Curs8Ctl();
    Curs8Ctl& def(char& data);
    Curs8Ctl& def(short& data);
    Curs8Ctl& def(int& data);
    Curs8Ctl& def(unsigned int& data);
    Curs8Ctl& def(long int& data);
    Curs8Ctl& def(unsigned long& data);
    Curs8Ctl& def(long long int& data);
    Curs8Ctl& def(unsigned long long int& data);
    Curs8Ctl& def(char* data, size_t len);
    Curs8Ctl& def(oracle_datetime& data);
    Curs8Ctl& def(boost::posix_time::ptime& data);
    Curs8Ctl& def(std::string& data);
    template <typename D> Curs8Ctl& def(D &t, typename std::enable_if<std::is_enum<D>::value>::type* /*dummy*/ = 0) {
      return def(reinterpret_cast<typename std::underlying_type<D>::type&>(t));  }
    template <typename D> Curs8Ctl& def(D &t, typename std::enable_if<not std::is_enum<D>::value>::type* /*dummy*/ = 0)  {
      return def(reinterpret_cast<char*>(oci_char_buf<D>::addr(&t)),oci_char_buf<D>::size(&t));  }

    Curs8Ctl& defClob(std::string& data);
    Curs8Ctl& defClob(std::vector<char>& data);
    Curs8Ctl& defClob(std::vector<unsigned char>& data);

    Curs8Ctl& defBlob(std::string& data);
    Curs8Ctl& defBlob(std::vector<char>& data);
    Curs8Ctl& defBlob(std::vector<unsigned char>& data);

    Curs8Ctl& defArr(ub4 position, char& data, ub4 skip);
    Curs8Ctl& defArr(ub4 position, int& data, ub4 skip);
    Curs8Ctl& defArr(ub4 position, unsigned int& data, ub4 skip);
    Curs8Ctl& defArr(ub4 position, long int& data, ub4 skip);
    Curs8Ctl& defArr(ub4 position, unsigned long int& data, ub4 skip);
    Curs8Ctl& defArr(ub4 position, long long int& data, ub4 skip); // allow on ORA instant client 11.02
    Curs8Ctl& defArr(ub4 position, unsigned long long int& data, ub4 skip); // allow on ORA instant client 11.02
    Curs8Ctl& defArr(ub4 position, char* data, size_t len, ub4 skip);

    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, char& data) { return defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_INT,NULL,NULL,NULL,NULL); }
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, int& data) { return defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_INT,NULL,NULL,NULL,NULL); }
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, unsigned int& data) { return defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_UIN,NULL,NULL,NULL,NULL); }
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, long int& data) { return defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_INT,NULL,NULL,NULL,NULL); }
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, unsigned long int& data) { return defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_UIN,NULL,NULL,NULL,NULL); }
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, long long int& data);
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, unsigned long long int& data);
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, boost::posix_time::ptime& data);
    template <typename T> Curs8Ctl& defVec(std::vector<T> &vec, char* data, size_t len) { return defVec__<T,int,int>(vec,(dvoid*)data,len,SQLT_STR,NULL,NULL,NULL,NULL); }
    template <typename T,typename D> Curs8Ctl& defVec(std::vector<T> &vec,D &t)  {
      return defVec__<T,int,int>(vec,oci_char_buf<D>::addr(&t),oci_char_buf<D>::size(&t),oci_char_buf<D>::type(),NULL,NULL,NULL,NULL);  }

    template <typename T,typename I> Curs8Ctl& defVec(std::vector<T> &vec, int& data,std::vector<I> &vind, short int& ind) { 
      return defVec__<T,I,int>(vec,(dvoid*)&data,sizeof(data),SQLT_INT,&vind,(dvoid*)&ind,NULL,NULL); }
    template <typename T,typename I,typename D> Curs8Ctl& defVec(std::vector<T> &vec,D &t,std::vector<I> &vind, short int& ind)  {
      return defVec__<T,I,int>(vec,oci_char_buf<D>::addr(&t),oci_char_buf<D>::size(&t),oci_char_buf<D>::type(),&vind,(dvoid*)&ind,NULL,NULL);  }

    template <typename T,typename I,typename D> Curs8Ctl& idefVec(std::vector<T> &vec,D &t,std::vector<I> &vind, short int& ind,ub2 type)  {
      return defVec__<T,I,int>(vec,oci_char_buf<D>::addr(&t),oci_char_buf<D>::size(&t),type,&vind,(dvoid*)&ind,NULL,NULL);  }
    template <typename T,typename I,typename L,typename D> Curs8Ctl& ldefVec(
      std::vector<T> &vec,D &t,std::vector<I> &vind, short int& ind,std::vector<L> &vl, unsigned short int& al,ub2 type)  {
      return defVec__(vec,oci_char_buf<D>::addr(&t),oci_char_buf<D>::size(&t),type,&vind,(dvoid*)&ind,&vl,&al);  }
      
    Curs8Ctl& bind(const char* name, dvoid* data, sb4 len, ub2 type); // common

    Curs8Ctl& bind(const char* name, const char& data);
    Curs8Ctl& bind(const char* name, const int& data);
    Curs8Ctl& bind(const char* name, const unsigned int& data);
    Curs8Ctl& bind(const char* name, const long int& data);
    Curs8Ctl& bind(const char* name, const unsigned long int& data);
    Curs8Ctl& bind(const char* name, const long long int& data);
    Curs8Ctl& bind(const char* name, const unsigned long long int& data);
    Curs8Ctl& bind(const char* name, const char* data);
    Curs8Ctl& bind(const char* name, const char* data, size_t len);
    Curs8Ctl& bind(const char* name, const std::string& data);
    Curs8Ctl& bind(const char* name, const oracle_datetime& data);
    Curs8Ctl& bind(const char* name, const boost::posix_time::ptime& data);
    template <typename D> Curs8Ctl& bind(const char* name, const D &t, typename std::enable_if<std::is_enum<D>::value>::type* /*dummy*/ = 0) {
      return bind(name, reinterpret_cast<const typename std::underlying_type<D>::type&>(t));  }

    template <typename D> Curs8Ctl& bind(const D &t)  {
      return bind(reinterpret_cast<const char*>(oci_char_buf<const D>::addr(&t)),oci_char_buf<const D>::size(&t));  }

    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const char& data) { return bindArr(name,data,sizeof(T)); }
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const int& data) { return bindArr(name,data,sizeof(T)); }
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const unsigned int& data) { return bindArr(name,data,sizeof(T)); }
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const long int& data) { return bindArr(name,data,sizeof(T)); }
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const unsigned long int& data) { return bindArr(name,data,sizeof(T)); }
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const long long int& data);
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const unsigned long long int& data);
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const boost::posix_time::ptime& data);
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const boost::posix_time::ptime& data,size_t size);
    template <typename T> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name, const char* data, size_t len) { return bindArr(name,data,len,sizeof(T)); }
    template <typename T,typename D> Curs8Ctl& bindVec(const std::vector<T> &vec,const char* name,const D &t) {
      return bindArr(name,reinterpret_cast<const char*>(oci_char_buf<const D>::addr(&t)),oci_char_buf<const D>::size(&t),sizeof(T));  }
    template <typename T,typename L,typename D> Curs8Ctl& lbindVec(
      std::vector<T> const& vec,const char* name,D &t,std::vector<L> const& vl, unsigned short int const& l,ub2 type)  {
      return lbindArr(name,reinterpret_cast<const char*>(oci_char_buf<const D>::addr(&t)),oci_char_buf<const D>::size(&t),sizeof(T),const_cast<ub2*>(&l),sizeof(L),type);  }

      
    Curs8Ctl& bindArr(const char* name, const char& data, ub4 skip);
    Curs8Ctl& bindArr(const char* name, const int& data, ub4 skip);
    Curs8Ctl& bindArr(const char* name, const unsigned int& data, ub4 skip);
    Curs8Ctl& bindArr(const char* name, const long int& data, ub4 skip);
    Curs8Ctl& bindArr(const char* name, const unsigned long int& data, ub4 skip);
    Curs8Ctl& bindArr(const char* name, const long long int& data, ub4 skip); // allow on ORA instant client 11.02
    Curs8Ctl& bindArr(const char* name, const unsigned long long int& data, ub4 skip); // allow on ORA instant client 11.02
    Curs8Ctl& bindArr(const char* name, const char* data, size_t len, ub4 skip);
    Curs8Ctl& lbindArr(const char* name, const char* data, size_t len, ub4 skip,ub2* alenp, ub4 alskip, ub2 type);

    
    Curs8Ctl& bindOut(const char* name, int& data);

    Curs8Ctl& bindClob(const char* name, const std::string& data);
    Curs8Ctl& bindClob(const char* name, const std::vector<char>& data);
    Curs8Ctl& bindClob(const char* name, const std::vector<uint8_t>& data);

    Curs8Ctl& bindBlob(const char* name, const std::string& data);
    Curs8Ctl& bindBlob(const char* name, const std::vector<char>& data);
    Curs8Ctl& bindBlob(const char* name, const std::vector<uint8_t>& data);
    
    int EXfet(ub4 mode = OCI_DEFAULT);
    int exec(ub4 mode = OCI_DEFAULT);
    int execn(ub4 iters,ub4 mode = OCI_DEFAULT,ub4 rowoff = 0);
    int EXfetVec(size_t load_by=20);
    int fen(ub4 nrows = 1);
    size_t rowcount() const {  return rows;  }
    Curs8Ctl& noThrowError(int err);
    Curs8Ctl& throwError(int err);

    Curs8Ctl& unstb() { return *this; } // all binds are unstb in any case
    
    bool has_the_same_session(const Oci8Session& s) const {  return os == s;  }
    const std::string& queryString() const {  return query;  }
};

template <typename T> 
Curs8Ctl& Curs8Ctl::defVec(std::vector<T> &vec, boost::posix_time::ptime& data) 
{ 
  return 
    defVecN__<T,TOci8NativeDate_PTIME>(vec,(dvoid*)&data,sizeof(data)); 
}

template <typename T> 
Curs8Ctl& Curs8Ctl::defVec(std::vector<T> &vec, long long int& data) 
{ 
  return 
    defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_INT,NULL,NULL,NULL,NULL);
}

template <typename T> 
Curs8Ctl& Curs8Ctl::defVec(std::vector<T> &vec, unsigned long long int& data) 
{ 
  return 
    defVec__<T,int,int>(vec,(dvoid*)&data,sizeof(data),SQLT_UIN,NULL,NULL,NULL,NULL);
}

template <typename T> 
Curs8Ctl& Curs8Ctl::bindVec(const std::vector<T> &vec,const char* name, const boost::posix_time::ptime& data)
{
  return 
    bindVecN__<TOci8NativeDate_PTIME>(vec.size(),name,&data,sizeof(T),vec.empty()?nullptr:&vec[0],vec.size(),sizeof(data)); 
}

template <typename T> 
Curs8Ctl& Curs8Ctl::bindVec(const std::vector<T> &vec,const char* name, const boost::posix_time::ptime& data, size_t size)
{
  return 
    bindVecN__<TOci8NativeDate_PTIME>(size,name,&data,sizeof(T),vec.empty()?nullptr:&vec[0],vec.size(),sizeof(data)); 
}

template <typename T> 
Curs8Ctl& Curs8Ctl::bindVec(const std::vector<T> &vec,const char* name, const long long int& data)
{
  return 
    bindArr(name,data,sizeof(T));    
}

template <typename T> 
Curs8Ctl& Curs8Ctl::bindVec(const std::vector<T> &vec,const char* name, const unsigned long long int& data)
{
  return 
    bindArr(name,data,sizeof(T));    
}

template <typename T,typename I,typename L> 
Curs8Ctl& Curs8Ctl::defVec__(std::vector<T> &v, dvoid* data,sb4 len,ub2 type,
  std::vector<I> *vi, dvoid* ind,std::vector<L> *vl, ub2* al)
{
  std::vector<sb2> *indicator=NULL;

  bool external_indicator=(vi!=NULL && ind!=NULL);
  
  if (!external_indicator && type==SQLT_STR)
  {
    indicatores.push_back(std::vector<sb2>());
    indicator=&indicatores.back();
  }
  
  if (v.empty())
    v.resize(1);
  
  size_t offset_data=(char*)data-(char*)&v[0];
  if (offset_data>sizeof(T))
    throw ociexception("invalid data address");

  def_vector.push_back(TDefVector(&v,offset_data,sizeof(v[0]),
    indicator,&VectorFunc::addr<T>,&VectorFunc::resize<T>));
  
  TDefVector* def_data=&def_vector.back();

  TDefVector* def_ind=NULL;
  if (external_indicator)
  {
    std::vector<I> &i=*vi;
    if (i.empty())
      i.resize(1);
  
    size_t offset_ind=(char*)ind-(char*)&i[0];
    if (offset_ind>sizeof(I))
      throw ociexception("invalid data address");
    
    def_vector.push_back(TDefVector(&i,offset_ind,sizeof(I),
      NULL,&VectorFunc::addr<I>,&VectorFunc::resize<I>));
    
    def_ind=&def_vector.back();
  }

  TDefVector* def_al=NULL;
  if (vl!=NULL && al!=NULL)
  {
    std::vector<L> &l=*vl;
    if (l.empty())
      l.resize(1);
  
    size_t offset_ind=(char*)al-(char*)&l[0];
    if (offset_ind>sizeof(L))
      throw ociexception("invalid data address");
    
    def_vector.push_back(TDefVector(&l,offset_ind,sizeof(L),
      NULL,&VectorFunc::addr<L>,&VectorFunc::resize<L>));
    
    def_al=&def_vector.back();
  }
  
  
  sts.push_back(select8_type(type,0,data,len,NULL,def_data,def_ind,def_al));
  
  return *this;
}

template <typename T,class N>
Curs8Ctl& Curs8Ctl::defVecN__(std::vector<T> &v, dvoid* data, sb4 len)
{
  N *native=new N();
  natives.push_back(native);
  
  std::vector<sb2> *indicator=NULL;
  
  if (native->type() == SQLT_STR)
  {
    indicatores.push_back(std::vector<sb2>());
    indicator=&indicatores.back();
  }
  
  if (v.empty())
    v.resize(1);
  
  size_t offset=(char*)data-(char*)&v[0];
  if (offset>sizeof(v[0]))
    throw ociexception("invalid data address");
  
  def_vector.push_back(TDefVector(&v,offset,sizeof(v[0]),
    indicator,&VectorFunc::addr<T>,&VectorFunc::resize<T>));

  sts.push_back(select8_type(native->type(),0,data,len,native,&def_vector.back()));
  
  return *this;
}

bool check_vec_size(size_t size,const dvoid* data,size_t skip,
  void const* vec_ptr_4_check, size_t vec_size_4_check, size_t data_size_4_check);

template <class N> 
Curs8Ctl& Curs8Ctl::bindVecN__(size_t size,const char* name, const dvoid* data,size_t skip,
  // for check:
  void const* vec_ptr_4_check, size_t vec_size_4_check, size_t data_size_4_check  
  )
{
  if (size==0)
    throw ociexception("bindVec: Invalid size");
  
  if (!check_vec_size(size,data,skip,vec_ptr_4_check,vec_size_4_check,data_size_4_check))
    throw ociexception("bindVec: out of bounds");

  return bindVec_internal__(N(),size,name, data,skip,true/*with_skip*/);
}

template <class N> 
Curs8Ctl& Curs8Ctl::bindN__(const char* name, const dvoid* data)
{
  return bindVec_internal__(N(),1/*size*/,name, data,0/*skip*/,false/*with_skip*/);
}

}

#endif // __OCI8CURSOR_H_
