#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>

#include <serverlib/slogger.h>

#ifdef HAVE_CONFIG_H
#endif
#include "express.h"
#include "gateway.h"

namespace telegrams
{
namespace express
{

std::string makeReceiveHeader(int routerNum, int ttl, const tlgnum_t& tlgNum)
{
    // sprintf used because it is MUCH faster then both ostream and boost::format
    static char buff[100] = {};
    if (ttl > 99) {
        LogTrace(TRACE0) << "invalid ttl: " << ttl;
        ttl = 99;
    }
    sprintf(buff, "RECEIVE %s TTL %d FROM %d ", tlgNum.num.get().c_str(), ttl, routerNum);
    return buff;
}

boost::optional<Header> parseReceiveHeader(const char* msg)
{
    if (!msg)
        return boost::none;
    const size_t sz = strnlen(msg, 40);
    if (sz < 9/*minsize*/) {
        return boost::none;
    }
    std::string msgStr(msg, sz);
    boost::smatch what;
    if (boost::regex_match(msgStr, what, boost::regex("(RECEIVE ([0-9]{1,10}) TTL ([0-9]{1,2}) FROM ([0-9]{1,9}) ).*"))) {
        tlgnum_t tlgNum = tlgnum_t(what[2].str());
        tlgNum.express = true;
        Header header(tlgNum);
        header.ttl = boost::lexical_cast<int>(what[3]);
        header.routerNum = boost::lexical_cast<int>(what[4]);
        header.size = what[1].str().size();
        return header;
    }
    else
    {
        LogTrace(TRACE1) << __FUNCTION__ << " failed: " << msgStr;
        return boost::none;
    }
}

std::string makeSendHeader(int routerNum, int ttl, const tlgnum_t& tlgNum)
{
    // sprintf used because it is MUCH faster then both ostream and boost::format
    static char buff[100] = {};
    if (ttl > 99) {
        LogTrace(TRACE0) << "invalid ttl: " << ttl;
        ttl = 99;
    }
    sprintf(buff, "SENDTO %d TTL %d #%s ", routerNum, ttl, tlgNum.num.get().c_str());
    return buff;
}
boost::optional<Header> parseSendHeader(const char* msg)
{
    static boost::regex rex("^SENDTO ([0-9]{1,9}) TTL ([0-9]{1,2}) #([0-9]{1,10}) .*");

    // example: [SENDTO 123 TTL 1 #1 HELLO]
    if (!msg)
        return boost::none;
    std::string msgStart = std::string(msg, strnlen(msg, 37));
    LogTrace(TRACE5) << "msgStart [" << msgStart << "]";
    boost::smatch what;
    if (boost::regex_match(msgStart, what, rex)) {
        LogTrace(TRACE5) << what[1].str() << "|" << what[2].str() << "|" << what[3].str();
        const std::string rtrStr = what[1].str();
        const std::string ttlStr = what[2].str();
        const std::string tlgNumStr = what[3].str();
        tlgnum_t tlgNum = tlgnum_t(tlgNumStr);
        Header header(tlgNum);
        header.ttl = boost::lexical_cast<int>(ttlStr);
        header.routerNum = boost::lexical_cast<int>(rtrStr);
        const size_t headerLen = (7 + rtrStr.length() + 5 + ttlStr.length() + 2 + tlgNumStr.length() + 1);
        header.size = headerLen;
        LogTrace(TRACE5) << "headerLen=" << headerLen << " ttl=" << header.ttl
            << " routerNum=" << header.routerNum << " tlgNum=" << tlgNum;
        return header;
    } else {
        LogTrace(TRACE1) << __FUNCTION__ << " failed: " << msgStart;
        return boost::none;
    }
}

boost::optional<ExpressMessage> parseExpressMessage(const char* buff, size_t buffSz)
{
    boost::optional<telegrams::express::Header> header = telegrams::express::parseReceiveHeader(buff);
    if (!header || (header->size >= buffSz)) {
        ProgTrace(TRACE0, "bad msg format: %.30s", buff);
        return boost::optional<ExpressMessage>();
    }

    ExpressMessage msg(header->tlgNum);
    msg.ttl = header->ttl;
    msg.routerNum = header->routerNum;

    char tlgText[MAX_TLG_SIZE] = {};
    hth::HthInfo hthInfo = {};
    const size_t hthHeadLen = hth::fromString(buff + header->size, hthInfo);
    if (hthHeadLen > 0) {
        hth::trace(TRACE5, hthInfo);
        strncpy(tlgText, buff + header->size + hthHeadLen, buffSz - header->size - hthHeadLen);
        hth::removeSpecSymbols(tlgText);
        msg.hthInfo = hthInfo;
    } else {
        strncpy(tlgText, buff + header->size, buffSz - header->size);
    }
    msg.tlgText = tlgText;
    LogTrace(TRACE5) << "routerNum " << msg.routerNum << " tlgNum " << msg.tlgNum
        << " [" << msg.tlgText << "]";
    return msg;
}

}//namespace express
}//namespace telegrams


#ifdef XP_TESTING
void initExpressTests()
{}

#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>

