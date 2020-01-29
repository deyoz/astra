#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#ifdef HAVE_CONFIG_H
#endif

#ifdef XP_TESTING

#include <string.h>
#include <string>
#include <fstream>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <tclmon/tcl_utils.h>
#include <serverlib/slogger.h>
#include <serverlib/xp_test_utils.h>
#include <serverlib/helpcppheavy.h>
#include <serverlib/expected.h>

#include "hth.h"
#include "telegrams.h"
#include "tlgsplit.h"
#include "external_tests.h"
#include "gateway.h"
#include "types.h"

#include <serverlib/checkunit.h>

namespace telegrams {

size_t getSplitterPartSize();   // from tlgsplit.cc

namespace external_tests {

using namespace std;

namespace {
boost::shared_ptr<ExternalDb> p_db;

string makeLongTlg(size_t length, const string& str = "Q")
{
    string tmp = "UNB+IATA:1+ETIT+ETDT+060413:1641+ETSET34040001+SPUL23270001++S'UNH+1+TKTRES:06:1 :IA+SPUL2327/ETSET3404'MSG+:79+7'ERC+911'IFT+3+UNABLE TO PROCESS - SYSTEM ERROR+ MESSAGE FUNCTION 79 NOT SUPPORTED'UNT+5+1'UNZ+1+ETSET34040001'";
    while (tmp.length() < length)
        tmp += str;
    return tmp;
}

string createTestTypeB()
{
    string tmp =
    "MOWETP2\n.MOWETDT 050837\nETE\nMOWDT 01TF7K\nSSR TKNX 2986151019548 N\nSSR TKNX 2986151019547 O";
    tmp += makeLongTlg(RouterInfo::defaultMaxTypeBPartSize * 2, "\n0123456789");
    return tmp;
}

hth::HthInfo getHthHeader()
{
    hth::HthInfo h2h;
    memset(&h2h, 0, sizeof(h2h));
    h2h.type = 'T';
    strcpy(h2h.sender, "Sender");
    strcpy(h2h.receiver, "Recvr");
    strcpy(h2h.tpr, "q");
    h2h.qri5 = 'W';
    h2h.qri6 = 'V';
    h2h.remAddrNum = '1';
    return h2h;
}

void readTlgFromFile(const string& filename, string& str)
{
    readFile(filename, str);
    if (str.empty()) {
        throw std::logic_error("empty file");
    }
}

void splitJoin(const string& tlgText, int rtr, bool isEdifact = false, hth::HthInfo* p_hthInfo = NULL, bool mustSplit = true)
{
    ProgTrace(TRACE5, "%s: rtr=%d, isEdifact=%d, hth=%d", __FUNCTION__, rtr, isEdifact, (p_hthInfo == NULL));
    OUT_INFO oi = {};
    oi.q_num = rtr;
    if (p_hthInfo)
    {
        oi.isHth = 1;
        oi.hthInfo = *p_hthInfo;
    }
    Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, tlgText.c_str());
    if (!result) {
        fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
    }
    std::string out_area = tlgText;
    std::string new_area;
    const tlgnum_t& tlgNum = result->tlgNum;
    const std::string old_area(out_area);
    ProgTrace(TRACE5, " ***  OUT_AREA LENGTH: %zd", out_area.size());
    tlgnums_t nums;
    ProgTrace(TRACE5, "BEFORE msgId: %s, nums: %zd, strlen(out_area): [%zd]", tlgNum.num.get().c_str(), nums.size(), out_area.size());
    telegrams::split_tlg(tlgNum, tlg_text_filter(), (oi.isHth ? &oi.hthInfo : NULL), rtr, isEdifact, out_area, nums);
    ProgTrace(TRACE5, "AFTER nums: %zd, strlen(out_area): [%zd]", nums.size(), out_area.size());

    if (mustSplit)
        fail_unless(nums.size(), "splitter failed");
    else {
        fail_unless(nums.size() == 0, "splitter failed: nums.size=%zd", nums.size());
        return;
    }

    boost::optional<tlgnum_t> newTlgNum;
    for(const tlgnum_t& num:  nums) {
        LogTrace(TRACE5) << "preparing to read tlg " << num;
        if (callbacks()->readTlg(num, new_area) < 0)
            fail_unless(0,"readTlg failed for %s", num.num.get().c_str());

        if (p_hthInfo) {
            if (!callbacks()->tlgIsHth(num))
                fail_unless(0,"part: %s - is not H2H", num.num.get().c_str());
            // Hth without edifact - this is part and we must not split it again
            tlgnums_t tmpNums;
            if (telegrams::split_tlg(num, tlg_text_filter(), (oi.isHth ? &oi.hthInfo : NULL), rtr, false , new_area, tmpNums) != 0)
                fail_unless(0,"split_tlg failed for %s", num.num.get().c_str());
        }
        INCOMING_INFO ii = {{0}};
        ii.q_num = callbacks()->defaultHandler();
        ii.router = rtr;
        if (p_hthInfo) {
            hth::HthInfo tmp;
            callbacks()->readHthInfo(num, tmp);
            fail_unless(tmp.part, "invalid part number");
            if (num == *(--nums.end()))
                fail_unless(tmp.end == 1, "no end attribute found");
            else
                fail_unless(tmp.end == 0, "invalid end attribute");

            ii.router = rtr;
            ii.hthInfo = tmp;
        }

        RouterInfo ri;
        ri.ida = rtr;
        callbacks()->getRouterInfo(ri.ida, ri);
        ii.isEdifact = isEdifact;
        newTlgNum = num;
        int ret = telegrams::joinTlgParts(new_area.c_str(), ri, ii, *newTlgNum, num);
        if (ret < 0)
            fail_unless(0,"joinTlgParts failed with code: %d", ret);
    }

