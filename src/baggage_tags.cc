#include "baggage_tags.h"

#include "astra_consts.h"
#include "base_tables.h"
#include "qrys.h"
#include "passenger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;

void GetTagRanges(const multiset<TBagTagNumber> &tags,
                    vector<string> &ranges)
{
  ranges.clear();
  if (tags.empty()) return;

  TBagTagNumber first=*(tags.begin());
  int diff=0;
  for(std::multiset<TBagTagNumber>::const_iterator iTag=tags.begin();; ++iTag)
  {
    const TBagTagNumber& curr=(iTag!=tags.end()?*iTag:*(tags.begin()));

    if (iTag==tags.end() ||
        !first.equal_pack(curr) ||
        first.number_in_pack()+diff!=curr.number_in_pack())
    {
      ostringstream range;
      range << first.str();
      if (diff!=1)
        range << "-" << first.number_in_pack_str(diff-1);

      ranges.push_back(range.str());

      if (iTag==tags.end()) break;
      first=curr;
      diff=0;
    };
    diff++;
  }
}

string GetTagRangesStrShort(const multiset<TBagTagNumber> &tags)
{
  ostringstream s;
  if (tags.empty()) return s.str();

  boost::optional<TBagTagNumber> first_in_pack;
  TBagTagNumber first_in_range=*(tags.begin());
  int diff=0;
  for(std::multiset<TBagTagNumber>::const_iterator iTag=tags.begin();; ++iTag)
  {
    const TBagTagNumber& curr=(iTag!=tags.end()?*iTag:*(tags.begin()));

    if (iTag==tags.end() ||
        !first_in_range.equal_pack(curr) ||
        first_in_range.number_in_pack()+diff!=curr.number_in_pack())
    {
      if (!first_in_pack || !first_in_range.equal_pack(first_in_pack.get()))
      {
        if (first_in_pack) s << ", ";
        s << first_in_range.str();
        first_in_pack=first_in_range;
      }
      else
      {
        s << "," << first_in_range.number_in_pack_str();
      }
      if (diff!=1)
        s << "-" << first_in_range.number_in_pack_str(diff-1);

      if (iTag==tags.end()) break;
      first_in_range=curr;
      diff=0;
    };
    diff++;
  };
  return s.str();
}

string GetTagRangesStr(const multiset<TBagTagNumber> &tags)
{
  vector<string> ranges;

  GetTagRanges(tags, ranges);

  ostringstream result;
  for(vector<string>::const_iterator r=ranges.begin(); r!=ranges.end(); ++r)
  {
    if (r!=ranges.begin()) result << " ";
    result << *r;
  };
  return result.str();
}

string GetTagRangesStr(const TBagTagNumber &tag)
{
  multiset<TBagTagNumber> tags;
  tags.insert(tag);
  return GetTagRangesStr(tags);
}

void TGeneratedTags::cleanOldRecords(int min_ago)
{
  LogTrace(TRACE6) << __func__;
  DB::TQuery Qry(PgOra::getRWSession("BAG_TAGS_GENERATED"), STDLOG);
  Qry.SQLText="DELETE FROM bag_tags_generated "
              "WHERE time_create <= :time";
  Qry.CreateVariable("time", otDate, BASIC::date_time::NowUTC()-min_ago/1440.0);
  Qry.Execute();
}

void TGeneratedTags::useDeferred(int grp_id, int tag_count)
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear(grp_id);

  if (tag_count<=0) return;

  DB::TQuery Qry(PgOra::getRWSession("BAG_TAGS_GENERATED"), STDLOG);
  Qry.SQLText="SELECT no FROM "
              "bag_tags_generated "
              "WHERE grp_id=:grp_id "
              "FOR UPDATE";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  if (Qry.Eof) return;

  for(; !Qry.Eof; Qry.Next())
  {
    auto res=_tags.emplace("", Qry.FieldAsFloat("no"));
    if (!res.second)
      throw EXCEPTIONS::Exception("%s: duplicated tag %s", __FUNCTION__, res.first->str().c_str());
  }

  std::set<TBagTagNumber>::const_iterator i=_tags.begin();
  for(int j=tag_count; j>0 && i!=_tags.end(); ++i, --j) ;
  _tags.erase(i, _tags.end());

  DB::TQuery QryDel(PgOra::getRWSession("BAG_TAGS_GENERATED"), STDLOG);
  QryDel.SQLText="DELETE FROM bag_tags_generated "
                 "WHERE grp_id=:grp_id "
                 "AND no=:no";
  QryDel.CreateVariable("grp_id", otInteger, grp_id);
  QryDel.DeclareVariable("no", otFloat);
  for(const TBagTagNumber& tag : _tags)
  {
    QryDel.SetVariable("no", tag.numeric_part);
    QryDel.Execute();
  }
}

