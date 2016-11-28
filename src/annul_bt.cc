#include "annul_bt.h"
#include "qrys.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

int get_pax_id(int grp_id, int bag_pool_num)
{
    int result = ASTRA::NoExists;
    TCachedQuery Qry(
            "begin "
            "   ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) "
            "from "
            "   dual",
            QParams()
            << QParam("grp_id", otInteger, grp_id)
            << QParam("bag_pool_num", otInteger, bag_pool_num));
    Qry.get().Execute();
    if(not Qry.get().Eof)
        result = Qry.get().FieldAsInteger(0);
    return result;
}

void TAnnulBT::toDB(const TBagNumMap &items, TDateTime time_annul)
{
    if(grp_id == ASTRA::NoExists) return;

    try {
        QParams qryParams;
        qryParams
            << QParam("id", otInteger)
            << QParam("grp_id", otInteger, grp_id)
            << QParam("pax_id", otInteger)
            << QParam("bag_type", otInteger)
            << QParam("rfisc", otString)
            << QParam("time_create", otDate)
            << QParam("time_annul", otDate)
            << QParam("amount", otInteger)
            << QParam("weight", otInteger);

        TCachedQuery Qry(
                "begin "
                "   insert into annul_bag ( "
                "      id, "
                "      grp_id, "
                "      pax_id, "
                "      bag_type, "
                "      rfisc, "
                "      time_create, "
                "      time_annul, "
                "      amount, "
                "      weight "
                "   ) values ( "
                "      id__seq.nextval, "
                "      :grp_id, "
                "      :pax_id, "
                "      :bag_type, "
                "      :rfisc, "
                "      :time_create, "
                "      :time_annul, "
                "      :amount, "
                "      :weight "
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
        for(TBagNumMap::const_iterator
                bag_num = items.begin();
                bag_num != items.end();
                bag_num++) {
            if(bag_num->second.bag_tags.empty()) continue;
            if(bag_num->second.pax_id == ASTRA::NoExists)
                Qry.get().SetVariable("pax_id", FNull);
            else
                Qry.get().SetVariable("pax_id", bag_num->second.pax_id);
            if(bag_num->second.bag_item.bag_type == ASTRA::NoExists)
                Qry.get().SetVariable("bag_type", FNull);
            else
                Qry.get().SetVariable("bag_type", bag_num->second.bag_item.bag_type);
            Qry.get().SetVariable("rfisc", bag_num->second.bag_item.rfisc);
            if(bag_num->second.bag_item.time_create == ASTRA::NoExists)
                Qry.get().SetVariable("time_create", FNull);
            else
                Qry.get().SetVariable("time_create", bag_num->second.bag_item.time_create);
            if(bag_num->second.time_annul == ASTRA::NoExists)
                Qry.get().SetVariable("time_annul", time_annul);
            else
                Qry.get().SetVariable("time_annul", bag_num->second.time_annul);
            Qry.get().SetVariable("amount", bag_num->second.bag_item.amount);
            Qry.get().SetVariable("weight", bag_num->second.bag_item.weight);

            Qry.get().Execute();
            int id = Qry.get().GetVariableAsInteger("id");
            for(list<CheckIn::TTagItem>::const_iterator
                    bag_tag = bag_num->second.bag_tags.begin();
                    bag_tag != bag_num->second.bag_tags.end();
                    bag_tag++) {
                tagsQry.get().SetVariable("id", id);
                tagsQry.get().SetVariable("no", bag_tag->no);
                tagsQry.get().Execute();
            }
        }
    } catch(Exception &E) {
        LogError(STDLOG) <<  "TAnnulBT::toDB failed: " << E.what();
    } catch(...) {
        LogError(STDLOG) <<  "TAnnulBT::toDB failed: something goes wrong";
    }
}

void TAnnulBT::toDB()
{
    TDateTime annul_date = NowUTC();
    toDB(backup_items, annul_date);
    toDB(items, annul_date);
}

void TAnnulBT::minus(const TAnnulBT &annul_bt)
{
    if(annul_bt.get_grp_id() == ASTRA::NoExists) {
        clear();
    } else
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
    grp_id = ASTRA::NoExists;
    items.clear();
    backup_items.clear();
}

void TAnnulBT::TBagTags::clear()
{
    pax_id = ASTRA::NoExists;
    time_annul = ASTRA::NoExists;
    bag_item.clear();
    bag_tags.clear();
}

void TAnnulBT::backup()
{
    if(grp_id == ASTRA::NoExists) return;

    TCachedQuery Qry("select * from annul_bag where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    TCachedQuery tagsQry("select * from annul_tags where id = :id",
            QParams() << QParam("id", otInteger));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_id = Qry.get().FieldIndex("id");
        int col_grp_id = Qry.get().FieldIndex("grp_id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_bag_type = Qry.get().FieldIndex("bag_type");
        int col_rfisc = Qry.get().FieldIndex("rfisc");
        int col_time_create = Qry.get().FieldIndex("time_create");
        int col_time_annul = Qry.get().FieldIndex("time_annul");
        int col_amount = Qry.get().FieldIndex("amount");
        int col_weight = Qry.get().FieldIndex("weight");
        int bag_num = 1;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            CheckIn::TBagItem bag_item;
            bag_item.id = Qry.get().FieldAsInteger(col_grp_id);
            bag_item.num = bag_num++;
            if(not Qry.get().FieldIsNULL(col_bag_type))
                bag_item.bag_type = Qry.get().FieldAsInteger(col_bag_type);
            bag_item.rfisc = Qry.get().FieldAsString(col_rfisc);
            if(not Qry.get().FieldIsNULL(col_time_create))
                bag_item.time_create =  Qry.get().FieldAsDateTime(col_time_create);
            if(not Qry.get().FieldIsNULL(col_amount))
                bag_item.amount =  Qry.get().FieldAsInteger(col_amount);
            if(not Qry.get().FieldIsNULL(col_weight))
                bag_item.weight =  Qry.get().FieldAsInteger(col_weight);
            TBagTags &bag_tags = backup_items[bag_item.num];
            if(not Qry.get().FieldIsNULL(col_pax_id))
                bag_tags.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            bag_tags.bag_item = bag_item;
            if(not Qry.get().FieldIsNULL(col_time_annul))
                bag_tags.time_annul = Qry.get().FieldAsDateTime(col_time_annul);

            tagsQry.get().SetVariable("id", Qry.get().FieldAsInteger(col_id));
            tagsQry.get().Execute();
            for(; not tagsQry.get().Eof; tagsQry.get().Next()) {
                CheckIn::TTagItem tag_item;
                tag_item.no = tagsQry.get().FieldAsFloat("no");
                backup_items[bag_item.num].bag_tags.push_back(tag_item);
            }

        }
    }
    TCachedQuery delQry(
            "begin "
            "   delete from annul_tags where id in "
            "       (select id from annul_bag where grp_id = :grp_id); "
            "   delete from annul_bag where grp_id = :grp_id; "
            "end; ",
            QParams() << QParam("grp_id", otInteger, grp_id));
    delQry.get().Execute();
}

