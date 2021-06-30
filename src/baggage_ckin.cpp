#include "baggage_ckin.h"
#include "dbo.h"
#include "unordered_map"
#include "serverlib/algo.h"
#include "serverlib/str_utils.h"
#include "astra_elems.h"
#include <functional>
#include "astra_utils.h"
#include "serverlib/algo.h"

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

namespace CKIN {


std::set<TagInfo> read_birks(GrpId_t grp_id, std::optional<DateTime_t> part_key);
std::set<TagInfo> read_birks(GrpId_t grp_id, int bag_pool_num, std::optional<DateTime_t> part_key);

bool operator <(const TagInfo & ls, const TagInfo & rs)
{
    return std::tie(ls.tag_type, ls.color, ls.no) < std::tie(rs.tag_type, rs.color, rs.no);
}

template<typename Container, typename T = typename Container::value_type>
std::string build_birks_str(const Container & tags, const std::string& lang)
{
    if(tags.empty()) {
        LogTrace(TRACE6) << __func__ << " tags empty";
        return "";
    }
    LogTrace(TRACE6) << __func__;
    std::string res; //VARCHAR2(4000)
    std::string noStr; //VARCHAR2(15);
    std::string firstStr; //VARCHAR2(17)
    std::string lastStr; //VARCHAR2(3)
    int diff; //BINARY_INTEGER;

    TagInfo curRow;
    TagInfo oldRow;
    TagInfo oldRow2;

    for(const auto & row : tags) {
        curRow = row;
        diff = 1;
        oldRow = curRow;
        oldRow2 = {};
        if(oldRow.tag_type == curRow.tag_type
          && ((!oldRow.color.empty() && !curRow.color.empty() && oldRow.color==curRow.color) || (oldRow.color.empty() && curRow.color.empty()))
          && oldRow.first==curRow.first
          && oldRow.last+diff==curRow.last)
        {
            diff += 1;
        } else {
            if(oldRow2.tag_type==oldRow.tag_type
               && ((!oldRow2.color.empty() && !oldRow.color.empty() && oldRow2.color==oldRow.color) || (oldRow2.color.empty() && oldRow.color.empty()))
               && oldRow2.first==oldRow.first )
            {
                firstStr = StrUtils::lpad(std::to_string(oldRow.last),3,'0');
                if(!res.empty()) {
                    res+=",";
                }
            } else {
                firstStr = ElemIdToPrefferedElem(etTagColor, oldRow.color, efmtCodeNative, lang);
                noStr = std::to_string(oldRow.first*1000+oldRow.last);
                if (noStr.length() < oldRow.no_len) {
                    firstStr += StrUtils::lpad(noStr,oldRow.no_len,'0');
                } else {
                    firstStr += noStr;
                }
                oldRow2 = oldRow;
                if(!res.empty()) {
                    res += ", ";
                }
            }

            if(diff != 1) {
                lastStr = StrUtils::lpad(std::to_string(oldRow.last+diff-1),3,'0');
                res = res + firstStr + '-' + lastStr;
            } else {
                res += firstStr;
            }
            diff = 1;
            oldRow = curRow;
        }
    }
    LogTrace(TRACE5) << "return res: " << res;
    return res;
}

std::optional<int> get_bag_pool_pax_id(GrpId_t grp_id, std::optional<int> bag_pool_num,
                                    std::optional<DateTime_t> part_key, int include_refused)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
              << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num.value_or(0)
              << " include_refused: "  << include_refused;
    if(!bag_pool_num) {
        return std::nullopt;
    }
    std::optional<int> res;
    int pax_id;
    std::string refuse;
    std::string table = part_key? "ARX_PAX" :"PAX" ;
    auto cur = make_db_curs(
                "SELECT pax_id, refuse "
                "FROM " + table +
                " WHERE GRP_ID=:grp_id and BAG_POOL_NUM=:bag_pool_num " +
                    (part_key ? " and PART_KEY=:part_key " : "") +
                " ORDER BY  CASE WHEN pers_type='ВЗ' THEN 0 WHEN pers_type='РБ' THEN 0 ELSE 1 END, "
                "          CASE WHEN seats=0 THEN 1 ELSE 0 END, "
                "          CASE WHEN refuse is null THEN 0 ELSE 1 END, "
                "          CASE WHEN pers_type='ВЗ' THEN 0 WHEN pers_type='РБ' THEN 1 ELSE 2 END, "
                "          reg_no ",
                PgOra::getROSession(table));
    cur.stb()
       .def(pax_id)
       .defNull(refuse, "")
       .bind(":grp_id", grp_id.get())
       .bind(":bag_pool_num", bag_pool_num.value_or(0));
       if(part_key) { cur.bind(":part_key", *part_key);};
       cur.exec();
    if(!cur.fen()) {
        res = pax_id;
        if(include_refused == 0 && !refuse.empty()) {
            res = std::nullopt;
        }
    }
    return res;
}