void TGeneratedTags::defer()
{
  LogTrace(TRACE5) << __FUNCTION__;

  if (!_grp_id) return;

  DB::TQuery Qry(PgOra::getRWSession("BAG_TAGS_GENERATED"), STDLOG);
  Qry.SQLText=
    "INSERT INTO bag_tags_generated( "
    "grp_id, no, time_create "
    ") VALUES( "
    ":grp_id, :no, :time_create "
    ") ";
  Qry.CreateVariable("grp_id", otInteger, _grp_id.get());
  Qry.DeclareVariable("no", otFloat);
  Qry.CreateVariable("time_create", otDate, BASIC::date_time::NowUTC());
  for(const TBagTagNumber& tag : _tags)
  {
    Qry.SetVariable("no", tag.numeric_part);
    Qry.Execute();
  }

  clear();
}

void TGeneratedTags::add(const TBagTagRange& range)
{
  for(double numeric_part=range.numeric_part_first();
             numeric_part<=range.numeric_part_last(); numeric_part+=1.0)
  {
    auto res=_tags.emplace(range.alpha_part(), numeric_part, range.numeric_part_max_len());
    if (!res.second)
      throw EXCEPTIONS::Exception("%s: duplicated tag %s", __FUNCTION__, res.first->str().c_str());
  }
}

void TGeneratedTags::generateUsingDeferred(int grp_id, int tag_count)
{
  useDeferred(grp_id, tag_count);

  if (tag_count>(int)tags().size())
  {
    TGeneratedTags newGenerated;
    newGenerated.generate(grp_id, tag_count-tags().size());
    _tags.insert(newGenerated.tags().begin(), newGenerated.tags().end());
  }
}

bool addLastRange(const int range, const int aircode)
{

    DbCpp::CursCtl cur = make_db_curs(
       "INSERT INTO last_tag_ranges2( aircode, range) "
                             "VALUES(:aircode,:range)",
        PgOra::getRWSession("LAST_TAG_RANGES2")
    );

    cur.stb()
       .bind(":aircode", aircode)
       .bind(":range", range)
       .exec();

    if (DbCpp::ResultCode::ConstraintFail == cur.err()) {
        return false;
    }

    return true;
}

std::optional<int> getLastRange(const int aircode)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT range FROM last_tag_ranges2 "
       "WHERE aircode = :aircode "
       "FOR UPDATE",
        PgOra::getRWSession("LAST_TAG_RANGES2")
    );

    int last_range;

    cur.stb()
       .bind(":aircode", aircode)
       .def(last_range)
       .EXfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        return std::nullopt;
    }

    return last_range;
}