    if (callbacks()->readTlg(*newTlgNum, new_area) < 0)
        fail_unless(0,"readTlg failed for %s", tlgNum.num.get().c_str());
    std::ofstream bf("/tmp/before.tlg");
    bf << old_area;
    std::ofstream af("/tmp/after.tlg");
    af << new_area;
    LogTrace(TRACE5) << "\nBEFORE (" << old_area.size() << "):\n" << old_area
        << "\nAFTER (" << new_area.size() << "):\n" << new_area;
    fail_unless(new_area == old_area, "bad joined");
}

void splitTlgFromFile(const std::string& filename, int rtr)
{
    string str;
    readTlgFromFile(filename, str);
    OUT_INFO oi = {0};
    oi.q_num = rtr;
    Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, str.c_str());
    if (!result) {
        fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
    }

    std::string out_area = str;
    const tlgnum_t& tlgNum = result->tlgNum;
    ProgTrace(TRACE5, " ***  OUT_AREA LENGTH: %zd", out_area.size());
    bool edifact = false;
    tlgnums_t nums;
    ProgTrace(TRACE5, "BEFORE msgId: %s, nums: %zd, strlen(out_area): [%zd]", tlgNum.num.get().c_str(), nums.size(), out_area.size());
    telegrams::split_tlg(tlgNum, tlg_text_filter(), NULL, rtr, edifact, out_area, nums);
    ProgTrace(TRACE5, "AFTER nums: %zd, strlen(out_area): [%zd]", nums.size(), out_area.size());
}

void check_bad_router_part_size(hth::HthInfo* const h2h, int rtr, const tlgnum_t& tlgNum, const string& tlgText, size_t maxPartSz)
{
    char out_area[MAX_TLG_LEN] = {0};
    strcpy(out_area, tlgText.c_str());
    bool edifact = false;
    tlgnums_t nums;
    telegrams::split_tlg(tlgNum, tlg_text_filter(), h2h, rtr, edifact, out_area, nums);
    size_t splPartSz = telegrams::getSplitterPartSize();
    fail_unless(splPartSz == maxPartSz, "invalid partSize: %d instead of %d", splPartSz, maxPartSz);
}


} // namespace

int ExternalDb::countParts()
{
    return airQPartManager()->size();
}

void setExternalDb(ExternalDb* db)
{
    p_db.reset(db);
}

/*** TESTS  ***/

void check_split_join_common()
{
    const int rtr = p_db->prepareCommonRouter();
    string tmp = makeLongTlg(MAX_TLG_SIZE * 2);
    splitJoin(tmp, rtr);
}

void check_split_join_hth()
{
    const int rtr = p_db->prepareHthRouter();
    string tmp = makeLongTlg(RouterInfo::defaultMaxHthPartSize * 20);
    hth::HthInfo tmpHth = getHthHeader();
    splitJoin(tmp, rtr, true, &tmpHth);
}

void check_split_join_typeb()
{
    const int rtr = p_db->prepareTypeBRouter();
    string tmp = createTestTypeB();
    splitJoin(tmp, rtr);
}

void check_split_join_true_typeb()
{
    const int rtr = p_db->prepareTrueTypeBRouter();
    string tmp = createTestTypeB();
    splitJoin(tmp, rtr);
}

void check_split_typeb_from_file(const std::string& filename)
{
    int rtr = p_db->prepareTypeBRouter();
    splitTlgFromFile(filename, rtr);
}

void check_split_join_common_from_file(const std::string& filename, bool mustSplit)
{
    string tlg;
    readTlgFromFile(filename, tlg);
    const int rtr = p_db->prepareCommonRouter();
    splitJoin(tlg, rtr, false, NULL, mustSplit);
}

void check_split_join_too_small_part_sizes(const std::string& filename)
{
    string tlg;
    readTlgFromFile(filename, tlg);
    const int rtr = p_db->prepareHthRouter();
    p_db->setRouterMaxPartSizes(rtr, 20);
    splitJoin(tlg, rtr, false, NULL, false);
}

void check_split_bad_router_sizes()
{
    size_t partSz = MAX_TLG_SIZE - 1;
    // common telegrams
    {
        int rtr = p_db->prepareCommonRouter();
        p_db->setRouterMaxPartSizes(rtr, partSz);
        string tmp = makeLongTlg(MAX_TLG_SIZE * 2);

        OUT_INFO oi = {0};
        oi.q_num = rtr;
        Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, tmp.c_str());
        if (!result) {
            fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
        }
        check_bad_router_part_size(NULL, rtr, result->tlgNum, tmp, partSz);
    }

    // HTH telegrams
    {
        int rtr = p_db->prepareHthRouter();
        p_db->setRouterMaxPartSizes(rtr, partSz);
        string tmp = makeLongTlg(RouterInfo::defaultMaxHthPartSize * 2);

        OUT_INFO oi = {0};
        oi.q_num = rtr;
        oi.isHth = 1;
        oi.hthInfo = getHthHeader();
        Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, tmp.c_str());
        if (!result) {
            fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
        }
        check_bad_router_part_size(&oi.hthInfo, rtr, result->tlgNum, tmp, telegrams::RouterInfo::defaultMaxHthPartSize);
    }
    // TypeB telegrams
    {
        int rtr = p_db->prepareTypeBRouter();
        p_db->setRouterMaxPartSizes(rtr, partSz);
        string tmp =
    "MOWETP2\n.MOWETDT 050837\nETE\nMOWDT 01TF7K\nSSR TKNX 2986151019548 N\nSSR TKNX 2986151019547 O";
        tmp += makeLongTlg(RouterInfo::defaultMaxTypeBPartSize * 2, "\n0123456789");

        OUT_INFO oi = {0};
        oi.q_num = rtr;
        Expected<TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, tmp.c_str());
        if (!result) {
            fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
        }

        check_bad_router_part_size(NULL, rtr, result->tlgNum, tmp, telegrams::RouterInfo::defaultMaxTypeBPartSize);
    }
}

// TODO  add checks for bad TypeB and bad TypeB parts
//void check_split_join_bad_tpb()
//{
    //int rtr = prepareTypeBRouter();
    //string tmp = makeLongTlg(RouterInfo::defaultMaxTypeBPartSize * 2, "BadTypeB_");
//}

