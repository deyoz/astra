#include "stat_orders.h"
#include <boost/crc.hpp>
#include <boost/shared_array.hpp>
#include <boost/filesystem.hpp>
#include "md5_sum.h"
#include "exceptions.h"
#include "stat_utils.h"
#include "report_common.h"
#include "astra_misc.h"
#include "jxtlib/zip.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace STAT;

const int arx_trip_date_range=5;

const char *TOrderStatusS[] = {
    "Готов",
    "Выполняется",
    "Поврежден",
    "Ошибка",
    "?"
};

TOrderStatus DecodeOrderStatus( const string &os )
{
  int i;
  for( i=0; i<(int)stNum; i++ )
    if ( os == TOrderStatusS[ i ] )
      break;
  if ( i == stNum )
    return stUnknown;
  else
    return (TOrderStatus)i;
}

const string EncodeOrderStatus(TOrderStatus s)
{
  return TOrderStatusS[s];
};

void commit_progress(TQuery &Qry, int parts, int size)
{
    Qry.SetVariable("progress", round((double)parts / size * 100));
    Qry.Execute();
    OraSession.Commit();
}

int GetCrc32(const string& my_string) {
    boost::crc_32_type result;
    result.process_bytes(my_string.data(), my_string.length());
    return result.checksum();
}

void TErrCommit::exec(int file_id, TOrderStatus st, const string &err)
{
    // Запускал processStatOrders из nosir, долго не мог понять, почему не работает.
    // А просто была ошибка вставки ошибки в базу.
    // Поэтому ошибка продублирована LogTrace-ом.
    LogTrace(TRACE5) << "TErrCommit::exec: " << err;
    Qry.get().SetVariable("file_id", file_id);
    Qry.get().SetVariable("status", st);
    Qry.get().SetVariable("error", err);
    Qry.get().Execute();
    OraSession.Commit();
}

TErrCommit::TErrCommit():
    Qry(
        "update stat_orders set "
        "   status = :status, "
        "   error = :error "
        "where "
        "   file_id = :file_id ",
        QParams()
        << QParam("file_id", otInteger)
        << QParam("status", otInteger)
        << QParam("error", otString)
       )
{
}

void TStatOrderDataItem::complete() const
{
    TCachedQuery Qry(
            "update stat_orders_data set download_times = download_times + 1 where "
            "   file_id = :file_id and "
            "   month = :month",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("month", otDate, month)
            );
    Qry.get().Execute();
}

void TStatOrder::del() const
{
    for(TStatOrderData::const_iterator i = so_data.begin(); i != so_data.end(); i++)
        remove( get_part_file_name(i->file_id, i->month).c_str());
    TCachedQuery delQry(
            "begin "
            "   delete from stat_orders_data where file_id = :file_id; "
            "   delete from stat_orders where file_id = :file_id; "
            "end; ", QParams() << QParam("file_id", otInteger, file_id)
            );
    delQry.get().Execute();
}

void TStatOrder::toDB()
{
    TCachedQuery Qry(
            "insert into stat_orders( "
            "   file_id, "
            "   user_id, "
            "   time_ordered, "
            "   source, "
            "   status, "
            "   progress "
            ") values ( "
            "   :file_id, "
            "   :user_id, "
            "   (select time from files where id = :file_id), "
            "   :os, "
            "   :status, "
            "   0 "
            ") ",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("user_id", otInteger, user_id)
            << QParam("os", otString, EncodeOrderSource(source))
            << QParam("status", otInteger, stRunning)
            );
    Qry.get().Execute();
}

void TStatOrder::check_integrity(TDateTime month)
{
    if(status == stRunning or status == stError)
        return;

    try {
        for(TStatOrderData::iterator data_item = so_data.begin(); data_item != so_data.end(); data_item++) {
            if(month != NoExists and data_item->month < month) continue;
            string file_name = get_part_file_name(data_item->file_id, data_item->month);
            ifstream is(file_name.c_str(), ifstream::binary);
            if(is.is_open()) {
                is.seekg (0, is.end);
                int length = is.tellg();
                is.seekg (0, is.beg);

                boost::shared_array<char> data (new char[length]);
                is.read (data.get(), length);

                TMD5Sum::Instance()->init();
                TMD5Sum::Instance()->update(data.get(), length);
                TMD5Sum::Instance()->Final();

                if(data_item->md5_sum != TMD5Sum::Instance()->str())
                    throw UserException((string)"The order is corrupted! part_file_name: " + file_name);
            } else
                throw UserException((string)"file open error: " + file_name);
        }

        status = stReady;
        TCachedQuery readyQry("update stat_orders set status = :status, error = :error where file_id = :file_id",
                QParams()
                << QParam("file_id", otInteger, file_id)
                << QParam("status", otInteger, status)
                << QParam("error", otString, string())
                );
        readyQry.get().Execute();
    } catch (Exception &E) {
        status = stCorrupted;
        TErrCommit::Instance()->exec(file_id, status, string(E.what()).substr(0, 250).c_str());
    } catch(...) {
        status = stCorrupted;
        TErrCommit::Instance()->exec(file_id, status, "unknown");
    }
}

