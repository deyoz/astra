#include "annul_bt.h"
#include "qrys.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;

void TAnnulBT::toDB()
{
    if(point_id == ASTRA::NoExists) return;

    QParams qryParams;
    qryParams
        << QParam("id", otInteger)
        << QParam("point_id", otInteger, point_id)
        << QParam("bag_type", otInteger)
        << QParam("rfisc", otString)
        << QParam("time_create", otDate)
        << QParam("amount", otInteger)
        << QParam("weight", otInteger)
        << QParam("time_annul", otDate, NowUTC())
        << QParam("trfer_airline", otString, trfer_airline)
        << QParam("trfer_suffix", otString, trfer_suffix);
    if(trfer_flt_no == ASTRA::NoExists)
        qryParams << QParam("trfer_flt_no", otInteger, FNull);
    else
        qryParams << QParam("trfer_flt_no", otInteger, trfer_flt_no);
    if(trfer_scd == ASTRA::NoExists)
        qryParams << QParam("trfer_scd", otDate, FNull);
    else
        qryParams << QParam("trfer_scd", otDate, trfer_scd);

    TCachedQuery Qry(
            "begin "
            "   insert into annul_bag ( "
            "      id, "
            "      point_id, "
            "      bag_type, "
            "      rfisc, "
            "      time_create, "
            "      time_annul, "
            "      amount, "
            "      weight, "
            "      trfer_airline, "
            "      trfer_flt_no, "
            "      trfer_suffix, "
            "      trfer_scd "
            "   ) values ( "
            "      id__seq.nextval, "
            "      :point_id, "
            "      :bag_type, "
            "      :rfisc, "
            "      :time_create, "
            "      :time_annul, "
            "      :amount, "
            "      :weight, "
            "      :trfer_airline, "
            "      :trfer_flt_no, "
            "      :trfer_suffix, "
            "      :trfer_scd "
            "   ) returning id into :id; "
            "end; ",
        qryParams
            );
    TCachedQuery tagsQry(
            "insert into annul_tags(id, no) "
            "   values(:id, :no)",
            QParams()
            << QParam("id", otInteger)
            << QParam("no", otFloat));
    for(TBagNumMap::iterator
            bag_num = items.begin();
            bag_num != items.end();
            bag_num++) {
        if(bag_num->second.bag_tags.empty()) continue;
        Qry.get().SetVariable("bag_type", bag_num->second.bag_item.bag_type);
        Qry.get().SetVariable("rfisc", bag_num->second.bag_item.rfisc);
        Qry.get().SetVariable("time_create", bag_num->second.bag_item.time_create);
        Qry.get().SetVariable("amount", bag_num->second.bag_item.amount);
        Qry.get().SetVariable("weight", bag_num->second.bag_item.weight);

        Qry.get().Execute();
        int id = Qry.get().GetVariableAsInteger("id");
        for(list<CheckIn::TTagItem>::iterator
                bag_tag = bag_num->second.bag_tags.begin();
                bag_tag != bag_num->second.bag_tags.end();
                bag_tag++) {
            tagsQry.get().SetVariable("id", id);
            tagsQry.get().SetVariable("no", bag_tag->no);
            tagsQry.get().Execute();
        }
    }
}

void TAnnulBT::minus(const TAnnulBT &annul_bt)
{
    for(TBagNumMap::const_iterator
            bag_num = annul_bt.items.begin();
            bag_num != annul_bt.items.end();
            bag_num++) {
        items.erase(bag_num->first);
    }
}

void TAnnulBT::minus(const map<int, CheckIn::TBagItem> &bag_items)
{
    for(map<int, CheckIn::TBagItem>::const_iterator
            i = bag_items.begin();
            i != bag_items.end();
            i++) {
        items.erase(i->second.num);
    }
}

void TAnnulBT::dump()
{
    LogTrace(TRACE5) << "---TAnnulBT::dump---";
    LogTrace(TRACE5) << "point_id: " << point_id;
    for(TBagNumMap::iterator
            bag_num = items.begin();
            bag_num != items.end();
            bag_num++) {
        LogTrace(TRACE5)
            << "[" << bag_num->first << "]"
            << ": "
            << bag_num->second.bag_tags.size();
    }
    LogTrace(TRACE5) << "---------------------------";
}

void TAnnulBT::clear()
{
    point_id = ASTRA::NoExists;

    trfer_airline.clear();
    trfer_flt_no = ASTRA::NoExists;
    trfer_suffix.clear();
    trfer_scd = ASTRA::NoExists;

    items.clear();
}

void TAnnulBT::get(int grp_id)
{
    clear();

    TCachedQuery pointQry("select point_dep from pax_grp where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    pointQry.get().Execute();
    if(not pointQry.get().Eof)
        point_id = pointQry.get().FieldAsInteger("point_dep");
    if(point_id == ASTRA::NoExists) return; // группа может быть удалена уже

    TCachedQuery trferQry(
            "select "
            "   trfer_trips.airline trfer_airline, "
            "   trfer_trips.flt_no trfer_flt_no, "
            "   trfer_trips.suffix trfer_suffix, "
            "   trfer_trips.scd trfer_scd "
            "from "
            "   transfer, "
            "   trfer_trips "
            "where "
            "   transfer.grp_id = :grp_id and "
            "   transfer.pr_final <> 0 and "
            "   transfer.point_id_trfer = trfer_trips.point_id ",
            QParams() << QParam("grp_id", otInteger, grp_id));
    trferQry.get().Execute();
    if(not trferQry.get().Eof) {
        trfer_airline = trferQry.get().FieldAsString("trfer_airline");
        trfer_flt_no = trferQry.get().FieldAsInteger("trfer_flt_no");
        trfer_suffix = trferQry.get().FieldAsString("trfer_suffix");
        trfer_scd = trferQry.get().FieldAsDateTime("trfer_scd");
    };

    TCachedQuery bagQry("SELECT * FROM bag2 WHERE grp_id=:grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    bagQry.get().Execute();
    for(; not bagQry.get().Eof; bagQry.get().Next()) {
        CheckIn::TBagItem bag_item;
        bag_item.fromDB(bagQry.get());
        items[bag_item.num].bag_item = bag_item;
    }
    TCachedQuery tagQry("SELECT * FROM bag_tags WHERE grp_id=:grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    tagQry.get().Execute();
    for(; not tagQry.get().Eof; tagQry.get().Next()) {
        CheckIn::TTagItem tag_item;
        tag_item.fromDB(tagQry.get());
        items[tag_item.bag_num].bag_tags.push_back(tag_item);
    }
}