std::optional<BagInfo> get_bagInfo2( GrpId_t grp_id, std::optional<int> pax_id, std::optional<int> bag_pool_num,
                       std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time) << " grp_id: " << grp_id
              << " bag_pool_num: " << bag_pool_num.value_or(0) << " pax_id: " << pax_id.value_or(0);
    BagInfo bagInfo{};

    std::optional<int> pool_pax_id = pax_id;
    if(pax_id) {
        if(!bag_pool_num) {
            return bagInfo;
        }
        pool_pax_id = get_bag_pool_pax_id(grp_id, bag_pool_num, part_key, 1);
    }

    std::string table = part_key? "ARX_BAG2" : "BAG2";

    if(!pax_id || (pool_pax_id && pool_pax_id == pax_id)) {
        std::string query = "SELECT "
                            "SUM(CASE WHEN pr_cabin=0 THEN amount ELSE NULL END) AS bagAmount, "
                            "SUM(CASE WHEN pr_cabin=0 THEN weight ELSE NULL END) AS bagWeight, "
                            "SUM(CASE WHEN pr_cabin=0 THEN NULL ELSE amount END) AS rkAmount, "
                            "SUM(CASE WHEN pr_cabin=0 THEN NULL ELSE weight END) AS rkWeight "
                            "FROM " + table +
                            " WHERE GRP_ID=:grp_id " + (part_key ? "and PART_KEY=:part_key " : "") +
                            (pax_id ? " and BAG_POOL_NUM=:bag_pool_num " : "");
        auto cur = make_db_curs(query, PgOra::getROSession(table));
        cur.stb()
           .defNull(bagInfo.bagAmount,0)
           .defNull(bagInfo.bagWeight,0)
           .defNull(bagInfo.rkAmount,0)
           .defNull(bagInfo.rkWeight,0)
           .bind(":grp_id", grp_id.get());
        if(part_key) { cur.bind(":part_key", *part_key);};
        if(pax_id) {
            cur.bind(":bag_pool_num", bag_pool_num.value_or(0));
        }
        cur.EXfet();
        if(cur.err() == DbCpp::ResultCode::NoDataFound) {
            LogTrace(TRACE6) << __func__ << " Query error. Not found data by grp_id: " << grp_id
                             << " part_key: " << part_key.value_or(not_a_date_time) ;
            return bagInfo;
        }
    }
    return bagInfo;
}

std::optional<int> get_bagAmount2(GrpId_t grp_id, std::optional<int> pax_id,
                                  std::optional<int> bag_pool_num, Dates::DateTime_t part_key)
{
    std::optional<BagInfo> bagInfo = get_bagInfo2(grp_id, pax_id, bag_pool_num, part_key);
    if(!bagInfo) return std::nullopt;
    return bagInfo->bagAmount;
}

std::optional<int> get_bagWeight2(GrpId_t grp_id, std::optional<int> pax_id,
                                  std::optional<int> bag_pool_num, Dates::DateTime_t part_key)
{
    std::optional<BagInfo> bagInfo = get_bagInfo2(grp_id, pax_id, bag_pool_num, part_key);
    if(!bagInfo) return std::nullopt;
    return bagInfo->bagWeight;
}