void check_save_bad_tlg()
{
    AIRSRV_MSG msg = {0};
    msg.num = 123;
    msg.type = 4;
    msg.TTL = 2;
    strcpy(msg.Sender, "Snd");
    strcpy(msg.body, "test tlg text");

    fail_unless(callbacks()->saveBadTlg(msg, telegrams::BAD_EDIFACT), "saveBadTlg failed");
}


namespace
{

const char* fullAdl =
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "ADL\n"
        "UT100/28MAY DME PART1\n"
        "CFG/004F008C014Y\n"
        "RBD F/F C/C Y/YK\n"
        "AVAIL\n"
        " DME  ROV  LED\n"
        "F004  004\n"
        "C008  008\n"
        "Y002  008\n"
        "-ROV000F\n"
        "-ROV000C\n"
        "-ROV006Y\n"
        "-ROV000K\n"
        "-LED000F\n"
        "-LED000C\n"
        "-LED006Y\n"
        "CHG\n"
        "1’’…‘’/’…‘’ ’…‘’‚ˆ—-B4\n"
        ".L/00030K\n"
        ".R/INFT HK1 ’’…‘’/’…‘’ˆ—Š€’…‘’‚ˆ— 01JAN01-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/TKNA HK1 2986000000004-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/VGML HK1 -1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/BBML HK1 -1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/DOCS HK1/P/RUS/1234567890/RUS/01JAN01/M//’’…‘’/’…‘’/’…‘’‚ˆ—\n"
        ".RN/-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/DOCS HK1/F/RUS/12345679/RUS/01JAN01/FI//’’…‘’/’…‘’ˆ—Š€\n"
        ".RN//’…‘’‚ˆ—-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        ".R/PSPT HK1 1234567890/RUS/01JAN01/’’…‘’/’…‘’ ’…‘’‚ˆ—/M-1’’…‘’\n"
        ".RN//’…‘’ ’…‘’‚ˆ—\n"
        ".R/FOID PP1234567890-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
        "1’’…‘’/’…‘’€ ’…‘’‚€-B4\n"
        ".R/INFT HK1 ’’…‘’/’…‘’ˆŠ’…‘’‚ˆ— 01JAN01-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/TKNA HK1 2986000000005-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/BBML HK1 -1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/VGML HK1 -1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/PETC HK1 ‘‹ˆŠ 3’›-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/DOCS HK1/P/RUS/1234567891/RUS/01JAN01/F//’’…‘’/’…‘’€\n"
        ".RN//’…‘’‚€-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/DOCS HK1/F/RUS/12345678/RUS/01JAN01/MI//’’…‘’/’…‘’ˆŠ\n"
        ".RN//’…‘’‚ˆ—-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/PSPT HK1 1234567891/RUS/01JAN01/’’…‘’/’…‘’€ ’…‘’‚€/F\n"
        ".RN/-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        ".R/FOID PP1234567891-1’’…‘’/’…‘’€ ’…‘’‚€\n"
        "1’’…‘’/‚‚—Š€-B4\n"
        ".R/CHLD HK1 01JAN01-1’’…‘’/‚‚—Š€\n"
        ".R/TKNA HK1 2986000000006-1’’…‘’/‚‚—Š€\n"
        ".R/CHML HK1 -1’’…‘’/‚‚—Š€\n"
        ".R/DOCS HK1/F/RUS/888/RUS/01JAN01/M//’’…‘’/‚‚—Š€-1’’…‘’\n"
        ".RN//‚‚—Š€\n"
        ".R/PSPT HK1 ‘888/RUS/01JAN01/’’…‘’/‚‚—Š€/M-1’’…‘’/‚‚—Š€\n"
        ".R/FOID PP‘888-1’’…‘’/‚‚—Š€\n"
        "1’’…‘’/‹…—Š€-B4\n"
        ".R/CHLD HK1 01JAN01-1’’…‘’/‹…—Š€\n"
        ".R/TKNA HK1 2986000000007-1’’…‘’/‹…—Š€\n"
        ".R/CHML HK1 -1’’…‘’/‹…—Š€\n"
        ".R/DOCS HK1/F/RUS/999/RUS/01JAN01/F//’’…‘’/‹…—Š€-1’’…‘’\n"
        ".RN//‹…—Š€\n"
        ".R/PSPT HK1 ‘999/RUS/01JAN01/’’…‘’/‹…—Š€/F-1’’…‘’/‹…—Š€\n"
        ".R/FOID PP‘999-1’’…‘’/‹…—Š€\n"
        "-LED000K\n"
        "ENDADL";

const std::string adlPart[] = {
    "MOWKT1H\n"
    ".MOWRMUT 271216\n"
    "ADL\n"
    "UT100/28MAY DME PART1\n"
    "ANA/123456\n"
    "CFG/004F008C014Y\n"
    "RBD F/F C/C Y/YK\n"
    "AVAIL\n"
    " DME  ROV  LED\n"
    "F004  004\n"
    "C008  008\n"
    "Y002  008\n"
    "-ROV000F\n"
    "-ROV000C\n"
    "-ROV006Y\n"
    "-ROV000K\n"
    "-LED000F\n"
    "-LED000C\n"
    "-LED006Y\n"
    "CHG\n"
    "1’’…‘’/’…‘’ ’…‘’‚ˆ—-B4\n"
    ".L/00030K\n"
    ".R/INFT HK1 ’’…‘’/’…‘’ˆ—Š€’…‘’‚ˆ— 01JAN01-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/TKNA HK1 2986000000004-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/VGML HK1 -1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/BBML HK1 -1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/DOCS HK1/P/RUS/1234567890/RUS/01JAN01/M//’’…‘’/’…‘’/’…‘’‚ˆ—\n"
    ".RN/-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/DOCS HK1/F/RUS/12345679/RUS/01JAN01/FI//’’…‘’/’…‘’ˆ—Š€\n"
    ".RN//’…‘’‚ˆ—-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    ".R/PSPT HK1 1234567890/RUS/01JAN01/’’…‘’/’…‘’ ’…‘’‚ˆ—/M-1’’…‘’\n"
    ".RN//’…‘’ ’…‘’‚ˆ—\n"
    ".R/FOID PP1234567890-1’’…‘’/’…‘’ ’…‘’‚ˆ—\n"
    "ENDPART1\n",
    "MOWKT1H\n"
    ".MOWRMUT 271216\n"
    "ADL\n"
    "UT100/28MAY DME PART2\n"
    "ANA/123456\n"
    "-LED006Y\n"
    "CHG\n"
    "1’’…‘’/’…‘’€ ’…‘’‚€-B4\n"
    ".R/INFT HK1 ’’…‘’/’…‘’ˆŠ’…‘’‚ˆ— 01JAN01-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/TKNA HK1 2986000000005-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/BBML HK1 -1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/VGML HK1 -1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/PETC HK1 ‘‹ˆŠ 3’›-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/DOCS HK1/P/RUS/1234567891/RUS/01JAN01/F//’’…‘’/’…‘’€\n"
    ".RN//’…‘’‚€-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/DOCS HK1/F/RUS/12345678/RUS/01JAN01/MI//’’…‘’/’…‘’ˆŠ\n"
    ".RN//’…‘’‚ˆ—-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/PSPT HK1 1234567891/RUS/01JAN01/’’…‘’/’…‘’€ ’…‘’‚€/F\n"
    ".RN/-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    ".R/FOID PP1234567891-1’’…‘’/’…‘’€ ’…‘’‚€\n"
    "1’’…‘’/‚‚—Š€-B4\n"
    ".R/CHLD HK1 01JAN01-1’’…‘’/‚‚—Š€\n"
    ".R/TKNA HK1 2986000000006-1’’…‘’/‚‚—Š€\n"
    ".R/CHML HK1 -1’’…‘’/‚‚—Š€\n"
    ".R/DOCS HK1/F/RUS/888/RUS/01JAN01/M//’’…‘’/‚‚—Š€-1’’…‘’\n"
    ".RN//‚‚—Š€\n"
    ".R/PSPT HK1 ‘888/RUS/01JAN01/’’…‘’/‚‚—Š€/M-1’’…‘’/‚‚—Š€\n"
    ".R/FOID PP‘888-1’’…‘’/‚‚—Š€\n"
    "ENDPART2\n",
    "MOWKT1H\n"
    ".MOWRMUT 271216\n"
    "ADL\n"
    "UT100/28MAY DME PART3\n"
    "ANA/123456\n"
    "-LED006Y\n"
    "CHG\n"
    "1’’…‘’/‹…—Š€-B4\n"
    ".R/CHLD HK1 01JAN01-1’’…‘’/‹…—Š€\n"
    ".R/TKNA HK1 2986000000007-1’’…‘’/‹…—Š€\n"
    ".R/CHML HK1 -1’’…‘’/‹…—Š€\n"
    ".R/DOCS HK1/F/RUS/999/RUS/01JAN01/F//’’…‘’/‹…—Š€-1’’…‘’\n"
    ".RN//‹…—Š€\n"
    ".R/PSPT HK1 ‘999/RUS/01JAN01/’’…‘’/‹…—Š€/F-1’’…‘’/‹…—Š€\n"
    ".R/FOID PP‘999-1’’…‘’/‹…—Š€\n"
    "-LED000K\n"
    "ENDADL\n"
};

const char* fullPnl =
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "PNL\n"
        "UT100/28MAY DME PART1 \n"
        "CFG/004F008C014Y\n"
        "RBD F/F C/C Y/YK\n"
        "AVAIL\n"
        " DME  SVX  LED\n"
        "F003  003\n"
        "C002  006\n"
        "Y007  012\n"
        "-SVX000F\n"
        "-SVX004C\n"
        "1IVANOV/AAA MR-A2\n"
        ".L/00030K\n"
        "1IVANOVA/CHILD CHD-A2\n"
        ".R/CHLD HK1 10OCT10-1IVANOVA/CHILD CHD\n"
        "2SIDOROV/AAA MR/CHILD CHD\n"
        ".L/00030M\n"
        ".R/INF HK1 SIDOROVA/INFANTA 10OCT10-1SIDOROV/AAA MR\n"
        ".R/CHLD HK1 10OCT10-1SIDOROV/CHILD CHD\n"
        "-SVX005Y\n"
        "1PETROV/AAA MR\n"
        ".L/00030L\n"
        ".R/INF HK1 PETROVA/INFANTA 10OCT10-1PETROV/AAA MR\n"
        "1RYLOV/EDIC MR-B4\n"
        ".L/00030N\n"
        "1ZADOV/VASILY MR-B4\n"
        "2ZADOVA/NINA MRS/TANYA CHD-B4\n"
        ".R/INF HK1 ZADOV/PAVEL 10OCT10-1ZADOVA/NINA MRS\n"
        ".R/CHLD HK1 10OCT10-1ZADOVA/TANYA CHD\n"
        "-SVX000K\n"
        "-LED001F\n"
        "1TYSON/MIKE MR\n"
        ".L/00030V\n"
        "-LED002C\n"
        "1KLITCHKO/VITALY MR-C2\n"
        ".L/00030G\n"
        "1LEWIS/LENNOX MR-C2\n"
        "-LED002Y\n"
        "2HOPKINS/BERNARD MR/JEANETTE MRS\n"
        ".L/00030D\n"
        "-LED000K\n"
        "ENDPNL";
}

