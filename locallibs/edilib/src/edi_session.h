#ifndef _EDILIB_EDI_SESSION_H_
#define _EDILIB_EDI_SESSION_H_

#include <libtlg/hth.h>
#include <libtlg/tlgnum.h>
#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <serverlib/dates.h>
#include "edilib/edi_types.h"
#include "edilib/edi_sess.h"
#include "edilib/EdiSessionId_t.h"

#include <serverlib/internal_msgid.h>

#include <utility>
#include <string>

//namespace ServerFramework {  class InternalMsgId;  }
namespace OciCpp
{
    class CursCtl;
}

namespace edilib
{
typedef std::pair<std::string, std::string> CarfPair_t;

/**
 * @enum edi_act_stat
*/
enum edi_act_stat
{
    edi_act_i = 'I',
    edi_act_o = 'O',
    edi_act_s = 'S',
    edi_act_r = 'R',
    edi_act_c = 'C',
    edi_act_e = 'E',
    edi_act_t = 'T',
};

/**
 * session type
*/
enum edi_sess_type
{
    only_in_series, // the only message in series
    last_in_series, // the last message in series
    middle_in_series, // middle message
    first_in_series, // the first one
};

/**
 * @enum edi_sess_chng
*/
enum edi_sess_chng{
    edi_sess_need_del       = 0x01,
    edi_sess_new            = 0x02,
    edi_sess_in_base        = 0x04,
    edi_sess_in_mem         = 0x08,

    edi_sess_first          = 0x10,
    edi_sess_continue       = 0x20,
    edi_sess_last           = 0x40,
    edi_sess_only           = 0x80,
    edi_batch_edifact       = 0x100,
};

enum edi_sess_db_flags
{
    edi_sess_db_save_log    = 0x01,
};

struct EdiSessionSearchKey {
    unsigned systemId;   // remote_system settings table ID
    std::string key; // PULT or RECLOC ...
    std::string systemType; // APT/DPT/ETS...
    std::string sessionType; // AV1, TR1, TR2, ...
    EdiSessionSearchKey(unsigned systemId, const std::string &key,
                        const std::string &systemType,
                        const std::string &sessionType);
};

std::ostream& operator << (std::ostream &os,
                           const EdiSessionSearchKey &sk);

/**
 * @class EdiSession
 * @brief edisession control
*/
class EdiSession
{
    edilib::EdiSessionId_t Ida; /*edisession ida*/
    boost::optional<EdiSessionSearchKey> search_key;
    std::string other_carf;
    std::string other_ref;
    int  other_ref_num;

    std::string our_ref;
    int  our_ref_num;
    std::string our_carf;
    std::string pred_p;
    std::string Pult;
    ServerFramework::InternalMsgId IntMsgId;

    edi_act_stat Status;

    boost::optional<tlgnum_t> MsgId;

    int Flags;
    int DbFlags;

    Dates::DateTime_t Timeout;

    /**
     * @brief Не обновлять/создавать/удалять сессию
     * @brief по окончании обработки
     */

    inline void clearSessionControlFalgs();
public:
    EdiSession();

    enum ReadStatus { ReadOK = 0, NoDataFound, Locked };
    struct ReadResult;

    void EdiSessNotUpdate();
    /**
     * @brief returns true if it batch edifact
    */
    bool isBatchEdifact() const;

    /**
     * @brief must be deleted in commit function
     */
    bool mustBeDeleted() const;

    /**
     * @brief Продолжение сессии ?
     * @return returns true if yes
     */
    bool isEdiSessExist() const;

    /**
     * @brief checks is session new or not
     * @return true if yes
     */
    bool isEdiSessNew() const;

    /// ============ session series type ==============
    /**
     * @brief Is edisession continuation
     * @return returns true if yes
     */
    bool isEdiSessContinue() const;

    /**
     * @brief Is edisession continuation
     * @return returns true if yes
     */
    bool isEdiSessLast() const;

    /**
     * @brief is only in series
     * @return returns true if yes
     */
    bool isEdiSessOnly() const;

    /**
     * @brief is first in series
     * @return returns true if yes
     */
    bool isEdiSessFirst() const;

    void setEdiSessFirst();
    void setEdiSessOnly();
    void setEdiSessLast();
    void setEdiSessContinue();
    /// ==========================

    bool isEdiSessInDB() const;

    /**
     * @brief returns a message ID
     * @return
     */
    boost::optional<tlgnum_t> msgId() const { return MsgId; }

    edilib::EdiSessionId_t ida() const;
    const std::string & predp() const;

    /**
     * @brief sets new our_ref
     * @param oref
     */
    void setOurRef(const std::string &oref);
    /**
     * @brief sets new our carf
     * @param carf
     */
    void setOurCarf(const std::string &carf);

    /**
     * @brief sets new edisession status
     * @param st
     */
    void setStatus(edi_act_stat st);

    /**
     * @brief set a message ID which this session is
     * @param msgid
     */
    void setMsgId(const tlgnum_t &msgid);

    /**
     * @brief Adds flag
     * @param flg
     */
    void addFlag(unsigned flg);
    void rmFlag(unsigned flg);

    /**
     * @brief sets new ida
     */
    void setIda(edilib::EdiSessionId_t id);
    void genOurRef(std::string base);
    void incOurRefNum();
    void setOurRefNum(unsigned n);

    void setPredp(const std::string &pp);
    void setSearchKey(const EdiSessionSearchKey &sk);
    void setPult(const std::string &p);
    void setOtherRef(const std::string &r);
    void setOtherRefNum(unsigned n);
    void setOtherCarf(const std::string &c);
    void setTimeout(const Dates::DateTime_t &to);
    void addDbFlag(int flg);

