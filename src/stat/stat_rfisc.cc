#include "stat_rfisc.h"
#include "../rfisc.h"
#include "qrys.h"
#include "points.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

template void RunRFISCStat(TStatParams const&, TOrderStatWriter&, TPrintAirline&);
template void RunRFISCStat(TStatParams const&, TRFISCStat&, TPrintAirline&);

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

struct TRFISCBag {

    struct TBagInfo {
        int excess, paid;
        TBagInfo():
            excess(NoExists),
            paid(NoExists)
        {}
    };

    typedef list<TBagInfo> TBagInfoList;
    typedef map<string, TBagInfoList> TRFISCGroup; // Группировка по кодам RFISC
    typedef map<int, TRFISCGroup> TGrpIdGroup; // Группировка по grp_id
    TGrpIdGroup items;

    TGrpIdGroup::iterator get(int grp_id)
    {
        TGrpIdGroup::iterator result = items.find(grp_id);
        if(result == items.end()) {
          TPaidRFISCListWithAuto paid;
          paid.fromDB(grp_id, true);
          TPaidRFISCStatusList statusList;
          for(TPaidRFISCListWithAuto::const_iterator i=paid.begin(); i!=paid.end(); ++i)
          {
            const TPaidRFISCItem &item=i->second;
            if (!item.list_item)
              throw Exception("TRFISCBag::get: item.list_item=boost::none! (%s)", item.traceStr().c_str());

            if (!item.list_item.get().isBaggageOrCarryOn()) continue;
            //только относящиеся к багажу или ручной клади

            // if (item.list_item.get().isCarryOn()) continue;
            //только относящиеся к багажу

            if (item.trfer_num!=0) continue;
            //только относящиеся к багажу и только на начальном сегменте
            statusList.add(item);
          };
          for(TPaidRFISCStatusList::const_iterator i=statusList.begin();
                                                   i!=statusList.end(); ++i)
          {
            TBagInfo val;

            switch(i->status) {
                case TServiceStatus::Free:
                    val.excess = 0;
                    break;
                case TServiceStatus::Paid:
                case TServiceStatus::Need:
                    val.excess = 1;
                    break;
                default:
                    val.excess = NoExists;
                    break;
            }

            switch(i->status) {
                case TServiceStatus::Paid:
                    val.paid = 1;
                    break;
                case TServiceStatus::Unknown:
                case TServiceStatus::Need:
                    val.paid = 0;
                    break;
                default:
                    val.paid = NoExists;
                    break;
            }

            items[grp_id][i->RFISC].push_back(val);
          }
          result = items.find(grp_id);
        }
        return result;
    }
};

