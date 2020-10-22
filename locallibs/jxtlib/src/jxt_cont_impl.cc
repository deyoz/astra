#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif /* HAVE_MPATROL */
#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/slogger.h>

#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <serverlib/dates_io.h>
#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>

#include "jxt_cont.h"
#include "jxt_cont_impl.h"
#include "jxtlib_db_callbacks.h"
#include <serverlib/oci_selector_char.h>

using namespace std;
using namespace OciCpp;

namespace JxtContext
{
/*****************************************************************************/
/************************    JxtContHandlerSir    ****************************/
/*****************************************************************************/

bool JxtContHandlerSir::isSavedCtxt(int handle) const
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContIsSavedCtxt(handle, getSessId(handle));
}

JxtCont *JxtContHandlerSir::createContext(int handle)
{
  JxtCont *jc=new JxtContSir(term,handle,isSavedCtxt(handle)?UNCHANGED:TO_ADD);
  contexts.push_back(jc);
  return contexts.back();
}

void JxtContHandlerSir::deleteSavedCtxt(int handle)
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContDeleteSavedCtxt(handle, getSessId(handle));
}

int JxtContHandlerSir::getNumbersOfSaved(std::vector<int> &contexts_vec) const
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContGetNumbersOfSaved(contexts_vec, term);
}

/*****************************************************************************/
/************************        JxtContSir      *****************************/
/*****************************************************************************/

bool JxtContSir::isSavedRow(const std::string &name)
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContIsSavedRow(name, getSessId());
}

const string JxtContSir::readSavedRow(const std::string &name)
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContReadSavedRow(name, getSessId());
}

void JxtContSir::Save()
{
  string sess_id=getSessId();
  for(JCRItr it=rows.begin();it!=rows.end();++it)
  {
    if(it->second->status==TO_SAVE)
    {
      //tst();
      saveRow(it->second);
    }
    else if(it->second->status==TO_ADD)
    {
      //tst();
      addRow(it->second);
    }
    else if(it->second->status==TO_DELETE)
      deleteRow(it->second);
  }
}

void JxtContSir::RemoveLikeL(const std::string &key)
{
    return jxtlib::JxtlibDbCallbacks::instance()->jxtContRemoveLikeL(key, getSessId());
}

void JxtContSir::saveRow(const JxtContRow *row)
{
  deleteRow(row);
  addRow(row);
}

void JxtContSir::addRow(const JxtContRow *row)
{
    jxtlib::JxtlibDbCallbacks::instance()->jxtContAddRow(row, getSessId(), PageSize);
}

void JxtContSir::deleteRow(const JxtContRow *row)
{
    jxtlib::JxtlibDbCallbacks::instance()->jxtContDeleteRow(row, getSessId());
}

void JxtContSir::readAll()
{
  const auto &&keys = jxtlib::JxtlibDbCallbacks::instance()->jxtContReadAllKeys(getSessId());
  for(const auto &name: keys)
    this->read(name);
}

void JxtContSir::dropAll()
{
  const auto &&keys = jxtlib::JxtlibDbCallbacks::instance()->jxtContReadAllKeys(getSessId());
  for(const auto &name: keys)
    this->remove(name);
}

} // namespace JxtContext


#include <serverlib/checkunit.h>
#ifdef XP_TESTING

#include <serverlib/xp_test_utils.h>
#ifdef ENABLE_PG_TESTS
#include <serverlib/pg_cursctl.h>
#include "jxtlib_dbpg_callbacks.h"
#endif // ENABLE_PG_TESTS

using namespace std;
using namespace JxtContext;

void JxtContextTests_setup()
{
#ifdef ENABLE_PG_TESTS
    setupPgManagedSession();
    jxtlib::JxtlibDbCallbacks::setJxtlibDbCallbacks(new jxtlib::JxtlibDbPgCallbacks(getPgSessionDescriptor()));
#else
  testInitDB();
#endif
  JxtContext::JxtContHolder::Instance()->setHandler(new JxtContext::JxtContHandlerSir(TEST_PUL));
}

void JxtContextTests_teardown()
{
#ifdef ENABLE_PG_TESTS
    PgCpp::commit();
#else
  commit_base();
  testShutDBConnection();
#endif
  JxtContext::JxtContHolder::Instance()->reset();
}

START_TEST(testRemoveLikeL)
{
    JxtContext::JxtCont *cont = getJxtContHandler()->currContext();
    //  write some values in context
    cont->write("VASIA", "man");
    cont->write("VALIA", "woman");
    cont->write("VALUTA", "bucks");
    cont->write("VODKA", "bottle");
#ifdef ENABLE_PG_TESTS
    make_pg_curs(getPgSessionDescriptor(), "insert into cont (sess_id, page_n, name, value) values (:s, :p, :n, :v)")
        .bind(":s", cont->getSessId()).bind(":p", 0).bind(":n", "VATRUSHKA").bind(":v", "tasty").exec();
#else
    // write some values in table exactly, so they are not cached in context
    make_curs("insert into cont (sess_id, page_n, name, value) values (:s, :p, :n, :v)")
        .bind(":s", cont->getSessId()).bind(":p", 0).bind(":n", "VATRUSHKA").bind(":v", "tasty").exec();
#endif

    cont->removeLikeL("VA");
    fail_unless(cont->read("VASIA") != "man", "vasia not removed");
    fail_unless(cont->read("VALIA") != "woman", "valia not removed");
    fail_unless(cont->read("VALUTA") != "bucks", "valuta not removed");
    fail_unless(cont->read("VODKA") == "bottle", "vodka removed");
    fail_unless(cont->read("VATRUSHKA") != "tasty", "vatrushka not removed");
}
END_TEST