    const std::string &pult() const;
    const std::string &ourRef() const;
    const std::string &ourCarf() const;
    const std::string &otherCarf() const;
    unsigned ourRefNum() const;
    unsigned otherRefNum() const;
    const std::string &otherRef() const;
    edi_act_stat status() const;
    const boost::optional<EdiSessionSearchKey> &searchKey() const;

    const ServerFramework::InternalMsgId& intmsgid() const {  return IntMsgId;  }
    void intmsgid(const ServerFramework::InternalMsgId& m) {  IntMsgId = m;  }
    /**
     * @brief flags to be saved in DB.
    */
    int dbFlags() const;


    Dates::DateTime_t timeout() const;

    static std::string OurRefByFull(const std::string &full, int *num);

    /**
     * @brief Вызывается в конце обработки
     * update / delete / insert текущей EDIFACT сессии
     */
    void CommitEdiSession();
    void mark2delete();

    static EdiSession readByIda(edilib::EdiSessionId_t sess_ida, bool update);

    /**
     * first  == our_carf
     * second == other_carf
     */
    static CarfPair_t splitCarf(const std::string &carf, int msg_type_req);
    /// Делит CARF на части
    static CarfPair_t splitCarf(const std::string &carf);
};

typedef boost::optional<EdiSession> EdiSession_o;

struct EdiSession::ReadResult
{
    EdiSession::ReadStatus status;
    EdiSession_o ediSession;
};

std::ostream & operator << (std::ostream& os, const  EdiSession::ReadResult &rres);

struct Qri5Flg
{
    static const int first = 0x2;
    static const int middle =0x0;
    static const int last = 0x1;
    static const int only = 0x3;
    static const int mask = 0x3;

    static const char base = 'T';
    static int flag(const char qri)
    {
        return qri & Qri5Flg::mask;
    }
    static edi_act_stat toAssoc(const char qri5);
    static char toQri5(edi_act_stat stat);
};

class EdiSessData
{
protected:
    void SetEdiSessMesAttr_();
    bool NeedSessionCreate();
public:
    static const unsigned int maxOurrefBase = 6;
    static const unsigned int maxOurrefLen = 10;
    virtual EdiSession *ediSession()=0;
    virtual hth::HthInfo *hth() = 0;

    virtual std::string sndrHthAddr() const =0;
    virtual std::string rcvrHthAddr() const =0;
    virtual std::string hthTpr() const = 0;

    // В СИРЕНЕ это recloc/ или our_name из sirena.cfg
    virtual std::string baseOurrefName() const =0;

    // Аттрибуты сообщения
    virtual edi_mes_head *edih() =0;
    virtual const edi_mes_head *edih() const = 0;

    void MakeHth();
    virtual ~EdiSessData(){}
};

// Данные для создания сиссии из сообщения (Обработка запроса или ответа)
class EdiSessRdData : public EdiSessData
{
    EdiSession *Sess;
    EdiSessRdData(const EdiSessRdData&);
    void CreateAnswerByAttr_();
public:
    EdiSessRdData();

    virtual void loadEdiSession(edilib::EdiSessionId_t id);

    virtual EdiSession *ediSession() { return Sess; }
    virtual const EdiSession *ediSession() const { return Sess; }
    virtual std::string baseOurrefName() const { return std::string(); }
    virtual ~EdiSessRdData();

    virtual void makeAnswerFromReq() {}
    virtual std::string syntax() const { return edih()->chset; }
    virtual unsigned syntaxVer() const { return edih()->syntax_ver; }
    virtual std::string ctrlAgency() const { return edih()->cntrl_agn; }
    virtual std::string version() const { return edih()->ver_num; }
    virtual std::string subVersion() const { return edih()->rel_num; }

    /**
     * @brief return true if we need to send T on O
     * @brief keep it true for 1S in ETS
    */
    virtual bool answerWithLanstOnOnly() const { return true; }

    void CreateAnswerByAttr();
    void UpdateEdiSession();
};

// Данные для создания сиссии (Составление Запроса)
class EdiSessWrData : public EdiSessData
{
    edi_sess_type SessionType;
    EdiSession EdiSess;
public:
    EdiSessWrData():SessionType(only_in_series){}
    // Внешняя ссылка на сессию
    virtual boost::optional<EdiSessionSearchKey> searchKey() const;
    // drop me
    virtual void externalIda() const {}
    // Пульт
    virtual std::string pult() const { return ""; }

    virtual bool isBatchEdifact() const { return false; }


    // Аттрибуты сообщения
    virtual std::string syntax() const { return "IATA"; }
    virtual unsigned syntaxVer() const { return 1; }
    virtual std::string ctrlAgency() const { return "IA"; }
    virtual std::string version() const { return "96"; }
    virtual std::string subVersion() const { return "2"; }
    virtual std::string ourUnbAddr() const =0;
    virtual std::string unbAddr() const = 0;
    virtual std::string ourUnbAddrExt() const { return ""; }
    virtual std::string unbAddrExt() const { return ""; }

    virtual void loadEdiSession(edilib::EdiSessionId_t sessId);

    virtual edi_sess_type sessionType() const { return SessionType; }
    virtual void setSessionType(edilib::edi_sess_type st) { SessionType = st; }

    virtual edilib::EdiSession *ediSession();
    virtual const edilib::EdiSession *ediSession() const;

    virtual bool isNew() const { return sessionType() == edilib::first_in_series; }

    /**
     * @brief Заполняет аттрибуты сообщения
     * @brief Создает EDIFACT сессию
     */
    void SetEdiSessMesAttr();

    virtual ~EdiSessWrData(){}
};


struct EdiSessCfg
{
    static const unsigned MaxCarfSize = 35;
};

} // namespace edilib

#endif /*_EDILIB_EDI_SESSION_H_*/