void get_rfisc_stat(int point_id)
{
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);

    TCachedQuery delQry("delete from rfisc_stat where point_id = :point_id", QryParams);
    delQry.get().Execute();

    TCachedQuery bagQry(
            "select "
            "    points.point_id, "
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, "
            "    trfer_trips.airline trfer_airline, "
            "    trfer_trips.flt_no trfer_flt_no, "
            "    trfer_trips.suffix trfer_suffix, "
            "    transfer.airp_arv trfer_airp_arv, "
            "    trfer_trips.scd trfer_scd, "
            "    arv_point.point_num, "
            "    pax_grp.grp_id, "
            "    pax_grp.airp_arv, "
            "    pax.pax_id, "
            "    pax.subclass, "
            "    bag2.rfisc, "
            "    bag2.num bag_num, "
            "    bag2.desk, "
            "    bag2.time_create, "
            "    bag2.pr_cabin, "
            "    users2.login, "
            "    users2.descr, "
            "    points.airp, "
            "    points.craft, "
            "    arv_point.airp airp_last "
            "from "
            "    pax_grp, "
            "    points arv_point, "
            "    points, "
            "    bag2, "
            "    pax, "
            "    transfer, "
            "    trfer_trips, "
            "    users2 "
            "where "
            "    points.point_id = :point_id and "
            "    arv_point.pr_del>=0 AND "
            "    pax_grp.point_dep = points.point_id and "
            "    pax_grp.point_arv = arv_point.point_id and "
            "    pax_grp.grp_id = bag2.grp_id and "
            "    pax_grp.status NOT IN ('E') and "
            "    ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and "
//            "    bag2.pr_cabin = 0 and "
            "    bag2.is_trfer = 0 and "
            "    bag2.rfisc is not null and "
            "    ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) = pax.pax_id(+)  and "
            "    pax_grp.grp_id=transfer.grp_id(+) and "
            "    transfer.pr_final(+) <> 0 and "
            "    transfer.point_id_trfer = trfer_trips.point_id(+) and "
            "    bag2.user_id = users2.user_id(+) ",
        QryParams
            );

    TCachedQuery insQry(
            "insert into rfisc_stat( "
            "  point_id, "
            "  pr_trfer, "
            "  trfer_airline, "
            "  trfer_flt_no, "
            "  trfer_suffix, "
            "  trfer_airp_arv, "
            "  trfer_scd, "
            "  point_num, "
            "  airp_arv, "
            "  rfisc, "
            "  travel_time, "
            "  desk, "
            "  time_create, "
            "  user_login, "
            "  user_descr, "
            "  tag_no, "
            "  fqt_no, "
            "  excess, "
            "  paid "
            ") values ( "
            "  :point_id, "
            "  :pr_trfer, "
            "  :trfer_airline, "
            "  :trfer_flt_no, "
            "  :trfer_suffix, "
            "  :trfer_airp_arv, "
            "  :trfer_scd, "
            "  :point_num, "
            "  :airp_arv, "
            "  :rfisc, "
            "  :travel_time, "
            "  :desk, "
            "  :time_create, "
            "  :login, "
            "  :descr, "
            "  :bag_tag, "
            "  :fqt_no, "
            "  :excess, "
            "  :paid "
            ") ",
        QParams()
            << QParam("point_id", otInteger)
            << QParam("pr_trfer", otInteger)
            << QParam("trfer_airline", otString)
            << QParam("trfer_flt_no", otInteger)
            << QParam("trfer_suffix", otString)
            << QParam("trfer_airp_arv", otString)
            << QParam("trfer_scd", otDate)
            << QParam("point_num", otInteger)
            << QParam("airp_arv", otString)
            << QParam("rfisc", otString)
            << QParam("travel_time", otDate)
            << QParam("login", otString)
            << QParam("descr", otString)
            << QParam("desk", otString)
            << QParam("time_create", otDate)
            << QParam("bag_tag", otFloat)
            << QParam("fqt_no", otString)
            << QParam("excess", otInteger)
            << QParam("paid", otInteger)
            );

    TCachedQuery tagsQry("select no from bag_tags where grp_id = :grp_id and bag_num = :bag_num",
            QParams()
            << QParam("grp_id", otInteger)
            << QParam("bag_num", otInteger)
            );

    TCachedQuery fqtQry(
        "select "
        "   pax_fqt.rem_code, "
        "   pax_fqt.airline, "
        "   pax_fqt.no, "
        "   pax_fqt.extra, "
        "   crs_pnr.subclass "
        "from "
        "   pax_fqt, "
        "   crs_pax, "
        "   crs_pnr "
        "where "
        "   pax_fqt.pax_id = :pax_id and "
        "   pax_fqt.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   pax_fqt.rem_code in('FQTV', 'FQTU', 'FQTR') ",
            QParams() << QParam("pax_id", otInteger));

    bagQry.get().Execute();
    if(not bagQry.get().Eof) {
        int col_point_id = bagQry.get().FieldIndex("point_id");
        int col_pr_trfer = bagQry.get().FieldIndex("pr_trfer");
        int col_trfer_airline = bagQry.get().FieldIndex("trfer_airline");
        int col_trfer_flt_no = bagQry.get().FieldIndex("trfer_flt_no");
        int col_trfer_suffix = bagQry.get().FieldIndex("trfer_suffix");
        int col_trfer_airp_arv = bagQry.get().FieldIndex("trfer_airp_arv");
        int col_trfer_scd = bagQry.get().FieldIndex("trfer_scd");
        int col_point_num = bagQry.get().FieldIndex("point_num");
        int col_grp_id = bagQry.get().FieldIndex("grp_id");
        int col_airp_arv = bagQry.get().FieldIndex("airp_arv");
        int col_pax_id = bagQry.get().FieldIndex("pax_id");
        int col_subclass = bagQry.get().FieldIndex("subclass");
        int col_rfisc = bagQry.get().FieldIndex("rfisc");
        int col_bag_num = bagQry.get().FieldIndex("bag_num");
        int col_desk = bagQry.get().FieldIndex("desk");
        int col_time_create = bagQry.get().FieldIndex("time_create");
        int col_pr_cabin = bagQry.get().FieldIndex("pr_cabin");
        int col_login = bagQry.get().FieldIndex("login");
        int col_descr = bagQry.get().FieldIndex("descr");
        int col_airp = bagQry.get().FieldIndex("airp");
        int col_craft = bagQry.get().FieldIndex("craft");
        int col_airp_last = bagQry.get().FieldIndex("airp_last");
        map<int, TDateTime> travel_times;
        TRFISCBag rfisc_bag;
        for(; not bagQry.get().Eof; bagQry.get().Next()) {
            int grp_id = bagQry.get().FieldAsInteger(col_grp_id);
            TRFISCBag::TGrpIdGroup::iterator rfisc_grp = rfisc_bag.get(grp_id);
            if(rfisc_grp == rfisc_bag.items.end()) continue;

            int point_id =  bagQry.get().FieldAsInteger(col_point_id);
            insQry.get().SetVariable("point_id", point_id);
            insQry.get().SetVariable("pr_trfer", bagQry.get().FieldAsInteger(col_pr_trfer));
            insQry.get().SetVariable("trfer_airline", bagQry.get().FieldAsString(col_trfer_airline));
            insQry.get().SetVariable("trfer_suffix", bagQry.get().FieldAsString(col_trfer_suffix));
            insQry.get().SetVariable("trfer_airp_arv", bagQry.get().FieldAsString(col_trfer_airp_arv));
            insQry.get().SetVariable("point_num", bagQry.get().FieldAsInteger(col_point_num));
            insQry.get().SetVariable("airp_arv", bagQry.get().FieldAsString(col_airp_arv));
            insQry.get().SetVariable("rfisc", bagQry.get().FieldAsString(col_rfisc));
            insQry.get().SetVariable("desk", bagQry.get().FieldAsString(col_desk));

            if(bagQry.get().FieldIsNULL(col_trfer_flt_no))
                insQry.get().SetVariable("trfer_flt_no", FNull);
            else
                insQry.get().SetVariable("trfer_flt_no", bagQry.get().FieldAsInteger(col_trfer_flt_no));

            if(bagQry.get().FieldIsNULL(col_trfer_scd))
                insQry.get().SetVariable("trfer_scd", FNull);
            else
                insQry.get().SetVariable("trfer_scd", bagQry.get().FieldAsDateTime(col_trfer_scd));


            map<int, TDateTime>::iterator travel_times_idx = travel_times.find(point_id);
            if(travel_times_idx == travel_times.end()) {
                pair<map<int, TDateTime>::iterator, bool> ret =
                    travel_times.insert(
                            make_pair(point_id,
                                getTimeTravel(
                                    bagQry.get().FieldAsString(col_craft),
                                    bagQry.get().FieldAsString(col_airp),
                                    bagQry.get().FieldAsString(col_airp_last)
                                    )
                                )
                            );
                travel_times_idx = ret.first;
            }

            if(travel_times_idx->second == NoExists)
                insQry.get().SetVariable("travel_time", FNull);
            else
                insQry.get().SetVariable("travel_time", travel_times_idx->second);

            if(bagQry.get().FieldIsNULL(col_time_create))
                insQry.get().SetVariable("time_create", FNull);
            else
                insQry.get().SetVariable("time_create", bagQry.get().FieldAsDateTime(col_time_create));

            insQry.get().SetVariable("login", bagQry.get().FieldAsString(col_login));
            insQry.get().SetVariable("descr", bagQry.get().FieldAsString(col_descr));

            string fqt_no;
            if(not bagQry.get().FieldIsNULL(col_pax_id)) {
                string subcls = bagQry.get().FieldAsString(col_subclass);
                fqtQry.get().SetVariable("pax_id", bagQry.get().FieldAsInteger(col_pax_id));
                fqtQry.get().Execute();


                if(!fqtQry.get().Eof) {
                    int col_rem_code = fqtQry.get().FieldIndex("rem_code");
                    int col_airline = fqtQry.get().FieldIndex("airline");
                    int col_no = fqtQry.get().FieldIndex("no");
                    int col_extra = fqtQry.get().FieldIndex("extra");
                    int col_subclass = fqtQry.get().FieldIndex("subclass");
                    for(; !fqtQry.get().Eof; fqtQry.get().Next()) {
                        string item;
                        string rem_code = fqtQry.get().FieldAsString(col_rem_code);
                        string airline = fqtQry.get().FieldAsString(col_airline);
                        string no = fqtQry.get().FieldAsString(col_no);
                        string extra = fqtQry.get().FieldAsString(col_extra);
                        string subclass = fqtQry.get().FieldAsString(col_subclass);
                        item +=
                            rem_code + " " +
                            ElemIdToElem(etAirline, airline, efmtCodeNative, LANG_EN) + " " +
                            transliter(no, 1, true);
                        if(rem_code == "FQTV") {
                            if(not subclass.empty() and subclass != subcls)
                                item += "-" + ElemIdToElem(etSubcls, subclass, efmtCodeNative, LANG_EN);
                        } else {
                            if(not extra.empty())
                                item += "-" + transliter(extra, 1, true);
                        }
                        fqt_no = item;
                        break;
                    }
                }
            }

            TRFISCBag::TBagInfoList &bag_info = rfisc_grp->second[bagQry.get().FieldAsString(col_rfisc)];
            TRFISCBag::TBagInfo paid_bag_item;
            if(not bag_info.empty()) {
                paid_bag_item = bag_info.back();
                bag_info.pop_back();
            }

            bool pr_cabin = bagQry.get().FieldAsInteger(col_pr_cabin);
            bool tags_exists = false;

            if(not pr_cabin) {
                tagsQry.get().SetVariable("grp_id", grp_id);
                tagsQry.get().SetVariable("bag_num", bagQry.get().FieldAsInteger(col_bag_num));
                tagsQry.get().Execute();
                tags_exists = not tagsQry.get().Eof;
            }

            while(true) {
                if(paid_bag_item.excess == NoExists)
                    insQry.get().SetVariable("excess", FNull);
                else
                    insQry.get().SetVariable("excess", paid_bag_item.excess);
                if(paid_bag_item.paid == NoExists)
                    insQry.get().SetVariable("paid", FNull);
                else
                    insQry.get().SetVariable("paid", paid_bag_item.paid);

                insQry.get().SetVariable("bag_tag",
                        tags_exists ?
                        tagsQry.get().FieldAsFloat("no"):
                        (pr_cabin ? NoExists : -1));
                insQry.get().SetVariable("fqt_no", fqt_no);
                fqt_no.clear();

                insQry.get().Execute();
                if(tags_exists) {
                    tagsQry.get().Next();
                    if(tagsQry.get().Eof) break;
                } else
                    break;
            }
        }
    }
}

