#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <cstdio>
#include <list>
#include <string>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/int_parameters_oci.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

#include "telegrams.h"
#include "types.h"
#include "tlgsplit.h"

#warning REFACTORING NEEDED: combine algorithm is rather suspicious

using telegrams::callbacks;


/* Формирует запись в AIR_Q_PART.
   Возвращает
   -1 - ошибка
   -2 - телеграмма не является чьей-либо частью
   1  - сформировалась полная телеграмма
      В этом случае грохаются ставшие ненужными записи в AIR_Q_PART
   и в AIR_Q.
   0  - нормально
*/
namespace
{
static bool emptyRow(const std::string& txt)
{
    std::string lastRow(txt);
    boost::algorithm::trim(lastRow);
    return (lastRow == "") ? true : false;
}
enum SimpleAirimpJoinerModes {Header, FlightElement, RepetitiveElements, TlgBody};

}

namespace telegrams
{

SimpleAirimpJoiner::SimpleAirimpJoiner(const std::string& tlgType, const std::list<std::string>& repetitiveHeaderStrings)
    : tlgType_(tlgType), repetitiveHeaderStrings_(repetitiveHeaderStrings)
{}

bool SimpleAirimpJoiner::combineParts(std::string& fullText, const parts_t& parts)
{
    int number = 0;
    for(const std::string& part:  parts) {
        std::string tmpPart(part);
        preparePart(tmpPart, number == 0);
        if (number)
            fullText += '\n';
        fullText += tmpPart;
        ++number;
    }
    return true;
}

bool SimpleAirimpJoiner::isRepetetiveElement(const std::string& txt)
{
    for(const std::string& str:  repetitiveHeaderStrings_) {
        if (boost::regex_match(txt, boost::regex(str))) {
            ProgTrace(TRACE5, "found RepetitiveElement: %s", txt.c_str());
            return true;
        }
    }
    return false;
}

void SimpleAirimpJoiner::preparePart(std::string& txt, bool firstPart)
{
    std::vector<std::string> rows;
    boost::split(rows, txt, boost::algorithm::is_any_of("\n"));
    if (rows.size() < 4) {
        ProgError(STDLOG, "ill-formed TypeB message: only %zd rows", rows.size());
        return;
    }
    SimpleAirimpJoinerModes mode = Header;
    std::vector<std::string> tmpRows;
    if (!firstPart) {
        for(const std::string& row:  rows) { // skip header
            ProgTrace(TRACE5, "mode=%d row=[%s]", mode, row.c_str());
            switch (mode) {
            case Header:
                if (StrUtils::trim(row) == tlgType_) {
                    mode = FlightElement;
                }
                break;
            case FlightElement:
                mode = RepetitiveElements;
                break;
            case RepetitiveElements:
                if (!isRepetetiveElement(row)) {
                    tmpRows.push_back(row);
                    mode = TlgBody;
                }
                break;
            default:
                tmpRows.push_back(row);
                break;
            }
        }
    } else {
        tmpRows = rows;
    }
    if (tmpRows.empty()) {
        ProgError(STDLOG, "ill-formed TypeB message: [%s] not found", tlgType_.c_str());
        return;
    }
    while (emptyRow(tmpRows[tmpRows.size() - 1])) { // cut off all newlines at the end of message
        tmpRows.erase(--tmpRows.end());
    }

    if (tmpRows.empty()) {
        ProgError(STDLOG, "ill-formed TypeB message: [%s] not found", tlgType_.c_str());
        return;
    }
    if (!isEnd(tmpRows[tmpRows.size() - 1])) { // cut off every last row except ENDETL/PFS/etc
        tmpRows.erase(--tmpRows.end());
    }
    txt = boost::join(tmpRows, "\n");
    ProgTrace(TRACE5, "last char=%c", txt[txt.length() - 1]);
}

int SimpleAirimpJoiner::isEnd(const std::string& txt)
{
    return (StrUtils::trim(txt) == std::string("END") + tlgType_) ? 1 : 0;
}

namespace
{

typedef std::list<std::string> parts_t;

using hth::HthInfo;

class TlgJoiner
{
public:
    TlgJoiner(const std::string& nm, int rtr)
        : m_name(nm), m_rtr(rtr)
    {}
    virtual ~TlgJoiner()
    {}
    int join(const char* tlgText, const INCOMING_INFO& ii_, tlgnum_t& localMsgId, const tlgnum_t& remoteMsgId)
    {
        LogTrace(TRACE5) << m_name << " joining tlg " << localMsgId;

        std::string fullTlgText(tlgText);

        INCOMING_INFO ii = {};
        JoinResult res = joinParts(fullTlgText, m_rtr, localMsgId, remoteMsgId);
        switch (res) {
        case WasJoined: {
                LogTrace(TRACE1) << m_name << " message joined";
                ii = getIncomingInfo(ii_);
                // TODO
                Expected<TlgResult, int> result = callbacks()->putTlg(fullTlgText.c_str(), tlg_text_filter(),
                        ii.router, 0, ii.hthInfo ? &ii.hthInfo.get() : 0);
                if (!result) {
                    ProgError(STDLOG, "writeTlg failed: ret=%d", result.err());
                    return WRITE_LONG_ERR;
                }
                localMsgId = result->tlgNum;
                break;
            }
        case IsPart:
            LogTrace(TRACE1) << m_name << ": part of smth";
            return 0;
            break;
        case IsWhole:
            LogTrace(TRACE5) << m_name << " Alone msg - do nothing";
            break;
        default:
            LogError(STDLOG) << m_name << " joinParts failed with code " << res;
            return WRITE_PART_ERR;
        }
        return 1;
    }
protected:
    enum JoinResult {JoinFailed = -1, IsWhole = 1, IsPart = 2, WasJoined = 3};
    int router() const { return m_rtr; }
    const std::string& name() const { return m_name; }

