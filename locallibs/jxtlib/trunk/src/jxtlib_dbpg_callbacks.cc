#ifdef ENABLE_PG

#include <serverlib/perfom.h>
#include <serverlib/pg_cursctl.h>
#include <serverlib/str_utils.h>

#include "jxtlib_dbpg_callbacks.h"
#include "jxtlib.h"
#include "xml_tools.h"
#include "jxt_tools.h"
#include "jxt_cont.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace jxtlib
{

static long getIfaceVersion(PgCpp::SessionDescriptor sd, const std::string &id)
{
    long res_ver = 0;
    auto cur3 = make_pg_curs(sd, "SELECT COALESCE(MAX(VERSION),-1) FROM XML_STUFF "
                                 "WHERE ID=:ID AND TYPE='interface' AND PAGE_N=1");
    cur3.bind(":ID", id).def(res_ver).EXfet();

    std::string link_name;
    auto cur = make_pg_curs(sd, "SELECT LINK_NAME FROM IFACE_LINKS WHERE LINK_TYPE='ipart' AND IFACE=:ID");
    cur.def(link_name).bind(":ID", id).exec();
    while(cur.fen() == 0) {
        long version;
        auto cur2 = make_pg_curs(sd, "SELECT COALESCE(MAX(VERSION),-1) FROM XML_STUFF "
                                     "WHERE ID=:ID AND TYPE='ipart' AND PAGE_N=1");
        cur2.bind(":ID", link_name).def(version).EXfet();
        if(version > res_ver)
            res_ver = version;
    }
    return res_ver;
}

long JxtlibDbPgCallbacks::getXmlDataVer(const std::string &type, const std::string &id,
                                      bool no_iparts)
{
  long ans_ver=0;

  // Если работаем без ipart'ов, версия интерфейса - максимум из
  // версии непосредственно интерфейса и версий всех его ipart'ов
  std::string query="SELECT getIfaceVersion(:V1)";
  if(!no_iparts || type!="interface") {
    query="SELECT COALESCE(MAX(VERSION),-1) FROM XML_STUFF "
          "WHERE ID=:V1 AND TYPE=:V2 AND PAGE_N=1";
    auto c=make_pg_curs(sd, query);
    c.bind(":V2",type);
    c.bind(":V1",id).def(ans_ver).exfet();
  } else {
    ans_ver = getIfaceVersion(sd, id);
  }

  if(ans_ver<0)
  {
    ProgTrace(TRACE1,"No resource found... wrong id (id='%s',type='%s')?",
              id.c_str(),type.c_str());
    return -1;
  }
  return ans_ver;
}

void JxtlibDbPgCallbacks::insertXmlStuff(const std::string &type, const std::string &id,
                                         const std::string &data)
{
    LogTrace(TRACE3) << __func__ << " type=" << type << " id=" << id;
  int deslen=data.size();
  long ver=time(0);
  int page_n=1;
  char page[2001]="";

  auto c=make_pg_curs(sd, "INSERT INTO XML_STUFF(ID,TYPE,VERSION,DESCR,PAGE_N) "
                       "VALUES (:V1,:V2,:V3,:V4,:V5)");
  c.unstb().bind(":V1",id).bind(":V2",type).bind(":V3",ver).bind(":V4",page)
   .bind(":V5",page_n);
  for(int j=0;j<deslen;j+=2000)
  {
    sprintf(page,"%.2000s",data.c_str()+j);
    c.exec();
    page_n++;
  }
  auto c2=make_pg_curs(sd, "DELETE FROM XML_STUFF WHERE ID=:V1 AND TYPE=:V2 "
                       "AND VERSION!=:V3");
  c2.bind(":V1",id).bind(":V2",type).bind(":V3",ver).exec();
}

void JxtlibDbPgCallbacks::deleteIfaceLinks(const std::string &id)
{
  auto c3=make_pg_curs(sd, "DELETE FROM IFACE_LINKS WHERE IFACE=:V1");
  c3.bind(":V1",id).exec();
}

std::string JxtlibDbPgCallbacks::getXmlData(const std::string &type,
                                          const std::string &id, long ver)
{
  std::string res;
  std::string page;
  int page_n;
  auto c=make_pg_curs(sd, "SELECT DESCR, PAGE_N FROM XML_STUFF WHERE "
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

std::list<std::string> JxtlibDbPgCallbacks::getIparts(const std::string &iface)
{
  std::list<std::string> res;
  std::string ipart_name;
  auto c=make_pg_curs(sd, "SELECT DISTINCT LINK_NAME FROM IFACE_LINKS "
                              "WHERE IFACE=:V1 AND LINK_TYPE='ipart'");
  c.autoNull().bind(":V1",iface).def(ipart_name).exec();
  while(!c.fen())
    res.push_back(ipart_name);
  return res;
}

std::string JxtlibDbPgCallbacks::getCachedIfaceWoIparts(const std::string &id, long answer_ver)
{
  ProgTrace(TRACE2,"getCachedIfaceWoIparts: '%s'",id.c_str());
  auto c=make_pg_curs(sd, "SELECT DESCR FROM CACHED_IFACES WHERE ID=:V1 AND "
          "VERSION=:V2 ORDER BY PAGE_N");
  std::string part;
  c.autoNull().bind(":V1",id).bind(":V2",answer_ver).def(part).exec();
  std::string res;
  while(!c.fen())
    res+=part;
  return res;
}

void JxtlibDbPgCallbacks::setCachedIfaceWoIparts(const std::string &iface_str, const std::string &id,
                                              long ver)
{
  ProgTrace(TRACE2,"setCachedIfaceWoIparts: '%s'",id.c_str());
  auto c=make_pg_curs(sd, "DELETE FROM CACHED_IFACES WHERE ID=:V1");
  c.bind(":V1",id).exec();
  auto c2=make_pg_curs(sd, "INSERT INTO CACHED_IFACES (ID,VERSION,DESCR,PAGE_N) "
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

void JxtlibDbPgCallbacks::insertIfaceLinks(const std::string &id, const std::vector<IfaceLinks> &ilinks,
                                         int version)
{
  std::string link_name,link_type,lang;
  auto c4=make_pg_curs(sd, "INSERT INTO IFACE_LINKS (IFACE,LINK_NAME,"
      "LINK_TYPE,LANG,VERSION) VALUES (:IFACE,:LINK_NAME,:LINK_TYPE,:LANG,:VERSION)");
  c4.unstb().noThrowError(PgCpp::ConstraintFail).
     bind(":IFACE",id).
     bind(":LINK_NAME",link_name).
     bind(":LINK_TYPE",link_type).
     bind(":VERSION",version).
     bind(":LANG", lang);
  for(const auto ilink: ilinks)
  {
    link_name=ilink.id;
    link_type=ilink.type;
    lang=ilink.lang;
    c4.exec();
  }
}

bool JxtlibDbPgCallbacks::ifaceLinkExists(const std::string &iface_id, long ver)
{
  auto c3=make_pg_curs(sd, "SELECT 1 FROM IFACE_LINKS WHERE IFACE=:iface AND "
                       "VERSION=:ver");
  c3.bind(":iface",iface_id).bind(":ver",ver).exfet();
  if(c3.err()==PgCpp::NoDataFound)
    return false;
  else
    return true;
}

std::vector<IfaceLinks> JxtlibDbPgCallbacks::getIfaceLinks(const std::string &iface_id,
                                                         const std::string &lang,
                                                         long ver)
{
  std::vector<IfaceLinks> result;
  /* Для того, чтобы все элементы интерфейса отобразились корректно, нужно */
  /* добавить к ответу секцию <links>, в которой необходимо перечислить */
  /* версии всех типов, данных, iparts и pparts */
  /* При этом нужно учитывать язык */
  std::string link_type, link_name;
  auto c=make_pg_curs(sd, "SELECT DISTINCT LINK_TYPE, LINK_NAME, "
    "(CASE WHEN LINK_TYPE = 'ppart' THEN 0 ELSE 1 END) LT_ORDER "
    "FROM IFACE_LINKS "
    "WHERE IFACE=:V1 AND (LANG IS NULL OR LOWER(LANG)=LOWER(:V2)) "
    "ORDER BY (CASE WHEN LINK_TYPE = 'ppart' THEN 0 ELSE 1 END)");
  c.autoNull().bind(":V1",iface_id).
    bind(":V2", lang).
    def(link_type).
    def(link_name).exec();

  while(!c.fen())
  {
    IfaceLinks link(link_name, link_type, lang, false);
    result.push_back(link);
  }
  return result;
}

long JxtlibDbPgCallbacks::getDataVer(const std::string &data_id)
{
  std::string tabname;
  auto c=make_pg_curs(sd, "SELECT TABNAME FROM REL_ID_TABNAME WHERE ID=:V1");
  c.autoNull().bind(":V1",data_id).def(tabname).EXfet();
  if(c.err()==PgCpp::NoDataFound)
  {
    ProgError(STDLOG,"No data found... wrong id ('%s')?",data_id.c_str());
     return -1;
  }

  long ans_ver=0;
  auto c2=make_pg_curs(sd, "SELECT COALESCE(MAX(VERSION),-1) FROM "+tabname);
  c2.def(ans_ver).EXfet();
  if(ans_ver<0)
  {
      ProgError(STDLOG,"No data found... wrong id ('%s')?",data_id.c_str());
      return -1;
  }

  return ans_ver;
}

RelIdTabName JxtlibDbPgCallbacks::getRelIdTab(const std::string &id)
{
  /* достаем имена таблицы и root_тега */
  std::string root, tabname;
  short undeletable;
  auto cur = make_pg_curs(sd, "SELECT ROOT, TABNAME, UNDELETABLE FROM REL_ID_TABNAME "
                       "WHERE ID=:V1");
  cur.bind(":V1",id).
      def(root).
      def(tabname).
      def(undeletable).
      EXfet();

  if(cur.err() == PgCpp::NoDataFound) {
      LogTrace(TRACE1) << "No data.id=" << id;
      throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }

  return RelIdTabName {
      root,
      tabname,
      undeletable
  };
}

std::vector<RelTabcolTab> JxtlibDbPgCallbacks::getRelTabcolTab(const std::string &id)
{
    std::vector<RelTabcolTab> result;

    auto cur = make_pg_curs(sd, "SELECT TABCOL, TAGNAME, KEY FROM "
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

std::vector<TabColsValues> JxtlibDbPgCallbacks::getTabValues(
        const RelIdTabName &relid, const std::vector<RelTabcolTab> &rel_tabcols,
        bool reset_ind, long term_ver)
{
    /* формируем запрос для поиска записей */
    bool no_close = relid.undeletable ? 0 : 1;
    std::string query="select ";
    for(const auto &tabcol: rel_tabcols)
        query += tabcol.tabcol+",";

    if(no_close)
        query += "0 from ";
    else
        query += "close from ";

    query += relid.tabname;

    if(!reset_ind)
        query += " where COALESCE(version,1) > :v_ver and ida<>0";
    else
        query += " where ida<>0";

#ifdef XML_DEBUG
    ProgTrace(TRACE5,"query='%s'",query.c_str());
#endif /* XML_DEBUG */

    auto cur = make_pg_curs(sd, query);
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
        for(auto &col: defcols.tabcols)
          col = StrUtils::rtrim(col);
        result.push_back(defcols);
    }

    return result;
}

bool JxtlibDbPgCallbacks::jxtContIsSavedCtxt(int handle, const std::string &session_id) const
{
    auto c = make_pg_curs(sd, "SELECT 1 FROM CONT WHERE SESS_ID=:sess_id");
    c.bind(":sess_id",session_id).exfet();
    if(c.err() == PgCpp::NoDataFound)
      return false;
    return true;
}

void JxtlibDbPgCallbacks::jxtContDeleteSavedCtxt(int handle, const std::string &session_id) const
{
  auto c = make_pg_curs(sd, "DELETE FROM CONT WHERE SESS_ID=:sess_id");
  c.bind(":sess_id",session_id).exec();
}

int JxtlibDbPgCallbacks::jxtContGetNumbersOfSaved(std::vector<int> &contexts_vec, const std::string &term) const
{
  int number_of_saved=0;
  std::string sess_id;
  auto c = make_pg_curs(sd, "SELECT DISTINCT SESS_ID FROM CONT WHERE SESS_ID LIKE "
                            ":pult || '%'");
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

bool JxtlibDbPgCallbacks::jxtContIsSavedRow(const std::string &name, const std::string &session_id) const
{
  LogTrace(TRACE3) << __func__ << " " << name;
  auto c = make_pg_curs(sd, "SELECT 1 FROM CONT WHERE SESS_ID=:sess_id AND "
                            "NAME=:name");
  int found;
  c.bind(":sess_id",session_id).bind(":name",name).def(found).exfet();
  if(c.err()==PgCpp::NoDataFound)
    return false;
  return true;
}

std::string JxtlibDbPgCallbacks::jxtContReadSavedRow(const std::string &name, const std::string &session_id) const
{
  LogTrace(TRACE3) << __func__ << " " << name;
  std::string value_part;
  int page_n;
  auto c = make_pg_curs(sd, "SELECT VALUE, PAGE_N FROM CONT WHERE SESS_ID=:sess_id "
                            "AND NAME=:name ORDER BY PAGE_N");
  c.autoNull().bind(":sess_id",session_id).bind(":name",name).
    def(value_part).def(page_n).exec();
  std::string value;
  while(!c.fen())
    value+=value_part;
  return value;
}

void JxtlibDbPgCallbacks::jxtContRemoveLikeL(const std::string &key, const std::string &session_id) const
{
    LogTrace(TRACE3) << __func__ << " " << key;
    std::string name = key + "%";
    auto c = make_pg_curs(sd, "DELETE FROM CONT WHERE SESS_ID=:sess_id AND NAME like :name");
    c.bind(":sess_id", session_id).bind(":name", name).exec();
}

void JxtlibDbPgCallbacks::jxtContAddRow(const JxtContext::JxtContRow *row, const std::string &sess_id,
                                         int page_size) const
{
    int page_n=0;
    std::string val_part;
    auto c = make_pg_curs(sd, "INSERT INTO CONT (SESS_ID,NAME,PAGE_N,VALUE) VALUES "
                          "(:sess_id,:name,:page_n,:val)");
    c.unstb().bind(":sess_id",sess_id).bind(":name",row->name).
     bind(":page_n",page_n).bind(":val",val_part);
    for(unsigned int i=0;i<row->value.size();i+=page_size,++page_n)
    {
      val_part = row->value.substr(i,page_size);
      c.exec();
    }
}

void JxtlibDbPgCallbacks::jxtContDeleteRow(const JxtContext::JxtContRow *row, const std::string &session_id) const
{
  auto c = make_pg_curs(sd, "DELETE FROM CONT WHERE SESS_ID=:sess_id AND NAME=:name");
  c.bind(":sess_id", session_id).bind(":name",row->name).exec();
}

std::set<std::string> JxtlibDbPgCallbacks::jxtContReadAllKeys(const std::string &session_id) const
{
  std::string name;
  auto c = make_pg_curs(sd, "SELECT DISTINCT NAME FROM CONT WHERE SESS_ID=:sess_id "
    "ORDER BY NAME");
  c.autoNull().bind(":sess_id",session_id).def(name).exec();

  std::set<std::string> res;
  while(!c.fen())
    res.insert(name);

  return res;
}


void JxtlibDbPgCallbacks::commit()
{
    LogTrace(TRACE3) << __func__;
    PgCpp::commit();
}

void JxtlibDbPgCallbacks::rollback()
{
    LogTrace(TRACE3) << __func__;
    PgCpp::rollback();
}
} // namespace jxtlib

#endif/*ENABLE_PG*/