namespace
{
void startTests()
{
}
void finishTests()
{
}

#define check_parseReceiveHeader_(text_, len_, ttl_, routerNum_, tlgNum_) { \
    boost::optional<telegrams::express::Header> header = parseReceiveHeader(text_); \
    fail_unless(header && true, "failed to parse: %s", text_); \
    ck_assert_int_eq(header->size, len_); \
    ck_assert_int_eq(header->routerNum, routerNum_); \
    fail_unless(header->tlgNum == tlgNum_, "failed tlgnum"); \
    ck_assert_int_eq(header->ttl, ttl_); \
}
START_TEST(check_parseReceiveHeader)
{
    using telegrams::express::parseReceiveHeader;
    fail_unless(!parseReceiveHeader("Hello world"));
    check_parseReceiveHeader_("RECEIVE 12 TTL 2 FROM 123 LOLKA", 26, 2, 123, tlgnum_t("12"));
    check_parseReceiveHeader_("RECEIVE 1 TTL 43 FROM 3 L", 24, 43, 3, tlgnum_t("1"));
    check_parseReceiveHeader_("RECEIVE 9999 TTL 0 FROM 1234567 LOLKA", 32, 0, 1234567, tlgnum_t("9999"));
    check_parseReceiveHeader_("RECEIVE 56449118 TTL 1 FROM 4 V.\rVHLG.WA/E5TSTDISTR/I5TSTINV/PJXMM\rVGYA\rUNB+IATA:1+DT+U6+100215:1243+000002JXMM'UNH+1+PAOREQ:96:2:IA+000002JXMM'MSG+:186'ORG+1H:MOW+:01åéÇ+BER++T+DE:RUB:RU+ÉÇñ1415+åéÇÅéä'ODI+MOW+LED'TVL+150210+SVO+LED+U6+9999+15+1+P'UNT+6+1'UNZ+1+000002JXMM'", 30, 1, 4, tlgnum_t("56449118"));
    check_parseReceiveHeader_("RECEIVE 149 TTL 12 FROM 5 V.\rVHLG.WA/E5TSTDISTR/I5TSTINV/PJZTG\rVGYA\rUNB+IATA:1+DT+YI+100218:1053+000002JZTG'UNH+1+PAOREQ:96:2:IA+000002JZTG'MSG+:186'ORG+1H:MOW+:AIRIM+MOW++T+RU:RUB:RU+ÉÇñ1111+åéÇÖÇÉ'ODI+MOW+LED'TVL+180210+SVO+LED+YII+1+33+1+P'UNT+6+1'UNZ+1+000002JZTG'21+2+P'UNT+7+1'UNZ+1+000002JZT8'", 26, 12, 5, tlgnum_t("149"));
}END_TEST

#define check_parseSendHeader_(text_, len_, ttl_, routerNum_, tlgNum_) { \
    boost::optional<telegrams::express::Header> header = parseSendHeader(text_); \
    fail_unless(header && true, "failed to parse: %s", text_); \
    ck_assert_int_eq(header->size, len_); \
    ck_assert_int_eq(header->routerNum, routerNum_); \
    fail_unless(header->tlgNum == tlgNum_, "failed tlgnum"); \
    ck_assert_int_eq(header->ttl, ttl_); \
}
START_TEST(check_parseSendHeader)
{
    using telegrams::express::parseSendHeader;
    fail_unless(!parseSendHeader("Hello world"));
    check_parseSendHeader_("SENDTO 123 TTL 1 #932 LOLKA", 22, 1, 123, tlgnum_t("932"));
    check_parseSendHeader_("SENDTO 3 TTL 0 #32 L", 19, 0, 3, tlgnum_t("32"));
    check_parseSendHeader_("SENDTO 1234567 TTL 12 #1017056439 LOLKA", 34, 12, 1234567, tlgnum_t("1017056439"));
}END_TEST

START_TEST(check_parse_make)
{
    using namespace telegrams::express;
    { // max values
        const int defRouterNum = 9999, defTtl = 123;
        const tlgnum_t defTlgNum = tlgnum_t("2147483609");
        {
            std::string hdr = makeSendHeader(defRouterNum, defTtl, defTlgNum);
            const int len = hdr.length();
            hdr += "TEXT TEXT TEXT";
            LogTrace(TRACE5) << "hdr: " << hdr;
            check_parseSendHeader_(hdr.c_str(), len, 99, defRouterNum, defTlgNum);
        }
        {
            std::string hdr = makeReceiveHeader(defRouterNum, defTtl, defTlgNum);
            const int len = hdr.length();
            hdr += "TEXT TEXT TEXT";
            LogTrace(TRACE5) << "hdr: " << hdr;
            check_parseReceiveHeader_(hdr.c_str(), len, 99, defRouterNum, defTlgNum);
        }
    }
    { // min values
        const int defRouterNum = 1, defTtl = 3;
        const tlgnum_t defTlgNum = tlgnum_t("2");
        {
            std::string hdr = makeSendHeader(defRouterNum, defTtl, defTlgNum);
            const int len = hdr.length();
            hdr += "TEXT TEXT TEXT";
            LogTrace(TRACE5) << "hdr: " << hdr;
            check_parseSendHeader_(hdr.c_str(), len, defTtl, defRouterNum, defTlgNum);
        }
        {
            std::string hdr = makeReceiveHeader(defRouterNum, defTtl, defTlgNum);
            const int len = hdr.length();
            hdr += "TEXT TEXT TEXT";
            LogTrace(TRACE5) << "hdr: " << hdr;
            check_parseReceiveHeader_(hdr.c_str(), len, defTtl, defRouterNum, defTlgNum);
        }
    }

}END_TEST

#define SUITENAME "libtlg::express"
TCASEREGISTER(startTests, finishTests)
{
    SET_TIMEOUT(30);
    ADD_TEST(check_parseReceiveHeader);
    ADD_TEST(check_parseSendHeader);
    ADD_TEST(check_parse_make);
}
TCASEFINISH

}

#endif // XP_TESTING
