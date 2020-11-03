#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <string.h>
#include <serverlib/exception.h>
#include <serverlib/str_utils.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>

#ifdef HAVE_CONFIG_H
#endif
#include "hth.h"
#include "tlgnum.h"
#include "gateway.h"

using std::string;

namespace hth
{

namespace
{
const string hthBegStr = "V.\rV";
// layer 4
//const char gfi = 'V'; // 01010110
//const char ver = '.'; // 00101110 - not implemented

const char stx = 0x02; // start-of-text
const char etx = 0x03; // end-of-text
}

void trace(int logLevel, const char* nick, const char* file, int line, const HthInfo& hth)
{
    LogTrace(logLevel, nick, file, line) << hth;
}

std::ostream& operator<<(std::ostream& os, const HthInfo& hth)
{
    os << "HhtInfo  type: [" << (hth.type ? hth.type : '0') << "], sender: [" << hth.sender 
        << "], receiver: [" << hth.receiver << "], tpr: [" << hth.tpr 
        << "], why: [" << hth.why << "], part: " << static_cast<int>(hth.part) << ", end: " << static_cast<int>(hth.end)
        << ", qri5: [" << (hth.qri5 ? hth.qri5 : '0') << "], qri6: [" << (hth.qri6 ? hth.qri6 : '0')
        << "], remAddrNum: [" << hth.remAddrNum << "]";
    return os;
}

bool operator==(const HthInfo& lv, const HthInfo& rv)
{
    ProgTrace(TRACE5, "comparing Hth headers");
    hth::trace(TRACE5, lv);
    hth::trace(TRACE5, rv);
    bool valid = true;
    if (lv.type != rv.type) {
        valid = false;
        ProgTrace(TRACE5, "type failed [%c]:[%c]", lv.type, rv.type);
    }
    if (strcmp(lv.sender, rv.sender)) {
        valid = false;
        ProgTrace(TRACE5, "sender failed [%s]:[%s]", lv.sender, rv.sender);
    }
    if (strcmp(lv.receiver, rv.receiver)) {
        valid = false;
        ProgTrace(TRACE5, "receiver failed [%s]:[%s]", lv.receiver, rv.receiver);
    }
    if (strcmp(lv.tpr, rv.tpr)) {
        valid = false;
        ProgTrace(TRACE5, "tpr failed [%s]:[%s]", lv.tpr, rv.tpr);
    }
    if (strcmp(lv.why, rv.why)) {
        valid = false;
        ProgTrace(TRACE5, "err failed [%s]:[%s]", lv.why, rv.why);
    }
    if (lv.part != rv.part) {
        valid = false;
        ProgTrace(TRACE5, "part failed [%d]:[%d]", lv.part, rv.part);
    }
    if (lv.end != rv.end) {
        valid = false;
        ProgTrace(TRACE5, "end failed [%d]:[%d]", lv.end, rv.end);
    }
    if (lv.qri5 != rv.qri5) {
        valid = false;
        ProgTrace(TRACE5, "qri5 failed [%c]:[%c]", lv.qri5, rv.qri5);
    }
    if (lv.qri6 != rv.qri6) {
        valid = false;
        ProgTrace(TRACE5, "qri6 failed [%c]:[%c]", lv.qri6 ? lv.qri6 : '.', rv.qri6 ? rv.qri6 : '.');
    }
    if (lv.remAddrNum != rv.remAddrNum) {
        valid = false;
        ProgTrace(TRACE5, "remAddrNum failed [%c]:[%c]", lv.remAddrNum, rv.remAddrNum);
    }
    return valid;
}



// Serg and 1H team,
//
// The problem we are having with the the large TKTREQ from 1H sent in more than one part is 1H is including an
// extra character at the end of each packet.  There is an hex 03 (ASCII .) at the end of each packet.
// This is causing our edifact translator to fail.   Our system ignores any extra characters at
// the end of an entire edifact message if they come after the UNZ segment but if they come at the
// end of intermediate packets.    EDS would have to make infrastructure changes to support that
// which Sabre would have to approve.
//
// Can 1H not send this extra character?
//
// Here is an example from one of the large TKTREQ from last week:
//
// End of packet 1:
// 2B444D452B4B47442B4B442B3939313A +DME+KGD+KD+991:
// 592B2B36275250492B2B4F03         Y++6'RPI++O.
//
// End of packet 2:
// 31352B4B47442B444D452B4B442B3939 15+KGD+DME+KD+99
// 383A592B2B37275250492B2B4F4B2703 8:Y++7'RPI++OK'.
//
// End of packet 3 and end of tktreq"
// 2B3238332B3127554E5A2B312B303247 +283+1'UNZ+1+02G
// 4B325639323536303030312703       K2V92560001'.
//
//
// Regards,
// Julanne

void createTlg(char* tlgBody, const HthInfo& hthInfo)
{
    LogTrace(TRACE5) << hthInfo;
    string tmpTlg = toString(hthInfo);
// Why do we skip STX? No answer yet :-/
#ifdef H2H_STX_ETX
    tmpTlg += stx;
#endif
    tmpTlg += tlgBody;
    if(hthInfo.end != 0) {
        // for Sabre system - do not send 0x03 at the end of middle part(see comment above)
        tmpTlg += etx;
    }
    if (tmpTlg.length() > MAX_TLG_SIZE) {
        ProgError(STDLOG, "tlg is too long, skipping HTH header?");
    }
    strcpy(tlgBody, tmpTlg.c_str());
}

namespace HTH_MASKS
{
namespace QRI3
{
const char MID_DATA_UNIT = 0x0;
const char LAST_DATA_UNIT = 0x1;
const char FIRST_DATA_UNIT = 0x2;
const char ONLY_DATA_UNIT = 0x3;
}
}

size_t fromString(const string& str, HthInfo& hth)
{
    return fromString(str.c_str(), hth);
}

size_t fromString(const char* tlgBody, HthInfo& hth)
{
    memset(&hth, 0, sizeof(hth));

    if (!tlgBody) {
        LogError(STDLOG) << "hth::fromString: tlgBody is NULL";
        return 0;
    }

    ProgTrace(TRACE5, "checking for HTH in [%.40s], size: %zd", tlgBody, strlen(tlgBody));

    if (strncmp(hthBegStr.c_str(), tlgBody, hthBegStr.size()))
        return 0;
    ProgTrace(TRACE0, "TLG seems to be in HTH format: tlgbeg=%.4s", tlgBody);
    // QRI3
    using namespace HTH_MASKS;
    switch (tlgBody[6] & 0x3) {
    case QRI3::MID_DATA_UNIT:
        hth.part = tlgBody[10] - 'A' + 1;
        break;
    case QRI3::LAST_DATA_UNIT:
        hth.part = tlgBody[10] - 'A' + 1;
        hth.end = 1;
        break;
    case QRI3::FIRST_DATA_UNIT:
        hth.part = tlgBody[10] - 'A' + 1;
        break;
    case QRI3::ONLY_DATA_UNIT:
        hth.part = 0;
        hth.end = 0;
        break;
    default:
        ProgError(STDLOG, "bad QRI3: 0x%2.2x", tlgBody[6]);
        return 0;
    }

    tst();
    hth.type = tlgBody[4];
    if (tlgBody[8] != '/') {
        hth.qri5 = tlgBody[8];
    }
    if (tlgBody[9] != '/') {
        hth.qri6 = tlgBody[9];
    }
    tst();
    if (tlgBody[10] == '.') {
        if (!hth.part && !hth.end) {
            hth.end = 1;
        }
    }
    tst();
    size_t len = 0;
    const char *p = 0, *p1 = 0, *why_p = 0, *end_p = 0;
    char sndAddrNum = 0, recAddrNum = 0;
    if ((p = strstr(tlgBody, "/E"))
            && (p1 = strchr(p + 1, '/'))
            && ((len = p1 - p - 3) <= (sizeof(hth.sender) - 1))) {
        sndAddrNum = (p + 2)[0];
        memcpy(hth.sender, p + 3, len);
        hth.sender[len] = 0;
    } else {
        ProgError(STDLOG, "Bad HTH Sender");
        return 0;
    }

    tst();
    if ((p = strstr(tlgBody, "/I"))
            && (p1 = strchr(p + 1, '/'))
            && ((len = p1 - p - 3) <= (sizeof(hth.receiver) - 1))) {
        recAddrNum = (p + 2)[0];
        memcpy(hth.receiver, p + 3, len);
        hth.receiver[len] = 0;
    } else {
        ProgError(STDLOG, "Bad HTH Receiver");
        return 0;
    }
    if (sndAddrNum != recAddrNum)
        ProgTrace(TRACE1, "sndAddrNum[%c] != recAddrNum[%c], using sndAddrNum", sndAddrNum, recAddrNum);
    // check layer 5 octet 2 b7-b4
    if ((sndAddrNum & 0x78) != 0x30) {
        ProgTrace(TRACE1, "bad sndAddrNum[%c]: leaving only last 3 bits", sndAddrNum);
        sndAddrNum = (sndAddrNum & 0x7)/*00000111*/ + 0x30;
    }
    hth.remAddrNum = sndAddrNum;

    p = strstr(p + 1, "/P");
    end_p = strchr(p, '\r');

    tst();
    if ((why_p = strchr(p + 1, '/')) && (why_p < end_p))
        p1 = why_p;
    else
        p1 = end_p;

    tst();
    // -2 = "/P"
    if ((len = p1 - p - 2) <= (sizeof(hth.tpr))) {
        memcpy(hth.tpr, p + 2, len);
        hth.tpr[len] = 0;
    } else {
        ProgError(STDLOG, "HTH TPR too long");
        return 0;
    }

    tst();
    if ((p1 != end_p) && (end_p - p1 - 1 == (sizeof(hth.why) - 1))) {
        memcpy(hth.why, p1 + 1, 2);
        hth.why[2] = 0;
    }

    end_p = strchr(end_p + 1, '\r');

    tst();
    if (*(end_p + 1) == stx)      // if STX - skip STX byte
        end_p += 2;
    else
        end_p += 1;

    hth::trace(TRACE1, hth);
    return end_p - tlgBody;
}

void removeSpecSymbols(char* str)
{
    if (!str)
        return;
// remove ETX - end of text special symbol
    size_t len = strlen(str);
    if (str[len - 1] == etx) {
        ProgTrace(TRACE5, "removed ETX from tlgBody");
        str[len - 1] = 0;
    }
}

string toStringOnTerm(const HthInfo& hth)
{
    return StrUtils::replaceSubstrCopy(toString(hth),"\r","\\");
}

string toString(const HthInfo& hth)
{
    char hth_head[MaxHthHeadSize];

    strcpy(hth_head, hthBegStr.c_str()); // V.\rV
    int i = hthBegStr.size();
    hth_head[i++] = hth.type;  //4 QRI1 query / reply / alone
    hth_head[i++] = 'L'; // 0x50;  //5 QRI2 'E' hold protection

    // QRI3
    if (!hth.part)
        hth_head[i++] = 0x47; // 'G'
    else {
        if (hth.end)
            hth_head[i++] = 0x45; // 'E'
        else if (hth.part == 1)
            hth_head[i++] = 0x46; // 'F'
        else
            hth_head[i++] = 0x44; // 'D'
    }
    hth_head[i++] = 0x2E;     //7 QRI4
    hth_head[i++] = hth.qri5;      //8 QRI5
    if(hth.qri6 > 0)
        hth_head[i++] = hth.qri6;      //9 QRI6

// TODO limit max value with 'Z'
    if (hth.part)    // QRI7 'A-Z'
        hth_head[i++] = 'A' + hth.part - 1;
    else if(hth.end)
        hth_head[i++] = 0x2E; // '.'

    ProgTrace(TRACE5, "part: %d, end: %d", hth_head[i - 1], hth_head[i - 5]);

    hth_head[i++] = 0x2F;     //10'/'

    hth_head[i++] = 0x45;     //'E'
    // remAddrNum can be only in ['1' - '7']
    if ((hth.remAddrNum < '1') || (hth.remAddrNum > '7'))
        hth_head[i++] = 0x35; // this is user defined field and we have chosen '5'
    else
        hth_head[i++] = hth.remAddrNum;
    strcpy(&hth_head[i], hth.sender); //Sender
    i += strlen(hth.sender);

    hth_head[i++] = 0x2F;     // '/'

    hth_head[i++] = 0x49;     // 'I'
    if ((hth.remAddrNum < '1') || (hth.remAddrNum > '7'))
        hth_head[i++] = 0x35; // this is user defined field and we have chosen '5'
    else
        hth_head[i++] = hth.remAddrNum;
    strcpy(&hth_head[i], hth.receiver); // Receiver
    i += strlen(hth.receiver);

    hth_head[i++] = 0x2F;     //'/'

    strcpy(&hth_head[i++], "P");
    strcpy(&hth_head[i], hth.tpr); // TPR
    i += strlen(hth.tpr);
    if (*hth.why) {
        hth_head[i++] = 0x2F;    // '/'
        strcpy(&hth_head[i], hth.why);
        i += strlen(hth.why);
    }
    hth_head[i++] = 0x0D;     // '\r'

    hth_head[i++] = 0x56;   // 'V'
    hth_head[i++] = 0x47;   // 'G'
    hth_head[i++] = 0x59;   // 'Y'
    hth_head[i++] = 0x41;   // 'A'
    hth_head[i++] = 0x0D;   // '\r'
    hth_head[i] = 0x00;

    return hth_head;
}


} // namespace hth

