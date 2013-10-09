#ifndef _QRYS_H_
#define _QRYS_H_

#include <utility>
#include <list>
#include <set>
#include "oralib.h"
#include "oralib.h"
#include "astra_consts.h"
#include <boost/any.hpp>
#include <tr1/memory>

struct QParam {
    std::string name;
    otFieldType ft;
    boost::any value;
    QParam(const std::string &aname, otFieldType aft, const boost::any &avalue):
        name(aname),
        ft(aft),
        value(avalue)
    {};
};

class QParams: public std::list<QParam> {
    public:
        QParams &operator << (const QParam &p) {
            this->push_back(p);
            return *this;
        }
};

struct TQry {
    TQuery Qry;
    BASIC::TDateTime last;
    TQry(): Qry(&OraSession), last(ASTRA::NoExists) {};
};

typedef std::tr1::shared_ptr<TQry> TQry_ptr;

struct time_queue_cmp {
    bool operator() (const TQry_ptr &l, const TQry_ptr &r) const
    {
        if(l->last == r->last) {
            return strcmp(l->Qry.SQLText.SQLText(), r->Qry.SQLText.SQLText()) < 0;
        } else
            return l->last < r->last;
    }
};


struct TQrys: public std::map<const std::string, TQry_ptr> {
    private:
        std::set<TQry_ptr, time_queue_cmp> time_queue;
        void update_time_queue(TQry_ptr qry);
    public:
        TQuery &get(const std::string &SQLText, const QParams &p);
        void dump();
        void dump_time_queue();
        static TQrys *Instance()
        {
            static TQrys *instance_ = 0;
            if ( !instance_ )
                instance_ = new TQrys();
            return instance_;
        }
};

#endif