void nosir_rfisc_stat_point(int point_id)
{
    TFlights flightsForLock;
    flightsForLock.Get( point_id, ftTranzit );
    flightsForLock.Lock(__FUNCTION__);

    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT count(*) from points where point_id=:point_id AND pr_del=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger(0) == 0)
    {
        OraSession.Rollback();
        return;
    }

    bool pr_stat = false;
    Qry.SQLText = "SELECT pr_stat FROM trip_sets WHERE point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) pr_stat = Qry.FieldAsInteger(0) != 0;

    int count = 0;
    Qry.SQLText = "select count(*) from rfisc_stat where point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) count = Qry.FieldAsInteger(0);

    if(pr_stat and count == 0)
        get_rfisc_stat(point_id);

    OraSession.Commit();
}

int nosir_rfisc_stat(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    list<int> point_ids;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select point_id from trip_sets";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) point_ids.push_back(Qry.FieldAsInteger(0));
    OraSession.Rollback();
    cout << point_ids.size() << " points to process." << endl;
    int count = 0;
    for(list<int>::iterator i = point_ids.begin(); i != point_ids.end(); i++, count++) {
        nosir_rfisc_stat_point(*i);
        cout << count << endl;
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 0;
}

int nosir_rfisc_all_xml(int argc,char **argv)
{
    TDateTime begin;
    TDateTime now = NowUTC();
    StrToDateTime("01.10.2015 00:00:00", "dd.mm.yyyy hh:nn:ss", begin);
    TQuery nextDateQry(&OraSession);
    nextDateQry.SQLText = "select add_months(:begin, 1) from dual";
    nextDateQry.DeclareVariable("begin", otDate);
    while(begin < now) {
        nextDateQry.SetVariable("begin", begin);
        nextDateQry.Execute();
        TDateTime end = nextDateQry.FieldAsDateTime(0);

        TStatParams params(TStatOverflow::ignore);
        TPrintAirline airline;
        TRFISCStat RFISCStat;
        RunRFISCStat(params, RFISCStat, airline);
        //createXMLRFISCStat(params,RFISCStat, airline, resNode);

        begin = end;
    }
    return 1;
}

int nosir_rfisc_all(int argc,char **argv)
{
    TDateTime begin;
    StrToDateTime("01.10.2015 00:00:00", "dd.mm.yyyy hh:nn:ss", begin);
    TQuery nextDateQry(&OraSession);
    nextDateQry.SQLText = "select add_months(:begin, 1) from dual";
    nextDateQry.DeclareVariable("begin", otDate);

    TQuery rfiscQry(&OraSession);
    rfiscQry.SQLText =
        "select "
        "   rfisc_stat.rfisc, "
        "   rfisc_stat.point_id, "
        "   rfisc_stat.point_num, "
        "   rfisc_stat.pr_trfer, "
        "   points.scd_out, "
        "   points.airline, "
        "   points.flt_no, "
        "   points.suffix, "
        "   points.airp, "
        "   rfisc_stat.airp_arv, "
        "   points.craft, "
        "   rfisc_stat.travel_time, "
        "   rfisc_stat.trfer_flt_no, "
        "   rfisc_stat.trfer_suffix, "
        "   rfisc_stat.trfer_airp_arv, "
        "   rfisc_stat.desk, "
        "   rfisc_stat.user_login, "
        "   rfisc_stat.user_descr, "
        "   rfisc_stat.time_create, "
        "   rfisc_stat.tag_no, "
        "   rfisc_stat.fqt_no, "
        "   rfisc_stat.excess, "
        "   rfisc_stat.paid "
        "from "
        "   points, "
        "   rfisc_stat "
        "where "
        "   rfisc_stat.point_id = points.point_id and "
        "   points.pr_del >= 0 and "
        "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
        "order by "
        "   rfisc_stat.point_id, "
        "   rfisc_stat.point_num, "
        "   rfisc_stat.pr_trfer, "
        "   rfisc_stat.airp_arv ";
    rfiscQry.DeclareVariable("FirstDate", otDate);
    rfiscQry.DeclareVariable("LastDate", otDate);
    TDateTime now = NowUTC();
    const char delim = ';';
    ostringstream header;
    header
        << "RFISC" << delim
        << "Платн." << delim
        << "Опл." << delim
        << "Бирка" << delim
        << "SPEC" << delim
        << "FQTV" << delim
        << "Дата вылета" << delim
        << "Рейс" << delim
        << "От" << delim
        << "До" << delim
        << "Тип ВС" << delim
        << "Время в пути" << delim
        << "Трфр.рейс" << delim
        << "От" << delim
        << "До" << delim
        << "АП рег." << delim
        << "Стойка" << delim
        << "LOGIN" << delim
        << "Агент" << delim
        << "Дата оформ.";
    ofstream rfisc_all("rfisc_all.csv");
    rfisc_all << header.str() << endl;
    int count = 0;
    while(begin < now) {
        nextDateQry.SetVariable("begin", begin);
        nextDateQry.Execute();
        TDateTime end = nextDateQry.FieldAsDateTime(0);
        cout << "begin: " << DateTimeToStr(begin, ServerFormatDateTimeAsString) << endl;
        cout << "end: " << DateTimeToStr(end, ServerFormatDateTimeAsString) << endl;
        rfiscQry.SetVariable("FirstDate", begin);
        rfiscQry.SetVariable("LastDate", end);
        rfiscQry.Execute();
        if(not rfiscQry.Eof) {
            string fname = "rfisc." + DateTimeToStr(begin, "yyyymm") + ".csv";
            ofstream rfisc_month(fname);
            rfisc_month << header.str() << endl;
            ostringstream out;

            int col_rfisc = rfiscQry.GetFieldIndex("rfisc");
            int col_scd_out = rfiscQry.GetFieldIndex("scd_out");
            int col_flt_no = rfiscQry.GetFieldIndex("flt_no");
            int col_suffix = rfiscQry.GetFieldIndex("suffix");
            int col_airp = rfiscQry.GetFieldIndex("airp");
            int col_airp_arv = rfiscQry.GetFieldIndex("airp_arv");
            int col_craft = rfiscQry.GetFieldIndex("craft");
            int col_travel_time = rfiscQry.GetFieldIndex("travel_time");
            int col_pr_trfer = rfiscQry.GetFieldIndex("pr_trfer");
            int col_trfer_flt_no = rfiscQry.GetFieldIndex("trfer_flt_no");
            int col_trfer_suffix = rfiscQry.GetFieldIndex("trfer_suffix");
            int col_trfer_airp_arv = rfiscQry.GetFieldIndex("trfer_airp_arv");
            int col_desk = rfiscQry.GetFieldIndex("desk");
            int col_user_login = rfiscQry.GetFieldIndex("user_login");
            int col_user_descr = rfiscQry.GetFieldIndex("user_descr");
            int col_time_create = rfiscQry.GetFieldIndex("time_create");
            int col_tag_no = rfiscQry.GetFieldIndex("tag_no");
            int col_fqt_no = rfiscQry.GetFieldIndex("fqt_no");
            int col_excess = rfiscQry.GetFieldIndex("excess");
            int col_paid = rfiscQry.GetFieldIndex("paid");

            for(; not rfiscQry.Eof; rfiscQry.Next(), count++) {
                // RFISC
                out
                    << rfiscQry.FieldAsString(col_rfisc) << delim;
                // Платн.
                if(not rfiscQry.FieldIsNULL(col_excess))
                    out << rfiscQry.FieldAsInteger(col_excess);
                out << delim;
                // Опл.
                if(not rfiscQry.FieldIsNULL(col_paid))
                    out << rfiscQry.FieldAsInteger(col_paid);
                out
                    << delim
                    // Бирка
                    << rfiscQry.FieldAsString(col_tag_no) << delim
                    // SPEQ
                    << delim
                    // FQTV
                    << rfiscQry.FieldAsString(col_fqt_no) << delim
                    // Дата вылета
                    << DateTimeToStr(rfiscQry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy") << delim
                    // Рейс
                    << rfiscQry.FieldAsInteger(col_flt_no) << rfiscQry.FieldAsString(col_suffix) << delim
                    // От
                    << rfiscQry.FieldAsString(col_airp) << delim
                    // До
                    << rfiscQry.FieldAsString(col_airp_arv) << delim
                    // Тип ВС
                    << rfiscQry.FieldAsString(col_craft) << delim;
                // Время в пути
                if(not rfiscQry.FieldIsNULL(col_travel_time))
                    out << DateTimeToStr(rfiscQry.FieldAsDateTime(col_travel_time), "hh:nn");
                out
                    << delim;
                if(rfiscQry.FieldAsInteger(col_pr_trfer) != 0) {
                    out
                        // Трфр.рейс
                        << rfiscQry.FieldAsInteger(col_trfer_flt_no) << rfiscQry.FieldAsString(col_trfer_suffix) << delim
                        // От
                        << rfiscQry.FieldAsString(col_airp) << delim
                        // До
                        << rfiscQry.FieldAsString(col_trfer_airp_arv) << delim;
                } else {
                    out << delim << delim << delim;
                }
                out
                    // АП рег
                    << rfiscQry.FieldAsString(col_airp) << delim
                    // Стойка
                    << rfiscQry.FieldAsString(col_desk) << delim
                    // Логин
                    << rfiscQry.FieldAsString(col_user_login) << delim
                    // Агент
                    << rfiscQry.FieldAsString(col_user_descr) << delim;
                // Дата оформ.
                if(not rfiscQry.FieldIsNULL(col_time_create))
                    out << DateTimeToStr(rfiscQry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy");
                out << endl;
                if(count % 10000 == 0)
                    cout << count << endl;
            }
            rfisc_month << out.str();
            rfisc_all << out.str();
        }
        OraSession.Rollback();
        begin = end;
    }
    cout << count << endl;
    return 1;
}

string get_tag_no(double tag_no)
{
    ostringstream result;
    if(tag_no == NoExists) {
        result << getLocaleText("Р/к");
    } else if(tag_no != -1) {
        result
            << fixed
            << setprecision(0)
            << setw(10)
            << setfill('0')
            << tag_no;
    }
    return result.str();
}

void TRFISCStatRow::add_data(ostringstream &buf) const
{
    //RFISC
    buf << rfisc << delim;
    // Платн.
    if(excess != NoExists)
        buf << excess;
    buf << delim;
    // Опл.
    if(paid != NoExists)
        buf << paid;
    buf
        << delim
        // Бирка
        << get_tag_no(tag_no) << delim
        // SPEQ
        << delim
        // FQTV
        << fqt_no << delim
        // Дата вылета
        << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim
        // Рейс
        << flt_no << ElemIdToCodeNative(etSuffix, suffix) << delim
        // От
        << ElemIdToCodeNative(etAirp, airp) << delim
        // До
        << ElemIdToCodeNative(etAirp, airp_arv) << delim
        // Тип ВС
        << ElemIdToCodeNative(etCraft, craft) << delim;
    // Время в пути
    if(travel_time != NoExists)
        buf << DateTimeToStr(travel_time, "hh:nn");
    buf << delim;

    // Трансфер на
    ostringstream trfer_flt_no;
    string trfer_airp_dep;
    string trfer_airp_arv;
    if(this->trfer_flt_no) {
        trfer_flt_no << setw(3) << setfill('0') << this->trfer_flt_no << ElemIdToCodeNative(etSuffix, trfer_suffix);
        trfer_airp_dep = airp;
        trfer_airp_arv = trfer_airp_arv;
    }
    // Рейс
    buf << trfer_flt_no.str() << delim;
    // От (совпадает с От рейса)
    buf << ElemIdToCodeNative(etAirp, trfer_airp_dep) << delim;
    // До
    buf << ElemIdToCodeNative(etAirp, trfer_airp_arv) << delim;

    // Информация об агенте
    // АП рег. (совпадает с От рейса)
    buf << ElemIdToCodeNative(etAirp, airp) << delim;
    // Стойка
    buf << desk << delim;
    // LOGIN
    buf << user_login << delim;
    // Агент
    buf << user_descr << delim;
    // Дата оформления
    if(time_create != NoExists)
        buf << DateTimeToStr(time_create, "dd.mm.yyyy");
    buf << endl;
}

void TRFISCStatRow::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("RFISC") << delim
        << getLocaleText("Платн.") << delim
        << getLocaleText("Опл.") << delim
        << getLocaleText("Бирка") << delim
        << getLocaleText("SPEC") << delim
        << getLocaleText("FQTV") << delim
        << getLocaleText("Дата вылета") << delim
        << getLocaleText("Рейс") << delim
        << getLocaleText("От") << delim
        << getLocaleText("До") << delim
        << getLocaleText("Тип ВС") << delim
        << getLocaleText("Время в пути") << delim
        << getLocaleText("Трфр.рейс") << delim
        << getLocaleText("От") << delim
        << getLocaleText("До") << delim
        << getLocaleText("АП рег.") << delim
        << getLocaleText("Стойка") << delim
        << getLocaleText("LOGIN") << delim
        << getLocaleText("Агент") << delim
        << getLocaleText("Дата оформ.")
        << endl;
}

template <class T>
void RunRFISCStat(
        const TStatParams &params,
        T &RFISCStat,
        TPrintAirline &prn_airline
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "select "
            "   rfisc_stat.rfisc, "
            "   rfisc_stat.point_id, "
            "   rfisc_stat.point_num, "
            "   rfisc_stat.pr_trfer, "
            "   points.scd_out, "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.airp, "
            "   rfisc_stat.airp_arv, "
            "   points.craft, "
            "   rfisc_stat.travel_time, "
            "   rfisc_stat.trfer_flt_no, "
            "   rfisc_stat.trfer_suffix, "
            "   rfisc_stat.trfer_airp_arv, "
            "   rfisc_stat.desk, "
            "   rfisc_stat.user_login, "
            "   rfisc_stat.user_descr, "
            "   rfisc_stat.time_create, "
            "   rfisc_stat.tag_no, "
            "   rfisc_stat.fqt_no, "
            "   rfisc_stat.excess, "
            "   rfisc_stat.paid "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_points points, \n"
                "   arx_rfisc_stat rfisc_stat \n";
        } else {
            SQLText +=
                "   points, \n"
                "   rfisc_stat \n";
        }
        SQLText +=
            "where ";
        SQLText +=
            "   rfisc_stat.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if (pass!=0)
          SQLText +=
            "    points.part_key >= :FirstDate AND points.part_key < :LastDate and \n"
            "    rfisc_stat.part_key >= :FirstDate AND rfisc_stat.part_key < :LastDate \n";
        else
          SQLText +=
            "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";

        TCachedQuery Qry(SQLText, QryParams);

        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_rfisc = Qry.get().GetFieldIndex("rfisc");
            int col_point_id = Qry.get().GetFieldIndex("point_id");
            int col_point_num = Qry.get().GetFieldIndex("point_num");
            int col_pr_trfer = Qry.get().GetFieldIndex("pr_trfer");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_airp = Qry.get().GetFieldIndex("airp");
            int col_airp_arv = Qry.get().GetFieldIndex("airp_arv");
            int col_craft = Qry.get().GetFieldIndex("craft");
            int col_travel_time = Qry.get().GetFieldIndex("travel_time");
            int col_trfer_flt_no = Qry.get().GetFieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().GetFieldIndex("trfer_suffix");
            int col_trfer_airp_arv = Qry.get().GetFieldIndex("trfer_airp_arv");
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_user_login = Qry.get().GetFieldIndex("user_login");
            int col_user_descr = Qry.get().GetFieldIndex("user_descr");
            int col_time_create = Qry.get().GetFieldIndex("time_create");
            int col_tag_no = Qry.get().GetFieldIndex("tag_no");
            int col_fqt_no = Qry.get().GetFieldIndex("fqt_no");
            int col_excess = Qry.get().GetFieldIndex("excess");
            int col_paid = Qry.get().GetFieldIndex("paid");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TRFISCStatRow row;
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.point_num = Qry.get().FieldAsInteger(col_point_num);
                row.pr_trfer = Qry.get().FieldAsInteger(col_pr_trfer);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                row.craft = Qry.get().FieldAsString(col_craft);
                if(not Qry.get().FieldIsNULL(col_travel_time))
                    row.travel_time = Qry.get().FieldAsDateTime(col_travel_time);
                row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.user_login = Qry.get().FieldAsString(col_user_login);
                row.user_descr = Qry.get().FieldAsString(col_user_descr);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                row.tag_no = Qry.get().FieldAsFloat(col_tag_no);
                row.fqt_no = Qry.get().FieldAsString(col_fqt_no);
                if(not Qry.get().FieldIsNULL(col_excess))
                    row.excess = Qry.get().FieldAsInteger(col_excess);
                if(not Qry.get().FieldIsNULL(col_paid))
                    row.paid = Qry.get().FieldAsInteger(col_paid);
                RFISCStat.insert(row);
                params.overflow.check(RFISCStat.size());
            }
        }
    }
}

