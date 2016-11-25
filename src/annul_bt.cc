#include "annul_bt.h"
#include "qrys.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

void TAnnulBT::toDB()
{
    try {
        QParams qryParams;
        qryParams
            << QParam("id", otInteger)
            << QParam("grp_id", otInteger, grp_id)
            << QParam("pax_id", otInteger)
            << QParam("bag_type", otInteger)
            << QParam("rfisc", otString)
            << QParam("time_create", otDate)
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
        /*
        for(TBagNumMap::iterator
                bag_num = items.begin();
                bag_num != items.end();
                bag_num++) {
            if(bag_num->second.bag_tags.empty()) continue;
            if(bag_num->second.bag_item.bag_type == ASTRA::NoExists)
                Qry.get().SetVariable("bag_type", FNull);
            else
                Qry.get().SetVariable("bag_type", bag_num->second.bag_item.bag_type);
            Qry.get().SetVariable("rfisc", bag_num->second.bag_item.rfisc);
            if(bag_num->second.bag_item.time_create == ASTRA::NoExists)
                Qry.get().SetVariable("time_create", FNull);
            else
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
        */
    } catch(Exception &E) {
        LogError(STDLOG) <<  "TAnnulBT::toDB failed: " << E.what();
    } catch(...) {
        LogError(STDLOG) <<  "TAnnulBT::toDB failed: something goes wrong";
    }
}

void TAnnulBT::minus(const TAnnulBT &annul_bt)
{
    /*
    for(TBagNumMap::const_iterator
            bag_num = annul_bt.items.begin();
            bag_num != annul_bt.items.end();
            bag_num++) {
        items.erase(bag_num->first);
    }
    */
}

void TAnnulBT::minus(const map<int, CheckIn::TBagItem> &bag_items)
{
    /*
    for(map<int, CheckIn::TBagItem>::const_iterator
            i = bag_items.begin();
            i != bag_items.end();
            i++) {
        items.erase(i->second.num);
    }
    */
}

void TAnnulBT::dump()
{
    /*
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
    */
}

void TAnnulBT::clear()
{
    grp_id = ASTRA::NoExists;

    items.clear();
}

void TAnnulBT::TBagTags::add(const CheckIn::TBagItem &abag_item)
{
    if(bag_item.amount == ASTRA::NoExists) {
        bag_item.amount = abag_item.amount;
        bag_item.weight = abag_item.weight;
    } else {
        bag_item.amount += abag_item.amount;
        bag_item.weight += abag_item.weight;
    }
}

void TAnnulBT::get(int grp_id)
{
    clear();

    this->grp_id = grp_id;

    try {
        TCachedQuery bagQry(
                "SELECT "
                "   ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) pax_id, "
                "   * FROM bag2 WHERE grp_id=:grp_id",
                QParams() << QParam("grp_id", otInteger, grp_id));
        bagQry.get().Execute();
        for(; not bagQry.get().Eof; bagQry.get().Next()) {
            int pax_id = ASTRA::NoExists;
            if(not bagQry.get().FieldIsNULL("pax_id"))
                pax_id = bagQry.get().FieldAsInteger("pax_id");
            CheckIn::TBagItem bag_item;
            bag_item.fromDB(bagQry.get());
            items[pax_id][bag_item.bag_type][bag_item.rfisc].add(bag_item);

            TCachedQuery tagQry("SELECT * FROM bag_tags WHERE grp_id=:grp_id and bag_num = :bag_num ",
                    QParams()
                    << QParam("grp_id", otInteger, grp_id)
                    << QParam("bag_num", otInteger, bag_item.num));
            tagQry.get().Execute();
            for(; not tagQry.get().Eof; tagQry.get().Next()) {
                CheckIn::TTagItem tag_item;
                tag_item.fromDB(tagQry.get());
                items[pax_id][bag_item.bag_type][bag_item.rfisc].bag_tags.push_back(tag_item);
            }


        }
    } catch(Exception &E) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: " << E.what();
    } catch(...) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: something goes wrong";
    }
}
