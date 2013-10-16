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

    bool empty;

    int int_value;
    double double_value;
    std::string string_value;
    char char_value;
    void *void_value;
    size_t void_size;

    QParam(const std::string &aname, otFieldType aft, void *value, size_t size)
    {
        empty = false;
        name = aname;
        ft = aft;
        void_value = value;
        void_size = size;
    }
    QParam(const std::string &aname, otFieldType aft, int value)
    {
        empty = false;
        name = aname;
        ft = aft;
        int_value = value;
    }
    QParam(const std::string &aname, otFieldType aft, double value)
    {
        empty = false;
        name = aname;
        ft = aft;
        double_value = value;
    }
    QParam(const std::string &aname, otFieldType aft, const std::string &value)
    {
        empty = false;
        name = aname;
        ft = aft;
        string_value = value;
    }
    QParam(const std::string &aname, otFieldType aft, char value)
    {
        empty = false;
        name = aname;
        ft = aft;
        char_value = value;
    }
    QParam(const std::string &aname, otFieldType aft):
        name(aname),
        ft(aft)
    {
        empty = true;
    };
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
    u_int64_t count;
    TQry(): Qry(&OraSession), count(0) {};
};

typedef std::tr1::shared_ptr<TQry> TQry_ptr;

struct TQrys: public std::map<const std::string, std::list<TQry_ptr>::iterator> {
    private:
        std::list<TQry_ptr> queue;
    public:
        long time;
        TQuery &get(const std::string &SQLText, const QParams &p);
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

#endif
