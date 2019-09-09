#ifndef _STAT_ORDERS_H_
#define _STAT_ORDERS_H_

#include "date_time.h"
#include "astra_consts.h"
#include "stat_common.h"
#include "qrys.h"

struct TStatOrderDataItem
{
    int file_id;
    BASIC::date_time::TDateTime month;
    std::string file_name;
    double file_size;
    double file_size_zip;
    std::string md5_sum;

    void complete() const;

    void clear()
    {
        file_id = ASTRA::NoExists;
        month = ASTRA::NoExists;
        file_name.clear();
        file_size = ASTRA::NoExists;
        file_size_zip = ASTRA::NoExists;
        md5_sum.clear();
    }

    TStatOrderDataItem()
    {
        clear();
    }

};

typedef std::list<TStatOrderDataItem> TStatOrderData;

enum TOrderStatus {
    stReady,
    stRunning,
    stCorrupted,
    stError,
    stUnknown,
    stNum
};

struct TStatOrder {
    int file_id;
    std::string name;
    int user_id;
    BASIC::date_time::TDateTime time_ordered;
    BASIC::date_time::TDateTime time_created;
    TOrderSource source;
    double data_size;
    double data_size_zip;
    TOrderStatus status;
    std::string error;
    int progress;

    TStatOrderData so_data;

    TStatOrder(int afile_id, int auser_id, TOrderSource asource):
        file_id(afile_id),
        user_id(auser_id),
        source(asource)
    { };
    TStatOrder() { clear(); }
    void clear()
    {
        file_id = ASTRA::NoExists;
        user_id = ASTRA::NoExists;
        time_ordered = ASTRA::NoExists;
        time_created = ASTRA::NoExists;
        source = osUnknown;
        data_size = ASTRA::NoExists;
        data_size_zip = ASTRA::NoExists;
        status = stUnknown;
        error.clear();
        progress = ASTRA::NoExists;
    }

    void del() const;
    void toDB();
    void fromDB(TCachedQuery &Qry);
    void get_parts();
    void check_integrity(BASIC::date_time::TDateTime month);
};

typedef std::map<BASIC::date_time::TDateTime, TStatOrder> TStatOrderMap;

struct TStatOrders {

    TStatOrderMap items;

    void get(int file_id);
    void get(int user_id, int file_id, const std::string &source = std::string());
    void get(const std::string &source = std::string());
    TStatOrderData::const_iterator get_part(int file_id, BASIC::date_time::TDateTime month);
    void toXML(xmlNodePtr resNode);
    bool so_data_empty(int file_id);
    void check_integrity(); // check integrity for each item
    double size(); // summary size of all orders in the list
    bool is_running(); // true if at least one order has stRunning state
};

struct TFileParams {
    std::map<std::string,std::string> items;
    void get(int file_id);
    TFileParams() {}
    TFileParams(const std::map<std::string, std::string> &vitems): items(vitems) {}
    std::string get_name();
};

struct TErrCommit {
    TCachedQuery Qry;
    TErrCommit();
    static TErrCommit *Instance()
    {
        static TErrCommit *instance_ = 0;
        if(not instance_)
            instance_ = new TErrCommit();
        return instance_;
    }
    void exec(int file_id, TOrderStatus st, const std::string &err);
};

const std::string EncodeOrderStatus(TOrderStatus s);
void commit_progress(TQuery &Qry, int parts, int size, long time_processing, int interval);

#endif