std::optional<int> get_rkAmount2(GrpId_t grp_id, std::optional<int> pax_id,
                                 std::optional<int> bag_pool_num, Dates::DateTime_t part_key)
{
    std::optional<BagInfo> bagInfo = get_bagInfo2(grp_id, pax_id, bag_pool_num, part_key);
    if(!bagInfo) return std::nullopt;
    return bagInfo->rkAmount;
}

std::optional<int> get_rkWeight2(GrpId_t grp_id, std::optional<int> pax_id,
                                 std::optional<int> bag_pool_num, Dates::DateTime_t part_key)
{
    std::optional<BagInfo> bagInfo = get_bagInfo2(grp_id, pax_id, bag_pool_num, part_key);
    if(!bagInfo) return std::nullopt;
    return bagInfo->rkWeight;
}

std::optional<int> get_excess_wt(GrpId_t grp_id, std::optional<int> pax_id,
                                 std::optional<int> excess_wt, std::optional<int> excess_nvl,
                                 int bag_refuse, std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
              << " grp_id: " << grp_id << " bag_refuse: " << bag_refuse
              << " pax_id: " << (pax_id.value_or(0)) << " excess_wt: " << excess_wt.value_or(0)
              << " excess_nvl: " << excess_nvl.value_or(0);

    int excess = 0 ;
    std::optional<int> main_pax_id;
    std::string table = part_key? "ARX_PAX_GRP" : "PAX_GRP";
    if((!excess_wt && !excess_nvl) || !bag_refuse) {
        auto cur = make_db_curs("SELECT (CASE WHEN BAG_REFUSE=0 THEN COALESCE(EXCESS_WT, EXCESS) ELSE 0 END) "
                                "FROM " + table +
                                " WHERE GRP_ID=:grp_id " + (part_key ? " and PART_KEY=:part_key" : ""),
                                PgOra::getROSession(table));
        cur.def(excess)
           .bind(":grp_id", grp_id.get());
        if(part_key) { cur.bind(":part_key", *part_key); }
        cur.EXfet();
        if(cur.err() == DbCpp::ResultCode::NoDataFound) {
            LogTrace(TRACE6) << __FUNCTION__ << " Query error. Not found data by grp_id: " << grp_id
                      << " part_key: " << part_key.value_or(not_a_date_time) ;
            return std::nullopt;
        }
    } else {
        if(bag_refuse == 0) {
            if(excess_wt) return excess_wt;
            if(excess_nvl) return excess_nvl;
            return std::nullopt;
        } else excess=0;
    }

    if(pax_id) {
        main_pax_id = get_main_pax_id2(grp_id, 1, part_key);
    }
    if(!(!pax_id || (main_pax_id && main_pax_id==pax_id))) {
        return std::nullopt;
    }
    if(excess == 0) return std::nullopt;
    return excess;
}


std::optional<int> get_main_pax_id2(GrpId_t grp_id, int include_refused, std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
                     << " grp_id: " << grp_id << " include_refused: " << include_refused;
    std::optional<int> res;
    int pax_id;
    std::string refuse;
    std::string table = part_key? "ARX_PAX" : "PAX";
    auto cur = make_db_curs("select PAX_ID, REFUSE "
                            "from " + table +
                            " where GRP_ID=:grp_id " + (part_key ? " and PART_KEY=:part_key " : "") +
                            " order by case when BAG_POOL_NUM is null  then 1 else 0 end, "
                            "         case when PERS_TYPE='ВЗ' then 0 when pers_type='РБ' then 0 else 1 end, "
                            "         case when SEATS=0 THEN 1 else 0 end, "
                            "         case when REFUSE is null then 0 else 1 end, "
                            "         case when PERS_TYPE='ВЗ' then 0 when PERS_TYPE='РБ' then 1 else 2 end, "
                            "         reg_no",
                            PgOra::getROSession(table));
    cur.def(pax_id)
       .defNull(refuse, "")
       .bind(":grp_id", grp_id.get());
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.exec();
    if(!cur.fen()) {
        res = pax_id;
        if(include_refused == 0 && !refuse.empty()) {
            res = std::nullopt;
        }
    }
    return res;
}

