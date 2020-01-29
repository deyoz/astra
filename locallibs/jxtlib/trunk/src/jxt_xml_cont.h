#ifndef __JXT_XML_CONT_H__
#define __JXT_XML_CONT_H__

#include <libxml/tree.h>
#include <serverlib/xml_context.h>

#ifdef __cplusplus
#include <string>
#include <list>

int getErrorMessageId();
int getMessageId();
int getUnknownMessageId();

class BottomMessage
{
    std::string msg;
    std::string id;
    int type;
  public:
    BottomMessage(const char *nick, const char *file, int line, std::string&& s, int t)
        : msg(std::move(s)), type(t)
    {
        id.assign(nick).append(":").append(file).append(":").append(std::to_string(line));
    }
    BottomMessage(const char *nick, const char *file, int line, const char *s, int t)
        : msg(s), type(t)
    {
        id.assign(nick).append(":").append(file).append(":").append(std::to_string(line));
    }
    bool isError() const
    {
        return type==getErrorMessageId();
    }
    bool isConflict(int err) const
    {
        if(type==getUnknownMessageId())
            return 0;
        return isError()!=(err!=0);
    }
    std::string getMsg() const
    {
        return msg;
    }
    std::string getId() const
    {
        return id;
    }
};

class XMLRequestCtxt
{
public:
  static XMLRequestCtxt *Instance(bool reset = false);

  virtual ~XMLRequestCtxt();

  int donotencode2UTF8()
  {
      return do_not_encode2utf8;
  }
  void set_donotencode2UTF8(int enc)
  {
      do_not_encode2utf8 = enc;
      getXmlContext()->set_donotencode2UTF8(enc);
  }
  const BottomMessage *lastBottomMessage()
  {
  	if( bmlist.empty())
  		return 0;
  	return &(*bmlist.rbegin());
  }
  void addBottomMessage(const BottomMessage& bm)
  {
      bmlist.push_back(bm);
  }
  void EraseMessages()
  {
    for(std::list<BottomMessage>::iterator itr=bmlist.begin();
        itr!=bmlist.end();)
    {
      if(!itr->isError())
        bmlist.erase(itr++);
      else
        ++itr;
    }
  }
  void EraseErrors()
  {
    for(std::list<BottomMessage>::iterator itr=bmlist.begin();
        itr!=bmlist.end();)
    {
      if(itr->isError())
        bmlist.erase(itr++);
      else
        ++itr;
    }
  }

  void Init(const std::string &body, const std::string &pult_, const std::string &opr_);
  void setNewOpr(std::string opr_code);

  int GetQueryHandle()  const
  {     return QueryHandle; }
  const std::string &GetOpr()  const
  {     return opr;  }
  const std::string &GetPult()  const
  {     return pult;  }
  int isResDocEmpty();
  char *GetPtrForMsgArea() {     return mes_buf; }
  size_t MsgAreaSize() const {  return sizeof(mes_buf);  }
  int getLang() const
  {     return _lang;   }
  void setLang(int new_lang)
  {     _lang = new_lang;  }
  /*void setMsgArea() {::setMsgArea(mes_buf);}*/
protected:
  XMLRequestCtxt();
public:
  xmlDocPtr resDoc;
  xmlDocPtr reqDoc;
  int AnswerHandle;
  std::string opr;
  std::string pult;
  std::string Query;
  int HandlerInd;
private:
  int QueryHandle;
  std::list<BottomMessage> bmlist;
  char mes_buf[2100];
  int _lang;
  int do_not_encode2utf8;
};

typedef void JxtProcFuncType (XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                              xmlNodePtr resNode);

/*
inline XMLRequestCtxt *getXmlCtxt()
{
    return jxtlib::JXTLib::Instance()->GetCallbacks()->GetXmlRequestCtxt();
//  return XMLRequestCtxt::Instance();
}
*/
XMLRequestCtxt *getXmlCtxt_(const char *file, int line);

#define getXmlCtxt() getXmlCtxt_(__FILE__,__LINE__)


inline xmlDocPtr getQueryDoc()
{
  return getXmlCtxt()->reqDoc;
}

void addXmlBM(XMLRequestCtxt &xmlRC);

void XmlCtxtHook();
void XmlCtxtHookNoWrite();
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void printDoc( xmlNodePtr Node );

xmlDocPtr getResDoc();
xmlNodePtr getResNode();
void setResDoc(xmlDocPtr newResDoc);

xmlDocPtr getReqDoc();

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __JXT_XML_CONT_H__ */
