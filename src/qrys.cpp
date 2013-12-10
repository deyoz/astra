#include "qrys.h"
#include "exceptions.h"
#include "misc.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const size_t MAX_QRYS = 20;

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

TQuery &TQrys::get(const std::string &SQLText, const QParams &p)
{
    TPerfTimer tm;
    tm.Init();
    TQrys::iterator im = find(SQLText);
    list<TQry_ptr>::iterator i_qry;
    if(im == end()) {
        if(size() < MAX_QRYS) {
            i_qry = queue.insert(queue.end(), TQry_ptr(new TQry()));
            pair<TQrys::iterator, bool> ret = insert(make_pair( SQLText, i_qry));
            if(ret.second == false)
                throw Exception("TQrys::get: insert failed; element already existed");
        } else {
            i_qry = queue.begin();
            queue.splice(queue.end(), queue, i_qry);
            (*i_qry)->Qry.Clear();
            (*i_qry)->count = 0;
        }
        (*i_qry)->Qry.SQLText = SQLText;
        for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++)
            (*i_qry)->Qry.DeclareVariable(iv->name, iv->ft);
    } else {
        i_qry = im->second;
        queue.splice(queue.end(), queue, i_qry);
    }

    (*i_qry)->count++;

    for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++) {
        if(iv->empty) continue;
        switch(iv->ft) {
            case otInteger:
                (*i_qry)->Qry.SetVariable(iv->name, iv->int_value);
                break;
            case otFloat:
                (*i_qry)->Qry.SetVariable(iv->name, iv->double_value);
                break;
            case otString:
                (*i_qry)->Qry.SetVariable(iv->name, iv->string_value);
                break;
            case otChar:
                (*i_qry)->Qry.SetVariable(iv->name, iv->char_value);
                break;
            case otDate:
                (*i_qry)->Qry.SetVariable(iv->name, iv->double_value);
                break;
            case otLong:
            case otLongRaw:
                (*i_qry)->Qry.SetLongVariable(iv->name, iv->void_value, iv->void_size);
                break;
        }
    }
    time += tm.Print();
    return (*i_qry)->Qry;
}