void createXMLRFISCStat(const TStatParams &params, const TRFISCStat &RFISCStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(RFISCStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", "RFISC");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Платн."));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Опл."));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бирка"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("SPEQ"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("FQTV"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип ВС"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время в пути"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трфр.рейс"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП рег."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", "LOGIN");
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата оформ."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    ostringstream buf;
    for(TRFISCStat::iterator i = RFISCStat.begin(); i != RFISCStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // RFISC
        NewTextChild(rowNode, "col", i->rfisc);
        // Признак платности багажа
        if(i->excess == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->excess);
        // Признак оплаты
        if(i->paid == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->paid);
        // Бирка
        NewTextChild(rowNode, "col", get_tag_no(i->tag_no));
        // SPEQ - признак спец. багажа
        NewTextChild(rowNode, "col");
        // Статус или признак FQTV
        NewTextChild(rowNode, "col", i->fqt_no);

        // Информация о рейсе
        // Дата вылета
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // Рейс
        buf.str("");
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // Тип ВС
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        // Время в пути
        if(i->travel_time == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->travel_time, "hh:nn"));

        // Трансфер на
        ostringstream trfer_flt_no;
        string trfer_airp_dep;
        string trfer_airp_arv;
        if(i->trfer_flt_no) {
            trfer_flt_no << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            trfer_airp_dep = i->airp;
            trfer_airp_arv = i->trfer_airp_arv;
        }
        // Рейс
        NewTextChild(rowNode, "col", trfer_flt_no.str());
        // От (совпадает с От рейса)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_dep));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_arv));

        // Информация об агенте
        // АП рег. (совпадает с От рейса)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // Стойка
        NewTextChild(rowNode, "col", i->desk);
        // LOGIN
        NewTextChild(rowNode, "col", i->user_login);
        // Агент
        NewTextChild(rowNode, "col", i->user_descr);
        // Дата оформления
        if(i->time_create == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Багажные RFISC"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));

    NewTextChild(variablesNode, "CAP.STAT.AGENT_INFO", getLocaleText("CAP.STAT.AGENT_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.BAG_INFO", getLocaleText("CAP.STAT.BAG_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.PAX_INFO", getLocaleText("CAP.STAT.PAX_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.FLT_INFO", getLocaleText("CAP.STAT.FLT_INFO"));
}