void TGeneratedTags::generate(int grp_id, int tag_count)
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear(grp_id);

  if (tag_count<=0) return;

  DB::TQuery Qry(PgOra::getROSession({"POINTS","PAX_GRP","TRANSFER"}),STDLOG);
  //Qry.ClearParams();
  Qry.SQLText=
      "SELECT "
        "POINTS.AIRLINE, "
        "PAX_GRP.POINT_DEP, "
        "PAX_GRP.AIRP_DEP, "
        "PAX_GRP.CLASS, "
        "COALESCE(TRANSFER.AIRP_ARV, PAX_GRP.AIRP_ARV) AS AIRP_ARV "
      "FROM PAX_GRP "
        "LEFT OUTER JOIN TRANSFER "
          "ON ( "
            "PAX_GRP.GRP_ID = TRANSFER.GRP_ID "
            "AND TRANSFER.PR_FINAL <> 0 "
          ") "
        "JOIN POINTS "
          "ON POINTS.POINT_ID = PAX_GRP.POINT_DEP "
      "WHERE PAX_GRP.GRP_ID = :grp_id";

  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) throw EXCEPTIONS::Exception("%s: group not found (grp_id=%d)", __FUNCTION__, grp_id);

  int point_id=Qry.FieldAsInteger("point_dep");
  string airp_dep=Qry.FieldAsString("airp_dep");
  string airp_arv=Qry.FieldAsString("airp_arv");
  string cl=Qry.FieldAsString("class");
  int aircode=ASTRA::NoExists;
  try
  {
    aircode=ToInt(base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline")).AsString("aircode"));
    if (aircode<=0 || aircode>999) throw EXCEPTIONS::EConvertError("");
  }
  catch(const EBaseTableError &) { aircode=ASTRA::NoExists; }
  catch(const EXCEPTIONS::EConvertError &)   { aircode=ASTRA::NoExists; };

  if (aircode==ASTRA::NoExists) aircode=954;

  int range = 0;
  int no = 0;
  bool use_new_range=false;
  int last_range=ASTRA::NoExists;
  while (tag_count>0)
  {
    if (!use_new_range)
    {
      int k;
      std::string sql;

      for(k=1;k<=5;k++)
      {
        DB::TQuery Qry(PgOra::getRWSession({"TAG_RANGES2", "POINTS"}), STDLOG);
        Qry.CreateVariable("aircode",otInteger,aircode);

        sql  =   "SELECT range, no FROM tag_ranges2 ";
        if (k >= 2) {
          sql += "LEFT OUTER JOIN points "
                 "ON tag_ranges2.point_id = points.point_id ";
        }
        sql +=   "WHERE aircode = :aircode ";

        if (k == 1) {
          sql +=   "AND point_id = :point_id AND airp_dep = :airp_dep AND airp_arv = :airp_arv "
                   "AND (class IS NULL AND :class IS NULL OR class = :class) ";
          Qry.CreateVariable("point_id",otInteger,point_id);
          Qry.CreateVariable("airp_dep",otString,airp_dep);
          Qry.CreateVariable("airp_arv",otString,airp_arv);
          Qry.CreateVariable("class",otString,cl);

        } else if (k >= 2) {
          sql +=   "AND (points.point_id IS NULL "
                     "OR points.pr_del <> 0 "
                     "OR COALESCE(points.act_out, points.est_out, points.scd_out) < :now_utc AND last_access < :now_utc - 2/24 "
                     "OR COALESCE(points.act_out, points.est_out, points.scd_out, :now_utc +1) >= :now_utc AND last_access < :now_utc - 2) ";
          Qry.CreateVariable("now_utc",otDate, BASIC::date_time::NowUTC());
          if (k == 2) {
            sql += "AND airp_dep = :airp_dep "
                   "AND airp_arv = :airp_arv "
                   "AND (class IS NULL AND :class IS NULL OR class = :class) ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);
            Qry.CreateVariable("airp_arv",otString,airp_arv);
            Qry.CreateVariable("class",otString,cl);

          } else if (k == 3) {
            sql += "AND airp_dep = :airp_dep "
                   "AND airp_arv = :airp_arv ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);
            Qry.CreateVariable("airp_arv",otString,airp_arv);

          } else if (k == 4) {
            sql += "AND airp_dep = :airp_dep ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);

          } else if (k == 5) {
            sql += "AND last_access < :now_utc - 1 ";
          }
        }
        sql +=   "ORDER BY last_access FOR UPDATE";

        Qry.SQLText = sql;
        Qry.Execute();
        if (!Qry.Eof)
        {
          range=Qry.FieldAsInteger("range");
          no=Qry.FieldAsInteger("no");
          break;
        };
      };
      if (k > 5) //среди уже существующих диапазонов нет подходящего - берем новый
      {
        use_new_range=true;

        if (last_range==ASTRA::NoExists)
        {
            if (std::optional<int> opt_last_range = getLastRange(aircode)) {
                last_range = opt_last_range.value();
            } else if (addLastRange(9999, aircode)) {
                last_range = 9999;
            } else {
                opt_last_range = getLastRange(aircode);
                last_range = opt_last_range.value();
            }
        };

        range=last_range;
      };
    };
    if (use_new_range)
    {
      range++;
      no=-1;
      if (range>9999)
      {
        range=0;
        no=0; //нулевая бирка 000000 запрещена IATA
      };

      if (last_range==ASTRA::NoExists)
        throw EXCEPTIONS::Exception("%s: last_range==ASTRA::NoExists! (aircode=%d)", __FUNCTION__, aircode);
      if (range==last_range)
        throw EXCEPTIONS::Exception("%s: free range not found (aircode=%d)", __FUNCTION__, aircode);

      DB::TQuery Qry(PgOra::getROSession("TAG_RANGES2"), STDLOG);
      Qry.SQLText="SELECT range FROM tag_ranges2 WHERE aircode=:aircode AND range=:range";
      Qry.CreateVariable("aircode",otInteger,aircode);
      Qry.CreateVariable("range",otInteger,range);
      Qry.Execute();
      if (!Qry.Eof) continue; //этот диапазон используется
    };

    double first_no=aircode*1000000+range*100+no+1; //первая неиспользовання бирка от старого диапазона

    if (tag_count>=99-no)
    {
      tag_count-=99-no;
      no=99;
    }
    else
    {
      no+=tag_count;
      tag_count=0;
    };

    double last_no=aircode*1000000+range*100+no; //последняя использовання бирка нового диапазона
    add(TBagTagRange("", first_no, last_no, 10));

    DB::TQuery Qry(PgOra::getRWSession("TAG_RANGES2"), STDLOG);
    Qry.CreateVariable("aircode", otInteger, aircode);
    Qry.CreateVariable("range", otInteger, range);
    if (no >= 99) {
        Qry.SQLText =
         "DELETE FROM tag_ranges2 "
         "WHERE aircode = :aircode "
         "AND range = :range";
    } else {
        Qry.SQLText = PgOra::supportsPg("TAG_RANGES2")
         ? "INSERT INTO tag_ranges2( aircode, range, no, airp_dep, airp_arv, class, point_id, last_access) "
                            "VALUES(:aircode,:range,:no,:airp_dep,:airp_arv,:class,:point_id,:last_access) "
           "ON CONFLICT(aircode, range) "
           "DO UPDATE SET no = :no, airp_dep = :airp_dep, airp_arv = :airp_arv, "
                         "class = :class, point_id = :point_id, last_access = :last_access"
         : "BEGIN "
             "UPDATE tag_ranges2 "
             "SET no=:no, airp_dep=:airp_dep, airp_arv=:airp_arv, class=:class, point_id=:point_id, "
                 "last_access=:last_access "
             "WHERE aircode=:aircode AND range=:range; "
             "IF SQL%NOTFOUND THEN "
               "INSERT INTO tag_ranges2( aircode, range, no, airp_dep, airp_arv, class, point_id, last_access) "
                                "VALUES(:aircode,:range,:no,:airp_dep,:airp_arv,:class,:point_id,:last_access); "
             "END IF; "
           "END;";
        Qry.CreateVariable("no", otInteger, no);
        Qry.CreateVariable("airp_dep", otString, airp_dep);
        Qry.CreateVariable("airp_arv", otString, airp_arv);
        Qry.CreateVariable("class", otString, cl);
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.CreateVariable("last_access", otDate, BASIC::date_time::NowUTC());
    };
    Qry.Execute();
  };
  if (use_new_range)
  {
    DB::TQuery Qry(PgOra::getRWSession("LAST_TAG_RANGES2"), STDLOG);
    Qry.SQLText = PgOra::supportsPg("LAST_TAG_RANGES2")
         ? "INSERT INTO last_tag_ranges2( aircode, range) "
                                 "VALUES(:aircode,:range) "
           "ON CONFLICT(aircode) "
           "DO UPDATE SET range = :range"
         : "BEGIN "
             "UPDATE last_tag_ranges2 SET range=:range WHERE aircode=:aircode; "
             "IF SQL%NOTFOUND THEN "
               "INSERT INTO last_tag_ranges2(aircode,range) VALUES(:aircode,:range); "
             "END IF; "
           "END;";
    Qry.CreateVariable("aircode",otInteger,aircode);
    Qry.CreateVariable("range",otInteger,range);
    Qry.Execute();
  };
}