int get_bag_pool_refused(GrpId_t grp_id, int bag_pool_num, std::optional<std::string> vclass, int bag_refuse,
                     std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time) << " grp_id: " << grp_id
              << " bag_pool_num: " << bag_pool_num << " bag_refuse: " << bag_refuse;
    if(bag_refuse != 0) return 1;
    if(!vclass) return 0;
    int n = 0;
    std::string table = part_key? "ARX_PAX" : "PAX";
    auto cur = make_db_curs("select sum(case when REFUSE is null then 1 else 0 end) "
                            " from " + table +
                            " where GRP_ID=:grp_id and BAG_POOL_NUM=:bag_pool_num " +
                              (part_key ? " and PART_KEY=:part_key" : ""),
                            PgOra::getROSession(table));
    cur.def(n)
       .bind(":grp_id", grp_id.get())
       .bind(":bag_pool_num", bag_pool_num);
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound || n==0) {
        return 1;
    } else
        return 0;
}

size_t readTagNo(const std::string& tag_type)
{
    size_t no_len;
    auto cur = make_db_curs( "select NO_LEN from TAG_TYPES where TAG_TYPES.CODE = :tag_type  ",
                             PgOra::getROSession("TAG_TYPES"));
    cur.def(no_len)
       .bind(":tag_type", tag_type)
       .EXfet();
    return no_len;
}

std::set<TagInfo> read_birks(GrpId_t grp_id, std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time) << " grp_id: " << grp_id;
    std::set<TagInfo> birks;
    TagInfo row;

    std::string table = part_key? "ARX_BAG_TAGS" : "BAG_TAGS";
    auto cur = make_db_curs("select TAG_TYPE, COLOR, TRUNC(NO/1000) AS first, MOD(NO,1000) AS last, no "
                            "from " + table +
                           " where grp_id=:grp_id " +
                            (part_key ? " and PART_KEY=:part_key " : ""),
                            PgOra::getROSession(table));
    cur.def(row.tag_type)
       .defNull(row.color, "")
       .def(row.first)
       .def(row.last)
       .def(row.no)
       .bind(":grp_id", grp_id.get());
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.exec();

    std::unordered_map<std::string, size_t> tagNoLens; // tag_type - tag_type.no_len
    while(!cur.fen()) {
        if(!algo::contains(tagNoLens, row.tag_type)) {
            tagNoLens[row.tag_type] = readTagNo(row.tag_type);
        }
        row.no_len = tagNoLens[row.tag_type];
        birks.insert(row);
    }
    return birks;
}

std::set<TagInfo> read_birks(GrpId_t grp_id, int bag_pool_num, std::optional<DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
                     << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num;
    std::set<TagInfo> birks;
    TagInfo row;

    std::string query = part_key
            ? " from ARX_BAG2, ARX_BAG_TAGS "
              "where ARX_BAG2.GRP_ID=ARX_BAG_TAGS.GRP_ID and "
              "      ARX_BAG2.NUM=ARX_BAG_TAGS.BAG_NUM and "
              "      ARX_BAG2.GRP_ID=:grp_id and "
              "      ARX_BAG2.BAG_POOL_NUM=:bag_pool_num and "
              "      ARX_BAG2.PART_KEY=ARX_BAG_TAGS.PART_KEY and "
              "      ARX_BAG2.PART_KEY=:part_key"
            :
                " from BAG2, BAG_TAGS "
                " where BAG2.GRP_ID = BAG_TAGS.GRP_ID and "
                "       BAG2.NUM = BAG_TAGS.BAG_NUM and "
                "       BAG2.GRP_ID = :grp_id and "
                "       BAG2.BAG_POOL_NUM=:bag_pool_num";


    auto cur = make_db_curs("select TAG_TYPE, COLOR, TRUNC(NO/1000) AS first, MOD(NO,1000) AS last, no " + query,
                            PgOra::getROSession(part_key? "ARX_BAG2" : "BAG2"));
    cur.def(row.tag_type)
       .defNull(row.color, "")
       .def(row.first)
       .def(row.last)
       .def(row.no)
       .bind(":grp_id", grp_id.get())
       .bind(":bag_pool_num", bag_pool_num);
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.exec();

    std::unordered_map<std::string, size_t> tagTypeNoLens; // tag_type - tag_type.no_len
    while(!cur.fen()) {
        if(!algo::contains(tagTypeNoLens, row.tag_type)) {
            tagTypeNoLens[row.tag_type] = readTagNo(row.tag_type);
        }
        row.no_len = tagTypeNoLens[row.tag_type];
        birks.insert(row);
    }
    return birks;
}