START_TEST(tst_getFieldFromCtxt)
{
    JxtCont *jxtcont=getJxtContHandler()->currContext();
    jxtcont->write("VASIA","lalala");
    fail_unless(jxtcont->read("VASIA")=="lalala","read wrong written value");
    fail_unless(jxtcont->read("VASIA2")=="","wrongly read unexisting value");
    fail_unless(jxtcont->read("VASIA2","no_vasia")=="no_vasia","wrongly read unexisting value with NVL");
    jxtcont->write("INT",5);
    fail_unless(jxtcont->readInt("INT")==5,"read wrong written int value");
    fail_unless(jxtcont->read("INT")=="5","read wrong(c) written int value");
    jxtcont->write("INT","5");
    fail_unless(jxtcont->readInt("INT")==5,"read wrong written int(c) value");
    string long_str="1234567890";
    for(int i=0;i<8;++i)
        long_str+=long_str;
    jxtcont->write("VASIA_LONG",long_str);
    JxtContHolder::Instance()->finalize();

    JxtContHolder::Instance()->setHandler(new JxtContHandlerSir(TEST_PUL));
    jxtcont=getJxtContHandler()->currContext();
    fail_unless(jxtcont->read("VASIA")=="lalala","read wrong saved value");
    fail_unless(jxtcont->read("VASIA_LONG")==long_str,"read wrong long value");
    JxtContHolder::Instance()->finalize();
}
END_TEST

START_TEST(testSavedRemove)
{
    string test_p=TEST_PUL;
#ifdef ENABLE_PG_TESTS
    auto c2=make_pg_curs(getPgSessionDescriptor(), "DELETE FROM CONT WHERE SESS_ID LIKE :pul || '%'");
#else
    auto c2=make_curs("DELETE FROM CONT WHERE SESS_ID LIKE :pul||'%'");
#endif
    c2.bind(":pul",test_p).exec();

    JxtCont *jxtcont=getJxtContHandler()->currContext();
    jxtcont->write("VASIA","lalala")->write("PETIA","bugaga")->write("MASHA","hihihi");
    JxtContext::JxtContHolder::Instance()->finalize(); // Сохраняем значение
    //commit_base(); // Коммит базы

    JxtContext::JxtContHolder::Instance()->setHandler(new JxtContext::JxtContHandlerSir(TEST_PUL));
    jxtcont=getJxtContHandler()->currContext();
    fail_unless(jxtcont->read("PETIA")=="bugaga","read(PETIA) returned invalid value");
    jxtcont->remove("PETIA"); // remove read value
    jxtcont->remove("VASIA"); // remove unread value
    JxtContext::JxtContHolder::Instance()->finalize(); // Сохраняем данные
    //commit_base(); // Коммит базы

    string name,val,sess_id;
#ifdef ENABLE_PG_TESTS
    auto c=make_pg_curs(getPgSessionDescriptor(), "SELECT NAME,VALUE,SESS_ID FROM CONT WHERE SESS_ID LIKE :pul || '%'");
#else
    auto c=make_curs("SELECT NAME,VALUE,SESS_ID FROM CONT WHERE SESS_ID LIKE :pul||'%'");
#endif
    c.autoNull().bind(":pul",test_p).def(name).def(val).def(sess_id).exec();
    int count=0;
    while(!c.fen())
    {
        fail_unless(name=="MASHA",(string("Нашли не машу, а '")+name+"'").c_str());
        fail_unless(val=="hihihi",(string("Нашли не hihihi, а '")+val+"'").c_str());
        ++count;
    }
    c2.exec();
    char str[100];
    sprintf(str,"count is %i, not 1",count);
    fail_unless(count==1,str);
}
END_TEST

START_TEST(testReadWriteDate)
{
    JxtCont *jxtcont = getJxtContHandler()->currContext();
    using namespace boost::gregorian;
    date d1(HelpCpp::date_cast("250108", "%d%m%y"));
    jxtcont->write("greg_date", d1);
    string val = jxtcont->read("greg_date");
    ProgTrace(TRACE5, "value: [%s]", val.c_str());
    fail_unless(d1 == jxtcont->readDate("greg_date"), "failed to read date");

    date d2 = jxtcont->readDate("greg_date2");
    fail_unless(d2 == date(not_a_date_time), "failed to return NVL");
}END_TEST

#define SUITENAME "JxtContextTests"
TCASEREGISTER(JxtContextTests_setup,JxtContextTests_teardown)
    ADD_TEST(tst_getFieldFromCtxt);
    ADD_TEST(testRemoveLikeL);
    ADD_TEST(testSavedRemove);
    ADD_TEST(testReadWriteDate);
TCASEFINISH

#endif // XP_TESTING