const std::string parts[] = {
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "PNL\n"
        "UT100/28MAY DME PART1\n"
        "CFG/004F008C014Y\n"
        "RBD F/F C/C Y/YK\n"
        "AVAIL\n"
        " DME  SVX  LED\n"
        "F003  003\n"
        "C002  006\n"
        "Y007  012\n"
        "-SVX000F\n"
        "-SVX004C\n"
        "1IVANOV/AAA MR-A2\n"
        ".L/00030K\n"
        "1IVANOVA/CHILD CHD-A2\n"
        ".R/CHLD HK1 10OCT10-1IVANOVA/CHILD CHD\n"
        "ENDPART1\n",
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "PNL\n"
        "UT100/28MAY DME PART2\n"
        "-SVX004C\n"
        "2SIDOROV/AAA MR/CHILD CHD\n"
        ".L/00030M\n"
        ".R/INF HK1 SIDOROVA/INFANTA 10OCT10-1SIDOROV/AAA MR\n"
        ".R/CHLD HK1 10OCT10-1SIDOROV/CHILD CHD\n"
        "-SVX005Y\n"
        "1PETROV/AAA MR\n"
        ".L/00030L\n"
        ".R/INF HK1 PETROVA/INFANTA 10OCT10-1PETROV/AAA MR\n"
        "ENDPART2\n",
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "PNL\n"
        "UT100/28MAY DME PART3\n"
        "-SVX005Y\n"
        "1RYLOV/EDIC MR-B4\n"
        ".L/00030N\n"
        "1ZADOV/VASILY MR-B4\n"
        "2ZADOVA/NINA MRS/TANYA CHD-B4\n"
        ".R/INF HK1 ZADOV/PAVEL 10OCT10-1ZADOVA/NINA MRS\n"
        ".R/CHLD HK1 10OCT10-1ZADOVA/TANYA CHD\n"
        "-SVX000K\n"
        "-LED001F\n"
        "1TYSON/MIKE MR\n"
        ".L/00030V\n"
        "-LED002C\n"
        "ENDPART3\n",
        "MOWKT1H\n"
        ".MOWRMUT 271216\n"
        "PNL\n"
        "UT100/28MAY DME PART4\n"
        "-LED002C\n"
        "1KLITCHKO/VITALY MR-C2\n"
        ".L/00030G\n"
        "1LEWIS/LENNOX MR-C2\n"
        "-LED002Y\n"
        "2HOPKINS/BERNARD MR/JEANETTE MRS\n"
        ".L/00030D\n"
        "-LED000K\n"
        "ENDPNL\n"
};