    virtual INCOMING_INFO getIncomingInfo(const INCOMING_INFO& ii_)
    {
        return ii_;
    }
    virtual bool processHeader(int& partnum, bool& endFlag, bool& needCombineFlag, std::string& fullText) = 0;
    /* Объединяет в единое целое кусочки телеграммы,
       первый из которых имеет номер msgIdStart.
       Пишет результат по указателю txt
       (соответственно, предполагается,
       что память выделена снаружи)
       Стирает ненужные теперь записи
       в таблицах AIR_Q_PART, AIR_Q и TEXT_TLG.

       Возвращает 0 при успехе.
    */
    virtual int combineParts(std::string& tlgText, parts_t& parts) = 0;
    virtual bool isEndPart(bool endFlag) const { return endFlag; }
    virtual std::string getMsgUid() const = 0;
    JoinResult joinParts(std::string& fullText, int rtr, const tlgnum_t& localMsgId, const tlgnum_t& remoteMsgId)
    {
        int partnum = 0;
        bool endFlag = false,
             needCombineFlag = false;
        ProgTrace(TRACE5, "fullText: begin=[%20.20s], end=[%20.20s]",
            fullText.c_str(), fullText.c_str() + fullText.size() - 20);
        if (!processHeader(partnum, endFlag, needCombineFlag, fullText)) {
            LogError(STDLOG) << "for tlg " << localMsgId << "(" << remoteMsgId << ") failed processHeader";
            return JoinFailed;
        }

        if (!partnum) {
            LogTrace(TRACE5) << "tlg " << localMsgId << " is complete and so is not need to be joined";
            return IsWhole;
        }
        LogTrace(TRACE5) << "part (" << partnum << "," << endFlag << ") of splitted tlg. localMsgId: "
            << localMsgId << ", remoteMsgId: " << remoteMsgId;

        LogTrace(TRACE5) << "needCombineFlag = " << needCombineFlag;

        std::string msgUid = getMsgUid();

        if (needCleanMsgParts(partnum))
            cleanMsgParts(rtr, msgUid);

        if (!addTlgPart(localMsgId, msgUid, partnum, isEndPart(endFlag)))
            return JoinFailed;

        if (needCombineFlag) {
            ProgTrace(TRACE5, "end flag found in tlg");

            // checking if amount of saved parts is equal to end tlg partnumber
            if (allPartsPresent(partnum, msgUid)) {
                parts_t parts;
                if (readAllParts(parts, rtr, msgUid) < 0) {
                    LogError(STDLOG) << m_name << ": readAllParts failed: router "
                        << m_rtr << " msgUid " << msgUid;
                    return JoinFailed;
                }
                removeParts(msgUid);
                // all parts are supposed to be recieved. trying to combine them
                ProgTrace(TRACE5, "combineParts: router %d, msgUid %s", rtr, msgUid.c_str());
                fullText.clear();
                if (combineParts(fullText, parts) < 0) {
                    LogError(STDLOG) << m_name << ": combineParts failed";
                    for(const std::string& partText:  parts) {
                        LogTrace(TRACE1) << partText;
                    }
                    return JoinFailed;
                }
                return WasJoined;
            } else {
                if(endFlag) {
                    removeParts(msgUid);
                    LogError(STDLOG) << m_name << " not all parts for tlg " << localMsgId << " - router "
                                     << m_rtr << ", msgUid " << msgUid;
                    return JoinFailed;
                }
            }

        }
        return IsPart;
    }
    virtual bool needCleanMsgParts(int partnum) {
        return (partnum == 1);
    }