#ifdef XP_TESTING

void initHthTests()
{
}

#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>

using namespace hth;

namespace
{

HthInfo dummyHthInfo()
{
    HthInfo hth;
    memset(&hth, 0, sizeof(hth));
    hth.type = 'T';
    strcpy(hth.sender, "Sender");
    strcpy(hth.receiver, "Receiver");
    strcpy(hth.tpr, "Tpr");
    hth.qri5 = '5';
    hth.qri6 = '6';
    hth.remAddrNum = '2';
    return hth;
}
}

START_TEST(parseQri3)
{
    // sample HTH header from MUC2A
    HthInfo msg;
    // part == 0
    fromString("V.\rVDDS.VA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(!msg.part, "part failed");
    fromString("V.\rVDDG.VA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(!msg.part, "part failed");
    fromString("V.\rVDDK.VA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(!msg.part, "part failed");
    fromString("V.\rVDDO.VA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(!msg.part, "part failed");

    // part == 1 -- first part
    fromString("V.\rVDDR.VAA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 1, "part failed");
    fromString("V.\rVDDF.VAA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 1, "part failed");
    fromString("V.\rVDDJ.VAA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 1, "part failed");
    fromString("V.\rVDDN.VAA/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 1, "part failed");

    // part == 2 && end == 1
    fromString("V.\rVDDQ.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fail_unless(msg.end == 1, "end failed");
    fromString("V.\rVDDE.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fail_unless(msg.end == 1, "end failed");
    fromString("V.\rVDDI.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fail_unless(msg.end == 1, "end failed");
    fromString("V.\rVDDM.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fail_unless(msg.end == 1, "end failed");

    // part == 2 -- mid part
    fromString("V.\rVDDP.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fromString("V.\rVDDD.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fromString("V.\rVDDH.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");
    fromString("V.\rVDDL.VAB/E11ARAC/I11HRAC/P7970\rVGYA\r", msg);
    fail_unless(msg.part == 2, "part failed");

    // sample hTh header from AIR FRANCE
    fromString("V.\rVHLG.WA/E1AFUNX1/I11HUNX1/P1001\rVGYA\r", msg);
}END_TEST

START_TEST(hth_header_parse_generate)
{
    HthInfo hth, tstIn, tstOut;
    memset(&hth, 0, sizeof(hth));

    hth.type = 'T';
    strcpy(hth.sender, "Sender");
    strcpy(hth.receiver, "Receiver");
    strcpy(hth.tpr, "Tpr");
    hth.qri5 = '5';
    hth.qri6 = '6';
    hth.remAddrNum = '1';

    string hthStr;

    tstIn = hth;
    hthStr = hth::toString(tstIn);
    memset(&tstOut, 0, sizeof(tstOut));
    hth::fromString(hthStr, tstOut);

    fail_unless(tstIn == tstOut, "part=0, end=0 failed, see logfile for details");

    tstIn = hth;
    tstIn.part = 3;
    hthStr = hth::toString(tstIn);
    memset(&tstOut, 0, sizeof(tstOut));
    hth::fromString(hthStr, tstOut);
    fail_unless(tstIn == tstOut, "part=3, end=0 failed, see logfile for details");

    tstIn = hth;
    tstIn.part = 1;
    tstIn.end = 1;
    hthStr = hth::toString(tstIn);
    hth.type = 'T';
    strcpy(hth.sender, "Sender");
    strcpy(hth.receiver, "Receiver");
    strcpy(hth.tpr, "Tpr");
    hth.qri5 = '5';
    hth.qri6 = '6';
    memset(&tstOut, 0, sizeof(tstOut));
    hth::fromString(hthStr, tstOut);
    fail_unless(tstIn == tstOut, "part=1, end=1 failed, see logfile for details");

    tstIn = hth;
    tstIn.part = 2;
    tstIn.end = 1;
    hthStr = hth::toString(tstIn);
    memset(&tstOut, 0, sizeof(tstOut));
    hth::fromString(hthStr, tstOut);
    fail_unless(tstIn == tstOut, "part=2, end=1 failed, see logfile for details");
}END_TEST

START_TEST(check_createTlg)
{
    char tlg[MAX_TLG_SIZE];
    strcpy(tlg, "test text");
    HthInfo hth = dummyHthInfo();
    createTlg(tlg, hth);
    string hthStr = toString(hth);
#ifdef H2H_STX_ETX
    fail_unless(tlg[hthStr.length()] == stx, "STX failed");
#endif
    // no etx for Sabre
    if (hth.end)
        fail_unless(tlg[strlen(tlg) - 1] == etx, "ETX failed");
    else
        fail_unless(tlg[strlen(tlg) - 1] != etx, "ETX failed");
}END_TEST

START_TEST(check_removeSpecSymbols)
{
    char tlg[MAX_TLG_SIZE];
    strcpy(tlg, "test text");
    HthInfo hth = dummyHthInfo();
    createTlg(tlg, hth);
    string hthStr = toString(hth);

    removeSpecSymbols(tlg + hthStr.length());

    fail_unless(tlg[strlen(tlg) - 1] != etx, "ETX failed");

}END_TEST

START_TEST(check_defaultRemoteAddrNum)
{
    HthInfo hth = dummyHthInfo();
    hth.remAddrNum = 0;
    fromString(toString(hth), hth);
    fail_unless(hth.remAddrNum == '5', "failed default remAddrNum");

    hth.remAddrNum = '8';
    fromString(toString(hth), hth);
    fail_unless(hth.remAddrNum == '5', "failed default remAddrNum");
}END_TEST

#define SUITENAME "Hth"
TCASEREGISTER(0, 0)
{
    ADD_TEST(parseQri3)
    ADD_TEST(hth_header_parse_generate)
    ADD_TEST(check_createTlg)
    ADD_TEST(check_removeSpecSymbols)
    ADD_TEST(check_defaultRemoteAddrNum)
}
TCASEFINISH

#endif // XP_TESTING


