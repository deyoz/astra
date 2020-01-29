#ifndef __JXT_CONT_H__
#define __JXT_CONT_H__
#ifdef __cplusplus

#include <string>
#include <map>
#include <list>
#include <vector>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "jxtlib.h"

namespace JxtContext
{

class JxtContException : public jxtlib::jxtlib_exception
{
  public:
    JxtContException(const std::string &msg) : jxtlib::jxtlib_exception(msg) {}
};

typedef enum
{
  UNCHANGED=0,
  TO_SAVE,
  TO_ADD,
  TO_DELETE
} JxtContStatus;



struct JxtContRow
{
  std::string name;
  std::string value;
  JxtContStatus status;
  JxtContRow(const std::string &nam, const std::string &val,
             const JxtContStatus &stat=UNCHANGED) :
             name(nam), value(val), status(stat) {}
};

class JxtCont
{
  private:
    virtual bool isSavedRow(const std::string &name)=0;

    JxtContRow *findRow(const std::string &name) const;

    JxtContRow *createRow(const std::string &name, const std::string &value={},
                          const JxtContStatus &stat=TO_ADD);

    JxtContRow *getRow(const std::string &name, const JxtContStatus &stat=TO_ADD);

    void trashRow(const std::string &name);

    virtual const std::string readSavedRow(const std::string &name)=0;

  protected:
    std::string term;
    int handle;
    JxtContStatus status;

    std::map<std::string,JxtContRow *> rows;
    typedef std::map<std::string,JxtContRow *>::iterator JCRItr;
  public:
    explicit JxtCont(const std::string &pult, int hnd,
                     const JxtContStatus &stat=UNCHANGED) :
                     term(pult), handle(hnd), status(stat)
    {
      rows.clear();
    }
    virtual ~JxtCont();

    const JxtContStatus &getStatus() const { return status; }

    int getHandle() const;

    std::string getSessId() const;

    template<typename T>
    JxtCont *write(const std::string &name, const T &t)
    {
      if(name.empty())
      {
        throw JxtContException("JxtCont::write() : name is empty!");
      }
      std::stringstream s;
      s<<t;
      JxtContRow *jcr=getRow(name,TO_SAVE);
      jcr->value=s.str();
      if(jcr->status!=TO_ADD)
        jcr->status=TO_SAVE;
      return this;
    }

    virtual void Save()=0;
    virtual void RemoveLikeL(const std::string &key) = 0;
    virtual void readAll()=0;
    virtual void dropAll() {};

    std::string read(const std::string &name,
                     const std::string &NVL=std::string());

    boost::gregorian::date readDate(const std::string& name, const boost::gregorian::date& dt = {});
    int readInt(const std::string &name, int NVL=0);
    JxtCont* removeLikeL(const std::string &key);

    JxtCont* remove(const std::string &name);

    void copyData(JxtCont *jc) const;
};

class JxtContHandler
{
  private:
    virtual JxtCont *createContext(int handle)=0;
    virtual void deleteSavedCtxt(int handle)=0;
    virtual int getNumbersOfSaved(std::vector<int> &contexts_vec) const =0;
  protected:
    std::list<JxtCont *> contexts;
    typedef std::list<JxtCont *>::iterator JCItr;
    std::string term;
    int current_handle;
    static const int MaxContextsNum=50;
  public:
    explicit JxtContHandler(const std::string &pult) :
                            term(pult), current_handle(0)
    {
      contexts.clear();
    }
    virtual ~JxtContHandler();
    void deleteContext(int handle, int new_current=0);
    void deleteContext() { deleteContext(current_handle); }
    JxtCont *findContext(int handle) const;
    JxtCont *getContext(int handle);
    JxtCont *setCurrentContext(int handle);
    JxtCont *setCurrentContext(JxtCont *jxtcont)
    {
      return setCurrentContext(jxtcont->getHandle());
    }
    enum GFC_flag {JUST_CREATE=0, SET_CURRENT=1};
    JxtCont *getFreeContext(GFC_flag flag=SET_CURRENT);
    static int getMaxContextsNum() { return MaxContextsNum; }

    JxtCont *sysContext() { return getContext(0); }
    JxtCont *currContext() { return getContext(current_handle); }

    std::string getSessId(int handle) const;

    void saveCtxts();

    virtual bool isSavedCtxt(int handle) const =0;
    bool doesCtxtExist(int handle) const;
    bool isCtxtOpen(int handle) const;

    int getNumberOfContexts(std::vector<int> &contexts_vec) const;
    int getNumberOfContexts() const
    {
      std::vector<int> contexts_vec;
      return getNumberOfContexts(contexts_vec);
    }
};

class JxtContHolder
{
  private:
    JxtContHandler *_jxtContHandler;
    explicit JxtContHolder(JxtContHandler *ptr) : _jxtContHandler(ptr) {}
  public:
    ~JxtContHolder()
    {
      delete _jxtContHandler;
    }
    static JxtContHolder *Instance();
    void setHandler(JxtContHandler *ptr)
    {
        delete _jxtContHandler;
        _jxtContHandler=ptr;
    }
    JxtContHandler *getHandler(bool check_ptr=true) const
    {
      if(check_ptr && _jxtContHandler==0)
        throw JxtContException("JxtContHolder::getHandler : jxtContHandler=0 !");
      return _jxtContHandler;
    }
    void reset()
    {
      if(_jxtContHandler)
        delete _jxtContHandler;
      _jxtContHandler=0;
    }
    void finalize(bool free_data=true)
    {
      if(_jxtContHandler)
        _jxtContHandler->saveCtxts();
      if(free_data)
        reset();
    }
};

inline JxtContHandler *getJxtContHandler()
{
  return JxtContHolder::Instance()->getHandler(true);
}

inline JxtCont *getCurrContext()
{
  return getJxtContHandler()->currContext();
}

inline JxtCont *getSysContext()
{
  return getJxtContHandler()->sysContext();
}

} // namespace JxtContext

#endif /* __cplusplus */

#endif /* __JXT_CONT_H__ */
