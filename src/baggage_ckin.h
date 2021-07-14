#ifndef BAGGAGE_CKIN_H
#define BAGGAGE_CKIN_H

#include "date_time.h"
#include "oralib.h"
#include "astra_dates.h"
#include "astra_types.h"
#include "astra_consts.h"
#include "hooked_session.h"
#include <optional>
#include "astra_types.h"
#include "baggage_tags.h"

DECL_HASH(GrpId_t, int)

namespace CKIN {

using BagNum_t = int;

struct BagInfo
{
    BagNum_t num;
    int bagAmount = 0;
    int bagWeight = 0;
    int rkAmount = 0;
    int rkWeight = 0;
};

struct TagInfo
{
    std::string tag_type;
    size_t no_len=0;
    std::string color;
    long long no=0;
    long long first=0;
    long long last=0;
};


using Dates::DateTime_t;
using Dates::not_a_date_time;


enum class READ : uint32_t
{
    BAGS,
    BAGS_AND_TAGS
};

class MainPax
{
public:
    MainPax() : is_unnacomp(false) {}
    MainPax(bool unnacomp) : is_unnacomp(unnacomp) {}
    int excessWt(GrpId_t grp_id, PaxId_t pax_id, int excess_wt_raw) const;
    int excessWtUnnacomp(GrpId_t grp_id, int excess_wt_raw, bool bag_refuse);
    bool isMainPax(GrpId_t grp_id, PaxId_t pax_id) const;
    bool isUnnacomp() const {return is_unnacomp;}
    void saveMainPax(GrpId_t grp_id, PaxId_t pax_id, bool bag_refuse);
private:
    struct PaxInfo
    {
        PaxId_t pax_id;
        bool bag_refuse;
    };
    std::map<GrpId_t, PaxInfo> first_paxes;
    bool is_unnacomp = false;
    std::set<GrpId_t> groups;
};


class BagReader
{
public:
    BagReader() = default;
    BagReader(PointId_t point_dep, std::optional<DateTime_t> part_key, READ var);
    BagReader(GrpId_t grp_id, std::optional<DateTime_t> part_key, READ var);


    int bagAmount(GrpId_t grp_id, std::optional<int> bag_pool_num) const;
    int bagWeight(GrpId_t grp_id, std::optional<int> bag_pool_num) const;
    int rkAmount(GrpId_t grp_id,  std::optional<int> bag_pool_num) const;
    int rkWeight(GrpId_t grp_id,  std::optional<int> bag_pool_num) const;

    int bagAmountUnaccomp(GrpId_t grp_id) const{return bagAmount(grp_id, 1);}
    int bagWeightUnaccomp(GrpId_t grp_id) const{return bagWeight(grp_id, 1);}
    int rkAmountUnaccomp(GrpId_t grp_id) const{return rkAmount(grp_id, 1);}
    int rkWeightUnaccomp(GrpId_t grp_id) const{return rkWeight(grp_id, 1);}

    std::string tags(GrpId_t grp_id, std::optional<int> bag_pool_num, const std::string& lang) const;
    std::string tagsUnaccomp(GrpId_t grp_id, const std::string& lang) const{return tags(grp_id, 1, lang);}

    struct BagKey
    {
        GrpId_t grp_id;
        int bag_pool_num;
        bool operator ==(const BagKey &other) const
        {
            return (grp_id == other.grp_id && bag_pool_num == other.bag_pool_num);
        }
        bool operator <(const BagKey &other) const
        {
            return std::tie(grp_id, bag_pool_num) < std::tie(other.grp_id, other.bag_pool_num);
        }
    };
    struct KeyHash
    {
        std::size_t operator() (const BagKey & key) const
        {
            return (17*31 + std::hash<int>()(key.grp_id.get())) * 31 + std::hash<int>()(key.bag_pool_num);
        }
    };

private:

    void readBags(PointId_t point_dep, std::optional<DateTime_t> part_key);
    void readBags(GrpId_t grp_id, std::optional<DateTime_t> part_key);
    void readTags(PointId_t point_dep, std::optional<DateTime_t> part_key);
    void readTags(GrpId_t grp_id, std::optional<DateTime_t> part_key);

    std::optional<std::reference_wrapper<const std::vector<BagInfo> > > bagInfo(GrpId_t grp_id, std::optional<int> bag_pool_num) const;

    std::unordered_map<BagKey, std::vector<BagInfo>, KeyHash> grp_bags;
    std::unordered_map<GrpId_t, std::map<BagNum_t, std::vector<TagInfo>>> grp_tags;
};

//std::optional<int> get_bagAmount2(GrpId_t grp_id, std::optional<int> pax_id,
//                                  std::optional<int> bag_pool_num, Dates::DateTime_t part_key);
//std::optional<int> get_bagWeight2(GrpId_t grp_id, std::optional<int> pax_id,
//                                  std::optional<int> bag_pool_num, Dates::DateTime_t part_key);
//std::optional<int> get_rkAmount2(GrpId_t grp_id, std::optional<int> pax_id,
//                                 std::optional<int> bag_pool_num, Dates::DateTime_t part_key);
//std::optional<int> get_rkWeight2(GrpId_t grp_id, std::optional<int> pax_id,
//                                 std::optional<int> bag_pool_num, Dates::DateTime_t part_key);

//std::optional<BagInfo> get_bagInfo2(GrpId_t grp_id, std::optional<PaxId_t> pax_id, std::optional<int> bag_pool_num,
//                      std::optional<DateTime_t> part_key);

std::optional<PaxId_t> get_bag_pool_pax_id(GrpId_t grp_id, std::optional<int> bag_pool_num,
                                       std::optional<DateTime_t> part_key, int include_refused = 1);

//std::optional<int> get_main_pax_id2(GrpId_t grp_id, int include_refused, std::optional<DateTime_t> part_key);

int get_bag_pool_refused(GrpId_t grp_id, int bag_pool_num, std::optional<std::string> vclass, int bag_refuse,
                         std::optional<DateTime_t> part_key);

std::optional<std::string> get_birks2(GrpId_t grp_id, std::optional<int> pax_id, int bag_pool_num,
                                      std::optional<DateTime_t> part_key, const std::string &lang);

//std::optional<std::string> get_birks2(GrpId_t grp_id, std::optional<int> pax_id, int bag_pool_num,
//                                     int pr_lat, std::optional<DateTime_t> part_key );

std::optional<int> get_excess_wt(GrpId_t grp_id, std::optional<int> pax_id, std::optional<int> excess_wt,
                                 std::optional<int> excess_nvl, int bag_refuse,
                                 std::optional<DateTime_t> part_key);

std::optional<std::string> next_airp(int first_point, int point_num, std::optional<DateTime_t> part_key);

}

#endif // BAGGAGE_CKIN_H
