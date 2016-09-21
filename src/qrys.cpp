#include "qrys.h"
#include "exceptions.h"
#include "misc.h"
#include "astra_consts.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

size_t MAX_QRYS()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("QUERY_CACHE_SIZE",1,1000,20);
  return VAR;
};

struct TQrys: public std::multimap<const std::string, std::list<TQry_ptr>::iterator> {
    private:
        std::list<TQry_ptr> queue;
    public:
        long time;
        TQry_ptr get(const std::string &SQLText, const QParams &p);
        void dump_queue();
        TQrys(): time(0) {};
        static TQrys *Instance()
        {
            static TQrys *instance_ = 0;
            if ( !instance_ ) {
                instance_ = new TQrys();
#ifdef SQL_COUNTERS
                queryCount = 0;
#endif
            }
            return instance_;
        }
};

TCachedQuery::~TCachedQuery()
{
    Qry->in_use = false;
}

TQuery &TCachedQuery::get()
{
    return Qry->Qry;
}

TCachedQuery::TCachedQuery(const string &SQLText) {
    QParams p;
    Qry = TQrys::Instance()->get(SQLText, p);
}

TCachedQuery::TCachedQuery(const string &SQLText, const QParams &p) {
    Qry = TQrys::Instance()->get(SQLText, p);
}

#ifdef XP_TESTING