namespace
{
const char* pfsWhole1 =
"ROMRMAZ\n"
".LHRKMBA 121720\n"
"PFS\n"
"AZ123/12APR LHR PART1\n"
"FCO 03/012\n"
"NBO 04/022\n"
"JNB /5/070\n"
"ENDPFS";

const char* pfsWhole2 =
"ROMRMAZ\n"
".LHRKMBA 121720\n"
" PFS \n"
"AZ123/12APR LHR PART1\n"
"FCO 03/012\n"
"NBO 04/022 \n"
"JNB /5/070 \n"
" ENDPFS \n \n   \n\n\n";

const char* pfsWhole3 =
"QU MOWIT1H\n"
".LEDAV7X UT/280925\n"
"  PFS  \n"
"UT530/28JAN LED PART1   \n"
"MMK 000/000/000/000/014/000/000/000/000/000/000/000/000/000/\n"
"    000/001/024/000/000/000/000 PAD000/000/000/000/000/000/000/\n"
"    000/000/000/000/000/000/000/000/000/000/000/000/000/000 \n"
"-MMK\n"
"NOREC 13Y   \n"
"1ABCANDZE/GELAMR\n"
"1ASATIANI/GEORGYMR  \n"
"1ASHITKOVA/INNAMRS  \n"
"1BELYAVSKAYA/EKATERINA  \n"
"1FEDOROV/DENISMR\n"
"1GONCHAROVA/ANASTASIAM RS   \n"
"1GONCHAROVA/ELENAMRS\n"
"1KAZAKOVA/SOFIAMRS  \n"
"1MIKHNOVETS/ALEKSANDERMR\n"
"1MOSKALEVA/MARINAMRS\n"
"1OSTROVSKIY/SERGEYMR\n"
"1REDCHIDZ/NADEZHDAMRS   \n"
"1SHELKUNOVA/ALEKSANDRA  \n"
"ENDPFS  \n";

const char* pfsPart1 =
"MOWRMUT\n"
".MOWKK1H 160633\n"
" PFS  \n"
"UT456/16DEC NOJ PART1\n"
"VKO 00/000/000\n"
"-VKO\n"
"NOSHO 18V\n"
"1SHISHKIN/ALEKSANDR\n"
".L/0W783P/UT\n"
"1SAMOKHVALOV/SERGEI\n"
".L/0W7G71/UT\n"
"1ZLAIA/LIUBOV\n"
".L/0W7DKN/UT\n"
"1BEGUN/ELENA\n"
".L/0W7W27/UT\n"
"1BERTOSH/VLADIMIR\n"
".L/0W8398/UT\n"
"ENDPART1\n";

const char* pfsPart2 =
"MOWRMUT\n"
".MOWKK1H 160633\n"
"PFS\n"
"UT456/16DEC NOJ PART2\n"
"1VOLKOV/ALEKSANDR\n"
".L/0W83ML/UT\n"
"1MESNIANKINA/IRINA\n"
".L/0W842N/UT\n"
"1ZAMIATIN/MIKHAIL\n"
"ENDPFS\n \n  \n\n";

const char* pfsPartsJoined =
"MOWRMUT\n"
".MOWKK1H 160633\n"
" PFS  \n"
"UT456/16DEC NOJ PART1\n"
"VKO 00/000/000\n"
"-VKO\n"
"NOSHO 18V\n"
"1SHISHKIN/ALEKSANDR\n"
".L/0W783P/UT\n"
"1SAMOKHVALOV/SERGEI\n"
".L/0W7G71/UT\n"
"1ZLAIA/LIUBOV\n"
".L/0W7DKN/UT\n"
"1BEGUN/ELENA\n"
".L/0W7W27/UT\n"
"1BERTOSH/VLADIMIR\n"
".L/0W8398/UT\n"
"1VOLKOV/ALEKSANDR\n"
".L/0W83ML/UT\n"
"1MESNIANKINA/IRINA\n"
".L/0W842N/UT\n"
"1ZAMIATIN/MIKHAIL\n"
"ENDPFS";
}