std::optional<std::string> get_birks2(GrpId_t grp_id, std::optional<int> pax_id, int bag_pool_num,
                                      std::optional<DateTime_t> part_key, const std::string& lang)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
                     << " grp_id: "<<grp_id << " bag_pool_num: " << bag_pool_num;
    std::optional<int> pool_pax_id;
    if(pax_id) {
        if(!bag_pool_num) return std::nullopt;
        pool_pax_id = get_bag_pool_pax_id(grp_id, bag_pool_num, part_key, 1);
    }

    std::set<TagInfo> birks;
    if(!pax_id || (pool_pax_id && pool_pax_id==pax_id)) {
        if(!pax_id) {
            birks = read_birks(grp_id, part_key);
        }else {
            if(bag_pool_num==1) {
                /*для тех групп которые регистрировались с терминала без обязательной привязки */
                std::set<TagInfo> birks1 = read_birks(grp_id, bag_pool_num, part_key);
                std::set<TagInfo> birks2 = read_birks(grp_id, part_key);
                //вместо SQL UNION
                std::merge(birks1.begin(), birks1.end(), birks2.begin(), birks2.end(),
                           std::inserter(birks, birks.begin()));
            } else {
                birks = read_birks(grp_id, bag_pool_num, part_key);
            }
        }
    }
    LogTrace5 << " tags_range size: " << birks.size();
    for(const TagInfo& tag : birks) {
        LogTrace5 << " tag_no: " << tag.no << " tag_type: " <<tag.tag_type << "tag_no_len: " << tag.no_len;
    }
    return build_birks_str(birks,lang);
}

std::optional<std::string> next_airp(int first_point, int point_num, std::optional<Dates::DateTime_t> part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key.value_or(not_a_date_time)
                     << " first_point: " << first_point << " point_num: " << point_num;
    dbo::Session session;
    std::optional<std::string> airp;
    if(!part_key) {
        airp = session.query<std::string>("SELECT airp")
                .from("POINTS")
                .where("first_point=:first_point AND point_num > :point_num AND pr_del=0 ORDER BY point_num")
                .setBind({{":first_point", first_point}, {":point_num", point_num}});
    } else {
        airp = session.query<std::string>("SELECT airp")
                .from("ARX_POINTS")
                .where("part_key = :part_key AND first_point=:first_point AND point_num > :point_num AND pr_del=0 ORDER BY point_num")
                .setBind({{":part_key", *part_key}, {":first_point", first_point}, {":point_num", point_num}});
    }

    return airp;
}