void TGeneratedTags::trace(const std::string& where) const
{
  LogTrace(TRACE5) << where << ": TGeneratedTags dump" << (_tags.empty()?": EMPTY!":"");
  for(const TBagTagNumber& tag : _tags)
    LogTrace(TRACE5) << tag.str();
}

void GetTagsByBagNum(int grp_id, int bag_num, std::multiset<TBagTagNumber> &tags, bool includeTagColor)
{
    tags.clear();
    DB::TCachedQuery Qry(PgOra::getROSession("BAG_TAGS"),
            "select color, no  from bag_tags where grp_id = :grp_id and bag_num = :bag_num",
            QParams() << QParam("grp_id", otInteger, grp_id) << QParam("bag_num", otInteger, bag_num),
            STDLOG);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
    {
        tags.insert(
                TBagTagNumber(
                    includeTagColor ? ElemIdToCodeNative(etTagColor, Qry.get().FieldAsString("color")) : "",
                    Qry.get().FieldAsFloat("no")));
    }
}

void GetTagsByPaxId(int pax_id, std::multiset<TBagTagNumber> &tags)
{
    tags.clear();
    CheckIn::TSimplePaxItem pax;
    pax.getByPaxId(pax_id);
    if(pax.bag_pool_num != ASTRA::NoExists)
        GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags, true);
}