    int readAllParts(parts_t& parts, int rtr, const std::string& msgUid) {
        const std::list<tlgnum_t> tlgnums(callbacks()->partsReadAll(rtr, msgUid));
        std::string tlgPartText;
        for(const tlgnum_t& msgId:  tlgnums) {
            if (callbacks()->readTlg(msgId, tlgPartText) < 0) {
                LogError(STDLOG) << name() << ": readTlg failed, tlg " << msgId;
                return -1;
            }
            parts.push_back(tlgPartText);            
        }
        return 0;
    }
    /**
     * @return true when all parts are present
     */
    virtual bool allPartsPresent(int nParts, const std::string& msgUid) const
    {
        int partsSaved = callbacks()->partsCountAll(m_rtr, msgUid);
        if (partsSaved != nParts) {
            callbacks()->partsDeleteAll(m_rtr, msgUid);
            ProgTrace(TRACE1, "not all parts are present, router=%d, msgUid=%s", m_rtr, msgUid.c_str());
            return false;
        }
        return true;
    }
    bool addTlgPart(const tlgnum_t& localMsgId, const std::string& msgUid, int partnumber, bool endFlag)
    {
        LogTrace(TRACE5) << "adding part: localMsgId=" << localMsgId << ", rtr=" << m_rtr
            << ", msgUid=" << msgUid << ", partnumber=" << partnumber << ", endFlag=" << endFlag;
        try  {
            callbacks()->partsAdd(m_rtr, localMsgId, partnumber, endFlag, msgUid);
            return true;
        } catch (const OciCpp::ociexception& e) {
            LogError(STDLOG) << "adding part failed: " << e.what();
            LogTrace(TRACE1) << "localMsgId=" << localMsgId << ", rtr=" << m_rtr
                << ", msgUid=" << msgUid << ", partnumber=" << partnumber << ", endFlag=" << endFlag;
            return false;
        }
    }
    virtual void removeParts(const std::string& msgUid) const
    {
        callbacks()->partsDeleteAll(m_rtr, msgUid);
    }
private:
    /**
     * moves all not joined messages into handle queue
     * */
    void cleanMsgParts(int rtr, const std::string& msgUid)
    {
        for(const tlgnum_t& msgId:  callbacks()->partsReadAll(rtr, msgUid)) {
            LogTrace(TRACE1) << "failed join, rtr=" << m_rtr << " msgUid=" << msgUid
                << " moveTlgToHandQueue: tlg " << msgId;
            callbacks()->joinError(msgId);
        }
        callbacks()->partsDeleteAll(rtr, msgUid);
    }
    std::string m_name;
    int m_rtr;
};
typedef boost::shared_ptr<TlgJoiner> TlgJoinerPtr;

class HthJoiner
    : public TlgJoiner
{
public:
    HthJoiner(int rtr, const HthInfo& h2h)
        : TlgJoiner("HthJoiner", rtr), m_hth(h2h)
    {}
    virtual ~HthJoiner()
    {}
protected:
    virtual void removeParts(const std::string& msgUid) const
    {
        for(const tlgnum_t& tlgNum:  callbacks()->partsReadAll(router(), msgUid)) {
            callbacks()->deleteHth(tlgNum);
        }
        TlgJoiner::removeParts(msgUid);
    }
    virtual INCOMING_INFO getIncomingInfo(const INCOMING_INFO& ii_)
    {
        INCOMING_INFO ii(ii_);
        ii.hthInfo->part = 0;
        ii.hthInfo->end = 0;
        return ii;
    }
    virtual bool processHeader(int& partnum, bool& endFlag, bool& needCombineFlag, std::string&)
    {
        partnum = m_hth.part;
        endFlag = false;
        needCombineFlag = true;
        return true;
    }
    virtual bool needCleanMsgParts(int)
    {
        return false;
    }
    virtual std::string getMsgUid() const
    {
        return m_hth.tpr;
    }
    virtual bool isEndPart(bool) const
    {
        return (bool)m_hth.end;
    }
    virtual int combineParts(std::string& fullText, parts_t& parts)
    {
        for(const std::string& part:  parts) {
            fullText += part;
        }
        return 0;
    }
    virtual bool allPartsPresent(int, const std::string& msgUid) const
    {
        const int endPartNumber = callbacks()->partsEndNum(router(), msgUid);
        LogTrace(TRACE5) << "end part have number[" << endPartNumber << "]"
                         << " for msgUid[" << msgUid << "]"
                         << " and peer[" << router() << "]";

        const int count = callbacks()->partsCountAll(router(), msgUid);
        LogTrace(TRACE5) << "parts count[" << count << "]"
                         << " for msgUid[" << msgUid << "]"
                         << " and peer[" << router() << "]";

        return (endPartNumber == count);
    }
private:
    const HthInfo& m_hth;
};

class TpbJoiner
    : public TlgJoiner
{
public:
    TpbJoiner(int rtr, bool trueTpb)
        : TlgJoiner("TpbJoiner", rtr), m_trueTpb(trueTpb)
    {}
    virtual ~TpbJoiner()
    {}
protected:
    virtual bool processHeader(int& partnum, bool& endFlag, bool& needCombineFlag, std::string& txt)
    {
        if (cutTpbHeader(txt) < 0)
            return false;

        needCombineFlag = endFlag = false;
        // PART n CONTINUE
        partnum = isPart(txt, m_trueTpb);
        if (partnum < 0)
            return false;
        if (partnum)
            return true;

        // PART n END
        partnum = isEnd(txt, m_trueTpb);
        if (partnum < 0)
            return false;
        if (partnum)
            needCombineFlag = endFlag = true;
        return true;
    }
    virtual std::string getMsgUid() const
    {
        return (m_trueTpb) ? "true typeb" : "typeb";
    }
    virtual int combineParts(std::string& fullText, parts_t& parts)
    {
        int number = 0;
        for(std::string& part:  parts) {
            if (number) {
                if (cutTpbHeader(part) < 0) {
                    LogError(STDLOG) << "cutTpbHeader failed";
                    return -1;
                }
                cutTpbPartSign(part, 0);
            } else
                cutTpbPartSign(part, locateTpbHeader(part) + 1);
            ++number;
            fullText += part;
        }
        return 0;
    }
private:
    void cutTpbPartSign(std::string& txt, size_t startPos)
    {
        ProgTrace(TRACE5, "trueTpb=%d startPos=%zd", m_trueTpb, startPos);
        if (m_trueTpb) {
            // last line is always PART CONTINUED/END
            std::string header(txt, 0, startPos);
            const size_t pos = txt.find("\nPART ", startPos);
            txt = header + std::string(txt, startPos, pos - startPos);
        } else {
        // first line after header is always PART CONTINUED/END
            std::string header(txt, 0, startPos);
            const size_t pos = txt.find('\n', startPos);
            txt = header + std::string(txt, pos + 1, txt.size());
            //const char* p = strchr(txt, '\n');
            //strcpy(txt, p + 1);
        }
    }
    size_t locateTpbHeader(const std::string& txt)
    {
        ProgTrace(TRACE5, "%s head of tlg: %30.30s", __FUNCTION__, txt.c_str());
        size_t pos = txt.find('.');
        if (pos != std::string::npos) {
            pos = txt.find('\n', pos);
            if (pos != std::string::npos) {
                if (std::string(txt, pos, 4) == "PDM\n")
                    pos += 4;
            }
        }
        ProgTrace(TRACE5, "pos=%zd", pos);
        return pos;
    }
    int cutTpbHeader(std::string& txt)
    {
        size_t pos = locateTpbHeader(txt);
        ProgTrace(TRACE5, "%s head of tlg: %30.30s", __FUNCTION__, txt.c_str());
        if (pos == std::string::npos) {
            ProgError(STDLOG, "invalid header for TypeB: [%30.30s]", txt.c_str());
            return -1;
        }
        txt.assign(txt, pos + 1, txt.size());
        ProgTrace(TRACE5, "%s head of tlg: %30.30s", __FUNCTION__, txt.c_str());
        return 0;
    }
    int isEnd(const std::string& txt, bool true_tpb)
    {
        int res = 0;
        char c = 0;
        if (true_tpb) {
            const size_t pos = txt.find("\nPART ", txt.size() - MIN_PART_LEN);
            if (pos == std::string::npos)
                return 0;
            int n = std::sscanf(txt.c_str() + pos, "\nPART %d END%c", &res, &c);
            if (n == 2 && c == '\n')
                return res;
        } else {
            int n = std::sscanf(txt.c_str(), "PART %d END%c", &res, &c);
            if (n == 2 && c == '\n')
                return res;
        }
        return 0;
    }

    int isPart(const std::string& txt, bool true_tpb)
    {
        int res = 0;
        char c = 0;
        if (true_tpb) {
            size_t pos = txt.find("\nPART ", txt.size() - MIN_PART_LEN);
            if (pos == std::string::npos)
                return 0;
            int n = std::sscanf(txt.c_str() + pos, "\nPART %d CONTINUED%c", &res, &c);
            if (n == 2 && c == '\n')
                return res;
        } else {
            int n = std::sscanf(txt.c_str(), "PART %d CONTINUED%c", &res, &c);
            if (n == 2 && c == '\n')
                return res;
        }
        return 0;
    }

    bool m_trueTpb;
};

class CommonJoiner
    : public TlgJoiner
{
public:
    CommonJoiner(int rtr)
        : TlgJoiner("CommonJoiner", rtr)
    {}
    virtual ~CommonJoiner()
    {}
protected:
    virtual bool processHeader(int& partnum, bool& endFlag, bool& needCombineFlag, std::string& txt)
    {
        needCombineFlag = endFlag = false;
        partnum = isPart(txt);
        if (partnum < 0)
            return false;
        if (partnum)
            return true;

        partnum = isEnd(txt);
        if (partnum < 0)
            return false;
        if (partnum)
            needCombineFlag = endFlag = true;
        return true;
    }
    int isPart(const std::string& txt)
    {
        ProgTrace(TRACE5, "txt=[%20.20s]", txt.c_str() + txt.size() - 20);
        char c = 0;
        int res = 0;
        const size_t pos = txt.find("\nPART ", txt.size() - MIN_PART_LEN);
        if (pos == std::string::npos)
            return 0;
        int n = std::sscanf(txt.c_str() + pos, "\nPART %d CONTINUED%c", &res, &c);
        ProgTrace(TRACE5, "n = %d, res = %d, c = [%d]", n, res, c);
        if (n == 2 && c == '\n')
            return res;
        return 0;
    }
    int isEnd(const std::string& txt)
    {
        ProgTrace(TRACE5, "txt=[%20.20s]", txt.c_str() + txt.size() - 20);
        char c = 0;
        int res = 0;
        const size_t pos = txt.find("\nPART ", txt.size() - MIN_PART_LEN);
        if (pos == std::string::npos)
            return 0;
        int n = std::sscanf(txt.c_str() + pos, "\nPART %d END%c", &res, &c);
        ProgTrace(TRACE5, "n = %d, res = %d, c = [%d]", n, res, c);
        if (n == 2 && c == '\n')
            return res;
        return 0;
    }
    /* Отстригает слова PART CONTINUED или END
       в конце телеграммы
     */
    int cutTlgPartSign(std::string& txt)
    {
        const size_t txtlen = txt.size();
        if (txtlen > MIN_PART_LEN) {
            ProgTrace(TRACE5, "txt=[%20.20s]", txt.c_str() + txt.size() - 20);
            const size_t pos = txt.find("\nPART ", txtlen - MIN_PART_LEN);
            if (pos != std::string::npos) {
                ProgTrace(TRACE5, "pos=%zd", pos);
                txt.assign(txt, 0, pos);
                ProgTrace(TRACE5, "txt=[%20.20s]", txt.c_str() + txt.size() - 20);
                return 0;
            } else {
                ProgTrace(TRACE0, "PART not found in tlg part");
                return -1;
            }
        }
        ProgTrace(TRACE0, "too small txt length: %zd", txtlen);
        return -1;
    }
    virtual std::string getMsgUid() const
    {
        return "sirena";
    }
    virtual int combineParts(std::string& fullText, parts_t& parts)
    {
        for(std::string& part:  parts) {
            cutTlgPartSign(part);
            fullText += part;
        }
        return 0;
    }
private:
};

class PfsJoiner
    : public TlgJoiner
{
public:
    PfsJoiner(int rtr)
        : TlgJoiner("PfsJoiner", rtr), saj_("PFS")
    {}
    virtual ~PfsJoiner()
    {}
protected:
    virtual bool processHeader(int& partnum, bool& endFlag, bool& needCombineFlag, std::string& txt)
    {
        std::vector<std::string> rows;
        boost::split(rows, txt, boost::algorithm::is_any_of("\n"));

        if (rows.size() < 4) {
            ProgTrace(TRACE0, "not found row with PART: %s", txt.c_str());
            return false;
        }
        while (emptyRow(*(--rows.end()))) { // cut all newlines at the end of message
            rows.erase(--rows.end());
        }

        needCombineFlag = endFlag = false;
        partnum = isPart(rows[3]);
        if (partnum < 0)
            return false;

        needCombineFlag = endFlag = saj_.isEnd(*(--rows.end()));
        if (partnum == 1 && endFlag) { // whole PFS message
            partnum = 0;
            needCombineFlag = endFlag = true;
        }
        return true;
    }
    int isPart(const std::string& txt)
    {
        LogTrace(TRACE5) << "txt=[" << txt << "]";
        int res = 0;
        const size_t pos = txt.rfind(" PART");
        if (pos == std::string::npos)
            return -1;
        int n = std::sscanf(txt.c_str() + pos, " PART%d", &res);
        ProgTrace(TRACE5, "n = %d, res = %d", n, res);
        if (n == 1)
            return res;
        return -1;
    }
    virtual std::string getMsgUid() const
    {
        return "pfs";
    }
    virtual int combineParts(std::string& fullText, parts_t& parts)
    {
        return saj_.combineParts(fullText, parts);
    }
private:
    SimpleAirimpJoiner saj_;
};
bool isPfs(const char* tlgBody)
{
    static int enablePfsJoiner = readIntFromTcl("ENABLE_PFS_JOINER", 0);
    if (!enablePfsJoiner)
        return false;
    static boost::regex rex(".*\n *PFS *\n");
    return boost::regex_search(tlgBody, rex);
}
}

int joinTlgParts(const char* tlgBody, const RouterInfo& ri, const INCOMING_INFO& ii, tlgnum_t& localMsgId, const tlgnum_t& remoteMsgId)
{
    TlgJoinerPtr joiner;
    if (ii.hthInfo) {
        joiner.reset(new HthJoiner(ri.ida, ii.hthInfo.get()));
    } else if (ii.isEdifact) { // EDIFACT without Hth-header
        joiner.reset(new CommonJoiner(ri.ida));
    } else if (isPfs(tlgBody)) {
        joiner.reset(new PfsJoiner(ri.ida));
    } else if (ri.tpb) {
        joiner.reset(new TpbJoiner(ri.ida, ri.true_tpb));
    } else {
        joiner.reset(new CommonJoiner(ri.ida));
    }
    return joiner->join(tlgBody, ii, localMsgId, remoteMsgId);
}

} // telegrams

