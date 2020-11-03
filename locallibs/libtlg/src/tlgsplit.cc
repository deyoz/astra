#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <serverlib/cursctl.h>
#include <serverlib/savepoint.h>

#ifdef HAVE_CONFIG_H
#endif

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

#include "types.h"
#include "telegrams.h"
#include "tlgsplit.h"
#include "tlgnum.h"
#include "gateway.h"

#define MAX_TLG_PART_NUMS 99
#if MAX_TLG_LEN/MAX_TLG_SIZE>(MAX_TLG_PART_NUMS-1)
#error max MAX_TLG_PART_NUMS parts allowed
#endif

//TODO delTlg move upper
namespace telegrams
{

size_t MIN_PART_LEN = 25;

namespace
{

/**
 * only for tests
 * */
static size_t splitterPartSize = 0;
static const size_t MaxPartNums = MAX_TLG_PART_NUMS;

class BaseSplitter
{
public:
    BaseSplitter(const std::string& name, size_t partSize)
        : m_partSize(partSize), m_name(name)
    {}
    virtual ~BaseSplitter()
    {}
    int split(const std::string& tlgText, tlg_text_filter filter,
            tlgnums_t& nums, const tlgnum_t& msgId) const
    {
        int res = 0;
        if (tlgText.size() > m_partSize) {
            int last_part = _split(tlgText, filter, nums, msgId);
            if (last_part < 0) {
                LogError(STDLOG) << "split failed: " << msgId;
                return -1;
            }

            if (nums.size() > MaxPartNums) {
                LogError(STDLOG) << "too long tlg " << msgId << ", parts " << nums.size()
                    << ", allowed only " << MaxPartNums << " parts";
                return -1;
            }

            if (last_part > 0) {
                std::stringstream ss;
                for(const tlgnum_t& num:  nums) {
                    ss << " " << num;
                }
                LogTrace(TRACE1) << m_name << ": splitted tlg(" << msgId << ") onto "
                    << nums.size() << " parts: " << ss.str();
                res = last_part;
            } else
                res = last_part;
        }
        return res;
    }
protected:
    virtual int _split(const std::string&, tlg_text_filter, tlgnums_t&, const tlgnum_t&) const = 0;

