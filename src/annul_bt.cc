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
            "select "
            "   ckin.get_bag_pool_pax_id(:grp_id, :bag_pool_num) "
            "from "
            "   dual",
            QParams()
            << QParam("grp_id", otInteger, grp_id)
            << QParam("bag_pool_num", otInteger, bag_pool_num));
    Qry.get().Execute();
    if(not Qry.get().Eof and not Qry.get().FieldIsNULL(0))
        result = Qry.get().FieldAsInteger(0);
    return result;
}

void TAnnulBT::toDB(const TBagIdMap &items, TDateTime time_annul)
{
    if(grp_id == ASTRA::NoExists
       || point_id == ASTRA::NoExists
       || scd_out == ASTRA::NoExists)
    {
      return;
    }

    try {
        DB::TCachedQuery paxQry(
              PgOra::getROSession("PAX"),
              "SELECT * FROM pax WHERE pax_id = :pax_id",
              QParams() << QParam("pax_id", otInteger),
              STDLOG
              );

        QParams qryParams;
        qryParams
            << QParam("id", otInteger)
            << QParam("grp_id", otInteger, grp_id)
            << QParam("pax_id", otInteger)
            << QParam("point_id", otInteger, point_id)
            << QParam("bag_type", otInteger)
            << QParam("rfisc", otString)
            << QParam("scd_out", otDate, scd_out)
            << QParam("time_create", otDate)
            << QParam("time_annul", otDate)
            << QParam("amount", otInteger)
            << QParam("weight", otInteger)
            << QParam("user_id", otInteger);

        DB::TCachedQuery Qry(
              PgOra::getRWSession("ANNUL_BAG"),
              "INSERT INTO annul_bag ( "
              "id, grp_id, pax_id, point_id, bag_type, rfisc, scd_out, "
              "time_create, time_annul, amount, weight, user_id "
              ") VALUES ("
              ":id, :grp_id, :pax_id, :point_id, :bag_type, :rfisc, :scd_out, "
              ":time_create, :time_annul, :amount, :weight, :user_id "
              ") ",
              qryParams, STDLOG
              );
        DB::TCachedQuery tagsQry(
              PgOra::getRWSession("ANNUL_TAGS"),
              "INSERT INTO ANNUL_TAGS("
              "id, no"
              ") VALUES("
              ":id, :no"
              ")",
              QParams() << QParam("id", otInteger)
                        << QParam("no", otFloat),
              STDLOG
              );
        for(TBagIdMap::const_iterator
                bag_id = items.begin();
                bag_id != items.end();
                bag_id++) {

            if(bag_id->second.pax_id != ASTRA::NoExists) {
                paxQry.get().SetVariable("pax_id", bag_id->second.pax_id);
                paxQry.get().Execute();
                // Если пакса нету в базе, то нефиг писать по нему статистику
                if(paxQry.get().Eof) continue;
            }

            const int id = PgOra::getSeqNextVal_int("CYCLE_ID__SEQ");
            Qry.get().SetVariable("id", id);

            if(bag_id->second.pax_id == ASTRA::NoExists)
                Qry.get().SetVariable("pax_id", FNull);
            else
                Qry.get().SetVariable("pax_id", bag_id->second.pax_id);
            Qry.get().SetVariable("rfisc", FNull);
            Qry.get().SetVariable("bag_type", FNull);
            if(bag_id->second.bag_item.pc)
              Qry.get().SetVariable("rfisc", bag_id->second.bag_item.pc.get().RFISC);
            else if(bag_id->second.bag_item.wt)
              BagTypeToDB(Qry.get(), bag_id->second.bag_item.wt.get().bag_type, "TAnnulBT::toDB");
            if(bag_id->second.bag_item.time_create == ASTRA::NoExists)
                Qry.get().SetVariable("time_create", FNull);
            else
                Qry.get().SetVariable("time_create", bag_id->second.bag_item.time_create);
            if(bag_id->second.time_annul == ASTRA::NoExists)
                Qry.get().SetVariable("time_annul", time_annul);
            else
                Qry.get().SetVariable("time_annul", bag_id->second.time_annul);
            if(bag_id->second.bag_item.amount == ASTRA::NoExists)
                Qry.get().SetVariable("amount", FNull);
            else
                Qry.get().SetVariable("amount", bag_id->second.bag_item.amount);
            if(bag_id->second.bag_item.weight == ASTRA::NoExists)
                Qry.get().SetVariable("weight", FNull);
            else
                Qry.get().SetVariable("weight", bag_id->second.bag_item.weight);
            if(bag_id->second.bag_item.user_id == ASTRA::NoExists)
                Qry.get().SetVariable("user_id", FNull);
            else
                Qry.get().SetVariable("user_id", bag_id->second.bag_item.user_id);

            Qry.get().Execute();
            for(list<CheckIn::TTagItem>::const_iterator
                    bag_tag = bag_id->second.bag_tags.begin();
                    bag_tag != bag_id->second.bag_tags.end();
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

bool TAnnulBT::find_tag(const CheckIn::TTagItem &tag) const
{
    bool result = false;
    for(TBagIdMap::const_iterator
            bag_id = items.begin();
            bag_id != items.end();
            bag_id++) {
        for(list<CheckIn::TTagItem>::const_iterator
                bag_tag = bag_id->second.bag_tags.begin();
                bag_tag != bag_id->second.bag_tags.end();
                bag_tag++) {
            result = bag_tag->no == tag.no;
            if(result) break;
        }
        if(result) break;
    }
    return result;
}

bool TAnnulBT::TRMExistTags::TCheck::operator()(const CheckIn::TTagItem &tag)
{
    return annul_bt.find_tag(tag);
}

bool TAnnulBT::TRMExistTags::exec(
        list<CheckIn::TTagItem> &bag_tags,
        const TAnnulBT &annul_bt
        )
{
    list<CheckIn::TTagItem>::iterator pbegin, pend;
    pbegin = bag_tags.begin();
    pend = bag_tags.end();
    pend = remove_if(pbegin, pend, TCheck(annul_bt));
    bool result = pend != bag_tags.end();
    bag_tags.erase(pend, bag_tags.end());
    return result;
}

void TAnnulBT::minus(const TAnnulBT &annul_bt)
{
    if(annul_bt.get_grp_id() == ASTRA::NoExists) {
        clear();
    } else {
        for(TBagIdMap::iterator
                bag_id = items.begin();
                bag_id != items.end();)
            if(TRMExistTags().exec(bag_id->second.bag_tags, annul_bt)) {
                if(bag_id->second.bag_tags.empty())
                    items.erase(bag_id++);
                else {
                    // Если у багажа остались бирки, то они были отвязаны
                    bag_id->second.pax_id = ASTRA::NoExists;
                    bag_id->second.bag_item.pc=boost::none;
                    bag_id->second.bag_item.wt=boost::none;
                    bag_id->second.bag_item.amount = ASTRA::NoExists;
                    bag_id->second.bag_item.weight = ASTRA::NoExists;
                    bag_id++;
                }
            } else
                bag_id++;
    }
}

void TAnnulBT::dump() const
{
    LogTrace(TRACE5) << "---TAnnulBT::dump---";
    LogTrace(TRACE5) << "grp_id: " << grp_id;
    for(TBagIdMap::const_iterator
            bag_id = items.begin();
            bag_id != items.end();
            bag_id++) {
        LogTrace(TRACE5)
            << "[" << bag_id->first << "]"
            << ": "
            << bag_id->second.bag_tags.size();
        bag_id->second.dump(__FILE__, __LINE__);
    }
    LogTrace(TRACE5) << "---------------------------";
}

void TAnnulBT::clear()
{
    grp_id = ASTRA::NoExists;
    point_id = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    items.clear();
    backup_items.clear();
}

void TAnnulBT::TBagTags::dump(const string &file, int line) const
{
    LogTrace(TRACE5) << "---TAnnulBT::TBagTags::dump(): " << file << ":" << (line == ASTRA::NoExists ? 0 : line) << "---";
    LogTrace(TRACE5) << "pax_id: " << pax_id;
    LogTrace(TRACE5) << "id: " << bag_item.id;
    LogTrace(TRACE5) << "bag_num: " << bag_item.num;
    LogTrace(TRACE5) << "time_create: " << DateTimeToStr(bag_item.time_create);
    LogTrace(TRACE5) << "time_annul: " << DateTimeToStr(time_annul);
    LogTrace(TRACE5) << "amount: " << bag_item.amount;
    LogTrace(TRACE5) << "weight: " << bag_item.weight;
    string tag_list;
    for(std::list<CheckIn::TTagItem>::const_iterator
            i = bag_tags.begin();
            i != bag_tags.end();
            i++) {
        if(not tag_list.empty())
            tag_list += ", ";
        tag_list += FloatToString(i->no, 0);
    }
    LogTrace(TRACE5) << "tag_list: " << tag_list;
    LogTrace(TRACE5) << "-----------------------------------";
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

    DB::TCachedQuery Qry(
          PgOra::getROSession("ANNUL_BAG"),
          "SELECT * FROM annul_bag WHERE grp_id = :grp_id",
          QParams() << QParam("grp_id", otInteger, grp_id), STDLOG);
    DB::TCachedQuery tagsQry(
          PgOra::getROSession("ANNUL_TAGS"),
          "SELECT * FROM annul_tags WHERE id = :id",
          QParams() << QParam("id", otInteger), STDLOG);
    Qry.get().Execute();
    list<int> ids;
    if(not Qry.get().Eof) {
        int col_id = Qry.get().FieldIndex("id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_rfisc = Qry.get().FieldIndex("rfisc");
        int col_time_create = Qry.get().FieldIndex("time_create");
        int col_time_annul = Qry.get().FieldIndex("time_annul");
        int col_amount = Qry.get().FieldIndex("amount");
        int col_weight = Qry.get().FieldIndex("weight");
        int col_user_id = Qry.get().FieldIndex("user_id");
        int bag_id = 1;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            CheckIn::TBagItem bag_item;
            bag_item.id = bag_id++;
            if(not Qry.get().FieldIsNULL(col_rfisc))
            {
              bag_item.pc=TRFISCKey();
              bag_item.pc.get().RFISC=Qry.get().FieldAsString(col_rfisc);
            }
            else
            {
              bag_item.wt=TBagTypeKey();
              bag_item.wt.get().bag_type=BagTypeFromDB(Qry.get());
            };
            if(not Qry.get().FieldIsNULL(col_time_create))
                bag_item.time_create =  Qry.get().FieldAsDateTime(col_time_create);
            if(not Qry.get().FieldIsNULL(col_amount))
                bag_item.amount =  Qry.get().FieldAsInteger(col_amount);
            if(not Qry.get().FieldIsNULL(col_weight))
                bag_item.weight =  Qry.get().FieldAsInteger(col_weight);
            if(not Qry.get().FieldIsNULL(col_user_id))
                bag_item.user_id =  Qry.get().FieldAsInteger(col_user_id);
            TBagTags &bag_tags = backup_items[bag_item.id];
            if(not Qry.get().FieldIsNULL(col_pax_id))
                bag_tags.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            bag_tags.bag_item = bag_item;
            if(not Qry.get().FieldIsNULL(col_time_annul))
                bag_tags.time_annul = Qry.get().FieldAsDateTime(col_time_annul);

            ids.push_back(Qry.get().FieldAsInteger(col_id));
            tagsQry.get().SetVariable("id", ids.back());
            tagsQry.get().Execute();
            for(; not tagsQry.get().Eof; tagsQry.get().Next()) {
                CheckIn::TTagItem tag_item;
                tag_item.no = tagsQry.get().FieldAsFloat("no");
                bag_tags.bag_tags.push_back(tag_item);
            }
        }
    }
    DB::TCachedQuery delTagsQry(
          PgOra::getRWSession("ANNUL_TAGS"),
          "DELETE FROM annul_tags WHERE id = :id",
          QParams() << QParam("id", otInteger), STDLOG);
    for(list<int>::iterator id = ids.begin(); id != ids.end(); id++)
    {
        delTagsQry.get().SetVariable("id", *id);
        delTagsQry.get().Execute();
    }
    DB::TCachedQuery delBagQry(
          PgOra::getRWSession("ANNUL_BAG"),
          "DELETE FROM annul_bag WHERE grp_id = :grp_id",
          QParams() << QParam("grp_id", otInteger, grp_id), STDLOG);
    delBagQry.get().Execute();
}

void TAnnulBT::get(int grp_id)
{
    LogTrace(TRACE6) << "TAnnulBT::" << __func__ << ": grp_id=" << grp_id;
    clear();

    DB::TCachedQuery grpQry(
          PgOra::getROSession("PAX_GRP"),
          "SELECT * FROM pax_grp WHERE grp_id = :grp_id",
          QParams() << QParam("grp_id", otInteger, grp_id),
          STDLOG
          );
    grpQry.get().Execute();
    if(grpQry.get().Eof) return;

    this->grp_id = grp_id;
    this->point_id = grpQry.get().FieldAsInteger("point_dep");

    DB::TCachedQuery pointQry(
          PgOra::getROSession("POINTS"),
          "SELECT * FROM points WHERE point_id = :point_id",
          QParams() << QParam("point_id", otInteger, this->point_id),
          STDLOG
          );
    pointQry.get().Execute();
    if(pointQry.get().Eof) return;
    this->scd_out = pointQry.get().FieldAsInteger("scd_out");

    backup();

    try {
        DB::TCachedQuery bagQry(
              PgOra::getROSession("BAG2"),
              "SELECT * FROM bag2 WHERE grp_id=:grp_id",
              QParams() << QParam("grp_id", otInteger, grp_id),
              STDLOG);
        bagQry.get().Execute();
        for(; not bagQry.get().Eof; bagQry.get().Next()) {

            CheckIn::TBagItem bag_item;
            bag_item.fromDB(bagQry.get());
            // Сюда не создавшего багаж, а тек. юзера
            bag_item.user_id = TReqInfo::Instance()->user.user_id;
            if(bag_item.pr_cabin) continue;
            TBagTags &bag_tags = items[bag_item.id];
            bag_tags.pax_id = get_pax_id(grp_id, bag_item.bag_pool_num);
            bag_tags.bag_item = bag_item;

            DB::TCachedQuery tagQry(
                  PgOra::getROSession("BAG_TAGS"),
                  "SELECT * FROM bag_tags WHERE grp_id=:grp_id and bag_num = :bag_num ",
                  QParams()
                  << QParam("grp_id", otInteger, grp_id)
                  << QParam("bag_num", otInteger, bag_item.num),
                  STDLOG);
            tagQry.get().Execute();
            for(; not tagQry.get().Eof; tagQry.get().Next()) {
                CheckIn::TTagItem tag_item;
                tag_item.fromDB(tagQry.get());
                bag_tags.bag_tags.push_back(tag_item);
            }
        }
    } catch(Exception &E) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: " << E.what();
    } catch(...) {
        LogError(STDLOG) <<  "TAnnulBT::get failed: something goes wrong";
    }
}