bool TStatOrders::is_running()
{
    bool result = false;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        if(i->second.status == stRunning) {
            result = true;
            break;
        }
    return result;
}

double TStatOrders::size()
{
    double result = 0;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        if(i->second.data_size != NoExists)
            result += i->second.data_size;
    return result;
}

TStatOrderData::const_iterator TStatOrders::get_part(int file_id, TDateTime month)
{
    get(file_id);
    if(items.size() != 1)
        throw Exception("TStatOrders::get_part: file not found");
    TStatOrderData &so_data = items.begin()->second.so_data;
    TStatOrderData::const_iterator result = so_data.begin();
    for(; result != so_data.end(); result++) {
        if(result->month == month)
            break;
    }
    if(result == so_data.end())
        throw Exception("TStatOrders::get_part: part not found");
    items.begin()->second.check_integrity(month);
    if(items.begin()->second.status != stReady)
        throw UserException("MSG.STAT_ORDERS.FILE_TRANSFER_ERROR",
                LParams() << LParam("status", EncodeOrderStatus(items.begin()->second.status)));
    return result;
}

bool TStatOrders::so_data_empty(int file_id)
{
    bool result = items.empty();
    if(not result) {
        TStatOrderMap::iterator order = items.begin();
        for(; order != items.end(); order++) {
            if(order->second.file_id == file_id) {
                result = order->second.so_data.empty();
                break;
            }
        }
        if(order == items.end()) result = true;
    }
    return result;
}

void seg_fault_emul()
{
    // seg fault emul
    char **a = NULL;
    char *b = a[0];
    LogTrace(TRACE5) << *b;

}

void stat_orders_synchro(void)
{
    TStatOrders so;
    so.get(NoExists);
    TDateTime time_out = NowUTC() - ORDERS_TIMEOUT();
    set<string> files;
    for(TStatOrderMap::iterator i = so.items.begin(); i != so.items.end(); i++) {
        const TStatOrder &so_item = i->second;
        if(so_item.status != stRunning and so_item.time_created <= time_out)
            so_item.del();
        else {
            for(TStatOrderData::const_iterator so_data = so_item.so_data.begin();
                    so_data != so_item.so_data.end();
                    so_data++) {
                files.insert(
                        get_part_file_name(
                            so_item.file_id,
                            so_data->month
                            )
                        );
            }
        }
    }

    namespace fs = boost::filesystem;
    fs::path so_path(ORDERS_PATH());
    fs::directory_iterator end_iter;
    for ( fs::directory_iterator dir_itr( so_path ); dir_itr != end_iter; ++dir_itr ) {
        if(files.find(dir_itr->path().c_str()) == files.end()) {
            fs::remove_all(dir_itr->path());
        }
    }
}

void TStatOrders::check_integrity()
{
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        i->second.check_integrity(NoExists);
}

void TStatOrders::get(int file_id)
{
    return get(NoExists, file_id);
}

void TStatOrders::get(const string &source)
{
    return get(TReqInfo::Instance()->user.user_id, NoExists, source);
}

string TFileParams::get_name()
{
    string result;
    string type = items[PARAM_TYPE];
    string name = items[PARAM_NAME];
    result = getLocaleText((type.empty() ? "Unknown" : type), LANG_EN) + "-";
    result += getLocaleText((name.empty() ? "Unknown" : name), LANG_EN);
    boost::replace_all(result, " ", "_");
    return result;
}