void TAnnulBT::get(int grp_id)
{
    clear();

    TCachedQuery grpQry("select * from pax_grp where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    grpQry.get().Execute();
    if(grpQry.get().Eof) return;

    this->grp_id = grp_id;

    backup();

    /*
    try {
    */
        TCachedQuery bagQry(
                "SELECT "
                "   ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) pax_id, "
                "   bag2.* FROM bag2 WHERE grp_id=:grp_id",
                QParams() << QParam("grp_id", otInteger, grp_id));
        bagQry.get().Execute();
        for(; not bagQry.get().Eof; bagQry.get().Next()) {
            int pax_id = ASTRA::NoExists;
            if(not bagQry.get().FieldIsNULL("pax_id"))
                pax_id = bagQry.get().FieldAsInteger("pax_id");
            CheckIn::TBagItem bag_item;
            bag_item.fromDB(bagQry.get());
            TBagTags &bag_tags = items[bag_item.num];
            bag_tags.pax_id = pax_id;
            bag_tags.bag_item = bag_item;

            TCachedQuery tagQry("SELECT * FROM bag_tags WHERE grp_id=:grp_id and bag_num = :bag_num ",
                    QParams()
                    << QParam("grp_id", otInteger, grp_id)
                    << QParam("bag_num", otInteger, bag_item.num));
            tagQry.get().Execute();
            for(; not tagQry.get().Eof; tagQry.get().Next()) {
                CheckIn::TTagItem tag_item;
                tag_item.fromDB(tagQry.get());
                items[bag_item.num].bag_tags.push_back(tag_item);
            }
        }
        /*
    } catch(Exception &E) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: " << E.what();
    } catch(...) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: something goes wrong";
    }
    */
}