void BagReader::readBags(PointId_t point_dep, std::optional<Dates::DateTime_t> part_key)
{
    LogTrace5 << __func__ << " point_dep: " << point_dep << " part_key: " << part_key.value_or(not_a_date_time);
    int grp_id;
    int bag_pool_num;
    int num;
    BagInfo bagInfo;
    std::string table = part_key ? "ARX_BAG2" : "BAG2";
    auto cur = make_db_curs("select GRP_ID, BAG_POOL_NUM, NUM, "
                            "(CASE WHEN pr_cabin=0 THEN amount ELSE NULL END) AS bagAmount, "
                            "(CASE WHEN pr_cabin=0 THEN weight ELSE NULL END) AS bagWeight, "
                            "(CASE WHEN pr_cabin=0 THEN NULL ELSE amount END) AS rkAmount, "
                            "(CASE WHEN pr_cabin=0 THEN NULL ELSE weight END) AS rkWeight "
                            "from " + table +
                            " where POINT_DEP = :point_dep " + (part_key ? " and PART_KEY = :part_key" : ""),
                            PgOra::getROSession(table));
    cur.def(grp_id)
       .def(bag_pool_num)
       .def(num)
       .defNull(bagInfo.bagAmount, 0)
       .defNull(bagInfo.bagWeight, 0)
       .defNull(bagInfo.rkAmount, 0)
       .defNull(bagInfo.rkWeight, 0)
       .bind(":point_dep", point_dep.get());
    if(part_key) {
        cur.bind("part_key", *part_key);
    }
    cur.exec();
    while(!cur.fen())
    {
        bagInfo.num = BagNum_t(num);
        grp_bags[BagKey{GrpId_t(grp_id), bag_pool_num}].push_back(bagInfo);
    }

//    LogTrace5 << " TEST: ";
//    for(const auto & [key, val] : grp_bags) {
//        LogTrace5 << "key: grp_id: " << key.grp_id << " key_bag_pool_num: " << key.bag_pool_num;
//        for(const BagInfo & bag : val) {
//            LogTrace5 << " num: " << bag.num << " am: " << bag.bagAmount << " wg: " << bag.bagWeight
//                      << " rkAm: " << bag.rkAmount << " rkWg: " << bag.rkWeight;
//        }
//    }
}

void BagReader::readBags(GrpId_t grp_id, std::optional<Dates::DateTime_t> part_key)
{
    BagInfo bagInfo;
    int bag_pool_num;
    int num;
    std::string table = part_key ? "ARX_BAG2" : "BAG2";
    auto cur = make_db_curs("select BAG_POOL_NUM, NUM, "
                            "(CASE WHEN pr_cabin=0 THEN amount ELSE NULL END) AS bagAmount, "
                            "(CASE WHEN pr_cabin=0 THEN weight ELSE NULL END) AS bagWeight, "
                            "(CASE WHEN pr_cabin=0 THEN NULL ELSE amount END) AS rkAmount, "
                            "(CASE WHEN pr_cabin=0 THEN NULL ELSE weight END) AS rkWeight "
                            "from " + table +
                            " where GRP_ID = :grp_id " + (part_key ? " and PART_KEY = :part_key" : ""),
                            PgOra::getROSession(table));
    cur.def(bag_pool_num)
       .def(num)
       .defNull(bagInfo.bagAmount, 0)
       .defNull(bagInfo.bagWeight, 0)
       .defNull(bagInfo.rkAmount, 0)
       .defNull(bagInfo.rkWeight, 0)
       .bind(":grp_id", grp_id.get());
    if(part_key) {
        cur.bind("part_key", *part_key);
    }
    cur.exec();
    while(!cur.fen())
    {
        bagInfo.num = BagNum_t(num);
        grp_bags[BagKey{grp_id, bag_pool_num}].push_back(bagInfo);
    }
}

void BagReader::readTags(PointId_t point_dep, std::optional<Dates::DateTime_t> part_key)
{
    LogTrace5 << __func__ << " point_dep: " << point_dep << " part_key: " << part_key.value_or(not_a_date_time);
    TagInfo tag;
    int grp_id;
    int bag_num;

    std::string query = part_key  ? " from ARX_BAG_TAGS where POINT_DEP = :point_dep and PART_KEY=:part_key"
                                  : " from BAG_TAGS where POINT_DEP = :point_dep ";

    auto cur = make_db_curs("select GRP_ID, BAG_NUM, TAG_TYPE, COLOR, NO " + query,
                            PgOra::getROSession(part_key ? "ARX_BAG_TAGS" : "BAG_TAGS"));
    cur.def(grp_id)
       .def(bag_num)
       .def(tag.tag_type)
       .defNull(tag.color, "")
       .def(tag.no)
       .bind(":point_dep", point_dep.get());
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.exec();

    std::unordered_map<std::string, size_t> tagTypeNoLens; //хранит уже прочитанные no_len из таблицы tag_types
    while(!cur.fen()) {
        if(!algo::contains(tagTypeNoLens, tag.tag_type)) {
            tagTypeNoLens[tag.tag_type] = readTagNo(tag.tag_type);
        }
        tag.no_len = tagTypeNoLens[tag.tag_type];
        grp_tags[GrpId_t(grp_id)][BagNum_t(bag_num)].push_back(tag);
    }
}