    void checkPartSize(size_t defaultPartSize, size_t maxPartSize)
    {
        if (m_partSize > maxPartSize) {
            ProgError(STDLOG, "%s: m_partSize(%zd) > maxPartSize(%zd)", m_name.c_str(), m_partSize, maxPartSize);
            m_partSize = defaultPartSize;
            ProgError(STDLOG, "%s: using default partSize(%zd)", m_name.c_str(), m_partSize);
        } else if (m_partSize <= (2 * MIN_PART_LEN)) {
            ProgError(STDLOG, "%s: m_partSize(%zd) <= 2*MIN_PART_LEN(%zd)", m_name.c_str(), m_partSize, 2*MIN_PART_LEN);
            m_partSize = defaultPartSize;
            ProgError(STDLOG, "%s: using default partSize(%zd)", m_name.c_str(), m_partSize);
        }
        // only for tests
        splitterPartSize = m_partSize;
    }
    size_t m_partSize;
    std::string m_name;
};

class HthSplitter
    : public BaseSplitter
{
public:
    HthSplitter(size_t partSize, const hth::HthInfo& hth)
        : BaseSplitter("HthSplitter", partSize), h2h_(hth)
    {
        const size_t maxHthBodySize = (MAX_TLG_SIZE - MAX_HTH_HEAD_SIZE);
        checkPartSize(RouterInfo::defaultMaxHthPartSize, maxHthBodySize);
    }
    virtual ~HthSplitter(){}

    virtual int _split(const std::string& tlgText, tlg_text_filter filter, tlgnums_t& nums, const tlgnum_t& msg_id) const
    {
        LogTrace(TRACE5) << m_name << " for tlg " << msg_id << " current part size: " << m_partSize;
        const char* p = tlgText.c_str();
        int bodyparts = 0;

        const size_t bodylen = tlgText.size();
        if ((bodyparts = bodylen / m_partSize + 1) < 2)
            bodyparts = 2;
        size_t bodypartlen = bodylen / bodyparts;

        const int shiftlen = bodypartlen - 2 * MIN_PART_LEN;
        LogTrace(TRACE5) << "Tlg_num " << msg_id << ": shiftlen=" << shiftlen;

        int shift = 0;
        bool splitFinished = false;
        hth::HthInfo hthInfo(h2h_); //const constrained
        for (int i = 1; !splitFinished; p += shift, ++i) {
            std::string curPartText;
            hthInfo.part = i;

            if (strlen(p) < bodypartlen - MIN_PART_LEN) {
                ProgTrace(TRACE5, "PART %d END", i);
                curPartText = p;
                hthInfo.end = 1;

                splitFinished = true;
            } else {
                ProgTrace(TRACE5, "PART %d CONTINUED", i);
                shift = shiftlen;
                for (int j = shiftlen; j == 0; --j) {
                    if (*(p+j) == '\n') {
                        shift = j;
                        break;
                    }
                }
                ProgTrace(TRACE5, "current shift: %d", shift);
                curPartText.assign(p, shift);
                hthInfo.end = 0;
            }

            Expected<TlgResult, int> result = callbacks()->putTlg(curPartText, filter, 0, 0, &hthInfo);
            if (!result) {
                ProgError(STDLOG, "HthSplitter: putTlg failed ret=%d", result.err());
                return -1;
            }
            nums.push_back(result->tlgNum);
        }
        callbacks()->gatewayQueue().delTlg(msg_id);
        LogTrace(TRACE5) << "HthSplitter: msg " << msg_id << " deleted - finishing";
        return nums.size();
    }

private:
    hth::HthInfo h2h_;
};

class TypeBSplitter
    : public BaseSplitter
{
public:
    TypeBSplitter(size_t partSize, bool trueTpb)
        : BaseSplitter("TypeBSplitter", partSize), m_trueTpb(trueTpb)
    {
        const size_t maxTypeBBodySize = (MAX_TLG_SIZE - MIN_PART_LEN);
        checkPartSize(RouterInfo::defaultMaxTypeBPartSize, maxTypeBBodySize);
    }
    virtual ~TypeBSplitter() {}

    virtual int _split(const std::string& tlgText, tlg_text_filter filter, tlgnums_t& nums,
            const tlgnum_t& msg_id) const
    {
        LogTrace(TRACE5) << m_name << ": " << msg_id;

        const char* p = NULL;
        if ((p = strchr(tlgText.c_str(), '.'))) {
            p = strchr(p, '\n');
            if (p) {
                if (strncmp(p + 1, "PDM\n", 5) == 0)
                    p += 4;
            }
        }

        if (!p) {
            LogError(STDLOG) << m_name << " " << msg_id << " head not found";
            return -1;
        }

        const int headLength = p - tlgText.c_str() + 1;
        const std::string tlgHead(tlgText, 0, headLength);
        const size_t bodylen = tlgText.size() - headLength;
        int bodyparts;
        if ((bodyparts = bodylen / (m_partSize - headLength) + 1) < 2)
            bodyparts = 2;
        const size_t bodypartlen = bodylen / bodyparts;
        const int maxShift = bodypartlen - 2 * MIN_PART_LEN;

        LogTrace(TRACE5) << "Tlg_num " << msg_id << " tlgHead=" << tlgHead << " maxShift=" << maxShift;

        bool splitFinished = false;
        p = tlgText.c_str() + headLength;
        int shift = 0;
        for (int i = 1; !splitFinished;p += shift, ++i) {
            std::string curPartText(tlgHead);

            if (strlen(p) < bodypartlen - MIN_PART_LEN) {
                ProgTrace(TRACE5, "PART %d END", i);

                if (m_trueTpb) {
                    curPartText += p;
                    curPartText += (boost::format("\nPART %d END\n") % i).str();
                } else {
                    curPartText += (boost::format("PART %d END\n") % i).str();
                    curPartText += p;
                }
                splitFinished = true;
            } else {
                ProgTrace(TRACE5, "PART %d CONTINUED", i);

                for (shift = maxShift; *(p + shift) != '\n'; --shift);
                shift++; // skip '\n'

                if (m_trueTpb) {
                    curPartText += std::string(p, shift);
                    curPartText += (boost::format("\nPART %d CONTINUED\n") % i).str();
                } else {
                    curPartText += (boost::format("PART %d CONTINUED\n") % i).str();
                    curPartText += std::string(p, shift);
                }
            }

            Expected<TlgResult, int> result = callbacks()->putTlg(curPartText, filter);
            if (!result) {
                ProgError(STDLOG, "SplitTlgTPB: TlgDeliv failed ret=%d", result.err());
                return -1;
            }
            nums.push_back(result->tlgNum);
        }
        callbacks()->gatewayQueue().delTlg(msg_id);
        LogTrace(TRACE5) << "SplitTlgTPB: msg " << msg_id << " deleted - finishing";
        return nums.size();
    }
private:
    bool m_trueTpb;
};

class CommonSplitter
    : public BaseSplitter
{
public:
    CommonSplitter(size_t partSize)
        : BaseSplitter("CommonSplitter", partSize)
    {
        const size_t maxTlgBodySize = MAX_TLG_SIZE;
        checkPartSize(RouterInfo::defaultMaxTypeBPartSize, maxTlgBodySize);
    }
    virtual ~CommonSplitter() {}

    virtual int _split(const std::string& tlgText, tlg_text_filter filter, tlgnums_t& nums,
            const tlgnum_t& msg_id) const
    {
        LogTrace(TRACE5) << m_name << ": split tlg: " << msg_id;
        const char* p = tlgText.c_str();
        const int shiftlen = m_partSize - 2 * MIN_PART_LEN;
        bool splitFinished = false;
        for (int i = 1; !splitFinished; p += shiftlen, ++i) {
            std::string curPartText;
            if (strlen(p) < /*MAX_TLG_SIZE*/ m_partSize - MIN_PART_LEN) {
                ProgTrace(TRACE5, "PART %d END", i);
                curPartText = p;
                curPartText += (boost::format("\nPART %d END\n") % i).str();
                splitFinished = true;
            } else {
                ProgTrace(TRACE5, "PART %d CONTINUED", i);
                curPartText.assign(p, shiftlen);
                curPartText += (boost::format("\nPART %d CONTINUED\n") % i).str();
            }
            boost::optional<tlgnum_t> msg_id_part;
            Expected<TlgResult, int> result = callbacks()->putTlg(curPartText, filter);
            if (!result) {
                ProgError(STDLOG, "SplitTlg: putTlg failed ret=%d", result.err());
                return -1;
            }
            nums.push_back(result->tlgNum);
        }
        callbacks()->gatewayQueue().delTlg(msg_id);
        LogTrace(TRACE5) << "SplitTlg: msg " << msg_id << " deleted - finishing";
        return nums.size();
    }
};

const std::string END_PNL_TAG = "ENDPNL";
const std::string END_ADL_TAG = "ENDADL";
const std::string END_PART_TAG = "ENDPART%d\n";

class PnlSplitter
    : public BaseSplitter
{
public:

    typedef std::vector<std::string> StringArray;

    PnlSplitter(size_t partSize)
        : BaseSplitter("PnlSplitter", partSize), partNum(1)
    {
    }

    virtual ~PnlSplitter() {}

    virtual int _split(const std::string& tlgText, tlg_text_filter filter, tlgnums_t& nums,
            const tlgnum_t& msg_id) const
    {
        LogTrace(TRACE5) << "PnlSplitter: " << m_name << ": split tlg: " << msg_id << " part size " << m_partSize;
        LogTrace(TRACE5) << "split telegram " << tlgText;
        std::istringstream is(tlgText);

        std::string address = readLine(is);
        std::string communicRef = readLine(is);
        std::string messId = readLine(is);
        std::string ana;
        if (isAdl(messId)) {
            char buff[20] = {};
#ifdef XP_TESTING
            const int secs = 123456;
#else // XP_TESTING
            const int secs = boost::posix_time::second_clock::local_time().time_of_day().total_seconds();
#endif // XP_TESTING
            snprintf(buff, sizeof(buff), "ANA/%03d\n", secs);
            ana = buff;
        }

        std::string flight = readLine(is);
        std::size_t pos = flight.find("PART1");
        if (pos == std::string::npos) {
            return -1;
        }
        flight.erase(pos);
        flight.append("PART%d\n");

        std::string part;
        std::string line;
        std::string totalsByDst;
        std::string adlType;
        bool startPassBlock = false;

        const std::string header = address + communicRef + messId;
        std::string telegram = header + getPartText(flight) + ana;
        bool afterTBD = false;
        while (!is.eof()) {
            line = readLine(is);
            bool isCmd = line == "DEL\n" || line == "CHG\n" || line == "ADD\n";
            if (*line.begin() == '-') {
                appendPart(telegram, part, header + flight + ana + totalsByDst + adlType, filter, nums);
                totalsByDst = line;
                appendPart(telegram, line, header + flight + ana + totalsByDst + adlType, filter, nums);
                afterTBD = true;
                continue;
            } else if (std::isdigit(*line.begin())) {
                appendPart(telegram, part, header + flight + ana + totalsByDst + adlType, filter, nums);
                startPassBlock = true;
            } else if (isAdl(messId) && (afterTBD || isCmd)) {
                if (!part.empty()) {
                    appendPart(telegram, part, header + flight + ana + totalsByDst + adlType, filter, nums);
                }
                adlType = line;
                afterTBD = false;
                appendPart(telegram, line, header + flight + ana + totalsByDst + adlType, filter, nums);
                continue;
            }

            if (startPassBlock) {
                part += line;
            } else {
                appendPart(telegram, line, header + flight + ana + totalsByDst + adlType, filter, nums);
            }
        }
        if (!putTelegram(telegram + part, filter, nums)) {
            return -1;
        }

        callbacks()->gatewayQueue().delTlg(msg_id);
        LogTrace(TRACE5) << "PnlSplitTlg: msg " << msg_id << " deleted - finishing";
        return nums.size();
    }

private:

    void appendPart(std::string& telegram
                    , std::string& part
                    , const std::string header
                    , tlg_text_filter filter
                    , tlgnums_t& nums) const {
        LogTrace(TRACE5) << "appendPart write size = " << part.size() << ": " << part;

        if (part.substr(0, END_PNL_TAG.size()) == END_PNL_TAG || part.substr(0, END_ADL_TAG.size()) == END_ADL_TAG) {
            telegram += part;
            return;
        }

        std::string endPartTag = getPartText(END_PART_TAG);
        std::size_t expectedTelegramSize = telegram.size() + part.size() + endPartTag.size();

        LogTrace(TRACE5) << "expectedTelegramSize " << expectedTelegramSize
                         << " m_partSize = " << m_partSize;
        if (expectedTelegramSize > m_partSize) {
            telegram += endPartTag;
            ++partNum;

            LogTrace(TRACE5) << "Put telegram with size " << telegram.size()
                             << " : " << telegram  << " New partnum = " << partNum;

            if (!putTelegram(telegram, filter, nums)) {
                return;
            }

            telegram = getPartText(header);
            telegram += part;
        } else {
            telegram += part;
        }
        part.clear();
    }

    const std::string readLine(std::istream& is) const {
        std::string line;
        std::getline(is, line);
        line.append("\n");
        LogTrace(TRACE5) << "Item is: "<< line;
        return line;
    }

    const std::string getPartText(const std::string& formatStr) const {
        boost::format fmt(formatStr);
        return (fmt % partNum).str();
    }

    bool isAdl(const std::string& messageId) const {
        return messageId == "ADL\n";
    }

    bool putTelegram(const std::string& telegram, tlg_text_filter filter, tlgnums_t& nums) const {
        boost::optional<tlgnum_t> msg_id_part;
        Expected<TlgResult, int> result = callbacks()->putTlg(telegram, filter);
        if (!result) {
            ProgError(STDLOG, "NewSplitTlg: putTlg failed ret=%d", result.err());
            return false;
        }
        nums.push_back(result->tlgNum);
        return true;
    }

    mutable std::size_t partNum;
};

BaseSplitter* createSplitter(const RouterInfo& ri, bool isEdifact, const hth::HthInfo* const h2h, bool isPnlAdl)
{
    if (h2h) {
        ProgTrace(TRACE5, "Using SplitterHth");
        return new HthSplitter(ri.max_hth_part_size, *h2h);
    } else if (isEdifact) {
        ProgTrace(TRACE5, "Using SplitterCommon");
        return new CommonSplitter(ri.max_part_size);
    } else if (isPnlAdl) {
        ProgTrace(TRACE5, "Using PnlSplitter");
        return new PnlSplitter(ri.max_typeb_part_size);
    } else if (ri.tpb) {
        ProgTrace(TRACE5, "Using SplitterTypeB");
        return new TypeBSplitter(ri.max_typeb_part_size, ri.true_tpb);
    }

    ProgTrace(TRACE5, "Using SplitterCommon");
    return new CommonSplitter(ri.max_part_size);
}

}

/**
 * @brief only for tests
 * @return message part size
 * */
size_t getSplitterPartSize()
{
    return splitterPartSize;
}

int split_tlg(const tlgnum_t& msgId, tlg_text_filter filter, const hth::HthInfo* const h2h,
              int router_num, bool edi_tlg, const std::string& tlgText, tlgnums_t& nums)
{
    LogTrace(TRACE5) << "split_tlg for tlg " << msgId << " size " << tlgText.size();

    RouterInfo ri;
    if (router_num > 0) {
        callbacks()->getRouterInfo(router_num, ri);
    }
    LogTrace(TRACE5) << "TLG IS " << (h2h ? "" : "NOT ") << "H2H";

    bool isPnlAdl = (tlgText.find("ENDPNL") != std::string::npos
                    || tlgText.find("ENDADL") != std::string::npos)
                    && tlgText.find(" PART1") != std::string::npos;
    boost::scoped_ptr<BaseSplitter> spl(createSplitter(ri, edi_tlg, h2h, isPnlAdl));

    callbacks()->savepoint("before_split");
    int res = spl->split(tlgText, filter, nums, msgId);

    if (res < 0) {
        callbacks()->rollback("before_split");
        callbacks()->splitError(msgId);
        LogError(STDLOG) << "Failed splitting tlg " << msgId
                         << " - rolling back splitting and moving tlg to REPEAT queue";
        res = -1;
    }

    return res;
}


} // namespace telegrams

