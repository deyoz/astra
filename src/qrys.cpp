#include "qrys.h"
#include "exceptions.h"
#include "misc.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const size_t MAX_QRYS = 1;

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

TCachedQuery::TCachedQuery(const string &SQLText, const QParams &p) {
    Qry = TQrys::Instance()->get(SQLText, p);
}

void TQrys::dump_queue()
{
    ProgTrace(TRACE5, "---TQrys::dump_queue---");
    for(list<TQry_ptr>::iterator is = queue.begin(); is != queue.end(); is++) {
        ProgTrace(TRACE5, "count: %zu", (*is)->count);
        ProgTrace(TRACE5, "qry: %s", (*is)->Qry.SQLText.SQLText());
#ifdef SQL_COUNTERS
        ProgTrace(TRACE5, "parse count: %d", sqlCounters[(*is)->Qry.SQLText.SQLText()]);
#endif
    }
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
    } else {
        if(size() < MAX_QRYS) { // Вставляем новый запрос
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
        if(iv->empty) continue;
        if(iv->pr_null)
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