static void check_pfs_whole(const char* tlgText, const INCOMING_INFO& ii)
{
    setTclVar("ENABLE_PFS_JOINER", "1");
    Expected<TlgResult, int> result = callbacks()->putTlg(tlgText, tlg_text_filter(), ii.router, 0, NULL);
    if (!result) {
        fail_unless(0,"putTlg failed with code %d", result.err());
    }
    RouterInfo ri;
    callbacks()->getRouterInfo(ii.router, ri);
    tlgnum_t newTlgNum = result->tlgNum;
    int ret = joinTlgParts(tlgText, ri, ii, newTlgNum, newTlgNum);
    if (ret < 0)
        fail_unless(0,"joinTlgParts failed with code: %d", ret);
    if (ret == 0)
        fail_unless(0,"this is not a part");
    if (newTlgNum != result->tlgNum)
        fail_unless(0,"nothing must be joined");
}

void check_split_pnl()
{
    const int rtr = p_db->prepareCommonRouter();
    const std::size_t MAX_PNL_PART_SIZE = 300;
    p_db->setRouterMaxPartSizes(rtr, MAX_PNL_PART_SIZE);
    OUT_INFO oi = {};
    oi.q_num = rtr;
    tlgnums_t nums;
    Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, fullPnl);
    if (!result) {
        fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
    }
    const tlgnum_t& tlgNum = result->tlgNum;
    std::string out_area = fullPnl;
    telegrams::split_tlg(tlgNum, tlg_text_filter(), NULL, rtr, false, out_area, nums);

    const std::string* part = parts;
    for(const tlgnum_t& num:  nums) {
        if (callbacks()->readTlg(num, out_area) < 0) {
            fail_unless(0,"readTlg failed for %s", tlgNum.num.get().c_str());
        }
        LogTrace(TRACE5) << "Telegram part size=" << out_area.size() << " is (" << out_area;
        LogTrace(TRACE5) << ")";
        fail_unless(out_area.size() < MAX_PNL_PART_SIZE, "a part size is greater MAX_PNL_PART_SIZE");
        fail_unless(out_area == *part, "parts doesn't match");
        LogTrace(TRACE5) << "Read part = " << out_area;
        LogTrace(TRACE5) << "Stored part = " << *part;
        ++part;
    }
}

std::vector<std::string> splitAdlFile(const std::string& adlFile, std::size_t partSize)
{
    const int rtr = p_db->prepareCommonRouter();
    string fullAdl;
    readTlgFromFile(adlFile, fullAdl);
    if (partSize != std::size_t(-1)) {
        p_db->setRouterMaxPartSizes(rtr, partSize);
    }
    OUT_INFO oi = {};
    oi.q_num = rtr;
    tlgnums_t nums;
    Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, fullAdl.c_str());
    if (!result) {
        fail_unless(0, "putTlg2OutQueue failed ret=%d", result.err());
    }
    const tlgnum_t& tlgNum = result->tlgNum;
    std::string out_area = fullAdl;
    telegrams::split_tlg(tlgNum, tlg_text_filter(), NULL, rtr, false, out_area, nums);

    std::vector<std::string> adlParts;
    for(const tlgnum_t& num:  nums) {
        if (callbacks()->readTlg(num, out_area) < 0) {
            fail_unless(0,"readTlg failed for %d", tlgNum.num.get().c_str());
        }
        adlParts.push_back(out_area);
    }
    return adlParts;
}

void check_split_adl()
{
    const int rtr = p_db->prepareCommonRouter();
    const std::size_t MAX_ADL_PART_SIZE = 1200;
    p_db->setRouterMaxPartSizes(rtr, MAX_ADL_PART_SIZE);
    OUT_INFO oi = {};
    oi.q_num = rtr;
    tlgnums_t nums;
    Expected<telegrams::TlgResult, int> result = callbacks()->putTlg2OutQueue(&oi, fullAdl);
    if (!result) {
        fail_unless(0,"putTlg2OutQueue failed ret=%d", result.err());
    }
    const tlgnum_t tlgNum = result->tlgNum;
    telegrams::split_tlg(tlgNum, tlg_text_filter(), NULL, rtr, false, fullAdl, nums);

    const std::string* part = adlPart;
    for(const tlgnum_t& num:  nums) {
        std::string out_area;
        if (callbacks()->readTlg(num, out_area) < 0) {
            fail_unless(0,"readTlg failed for %s", tlgNum.num.get().c_str());
        }
        LogTrace(TRACE5) << "Telegram part size=" << out_area.size() << " is (" << out_area << ")";
        fail_unless(out_area.size() < MAX_ADL_PART_SIZE, "a part size is greater MAX_ADL_PART_SIZE");
        fail_unless(out_area == *part, "parts doesn't match");
        LogTrace(TRACE5) << "Read part = " << out_area;
        LogTrace(TRACE5) << "Stored part = " << *part;
        ++part;
    }
}

void check_split_adl_file(const std::string& fullAdlFile, const std::string& splitFile)
{
    std::vector<std::string> adls = splitAdlFile(fullAdlFile, 1000);
    std::string splittedAdl;
    for(const std::string& adlPart:  adls) {
        splittedAdl += adlPart;
    }
    string splittedAdlFile;
    readTlgFromFile(splitFile, splittedAdlFile);

    writeFile("/tmp/adl.tlg", splittedAdl);
    LogTrace(TRACE5) << "splitted adl: " << splittedAdl;
    LogTrace(TRACE5) << "splitted adl from file: " << splittedAdlFile;
    fail_unless(splittedAdl == splittedAdlFile, "parts doesn't match");
}