void BagReader::readTags(GrpId_t grp_id, std::optional<Dates::DateTime_t> part_key)
{
    TagInfo tag;
    int bag_num;

    std::string query = part_key
        ? " from ARX_BAG_TAGS where GRP_ID = :grp_id and PART_KEY=:part_key "
        : " from BAG_TAGS where GRP_ID = :grp_id ";

    auto cur = make_db_curs("select BAG_NUM, TAG_TYPE, COLOR, NO " + query,
                            PgOra::getROSession(part_key ? "ARX_BAG_TAGS" : "BAG_TAGS"));

    cur.def(bag_num)
       .def(tag.tag_type)
       .defNull(tag.color, "")
       .def(tag.no)
       .bind(":grp_id", grp_id.get());
    if(part_key) {cur.bind(":part_key", *part_key);}
    cur.exec();

    std::unordered_map<std::string , size_t> tagTypeNoLens; //хранит уже прочитанные no_len из таблицы tag_types
    while(!cur.fen()) {
        if(!algo::contains(tagTypeNoLens, tag.tag_type)) {
            tagTypeNoLens[tag.tag_type] = readTagNo(tag.tag_type);
        }
        tag.no_len = tagTypeNoLens[tag.tag_type];
        grp_tags[grp_id][BagNum_t(bag_num)].push_back(tag);
    }
}

std::string BagReader::tags(GrpId_t grp_id, std::optional<int> bag_pool_num, const string &lang) const
{
    if(!bag_pool_num) {
        return "";
    }
    BagKey key{grp_id, *bag_pool_num};
    if(!algo::contains(grp_bags, key) || !algo::contains(grp_tags, GrpId_t(grp_id))) {
        return "";
    }
    const std::vector<BagInfo> & bags = grp_bags.at(key);
    const std::vector<BagNum_t> bag_nums = algo::transform(bags, [&](const BagInfo & bag){return bag.num;});
    const std::map<BagNum_t, std::vector<TagInfo>> & tags = grp_tags.at(GrpId_t(grp_id));

    multiset<TBagTagNumber> tags_range;
    for(const BagNum_t& num : bag_nums) {
        const auto& opt_tags = algo::find_opt<std::optional>(tags, num);
        if(opt_tags) {
            for(const TagInfo & tag : *opt_tags) {
                std::string color_view = ElemIdToPrefferedElem(etTagColor, tag.color, efmtCodeNative, lang); //alpha_part
                tags_range.insert(TBagTagNumber(color_view, tag.no, tag.no_len));
            }
        }
    }
    return GetTagRangesStrShort(tags_range);
}



BagReader::BagReader(PointId_t point_dep, std::optional<Dates::DateTime_t> part_key, READ var)
{
//    ASTRA::dumpTable("BAG2", TRACE5);
//    ASTRA::dumpTable("BAG_TAGS", TRACE5);
    switch (var) {
    case READ::BAGS_AND_TAGS:
        readBags(point_dep, part_key);
        readTags(point_dep, part_key);
        break;
    case READ::BAGS:
        readBags(point_dep, part_key);
        break;
    }
}

BagReader::BagReader(GrpId_t grp_id, std::optional<DateTime_t> part_key, READ var)
{
    switch (var) {
    case READ::BAGS_AND_TAGS:
        readBags(grp_id, part_key);
        readTags(grp_id, part_key);
        break;
    case READ::BAGS:
        readBags(grp_id, part_key);
        break;
    }
}

