#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif /* HAVE_MPATROL */
#include <boost/lexical_cast.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include <string>
#include <vector>
#include <map>

#include "jxt_cont.h"
using namespace std;

namespace JxtContext
{
/*****************************************************************************/
/************************     JxtContHandler     *****************************/
/*****************************************************************************/

void JxtContHandler::deleteContext(int handle, int new_current)
{
    deleteSavedCtxt(handle);
    for(JCItr it=contexts.begin();it!=contexts.end();++it)
    {
        if((*it)->getHandle()==handle)
        {
            delete *it; // free memory
            contexts.erase(it); // remove context from list
            break;
        }
    }
    if(current_handle==handle)
        current_handle=new_current;
}

int JxtContHandler::getNumberOfContexts(std::vector<int> &contexts_vec) const
{
    contexts_vec.resize(getMaxContextsNum()+1);
    int number_of_saved=getNumbersOfSaved(contexts_vec);
    int number_of_new=0;
    for(auto& it : contexts)
    {
        if(contexts_vec[it->getHandle()]==0)
        {
            ++number_of_new;
            contexts_vec[it->getHandle()]=2; // new ones
        }
    }
    return number_of_new+number_of_saved;
}

std::string JxtContHandler::getSessId(int handle) const
{
    std::stringstream s;
    s<<term<<handle;
    return s.str();
}

void JxtContHandler::saveCtxts()
{
    std::for_each(contexts.begin(), contexts.end(), [](auto&c){ c->Save(); });
}

bool JxtContHandler::doesCtxtExist(int handle) const
{
    return findContext(handle) || isSavedCtxt(handle);
}

bool JxtContHandler::isCtxtOpen(int handle) const
{
    return findContext(handle);
}

JxtContHandler::~JxtContHandler()
{
  //ProgTrace(TRACE1,"~JxtContHandler(%s)",term.c_str());
  for(JCItr pos=contexts.begin();pos!=contexts.end();++pos)
  {
    delete(*pos);
  }
}

JxtCont *JxtContHandler::findContext(int handle) const
{
    for(auto& pos : contexts)
        if(pos->getHandle() == handle)
            return pos;
    return nullptr;
}

JxtCont *JxtContHandler::getContext(int handle)
{

  JxtCont *jc=findContext(handle);
  if(!jc)
  {
    jc=createContext(handle);
  }
  return jc;
}

JxtCont *JxtContHandler::setCurrentContext(int handle)
{
  JxtCont *jc=getContext(handle);
  current_handle=handle;
  return jc;
}

JxtCont *JxtContHandler::getFreeContext(GFC_flag flag)
{
    vector<int> handles;
    for (JCItr it=contexts.begin(); it!=contexts.end(); ++it)
        handles.push_back((*it)->getHandle());
    sort(handles.begin(),handles.end());
    int i = 1;
    int n = handles.size();
    for (; i < n; ++i)
    {
        if (handles[i] != handles[i-1] + 1 && !isSavedCtxt(i))
            break;
    }
    if (i == n) // not found free among open or saved
    {
        for(; i < MaxContextsNum; ++i)
        {
            if (!isSavedCtxt(i))
                break;
        }
        if (i == MaxContextsNum) {
            ProgTrace(TRACE1, "Too many opened handles: MaxContextsNum=%d, handles.size()=%zd",
                    MaxContextsNum, handles.size());
            throw JxtContException("Too many opened handles: can't find free one");
        }
    }
    if (flag == SET_CURRENT)
        return setCurrentContext(i);
    return getContext(i);
}

JxtContHolder *JxtContHolder::Instance()
{
  static JxtContHolder *_instance=0;
  if(!_instance)
    _instance=new JxtContHolder(0);
  return _instance;
}

/*****************************************************************************/
/************************         JxtCont        *****************************/
/*****************************************************************************/
JxtCont::~JxtCont()
{
  //ProgTrace(TRACE1,"~JxtCont(%s%i)",term.c_str(),handle);
  for(JCRItr pos=rows.begin();pos!=rows.end();++pos)
    delete (pos->second);
}

boost::gregorian::date JxtCont::readDate(const string& name, const boost::gregorian::date& nvl)
{
    string val = read(name);
    if (!val.length())
        return nvl;
    return boost::lexical_cast<boost::gregorian::date>(val);
}

int JxtCont::readInt(const string &name, int NVL)
{
  std::string tmp = read(name);
  if(tmp.empty())
      return NVL;
  char buf[tmp.size()+1];
  strncpy(buf, tmp.c_str(), tmp.size());
  buf[tmp.size()] = '\0';
  char* endptr = 0;
  int res = strtol(tmp.c_str(), &endptr, 10);
  return *endptr=='\0' ? res : NVL;
  /*
  const char *tmp=readC(name);

  if(!tmp)
    return NVL; // ???
  char *ptr=0;
  int res=NVL;
  res=strtol(tmp,&ptr,10);
  if(ptr[0]!='\0')
    return NVL;
  return res;
  */
}

string JxtCont::read(const string &name, const string &NVL)
{
  if(name.empty())
  {
    ProgTrace(TRACE1,"JxtCont::read() : name is empty!");
    throw JxtContException("JxtCont::read() : name is empty!");
  }
  JxtContRow *jcr=findRow(name);
  if(!jcr)
  {
    // read row data
    string val=readSavedRow(name); // returns "" if not found

    // create row with data (empty, if no data is read from DB)
    jcr=createRow(name,val,UNCHANGED);
  }
  // We do not check row's status because newly added rows are created with
  // empty values, and rows to be deleted also have empty values
  if(jcr->value.empty())
    return NVL;
  else
    return jcr->value;
}

JxtCont* JxtCont::remove(const string &name)
{
  JxtContRow *jcr=findRow(name);
  if(!jcr) // No such record in memory
  {
    // let's pretend it's stored in DB
    jcr=createRow(name,"",TO_DELETE); // empty value and TO_DELETE status
  }
  else
  {
    if(jcr->status==TO_ADD) // created row just to delete it? No way!
    {
      trashRow(name);
    }
    else
    {
      // change row's status 
      jcr->status=TO_DELETE;
      // and clear it's value to let read() not check it's status
      jcr->value.clear();
    }
  }
  return this;
}

JxtCont* JxtCont::removeLikeL(const string &key)
{
    JCRItr pos = rows.begin();
    while (pos != rows.end())
    {
        if (!strncmp(key.c_str(), pos->first.c_str(), key.length()))
            remove(pos->first);
        pos++;
    }
    RemoveLikeL(key);
    return this;
}

void JxtCont::copyData(JxtCont *jc) const
{
    for(auto& it : rows)
        if(not it.second->value.empty())
            jc->write(it.first, it.second->value);
}

std::string JxtCont::getSessId() const
{
    return term + std::to_string(handle);
      std::stringstream s;
      s<<handle;
      return term+s.str();
}

int JxtCont::getHandle() const
{
    return handle;
}

JxtContRow* JxtCont::findRow(const std::string &name) const
{
    auto pos = rows.find(name);
    return pos==rows.end() ? nullptr : pos->second;
}

JxtContRow* JxtCont::createRow(const std::string &name, const std::string &value,
                          const JxtContStatus &stat)
{
    JxtContRow *jcr=new JxtContRow(name,value,stat);
    rows[name]=jcr;
    return jcr;
}

JxtContRow* JxtCont::getRow(const std::string &name, const JxtContStatus &stat)
{
    JxtContRow *jcr=findRow(name);
    if(!jcr)
        jcr=createRow(name,"",stat);
    return jcr;
}

void JxtCont::trashRow(const std::string &name)
{
    rows.erase(name);
}

} // namespace JxtContext