int test_qrys(int argc,char **argv)
{
  TQrys::Instance()->dump_queue();
  {
    TCachedQuery Qry("1", QParams());
    {
      TQrys::Instance()->dump_queue();
      TCachedQuery Qry("1", QParams());
      {
        TQrys::Instance()->dump_queue();
        TCachedQuery Qry("1", QParams());
        TQrys::Instance()->dump_queue();
      }
      {
        TQrys::Instance()->dump_queue();
        TCachedQuery Qry("1", QParams());
        TQrys::Instance()->dump_queue();
      }
      TQrys::Instance()->dump_queue();
    }
    TQrys::Instance()->dump_queue();
  }
  TQrys::Instance()->dump_queue();
  {
    TCachedQuery Qry("2", QParams());
    {
      TQrys::Instance()->dump_queue();
      TCachedQuery Qry("2", QParams());
      {
        TQrys::Instance()->dump_queue();
        TCachedQuery Qry("2", QParams());
        TQrys::Instance()->dump_queue();
      }
      {
        TQrys::Instance()->dump_queue();
        TCachedQuery Qry("2", QParams());
        TQrys::Instance()->dump_queue();
      }
      TQrys::Instance()->dump_queue();
    }
    TQrys::Instance()->dump_queue();
  }
  TQrys::Instance()->dump_queue();
  for(int i=0; i<10; i++)
  {
    TCachedQuery Qry("0", QParams());
    TCachedQuery Qry2("1", QParams());
  };
  TQrys::Instance()->dump_queue();
  ProgTrace(TRACE5, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1");
  {
  TCachedQuery Qry0("0", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry1("1", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry2("2", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry3("3", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry4("4", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry5("5", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry6("6", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry7("7", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry8("8", QParams());
  TQrys::Instance()->dump_queue();
  }{
  TCachedQuery Qry9("9", QParams());
  TQrys::Instance()->dump_queue();
  }
  TQrys::Instance()->dump_queue();
  return 0;
}

#endif /*XP_TESTING*/

#include <boost/crc.hpp>

void TQrys::dump_queue()
{
    boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );

    ProgTrace(TRACE5, "---TQrys---");
    ProgTrace(TRACE5, "%-10s %10s ", "sql_crc", "addr");
    for(TQrys::iterator it = begin(); it != end(); ++it)
    {
      crc32.reset();
      crc32.process_bytes( it->first.c_str(), it->first.size() );
      ProgTrace(TRACE5, "%-10u %10p", crc32.checksum(), &(*it->second));
    };
    ProgTrace(TRACE5, "---TQrys::queue---");
    ProgTrace(TRACE5, "%10s %-10s %-6s %s", "addr", "sql_crc", "in_use", "count");
    for(list<TQry_ptr>::iterator is = queue.begin(); is != queue.end(); ++is) {
        crc32.reset();
        crc32.process_bytes( (*is)->Qry.SQLText.SQLText(), strlen((*is)->Qry.SQLText.SQLText()) );
        ProgTrace(TRACE5, "%10p %-10u %-6s %zu", &(*is), crc32.checksum(), (*is)->in_use?"true":"false",  (*is)->count);
#ifdef SQL_COUNTERS
        ProgTrace(TRACE5, "parse count: %d", sqlCounters[(*is)->Qry.SQLText.SQLText()]);
#endif
    }
    ProgTrace(TRACE5, "=================================================");
#ifdef SQL_COUNTERS
    ProgTrace(TRACE5, "queryCount: %d", queryCount);
#endif
}

TQry_ptr TQrys::get(const std::string &SQLText, const QParams &p)
{
    TPerfTimer tm;
    tm.Init();

    TQrys::iterator it, itlow, itup;
    itlow = lower_bound(SQLText);
    itup = upper_bound(SQLText);
    it = itlow;
    for(; it != itup; it++) {
        if(not (*it->second)->in_use)
            break;
    }
    TQry_ptr result;
    list<TQry_ptr>::iterator i_qry;
    if(it != itup) { // Нашли неиспользуемый запрос
        i_qry = it->second;
        queue.splice(queue.end(), queue, i_qry);
    } else {
        if(size() < MAX_QRYS()) { // Вставляем новый запрос
            i_qry = queue.insert(queue.end(), TQry_ptr(new TQry()));
            insert(pair<string, list<TQry_ptr>::iterator>( SQLText, i_qry));
        } else { // Меняем старый
            i_qry = queue.begin();
            for(; i_qry != queue.end(); i_qry++) // ишем первый свободный запрос в очереди
                if(not (*i_qry)->in_use) break;
            if(i_qry != queue.end()) {
                // надо найти соотв. запись в мультимэпе
                itlow = lower_bound((*i_qry)->Qry.SQLText.SQLText());
                itup = upper_bound((*i_qry)->Qry.SQLText.SQLText());
                it = itlow;
                for(; it != itup; it++)
                    if(it->second == i_qry)
                        break;
                if(it == itup)
                    throw Exception("TQrys::get: corresponding old query not found in multimap");
                erase(it);
                insert(pair<string, list<TQry_ptr>::iterator>( SQLText, i_qry));
                queue.splice(queue.end(), queue, i_qry);
                (*i_qry)->Qry.Clear();
                (*i_qry)->count = 0;
            } else { // В очереди нет свободных запросов
                i_qry = queue.insert(queue.end(), TQry_ptr(new TQry()));
                insert(pair<string, list<TQry_ptr>::iterator>( SQLText, i_qry));
            }
        }
        (*i_qry)->Qry.SQLText = SQLText;
        for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++)
            (*i_qry)->Qry.DeclareVariable(iv->name, iv->ft);
    }

    (*i_qry)->count++;
    (*i_qry)->in_use = true;

    TQuery &Qry = (*i_qry)->Qry;

    for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++) {
        if(iv->empty)
            Qry.SetVariable(iv->name, FNull);
        else
            switch(iv->ft) {
                case otInteger:
                    Qry.SetVariable(iv->name, iv->int_value);
                    break;
                case otFloat:
                    Qry.SetVariable(iv->name, iv->double_value);
                    break;
                case otString:
                case otChar:
                    Qry.SetVariable(iv->name, iv->string_value);
                    break;
                case otDate:
                    Qry.SetVariable(iv->name, iv->double_value);
                    break;
                case otLong:
                case otLongRaw:
                    Qry.SetLongVariable(iv->name, iv->void_value, iv->void_size);
                    break;
            }
    }

    return *i_qry;
}
