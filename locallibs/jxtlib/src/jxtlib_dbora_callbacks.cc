#include <serverlib/perfom.h>
#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>
#include "jxtlib_dbora_callbacks.h"
#include "jxtlib.h"
#include "jxt_cont.h"
#include "xml_tools.h"
#include <serverlib/oci_selector_char.h>


#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace jxtlib
{
using namespace OciCpp;

long JxtlibDbOraCallbacks::getXmlDataVer(const std::string &type, const std::string &id,
                                      bool no_iparts)
{
  long ans_ver=0;

  // Если работаем без ipart'ов, версия интерфейса - максимум из
  // версии непосредственно интерфейса и версий всех его ipart'ов
  std::string query="SELECT getIfaceVersion(:V1) FROM DUAL";
  if(!no_iparts || type!="interface")
    query="SELECT NVL(MAX(VERSION),-1) FROM XML_STUFF "
          "WHERE ID=:V1 AND TYPE=:V2 AND PAGE_N=1";

  CursCtl c=make_curs(query.c_str());
  if(!no_iparts || type!="interface")
    c.bind(":V2",type);
  c.bind(":V1",id).def(ans_ver).exfet(1);

  if(c.err()==NO_DATA_FOUND || ans_ver<0)
  {
    ProgTrace(TRACE1,"No resource found... wrong id (id='%s',type='%s')?",
              id.c_str(),type.c_str());
    return -1;
  }
  return ans_ver;
}

void JxtlibDbOraCallbacks::insertXmlStuff(const std::string &type, const std::string &id,
                                         const std::string &data)
{
    LogTrace(TRACE3) << __func__ << " type=" << type << " id=" << id;
  int deslen=data.size();
  long ver=time(0);
  int page_n=1;
  char page[2001]="";

  CursCtl c=make_curs("INSERT INTO XML_STUFF(ID,TYPE,VERSION,DESCR,PAGE_N) "
                       "VALUES (:V1,:V2,:V3,:V4,:V5)");
  c.unstb().bind(":V1",id).bind(":V2",type).bind(":V3",ver).bind(":V4",page)
   .bind(":V5",page_n);
  for(int j=0;j<deslen;j+=2000)
  {
    sprintf(page,"%.2000s",data.c_str()+j);
    c.exec();
    page_n++;
  }
  CursCtl c2=make_curs("DELETE FROM XML_STUFF WHERE ID=:V1 AND TYPE=:V2 "
                       "AND VERSION!=:V3");
  c2.bind(":V1",id).bind(":V2",type).bind(":V3",ver).exec();
}

void JxtlibDbOraCallbacks::deleteIfaceLinks(const std::string &id)
{
  CursCtl c3=make_curs("DELETE FROM IFACE_LINKS WHERE IFACE=:V1");
  c3.bind(":V1",id).exec();
}

std::string JxtlibDbOraCallbacks::getXmlData(const std::string &type,
                                          const std::string &id, long ver)
{
  std::string res;
  std::string page;
  int page_n;
  CursCtl c=make_curs("SELECT DESCR, PAGE_N FROM XML_STUFF WHERE "
                      "ID=:V1 AND VERSION=:V2 AND TYPE=:V3 ORDER BY PAGE_N");
  c.bind(":V1",id).bind(":V2",ver).bind(":V3",type).def(page).def(page_n)
   .exec();
  int prev_page_n=0;
  while(!c.fen())
  {
    if(page_n!=++prev_page_n)
      throw jxtlib_exception(STDLOG,"xml resource corrupted - missing page");
    res+=page;
  }
  return res;
}

std::string JxtlibDbOraCallbacks::getCachedIfaceWoIparts(const std::string &id, long answer_ver)
{
  ProgTrace(TRACE2,"getCachedIfaceWoIparts: '%s'",id.c_str());
  CursCtl c=make_curs("SELECT DESCR FROM CACHED_IFACES WHERE ID=:V1 AND "
          "VERSION=:V2 ORDER BY PAGE_N");
  std::string part;
  c.autoNull().bind(":V1",id).bind(":V2",answer_ver).def(part).exec();
  std::string res;
  while(!c.fen())
    res+=part;
  return res;
}

void JxtlibDbOraCallbacks::setCachedIfaceWoIparts(const std::string &iface_str, const std::string &id,
                                              long ver)
{
  ProgTrace(TRACE2,"setCachedIfaceWoIparts: '%s'",id.c_str());
  CursCtl c=make_curs("DELETE FROM CACHED_IFACES WHERE ID=:V1");
  c.bind(":V1",id).exec();
  CursCtl c2=make_curs("INSERT INTO CACHED_IFACES (ID,VERSION,DESCR,PAGE_N) "
          "VALUES (:V1,:V2,:V3,:V4)");
  char page[2001]="";
  int page_n=1;
  c2.unstb().bind(":V1",id).bind(":V2",ver).bind(":V3",page).bind(":V4",page_n);
  for(std::string::size_type i=0;i<iface_str.length();i+=2000)
  {
    sprintf(page,"%.2000s",iface_str.c_str()+i);
    c2.exec();
    page_n++;
  }
}

void JxtlibDbOraCallbacks::insertIfaceLinks(const std::string &id, const std::vector<IfaceLinks> &ilinks, 
                                         int version)
{
  std::string link_name,link_type,lang;
  CursCtl c4=make_curs("INSERT INTO IFACE_LINKS (IFACE,LINK_NAME,"
      "LINK_TYPE,LANG,VERSION) VALUES (:IFACE,:LINK_NAME,:LINK_TYPE,:LANG,:VERSION)");
  c4.unstb().noThrowError(CERR_DUPK).
     bind(":IFACE",id).
     bind(":LINK_NAME",link_name).
     bind(":LINK_TYPE",link_type).
     bind(":VERSION",version).
     bind(":LANG", lang);
  for(const auto& ilink: ilinks)
  {
    link_name=ilink.id;
    link_type=ilink.type;
    lang=ilink.lang;
    c4.exec();
  }
}

std::list<std::string> JxtlibDbOraCallbacks::getIparts(const std::string &iface)
{
  std::list<std::string> res;
  std::string ipart_name;
  OciCpp::CursCtl c=make_curs("SELECT DISTINCT LINK_NAME FROM IFACE_LINKS "
                              "WHERE IFACE=:V1 AND LINK_TYPE='ipart'");
  c.autoNull().bind(":V1", iface).def(ipart_name).exec();
  while(!c.fen())
    res.push_back(ipart_name);
  return res;
}

bool JxtlibDbOraCallbacks::ifaceLinkExists(const std::string &iface_id, long ver)
{
  CursCtl c3=make_curs("SELECT 1 FROM IFACE_LINKS WHERE IFACE=:iface AND "
                       "VERSION=:ver");
  c3.bind(":iface",iface_id).bind(":ver",ver).exfet();
  if(c3.err()==NO_DATA_FOUND)
    return false;
  else
    return true;
}

std::vector<IfaceLinks> JxtlibDbOraCallbacks::getIfaceLinks(const std::string &iface_id,
                                                         const std::string &lang,
                                                         long ver)
{
  std::vector<IfaceLinks> result;
  /* Для того, чтобы все элементы интерфейса отобразились корректно, нужно */
  /* добавить к ответу секцию <links>, в которой необходимо перечислить */
  /* версии всех типов, данных, iparts и pparts */
  /* При этом нужно учитывать язык */
  std::string link_type, link_name;
  CursCtl c=make_curs("SELECT DISTINCT LINK_TYPE, LINK_NAME FROM IFACE_LINKS "
    "WHERE IFACE=:V1 AND (LANG IS NULL OR LOWER(LANG)=LOWER(:V2)) "
    "ORDER BY DECODE(LINK_TYPE,'ppart',0,1)");
  c.autoNull().bind(":V1",iface_id).
    bind(":V2", lang).
    def(link_type).
    def(link_name).exec();

  while(!c.fen())
  {
    IfaceLinks link(link_name, link_type, lang, false);
    result.push_back(link);
  }
  LogTrace(TRACE3) << __func__ << " found " << result.size() 
                   << " iface_links for id=" << iface_id
                   << " lang=" << lang;
  return result;
}

long JxtlibDbOraCallbacks::getDataVer(const std::string &data_id)
{
  std::string tabname;
  CursCtl c=make_curs("SELECT TABNAME FROM REL_ID_TABNAME WHERE ID=:V1");
  c.autoNull().bind(":V1",data_id).def(tabname).EXfet();
  if(c.err()==NO_DATA_FOUND)
  {
    ProgError(STDLOG,"No data found... wrong id ('%s')?",data_id.c_str());
    return -1;
  }

  long ans_ver=0;
  CursCtl c2=make_curs("SELECT NVL(MAX(VERSION),-1) FROM "+tabname);
  c2.def(ans_ver).EXfet();
  if(ans_ver<0)
  {
      ProgError(STDLOG,"No data found... wrong id ('%s')?",data_id.c_str());
      return -1;
  }

  return ans_ver;
}

RelIdTabName JxtlibDbOraCallbacks::getRelIdTab(const std::string &id)
{
  /* достаем имена таблицы и root_тега */
  std::string root, tabname;
  short undeletable;
  auto cur = make_curs("SELECT ROOT, TABNAME, UNDELETABLE FROM REL_ID_TABNAME "
                       "WHERE ID=:V1");
  cur.bind(":V1",id).
      def(root).
      def(tabname).
      def(undeletable).
      EXfet();

  if(cur.err() == NO_DATA_FOUND) {
      LogTrace(TRACE1) << "No data.id=" << id;
      throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }

  return RelIdTabName {
      root,
      tabname,
      undeletable
  };
}

std::vector<RelTabcolTab> JxtlibDbOraCallbacks::getRelTabcolTab(const std::string &id)
{
    std::vector<RelTabcolTab> result;

    auto cur = make_curs("SELECT TABCOL, TAGNAME, KEY FROM "
                         "REL_TABCOL_TAG WHERE ID=:ID");
    RelTabcolTab reltab;
    cur.
        def(reltab.tabcol).
        def(reltab.tagname).
        def(reltab.key).
        bind(":ID", id).
        exec();

    while(!cur.fen()) {
        result.push_back(reltab);
    }

    return result;
}

std::vector<TabColsValues> JxtlibDbOraCallbacks::getTabValues(
        const RelIdTabName &relid, const std::vector<RelTabcolTab> &rel_tabcols,
        bool reset_ind, long term_ver)
{
    /* формируем запрос для поиска записей */
    bool no_close = relid.undeletable ? 0 : 1;
    std::string query="select ";
    for(const auto &tabcol: rel_tabcols)
        query += "rtrim("+tabcol.tabcol+"),";

    if(no_close)
        query += "0 from ";
    else
        query += "close from ";

    query += relid.tabname;

    if(!reset_ind)
        query += " where nvl(version,1) > :v_ver and ida<>0";
    else
        query += " where ida<>0";

#ifdef XML_DEBUG
    ProgTrace(TRACE5,"query='%s'",query.c_str());
#endif /* XML_DEBUG */ 

    auto cur = make_curs(query);
    if(!reset_ind)
        cur.bind(":v_ver", term_ver);

    TabColsValues defcols;
    defcols.tabcols.resize(rel_tabcols.size());
    defcols.ind.resize(rel_tabcols.size());

    for(size_t i = 0; i < rel_tabcols.size(); i++)
        cur.def(defcols.tabcols[i], &defcols.ind[i]);
    cur.def(defcols.close);
    cur.exec();

    std::vector<TabColsValues> result;
    while(!cur.fen()) {
        result.push_back(defcols);
    }

    return result;
}

bool JxtlibDbOraCallbacks::jxtContIsSavedCtxt(int handle, const std::string &session_id) const
{
    CursCtl c=make_curs("SELECT 1 FROM CONT WHERE SESS_ID=:sess_id");
    c.bind(":sess_id", session_id).exfet(1);
    if(c.err() == NO_DATA_FOUND)
      return false;
    return true;
}

void JxtlibDbOraCallbacks::jxtContDeleteSavedCtxt(int handle, const std::string &session_id) const
{
  CursCtl c=make_curs("DELETE FROM CONT WHERE SESS_ID=:sess_id");
  c.bind(":sess_id",session_id).exec();
}

int JxtlibDbOraCallbacks::jxtContGetNumbersOfSaved(std::vector<int> &contexts_vec, const std::string &term) const
{
  int number_of_saved=0;
  std::string sess_id;
  CursCtl c=make_curs("SELECT DISTINCT SESS_ID FROM CONT WHERE SESS_ID LIKE "
                      ":pult||'%'");
  c.bind(":pult",term).def(sess_id).exec();
  while(!c.fen())
  {
    int sess_num=atoiNVL(sess_id.c_str()+6,0);
    if(sess_num<0)
      continue;
    contexts_vec.at(sess_num)=1;
    ++number_of_saved;
  }
  return number_of_saved;
}

bool JxtlibDbOraCallbacks::jxtContIsSavedRow(const std::string &name, const std::string &session_id) const
{
  LogTrace(TRACE3) << __func__ << " " << name;
  CursCtl c=make_curs("SELECT 1 FROM CONT WHERE SESS_ID=:sess_id AND "
                      "NAME=:name");
  int found;
  c.bind(":sess_id",session_id).bind(":name",name).def(found).exfet(1);
  if(c.err()==NO_DATA_FOUND)
    return false;
  return true;
}

std::string JxtlibDbOraCallbacks::jxtContReadSavedRow(const std::string &name, const std::string &session_id) const
{
  std::string value_part;
  int page_n;
  CursCtl c=make_curs("SELECT VALUE, PAGE_N FROM CONT WHERE SESS_ID=:sess_id "
    "AND NAME=:name ORDER BY PAGE_N");
  c.autoNull().bind(":sess_id",session_id).bind(":name",name).
    def(value_part).def(page_n).exec();
  std::string value;
  while(!c.fen())
    value+=value_part;
  LogTrace(TRACE5) << __func__ << " " << name << " : " << c.rowcount() << " pages, value = <" << value << '>';
  return value;
}

void JxtlibDbOraCallbacks::jxtContRemoveLikeL(const std::string &key, const std::string &session_id) const
{
    LogTrace(TRACE3) << __func__ << " " << key;
    std::string name = key + "%";
    CursCtl c = make_curs("DELETE FROM CONT WHERE SESS_ID=:sess_id AND NAME like :name");
    c.bind(":sess_id", session_id).bind(":name", name).exec();
}

void JxtlibDbOraCallbacks::jxtContAddRow(const JxtContext::JxtContRow *row, const std::string &sess_id,
                                         int page_size) const
{
    LogTrace(TRACE3) << __func__ << " " << row->name;
    int page_n=0;
    std::string val_part;
    CursCtl c=make_curs("INSERT INTO CONT (SESS_ID,NAME,PAGE_N,VALUE) VALUES "
                        "(:sess_id,:name,:page_n,:val)");
    c.unstb().bind(":sess_id",sess_id).bind(":name",row->name).
     bind(":page_n",page_n).bind(":val",val_part);
    for(unsigned int i=0;i<row->value.size();i+=page_size,++page_n)
    {
      val_part = row->value.substr(i,page_size);
      c.exec();
    }
}

void JxtlibDbOraCallbacks::jxtContDeleteRow(const JxtContext::JxtContRow *row, const std::string &session_id) const
{
  CursCtl c=make_curs("DELETE FROM CONT WHERE SESS_ID=:sess_id AND NAME=:name");
  c.bind(":sess_id", session_id).bind(":name",row->name).exec();
}

std::set<std::string> JxtlibDbOraCallbacks::jxtContReadAllKeys(const std::string &session_id) const
{
  std::string name;
  CursCtl c=make_curs("SELECT DISTINCT NAME FROM CONT WHERE SESS_ID=:sess_id "
    "ORDER BY NAME");
  c.autoNull().bind(":sess_id",session_id).def(name).exec();

  std::set<std::string> res;
  while(!c.fen())
    res.insert(name);

  return res;
}

void JxtlibDbOraCallbacks::commit()
{
    OciCpp::mainSession().commit();
}

void JxtlibDbOraCallbacks::rollback()
{
    OciCpp::mainSession().rollback();
}
} // namespace jxtlib