void BagTagContainerFromDB(std::multiset<TBagTagNumber>& container, DB::TQuery& Qry, bool includeTagColor)
{
  container.clear();
  for(; not Qry.Eof; Qry.Next())
    container.emplace(includeTagColor ? ElemIdToCodeNative(etTagColor, Qry.FieldAsString("color")) : "", Qry.FieldAsFloat("no"));
}

void FlattenBagTags(const std::multiset<TBagTagNumber> &tags, std::set<std::string> &result)
{
  result.clear();
  for (auto tag : tags) result.insert(tag.str());
}

template <typename TBagItem>
void BagTagContainerFromDB(std::multimap<TBagTagNumber, TBagItem>& container, DB::TQuery& Qry, bool includeTagColor)
{
  container.clear();
  for(; not Qry.Eof; Qry.Next())
  {
    TBagTagNumber tag_number(includeTagColor ? ElemIdToCodeNative(etTagColor, Qry.FieldAsString("color")) : "", Qry.FieldAsFloat("no"));
    TBagItem bag_item;
    bag_item.fromDB(Qry);
    container.emplace(tag_number, bag_item);
  }
}

template <typename T>
void GetTagsByPool(int grp_id, int bag_pool_num , T& container, bool includeTagColor)
{
    if (bag_pool_num == ASTRA::NoExists) return;
    DB::TCachedQuery Qry(PgOra::getROSession({"BAG2","BAG_TAGS"}),
            "select "
            "    bag_tags.color, "
            "    bag_tags.no, "
            "    bag2.* "
            "from "
            "    bag2, "
            "    bag_tags "
            "where "
            "    bag2.grp_id = :grp_id and "
            "    bag2.bag_pool_num = :bag_pool_num and "
            "    bag2.grp_id = bag_tags.grp_id and "
            "    bag2.num = bag_tags.bag_num ",
            QParams() << QParam("grp_id", otInteger, grp_id) << QParam("bag_pool_num", otInteger, bag_pool_num),
            STDLOG);
    Qry.get().Execute();
    container.clear();
    BagTagContainerFromDB(container, Qry.get(), includeTagColor);
}

// zamar_dsm
template void GetTagsByPool<std::multimap<TBagTagNumber, CheckIn::TBagItem, std::less<TBagTagNumber>, 
                            std::allocator<std::pair<TBagTagNumber const, CheckIn::TBagItem> > > >
                            (int grp_id, int bag_pool_num , std::multimap<TBagTagNumber, CheckIn::TBagItem, std::less<TBagTagNumber>, 
                             std::allocator<std::pair<TBagTagNumber const, CheckIn::TBagItem> > >& container, bool includeTagColor);

