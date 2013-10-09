#include "qrys.h"
#include "exceptions.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const size_t MAX_QRYS = 1000;

void TQrys::dump_time_queue()
{
    ProgTrace(TRACE5, "---TQrys::dump_time_queue---");
    for(set<TQry_ptr, time_queue_cmp>::iterator is = time_queue.begin(); is != time_queue.end(); is++) {
        ProgTrace(TRACE5, "last: %s", DateTimeToStr((*is)->last, ServerFormatDateTimeAsString).c_str());
        ProgTrace(TRACE5, "qry: %s", (*is)->Qry.SQLText.SQLText());
    }
}

void TQrys::dump()
{
    ProgTrace(TRACE5, "---TQrys::dump---");
    ProgTrace(TRACE5, "TQrys.size(): %zu", size());
    int idx0 = 0;
    for(TQrys::iterator im = begin(); im != end(); im++, idx0++) {
        ProgTrace(TRACE5, "%d: last: %s", idx0, DateTimeToStr(im->second->last, ServerFormatDateTimeAsString).c_str());
        TQrys::iterator im1 = im;
        im1++;
        int idx1 = idx0 + 1;
        for(; im1 != end(); im1++, idx1++) {
            if(im->second->last == im1->second->last)
                ProgTrace(TRACE5, "same times encountered: %d, %d", idx0, idx1);
        }
    }
}

void TQrys::update_time_queue(TQry_ptr qry)
{
    size_t ret = time_queue.erase(qry);
    if(ret > 1)
        throw Exception("time_queue.erase = %zu", ret);
    qry->last = NowUTC();
    if(not time_queue.insert(qry).second)
        throw Exception("TQrys::update_time_queue: time_queue insert failed; element already existed");
}

TQuery &TQrys::get(const std::string &SQLText, const QParams &p)
{
    TQrys::iterator im = find(SQLText);
    if(im == end()) {
        if(size() < MAX_QRYS) {
            pair<TQrys::iterator, bool> ret = insert(make_pair(SQLText, TQry_ptr(new TQry())));
            if(ret.second == false)
                throw Exception("TQrys::get: insert failed; element already existed");
            im = ret.first;
        } else {
            im = find((*time_queue.begin())->Qry.SQLText.SQLText());
            if(im == end())
                throw Exception("TQrys::get: qry from time_queue not found");
            time_queue.erase(time_queue.begin());
            im->second->Qry.Clear();
        }
        im->second->Qry.SQLText = SQLText;
        for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++)
            im->second->Qry.DeclareVariable(iv->name, iv->ft);
    }
    for(QParams::const_iterator iv = p.begin(); iv != p.end(); iv++) {
        switch(iv->ft) {
            case otInteger:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<int>(iv->value));
                break;
            case otFloat:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<double>(iv->value));
                break;
            case otString:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<string>(iv->value));
                break;
            case otChar:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<char>(iv->value));
                break;
            case otDate:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<BASIC::TDateTime>(iv->value));
                break;
            case otLong:
            case otLongRaw:
                im->second->Qry.SetVariable(iv->name, boost::any_cast<void *>(iv->value));
                break;
        }
    }
    update_time_queue(im->second);
    return im->second->Qry;
}