//int joinTlgParts(const char* tlgBody, const RouterInfo& ri, const INCOMING_INFO& ii, tlgnum_t& localMsgId, const tlgnum_t& remoteMsgId)
void check_join_pfs()
{
    setTclVar("ENABLE_PFS_JOINER", "1");
    const int rtr = p_db->prepareCommonRouter();
    INCOMING_INFO ii = {};
    ii.q_num = callbacks()->defaultHandler();
    ii.router = rtr;
    ProgTrace(TRACE5, "pfsWhole1");
    check_pfs_whole(pfsWhole1, ii);
    ProgTrace(TRACE5, "pfsWhole2");
    check_pfs_whole(pfsWhole2, ii);
    ProgTrace(TRACE5, "pfsWhole3");
    check_pfs_whole(pfsWhole3, ii);
    {
        Expected<TlgResult, int> result = callbacks()->putTlg(pfsPart1, tlg_text_filter(), ii.router, 0, NULL);
        if (!result) {
            fail_unless(0,"putTlg failed with code %d", result.err());
        }
        RouterInfo ri;
        callbacks()->getRouterInfo(rtr, ri);
        tlgnum_t newTlgNum = result->tlgNum;
        int ret = joinTlgParts(pfsPart1, ri, ii, newTlgNum, newTlgNum);
        if (ret < 0)
            fail_unless(0,"joinTlgParts failed with code: %d", ret);
        fail_unless(ret == 0, "this is a part");
        fail_unless(p_db->countParts() == 1, "bad recs count in AIR_Q_PART");
    }
    {
        Expected<TlgResult, int> result = callbacks()->putTlg(pfsPart2, tlg_text_filter(), ii.router, 0, NULL);
        if (!result) {
            fail_unless(0,"putTlg failed with code %d", result.err());
        }
        RouterInfo ri;
        callbacks()->getRouterInfo(rtr, ri);
        tlgnum_t newTlgNum = result->tlgNum;
        int ret = joinTlgParts(pfsPart2, ri, ii, newTlgNum, newTlgNum);
        if (ret < 0)
            fail_unless(0,"joinTlgParts failed with code: %d", ret);
        fail_unless(ret != 0, "this is last part");
        fail_unless(newTlgNum != result->tlgNum, "parts must be joined");
        fail_unless(p_db->countParts() == 0, "bad recs count in AIR_Q_PART");


        std::string new_area;
        if (callbacks()->readTlg(newTlgNum, new_area) < 0)
            fail_unless(0,"readTlg failed");
        std::ofstream bf("/tmp/before.tlg");
        bf << pfsPartsJoined;
        bf.close();
        std::ofstream af("/tmp/after.tlg");
        af << new_area;
        af.close();
        LogTrace(TRACE5) << "\nBEFORE (" << strlen(pfsPartsJoined) << "):\n" << pfsPartsJoined
            << "\nAFTER (" << new_area.size() << "):\n" << new_area;
        fail_unless(new_area == pfsPartsJoined, "bad joined");
    }
}

namespace
{

//752359995 | TYPE: EDIFACT | ROUTER: MOWET | TSTAMP: 2011-Jun-24 13:31:13
const char * part1 =
"V.\rVHLF.WAA/E51HGDS/I51HETS/PV22GM5ERFS\rVGYA\r"
"UNB+IATA:1+ET1H+ETR2+110624:0931+V22GM5ERFS0001+++O'"
"UNH+1+TKTREQ:04:1:IA+V22GM5ERFS'"
"MSG+:130'"
"ORG+1H:MOW+2000058:99ƒ+REN++T+RU:RUB:RU+6+ƒ508'"
"TAI+2914+6:B'"
"RCI+1H:V22GM5:1+R2:0BKB67:1'"
"EQN+30:TD'"
"TIF+ANTIMONOV+NIKITA'"
"MON+B:FREE+T:FREE'"
"FOP+MS:3:0.00::::::::::CH/'"
"PTK+N::I++240611+++:RU'"
"ODI+KUF+KUF'"
"ATI+CH414790'"
"EQN+2:TF'"
"TXD++EXEMPT:::RU'"
"IFT+4:15:0+KUF R2 HER0.00R2 KUF0.00RUB0.00END'"
"IFT+4:39+…“ƒ+€ …“ƒ‘Šˆ… €‚ˆ€‹ˆˆˆ'"
"IFT+4:5+(1) 1'"
"CRI+PP:637197739:RU'"
"TKT+2912427348406:T:1:3'"
"CPN+1:I'"
"TVL+260611:0200+KUF+HER+R2+9651:Y++1'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"CPN+2:I'"
"TVL+060711:1905+HER+KUF+R2+9652:Y++2'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"TIF+ANTIMONOVA+OLGA'"
"MON+B:FREE+T:FREE'"
"FOP+MS:3:0.00::::::::::CH/'"
"PTK+N::I++240611+++:RU'"
"ODI+KUF+KUF'"
"ATI+CH414790'"
"EQN+2:TF'"
"TXD++EXEMPT:::RU'"
"IFT+4:15:0+KUF R2 HER0.00R2 KUF0.00RUB0.00END'"
"IFT+4:39+…“ƒ+€ …“ƒ‘Šˆ… €‚ˆ€‹ˆˆˆ'"
"IFT+4:5+(1) 1'"
"CRI+PP:637193701:RU'"
"TKT+2912427348407:T:1:3'"
"CPN+1:I'"
"TVL+260611:0200+KUF+HER+R2+9651:Y++1'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"CPN+2:I'"
"TVL+060711:1905+HER+KUF+R2+9652:Y++2'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"MON+B:FRE";

//752359996 | TYPE: EDIFACT | ROUTER: MOWET | TSTAMP: 2011-Jun-24 13:31:13
const char * part2 =
"V.\rVHLD.WAB/E51HGDS/I51HETS/PV22GM5ERFS\rVGYA\r"
"E+T:FREE'"
"FOP+MS:3:0.00::::::::::CH/'"
"PTK+N::I++240611+++:RU'"
"ODI+KUF+KUF'"
"ATI+CH414790'"
"EQN+2:TF'"
"TXD++EXEMPT:::RU'"
"IFT+4:15:0+KUF R2 HER0.00R2 KUF0.00RUB0.00END'"
"IFT+4:39+…“ƒ+€ …“ƒ‘Šˆ… €‚ˆ€‹ˆˆˆ'"
"IFT+4:5+(1) 1'"
"CRI+PP:703690701:RU'"
"TKT+2912427348413:T:1:3'"
"CPN+1:I'"
"TVL+260611:0200+KUF+HER+R2+9651:Y++1'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"CPN+2:I'"
"TVL+060711:1905+HER+KUF+R2+9652:Y++2'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"TIF+BOCHAROVA+IRINA'"
"MON+B:FREE+T:FREE'"
"FOP+MS:3:0.00::::::::::CH/'"
"PTK+N::I++240611+++:RU'"
"ODI+KUF+KUF'";

//752359997 | TYPE: EDIFACT | ROUTER: MOWET | TSTAMP: 2011-Jun-24 13:31:13
const char * part3 =
"V.\rVHLD.WAC/E51HGDS/I51HETS/PV22GM5ERFS\rVGYA\r"
"ODI+KUF+KUF'"
"ATI+CH414790'"
"EQN+2:TF'"
"TXD++EXEMPT:::RU'"
"IFT+4:15:0+KUF R2 HER0.00R2 KUF0.00RUB0.00END'"
"IFT+4:39+…“ƒ+€ …“ƒ‘Šˆ… €‚ˆ€‹ˆˆˆ'"
"IFT+4:5+(1) 1'"
"CRI+PP:633341774:RU'"
"TKT+2912427348421:T:1:3'"
"CPN+1:I'"
"TVL+260611:0200+KUF+HER+R2+9651:Y++1'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'";

//752359998 | TYPE: EDIFACT | ROUTER: MOWET | TSTAMP: 2011-Jun-24 13:31:13
const char * part4 =
"V.\rVHLD.WAD/E51HGDS/I51HETS/PV22GM5ERFS\rVGYA\r"
"EBD++20::W:K'"
"CPN+2:I'"
"TVL+060711:1905+HER+KUF+R2+9652:Y++2'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"TIF+BOCHAROVA:C+ANASTASIYA'"
"MON+B:FREE+T:FREE'"
"FOP+MS:3:0.00::::::::::CH/'"
"PTK+N::I++240611+++:RU'"
"ODI+KUF+KUF'"
"ATI+CH414790'"
"EQN+2:TF'"
"TXD++EXEMPT:::RU'"
"IFT+4:15:0+KUF R2 HER0.00R2 KUF0.00RUB0.00END'"
"IFT+4:39+…“ƒ+€ …“ƒ‘Šˆ… €‚ˆ€‹ˆˆˆ'"
"IFT+4:5+(1) 1'"
"CRI+PP:631209463:RU'"
"TKT+2912427348429:T:1:3'"
"CPN+1:I'"
"TVL+260611:0200+KUF+HER+R2+9651:Y++1'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'";

const char * part5 =
"V.\rVHLE.WAE/E51HGDS/I51HETS/PV22GM5ERFS\rVGYA\r"
"CPN+2:I'"
"TVL+060711:1905+HER+KUF+R2+9652:Y++2'"
"RPI++OK'"
"PTS++YY1'"
"EBD++20::W:K'"
"UNT+697+1'"
"UNZ+1+V22GM5ERFS0001'";

int joinPart(const char * partBody, const char * partName, INCOMING_INFO& ii)
{
    using namespace telegrams;
    {
        Expected<TlgResult, int> result = callbacks()->putTlg(partBody, tlg_text_filter(), ii.router, 0, NULL);
        if (!result) {
            fail_unless(0,"putTlg failed with code %d", result.err());
        }
        RouterInfo ri;
        callbacks()->getRouterInfo(ii.router, ri);

        hth::HthInfo hthInfo = {};
        size_t hthHeadLen = hth::fromString(partBody, hthInfo);
        fail_unless( hthHeadLen > 0 );

        ii.isEdifact = true;
        ii.hthInfo = hthInfo;

        tlgnum_t localMsgId(result->tlgNum);
        return joinTlgParts(partBody, ri, ii, localMsgId, result->tlgNum);
    }
}

} // namespace


