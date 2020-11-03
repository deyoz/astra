#ifndef EXTERNAL_TESTS_H
#define EXTERNAL_TESTS_H

#include <string>

namespace telegrams {
namespace external_tests {

/*** TEST FRAMEWORK UTILS ***/
class ExternalDb
{
public:
    virtual ~ExternalDb() {}
    // prepares Common router 
    // returns router id
    virtual int prepareCommonRouter() = 0;
    // prepares TrueTypeB router 
    // returns router id
    virtual int prepareTrueTypeBRouter() = 0;
    // prepares TypeB router 
    // returns router id
    virtual int prepareTypeBRouter() = 0;
    // prepares Hth router 
    // returns router id
    virtual int prepareHthRouter() = 0;
    // sets router max part sizes
    virtual void setRouterMaxPartSizes(int rtr, size_t partSz) = 0;
    // inserts tlg with given number
    virtual void insertTlgWithNum(const tlgnum_t& tlgNum) = 0;
    // returns queue number for different tlg
    virtual int getQueueNumber(bool isEdifact) = 0;
    // return number of records in AIR_Q_PART
    virtual int countParts();
};

void setExternalDb(ExternalDb* db);

/*** TESTS  ***/
void check_split_join_common();
void check_split_join_hth();
void check_split_join_typeb();
void check_split_join_true_typeb();

void check_join_pfs();
void check_split_bad_router_sizes();
void check_split_typeb_from_file(const std::string& filename);
void check_split_join_common_from_file(const std::string& filename, bool mustSplit = true);
void check_split_join_too_small_part_sizes(const std::string& filename);

void check_save_bad_tlg();

void check_hth_join_asc_order();
void check_hth_join_desc_order();
void check_hth_join_rand_order();
void check_split_pnl();
void check_split_adl();
void check_split_adl_file(const std::string& fullAdlFile, const std::string& splitAdlFile);

} // namespace external_tests
} // namespace telegrams

#endif /* EXTERNAL_TESTS_H */