std::optional<std::reference_wrapper<const std::vector<BagInfo>>>
    BagReader::bagInfo(GrpId_t grp_id, std::optional<int> bag_pool_num) const
{
    if(!bag_pool_num) return std::nullopt;
    BagKey key{grp_id, *bag_pool_num};
    if(!algo::contains(grp_bags, key)) {
        LogTrace5 << __func__ << "grp_bags not contain key_grp_id: " << key.grp_id << " key_num: " << key.bag_pool_num;
        return std::nullopt;
    }
    return std::optional{std::ref(grp_bags.at(key))};
}

int BagReader::bagAmount(GrpId_t grp_id, std::optional<int> bag_pool_num) const
{
    LogTrace5 << __func__ << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num.value_or(-1);
    if(!bag_pool_num) return 0;
    auto opt_bags = bagInfo(grp_id, bag_pool_num);
    int total_amount = 0;
    if(opt_bags) {
        for(const auto & bag: opt_bags->get()) {total_amount += bag.bagAmount;}
    }
    return total_amount;
}

int BagReader::bagWeight(GrpId_t grp_id, std::optional<int> bag_pool_num) const
{
    LogTrace5 << __func__ << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num.value_or(-1);
    if(!bag_pool_num) return 0;
    auto opt_bags = bagInfo(grp_id, bag_pool_num);
    int total_weight = 0;
    if(opt_bags) {
        for(const auto & bag: opt_bags->get()) {total_weight += bag.bagWeight;}
    }
    return total_weight;
}

int BagReader::rkAmount(GrpId_t grp_id, std::optional<int> bag_pool_num) const
{
    LogTrace5 << __func__ << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num.value_or(-1);
    if(!bag_pool_num) return 0;
    auto opt_bags = bagInfo(grp_id, bag_pool_num);
    int total_rkAmount = 0;
    if(opt_bags) {
        for(const auto & bag: opt_bags->get()) {total_rkAmount += bag.rkAmount;}
    }
    return total_rkAmount;
}

int BagReader::rkWeight(GrpId_t grp_id, std::optional<int> bag_pool_num) const
{
    LogTrace5 << __func__ << " grp_id: " << grp_id << " bag_pool_num: " << bag_pool_num.value_or(-1);
    if(!bag_pool_num) return 0;
    auto opt_bags = bagInfo(grp_id, bag_pool_num);
    int total_rkWeight = 0;
    if(opt_bags) {
        for(const auto & bag: opt_bags->get()) {total_rkWeight += bag.rkWeight;}
    }
    return total_rkWeight;
}

int ExcessWt::excessWt(GrpId_t grp_id, PaxId_t pax_id, int excess_wt_raw, bool bag_refuse)
{
    return isMainPax(GrpId_t(grp_id), PaxId_t(pax_id), bag_refuse) ? excess_wt_raw : 0;
}

int ExcessWt::excessWtUnnacomp(GrpId_t grp_id, int excess_wt_raw, bool bag_refuse)
{
    LogTrace5 << __func__ << " grp_id: " << grp_id << " excess_wt_raw: " << excess_wt_raw
              << " bag_refuse: " << bag_refuse;
    if(!bag_refuse) {
        if(!algo::contains(groups, grp_id)) {
            groups.insert(grp_id);
            return excess_wt_raw;
        }
    }
    LogTrace5 << __func__ << " return 0";
    return 0;
}

bool ExcessWt::isMainPax(GrpId_t grp_id, PaxId_t pax_id, bool bag_refuse)
{
    if(!bag_refuse) {
        if(algo::contains(first_paxes, grp_id)) {
            if(first_paxes.at(grp_id) == pax_id) {
                return true;
            }
        }
    }
    return false;
}

void ExcessWt::saveMainPax(GrpId_t grp_id, PaxId_t pax_id)
{
    if(!algo::contains(first_paxes, GrpId_t(grp_id))) {
        first_paxes.insert(std::make_pair(GrpId_t{grp_id}, PaxId_t{pax_id}));
    }
}

}