void check_hth_join_asc_order()
{
    const int rtr = p_db->prepareCommonRouter();
    INCOMING_INFO ii = {};
    ii.q_num = callbacks()->defaultHandler();
    ii.router = rtr;

    fail_unless(p_db->countParts() == 0);
    fail_unless(joinPart(part1, "part1", ii) == 0);
    fail_unless(p_db->countParts() == 1);
    fail_unless(joinPart(part2, "part2", ii) == 0);
    fail_unless(p_db->countParts() == 2);
    fail_unless(joinPart(part3, "part3", ii) == 0);
    fail_unless(p_db->countParts() == 3);
    fail_unless(joinPart(part4, "part4", ii) == 0);
    fail_unless(p_db->countParts() == 4);
    fail_unless(joinPart(part5, "part5", ii) == 1);
    fail_unless(p_db->countParts() == 0);
}

void check_hth_join_desc_order()
{
    const int rtr = p_db->prepareCommonRouter();
    INCOMING_INFO ii = {};
    ii.q_num = callbacks()->defaultHandler();
    ii.router = rtr;

    fail_unless(p_db->countParts() == 0);
    fail_unless(joinPart(part5, "part5", ii) == 0);
    fail_unless(p_db->countParts() == 1);
    fail_unless(joinPart(part4, "part4", ii) == 0);
    fail_unless(p_db->countParts() == 2);
    fail_unless(joinPart(part3, "part3", ii) == 0);
    fail_unless(p_db->countParts() == 3);
    fail_unless(joinPart(part2, "part2", ii) == 0);
    fail_unless(p_db->countParts() == 4);
    fail_unless(joinPart(part1, "part1", ii) == 1);
    fail_unless(p_db->countParts() == 0);
}

void check_hth_join_rand_order()
{
    const int rtr = p_db->prepareCommonRouter();
    INCOMING_INFO ii = {};
    ii.q_num = callbacks()->defaultHandler();
    ii.router = rtr;

    fail_unless(p_db->countParts() == 0);
    fail_unless(joinPart(part3, "part3", ii) == 0);
    fail_unless(p_db->countParts() == 1);
    fail_unless(joinPart(part5, "part5", ii) == 0);
    fail_unless(p_db->countParts() == 2);
    fail_unless(joinPart(part4, "part4", ii) == 0);
    fail_unless(p_db->countParts() == 3);
    fail_unless(joinPart(part1, "part1", ii) == 0);
    fail_unless(p_db->countParts() == 4);
    fail_unless(joinPart(part2, "part2", ii) == 1);
    fail_unless(p_db->countParts() == 0);
}

}

} // namespace telegrams

#endif // XP_TESTING