void TFileParams::get(int file_id)
{
    items.clear();
    TCachedQuery Qry(
            "select name, value from file_params where id = :file_id", QParams() << QParam("file_id", otInteger, file_id)
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        items[Qry.get().FieldAsString("name")] = Qry.get().FieldAsString("value");
}

void TStatOrder::get_parts()
{
    TCachedQuery Qry(
            "select * from stat_orders_data where "
            "   file_id = :file_id "
            "order by month "
            ,
            QParams() << QParam("file_id", otInteger, file_id)
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        TStatOrderDataItem item;
        item.file_id = file_id;
        item.month = Qry.get().FieldAsDateTime("month");
        item.file_name = Qry.get().FieldAsString("file_name");
        item.file_size = Qry.get().FieldAsFloat("file_size");
        item.file_size_zip = Qry.get().FieldAsFloat("file_size_zip");
        item.md5_sum = Qry.get().FieldAsString("md5_sum");
        so_data.push_back(item);
    }
}

void TStatOrder::fromDB(TCachedQuery &Qry)
{
    clear();
    if(not Qry.get().Eof) {
        file_id = Qry.get().FieldAsInteger("file_id");
        TFileParams params;
        params.get(file_id);
        name = params.get_name();
        user_id = Qry.get().FieldAsInteger("user_id");
        time_ordered = Qry.get().FieldAsDateTime("time_ordered");
        if(not Qry.get().FieldIsNULL("time_created"))
            time_created = Qry.get().FieldAsDateTime("time_created");
        if(time_ordered == time_created)
            time_created = NoExists;
        if(time_created != NoExists) { // Если заказ еще не сформирован, size неизвестен (NoExists)
            data_size = Qry.get().FieldAsFloat("data_size");
            data_size_zip = Qry.get().FieldAsFloat("data_size_zip");
        }
        source = DecodeOrderSource(Qry.get().FieldAsString("source"));
        status = (TOrderStatus)Qry.get().FieldAsInteger("status");
        error = Qry.get().FieldAsString("error");
        progress = Qry.get().FieldAsInteger("progress");
        get_parts();
    }
}

void TStatOrders::get(int user_id, int file_id, const string &source)
{
    items.clear();
    QParams QryParams;
    string condition;
    if(user_id != NoExists) {
        condition = " user_id = :user_id ";
        QryParams << QParam("user_id", otInteger, user_id);
    }
    if(file_id != NoExists) {
        if(not condition.empty())
            condition += " and ";
        condition += " file_id = :file_id ";
        QryParams << QParam("file_id", otInteger, file_id);
    } else if(not source.empty()) {
        if(not condition.empty())
            condition += " and ";
        condition += "   source = :source ";
        QryParams << QParam("source", otString, source);
    }
    string SQLText = "select * from stat_orders ";
    if(not condition.empty())
        SQLText += " where " + condition;
    TCachedQuery Qry(SQLText, QryParams);

    Qry.get().Execute();
    TPerfTimer tm;
    tm.Init();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        TStatOrder item;
        item.fromDB(Qry);
        items[item.time_ordered] = item;
    }
    ProgTrace(TRACE5, "Stat Orders from DB: %s", tm.PrintWithMessage().c_str());
}

enum TColType { ctCompressRate };

void TStatOrders::toXML(xmlNodePtr resNode)
{
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Отчет"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время заказа"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время выполнения"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время удаления"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Статус"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Готовность"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Размер"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    double total_size = 0;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++) {
        const TStatOrder &curr_order = i->second;
        rowNode = NewTextChild(rowsNode, "row");

        SetProp(rowNode, "file_id", curr_order.file_id);
        SetProp(rowNode, "size", FloatToString(curr_order.data_size));
        SetProp(rowNode, "size_zip", FloatToString(curr_order.data_size_zip));
        SetProp(rowNode, "status", EncodeOrderStatus(curr_order.status));

        // Отчет
        NewTextChild(rowNode, "col", curr_order.name);

        // Время заказа
        NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_ordered));

        // Время выполнения и удаления
        if(curr_order.time_created == NoExists) {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created));
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created + ORDERS_TIMEOUT()));
        }

        // Статус
        NewTextChild(rowNode, "col", getLocaleText(EncodeOrderStatus(curr_order.status)));

        // Готовность
        NewTextChild(rowNode, "col", IntToString(curr_order.progress) + "%");

        // Размер
        if(curr_order.data_size == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", getFileSizeStr(curr_order.data_size));

        total_size += (curr_order.data_size == NoExists ? 0 : curr_order.data_size);
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", getFileSizeStr(total_size));
}

int nosir_stat_order(int argc,char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select text from stat_orders_data where file_id = 14167838 order by page_no";
    Qry.Execute();
    string data;
    for(; not Qry.Eof; Qry.Next()) {
        data += Qry.FieldAsString(0);
    }
    ofstream out("decompressed14167838.csv");
    out << Zip::decompress(StrUtils::b64_decode(data));
    ofstream b64("b64_14167838");
    b64 << data;
    return 1;
}

int nosir_md5(int argc,char **argv)
{
    ifstream is("14213060.1508.0005", ifstream::binary);


    if (is) {
        // get length of file:
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0, is.beg);

        char * buffer = new char [length];

        std::cout << "Reading " << length << " characters... ";
        // read data as a block:
        is.read (buffer,length);

        u_char digest[16];
        //MD5((unsigned char*) buffer, length, digest);

        MD5_CTX c;
        MD5_Init(&c);
        MD5_Update(&c, (unsigned char*) buffer, length);
        MD5_Final(digest, &c);

        /*
        MD5Context ctxt;
        MD5Init(&ctxt);
        MD5Update(&ctxt, (const u_char *)buffer, length);
        u_char digest[16];
        MD5Final(digest, &ctxt);
        */
        ostringstream md5sum;
        for(size_t i = 0; i < sizeof(digest) / sizeof(u_char); i++)
            md5sum << hex << setw(2) << setfill('0') << (int)digest[i];

        if (is)
            std::cout << "all characters read successfully.";
        else
            std::cout << "error: only " << is.gcount() << " could be read";
        is.close();

        // ...buffer contains the entire file...

        delete[] buffer;
    }
    return 1;
}

