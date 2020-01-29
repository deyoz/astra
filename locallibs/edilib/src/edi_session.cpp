#include <boost/optional/optional_io.hpp>
#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string>
#include <stdlib.h>
#include <string.h>

#include <serverlib/string_cast.h>
#include <serverlib/str_utils.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/internal_msgid.h>
#include <serverlib/query_runner.h>
#include <boost/date_time.hpp>

#include "edilib/edilib_db_callbacks.h"
#include "edilib/edi_user_func.h"
#include "edilib/edi_sess.h"
#include "edilib/edi_sess_except.h"

#include "edilib/edi_session.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

/*********************** КОНТРОЛЬ EDIFACT СЕССИЙ ****************************/

namespace edilib
{
using namespace std;

std::ostream & operator << (std::ostream& os, const  EdiSession::ReadResult &rres)
{
    switch(rres.status)
    {
        case EdiSession::ReadOK:
            os << "Read EdiSession OK: ourref:" << rres.ediSession->ourRef() <<
                  " otherref:" << rres.ediSession->otherRef() <<
                  " ida:" << rres.ediSession->ida();
            break;
        case EdiSession::NoDataFound:
            os << "No Data Found.";
            break;
        case EdiSession::Locked:
            os << "EdiSession locked.";
            break;
    }

    return os;
}

/*
 Заполняет аттрибуты сообщения
 Создает EDIFACT сессию
 return -1  - error
 return  0  - Ok
 */
void EdiSessData::SetEdiSessMesAttr_()
{
    EdiSession *pSess = ediSession();

    LogTrace(TRACE3) << "pSess->ourRefNum() = " << pSess->ourRefNum();

    if(baseOurrefName().size() > EdiSessData::maxOurrefBase){
        throw EdiSessFatal(STDLOG, "Too long 'base ourref name'");
    }

    if(pSess->isEdiSessExist())
    {
        /*Продолжение сессии*/

        if(!(pSess->isEdiSessContinue() || pSess->isEdiSessLast()) &&
           !(edih()->msg_type_req == RESPONSE && pSess->isEdiSessOnly()))
        {
            throw EdiSessFatal(STDLOG,"Edifact session already exists");
        }

        if(pSess->isEdiSessContinue())
        {
            pSess->setStatus(edi_act_e);
        }
        else if(pSess->isEdiSessOnly())
        {
            // Sabre asked us to send 'T' on answer of 'O' (suits IATA rule)
            pSess->setStatus(edi_act_t);
        }
        else if(pSess->isEdiSessFirst())
        {
            pSess->setStatus(edi_act_s);
        }
        else
        {
            pSess->setStatus(edi_act_t);
        }

    }
    else
    {
        /*Создание новой сессии*/
        if(!pSess->isEdiSessOnly() && !pSess->isEdiSessFirst())
        {
            throw EdiSessFatal(STDLOG,"Edifact session is not found");
        }

        if(pSess->isEdiSessOnly()) {
            pSess->setStatus(edi_act_o);
        } else {
            pSess->setStatus(edi_act_s);
        }

        pSess->genOurRef(baseOurrefName());
        pSess->setOurCarf(pSess->ourRef());
        pSess->addFlag(edi_sess_new);
        pSess->setIda(EdilibDbCallbacks::instance()->ediSessionNextIda());
    }

    if(edih()->msg_type_req == QUERY)
    {
        EdiSessWrData &wrData = dynamic_cast<EdiSessWrData &>(* this);
        pSess->incOurRefNum();

        if(auto&& sk = wrData.searchKey())
            pSess->setSearchKey(sk.get());
        pSess->setPult( wrData.pult() );
        wrData.edih()->syntax_ver = wrData.syntaxVer();
        strcpy(wrData.edih()->chset,     wrData.syntax().c_str());
        strcpy(wrData.edih()->cntrl_agn, wrData.ctrlAgency().c_str());
        strcpy(wrData.edih()->ver_num,   wrData.version().c_str());
        strcpy(wrData.edih()->rel_num,   wrData.subVersion().c_str());
        if(wrData.isBatchEdifact())
        {
            ProgTrace(TRACE2, "Batch edifact");
            pSess->addFlag(edi_batch_edifact);
        }

        strcpy(edih()->from, wrData.ourUnbAddr().c_str());
        strcpy(edih()->to,   wrData.unbAddr().c_str());
        strcpy(edih()->fromAddExt, wrData.ourUnbAddrExt().c_str());
        strcpy(edih()->toAddExt,   wrData.unbAddrExt().c_str());
        strcpy(edih()->acc_ref, pSess->ourCarf().c_str());

        if(!pSess->otherCarf().empty())
            sprintf(edih()->acc_ref+
                    strlen(edih()->acc_ref),
                    "/%s", pSess->otherCarf().c_str());

        pSess->intmsgid(ServerFramework::getQueryRunner().getEdiHelpManager().msgId());

        ProgTrace(TRACE3,"Unb_addr = <%s/%s> <-> <%s/%s>",
                  edih()->from, edih()->fromAddExt,
                  edih()->to,   edih()->toAddExt);
    }
    else
    {
        /* RESPONSE */
        EdiSessRdData &rdData = dynamic_cast<EdiSessRdData &>(* this);
        /* Тупо меняем источник и приемник местами */

        if(pSess->isEdiSessOnly() && !rdData.answerWithLanstOnOnly())
            pSess->setStatus(edi_act_o);

        rdData.makeAnswerFromReq();

        std::string tmp(edih()->from);
        strcpy(edih()->from, edih()->to);
        strcpy(edih()->to, tmp.c_str());

        tmp = edih()->fromAddExt;
        strcpy(edih()->fromAddExt, edih()->toAddExt);
        strcpy(edih()->toAddExt, tmp.c_str());

        strcpy(edih()->acc_ref, pSess->otherCarf().c_str());
        if(!pSess->ourCarf().empty())
            sprintf(edih()->acc_ref+
                    strlen(edih()->acc_ref),
                    "/%s", pSess->ourCarf().c_str());

        /* Char set, Syntax_ver, ver_num, rel_num*/
        /* остаются прежними (по умолчанию)*/
        edih()->syntax_ver      = rdData.syntaxVer();
        strcpy(edih()->chset,     rdData.syntax().c_str());
        strcpy(edih()->cntrl_agn, rdData.ctrlAgency().c_str());
        strcpy(edih()->ver_num,   rdData.version().c_str());
        strcpy(edih()->rel_num,   rdData.subVersion().c_str());
    }

    edih()->mes_num = pSess->ourRefNum();
    LogTrace(TRACE3) << "edih()->mes_num = " << edih()->mes_num;

    if(pSess->isEdiSessInDB() || pSess->isEdiSessNew())
    {
        /*Если создали сессию*/
        if(pSess->isBatchEdifact())
        {
            ProgTrace(TRACE3,"Batch edifact");
            strcpy(edih()->our_ref, pSess->ourRef().c_str());
        } else {
            ProgTrace(TRACE3,"Interactive edifact");
            sprintf(edih()->our_ref,"%s%04d" ,pSess->ourRef().c_str(), edih()->mes_num);
        }
    }
    else
    {
        /*Возвращаем удаленную (далеко) ссылку*/
        strcpy(edih()->our_ref, pSess->ourRef().c_str());
    }

    if(pSess->isBatchEdifact()) {
        *edih()->other_ref = 0;
    } else {
        strcpy(edih()->other_ref, pSess->otherRef().c_str());
    }

    if(!pSess->isBatchEdifact())
    {
        edih()->assoc_code[0] = pSess->status();
        edih()->assoc_code[1] = '\0';
    }
    else {
        edih()->assoc_code[0]='\0';
    }
    if (hth())
        MakeHth();
}

/*
 Преобразует структуру EDIFACT сообщения (запроса!)
 в ответную
 return -1  - error
 return  0  - ok
 */
void EdiSessRdData::CreateAnswerByAttr()
{
    CreateAnswerByAttr_();
}

EdiSessRdData::EdiSessRdData()
    : Sess( new EdiSession() )
{
    ProgTrace(TRACE3, "Create EdiSessRdData");
}
EdiSessRdData::~ EdiSessRdData()
{
    delete Sess;
    ProgTrace(TRACE3, "Delete EdiSessRdData done.");
}

void EdiSessRdData::loadEdiSession(edilib::EdiSessionId_t id)
{
    if(Sess)
        delete Sess;
    Sess = 0;
    Sess = new EdiSession(EdilibDbCallbacks::instance()->ediSessionReadByIda(id, true));
}

boost::optional<EdiSessionSearchKey> EdiSessWrData::searchKey() const
{
    return boost::optional<EdiSessionSearchKey>();
}

void EdiSessWrData::loadEdiSession(edilib::EdiSessionId_t sessId)
{
    EdiSess = EdilibDbCallbacks::instance()->ediSessionReadByIda(sessId, true);
    setSessionType(edilib::middle_in_series);
}

edilib::EdiSession * EdiSessWrData::ediSession()
{
    return &EdiSess;
}

const edilib::EdiSession * EdiSessWrData::ediSession() const
{
    return &EdiSess;
}

void EdiSessWrData::SetEdiSessMesAttr()
{
    EdiTrace(TRACE3, "%s",  __FUNCTION__);
    switch(sessionType())
    {
        case only_in_series:
            ediSession()->addFlag(edi_sess_only);
            break;
        case last_in_series:
            ediSession()->addFlag(edi_sess_last);
            break;
        case middle_in_series:
            ediSession()->addFlag(edi_sess_continue);
            break;
        case first_in_series:
            ediSession()->addFlag(edi_sess_first);
            break;
        default:
            throw EdiSessFatal(STDLOG,
                               "Unknown edi session type: only, first, middle in series?");
    }
    SetEdiSessMesAttr_();
}

void EdiSessRdData::CreateAnswerByAttr_()
{
    if(ediSession()->isEdiSessFirst())
        ediSession()->setEdiSessContinue();
    SetEdiSessMesAttr_();
}

bool EdiSessData::NeedSessionCreate()
{
    if ((hth() && Qri5Flg::flag(hth()->qri5) == Qri5Flg::only) || edih()->assoc_code[0]==edi_act_o)
    {
        return false;
    }

    return true;
}

static std::string getOutRef(const std::string& baseIn, const std::string& hthTpr)
{
    std::string base = baseIn.substr(0, EdiSessData::maxOurrefBase);
    return base + StrUtils::LPad(hthTpr, EdiSessData::maxOurrefLen - base.length(), '0');
}

void EdiSessRdData::UpdateEdiSession()
{
    EdiSession *pSess = ediSession();
    EdiSession CurSess;

    if (hth())
        hth::trace(TRACE1, *hth());

    if (*edih()->assoc_code == 0)
    {
        /*У Габриэля - обычная практика*/
        /*  WriteLog(STDLOG,"Wrong or empty associacion code length - %d",len_tmp); */

        if (hth())
        {
            edih()->assoc_code[0] = Qri5Flg::toAssoc(hth()->qri5);
            edih()->assoc_code[1] = '\0';
            pSess->addFlag(edi_batch_edifact);
        }
        else
        {
            throw EdiSessFatal(STDLOG,
                               "There is no assoc code and HTH information found");
        }
    }
    else if(strlen(edih()->assoc_code)>1)
    {
        throw EdiSessFatal(STDLOG,
                           "Invalid assoc code length");
    }

    pSess->setStatus(static_cast<edi_act_stat>(edih()->assoc_code[0]));

    if(pSess->isBatchEdifact())
    {
        // Если пакетный, не нужно отрезать нули от референса
        ProgTrace(TRACE1,"Batch edifact");
        pSess->setOurRef(edih()->our_ref);
        if(hth()->qri6 > 0)
            pSess->setOurRefNum(hth()->qri6-'A'+1);
    } else {
        if(*edih()->our_ref)
        {
            int num = 0;
            pSess->setOurRef(EdiSession::OurRefByFull(edih()->our_ref, &num));
            if(!num)
                num = 1;
            pSess->setOurRefNum(num);
        }
        if(hth() && hth()->qri6 > 0)
            pSess->setOurRefNum(hth()->qri6-'A'+1);
    }
    if(pSess->ourRefNum() == 0)
        pSess->incOurRefNum();
    pSess->setOtherRef(edih()->other_ref);
    pSess->setOtherRefNum(edih()->mes_num);

    CarfPair_t carf = EdiSession::splitCarf(edih()->acc_ref, edih()->msg_type_req);
    pSess->setOurCarf(carf.first);
    pSess->setOtherCarf(carf.second);

    switch(pSess->status())
    {
        case edi_act_i:
            ProgTrace(TRACE3,"Status = '%c' - Ignore", pSess->status());
            return ;
        case edi_act_o:
            pSess->setEdiSessOnly();
            break;
        case edi_act_s: /*Первичное сообщение*/
            pSess->setEdiSessFirst();
            break;
        case edi_act_e: /* Продолжение сессии */
            pSess->setEdiSessContinue();
            break;
        case edi_act_t: /* Завершение  сессии */
            pSess->setEdiSessLast();
            break;
        default:
            throw EdiSessFatal(STDLOG, (std::string("unknown assoc.code ") +
                                        std::string(1,pSess->status())).c_str());
    }

    if(pSess->ourCarf().empty() && !pSess->ourRef().empty())
        // 1S не шлёт нам нашу часть карфа
        pSess->setOurCarf(pSess->ourRef());

    if(pSess->isEdiSessContinue() && pSess->ourCarf().empty())
        // 1G sends middle in series (h2h) with new reference wo our carf and reference
        pSess->setEdiSessFirst();

    if(edih()->msg_type_req == QUERY && (pSess->isEdiSessOnly() || pSess->isEdiSessFirst()))
    {
        ProgTrace(TRACE3,"Start edifact session");

        if(*edih()->our_ref)
        {
            ProgError(STDLOG,"First message in series, but our_ref=%s",
                      pSess->ourRef().c_str());
            pSess->setOurRef("");
        }

        if(NeedSessionCreate())
        {
            pSess->setIda(EdilibDbCallbacks::instance()->ediSessionNextIda());
            pSess->genOurRef(baseOurrefName());
            LogTrace(TRACE3) << "our_ref = <" << pSess->ourRef() << ">";

            pSess->setOurCarf( pSess->ourRef() );
            pSess->addFlag(edi_sess_new);
            pSess->addFlag(edi_sess_in_mem);
        }
        else
        {
            ProgTrace(TRACE3, "Not session create needed");
            pSess->setOurRef(pSess->otherRef());
            pSess->setOtherRef("");
//            pSess->setOtherFullRef("");
            pSess->addFlag(edi_sess_in_mem);
        }

        if(pSess->isBatchEdifact() && pSess->ourRefNum() != 1 &&
           !pSess->isEdiSessContinue())
        {
            WriteLog(STDLOG,"Num is not 1 in the first message");
        }

        if(!pSess->isEdiSessContinue())
            pSess->setOurRefNum(1);
    } else {
        // Продолжение сессии
        ProgTrace(TRACE3,"Continue edifact session");
        if(pSess->ourRef().empty())
        {
            if(edih()->msg_type_req == QUERY)
            {
                // ЗАПРОС
                // Амадеус продолжаетсессию, не присылая нам наш старый референс
                pSess->setOtherRef( edih()->other_ref );
                pSess->setOurRef("");
            } else {
                // ОТВЕТ
                // Так могут отвечать на односессионные сообщения
                // (напр. SMPRES, HSFRES, ...)
            }
        }

        if(pSess->ourCarf().empty() && pSess->ourRef().empty())
        {
            // travelsky не шлёт практически ничего
            if(hth()) {
                pSess->setOurRef(getOutRef(baseOurrefName(), hth()->tpr));
            } else {
                //int num = 0;
                //pSess->setOurRef(EdiSession::OurRefByFull(edih()->other_ref, &num));
            }
            pSess->setOurCarf(pSess->ourRef());
        }

        const auto search_oth_carf = (edih()->msg_type_req == QUERY) ? pSess->otherCarf() : "";
        CurSess = EdilibDbCallbacks::instance()->ediSessionReadByCarf(pSess->ourCarf(), search_oth_carf, 
                                                                      true /*for update*/);

        if(CurSess.otherRefNum() < pSess->otherRefNum() + 1) {
            /* были потери запросов к нам */
            WriteLog(STDLOG,"Message num=%d, otherrefnum=%d - message may be lost",
                     pSess->otherRefNum(), CurSess.otherRefNum());
        }
        if(pSess->ourRef().empty() && pSess->isBatchEdifact())
        {
            pSess->setOurRef(CurSess.ourRef());
        }

        if(edih()->msg_type_req == QUERY)
        {
            pSess->setOurRefNum(pSess->ourRefNum());
        }
        pSess->setIda(CurSess.ida());
        if(CurSess.searchKey())
            pSess->setSearchKey(CurSess.searchKey().get());
        pSess->setPredp(CurSess.predp());
        pSess->setPult (CurSess.pult());
        if (CurSess.msgId())
            pSess->setMsgId(*CurSess.msgId());
        pSess->addDbFlag(CurSess.dbFlags());
        pSess->setTimeout(CurSess.timeout());
        if(pSess->ourRef().empty())
        {
            pSess->setOurRef( CurSess.ourRef() );
        }
        pSess->intmsgid(CurSess.intmsgid());

        pSess->addFlag(edi_sess_in_base);
        if(edih()->msg_type_req == RESPONSE)
        {
            if(pSess->isEdiSessOnly() || pSess->isEdiSessLast())
               pSess->mark2delete();
        }
    }

    LogTrace(TRACE3) << "pSess->ourRefNum() = " << pSess->ourRefNum();

    if(pSess->searchKey())
        LogTrace(TRACE1) << "SearchKey = " << pSess->searchKey().get();
    LogTrace(TRACE1) << "EDISESSION.IDA=" << pSess->ida() <<
                        ", Pred_p=" << pSess->predp() <<
                        ", MsgId =" << pSess->msgId() <<
                        ", IntMsgId = " << pSess->intmsgid();
}

EdiSession::EdiSession()
    :   Ida(),
        other_ref_num(0),
        our_ref_num(0),
        IntMsgId(ServerFramework::getQueryRunner().getEdiHelpManager().msgId()),
        Status(edi_act_o),
        Flags(0),
        DbFlags(0)
{
    ProgTrace(TRACE5, "Create EdiSession struct! %s", IntMsgId.asString().c_str());
}

bool EdiSession::isBatchEdifact() const
{
    return Flags&edi_batch_edifact;
}

bool EdiSession::mustBeDeleted() const
{
    return Flags&edi_sess_need_del;
}

bool EdiSession::isEdiSessExist() const
{
    return Flags&(edi_sess_in_base|edi_sess_in_mem);
}

bool EdiSession::isEdiSessNew() const
{
    return Flags&edi_sess_new;
}

bool EdiSession::isEdiSessContinue() const
{
    return Flags&edi_sess_continue;
}

bool EdiSession::isEdiSessLast() const
{
    return Flags&edi_sess_last;
}

bool EdiSession::isEdiSessOnly() const
{
    return Flags&edi_sess_only;
}

bool EdiSession::isEdiSessFirst() const
{
    return Flags&edi_sess_first;
}

void EdiSession::setEdiSessFirst()
{
    clearSessionControlFalgs();
    addFlag(edi_sess_first);
}

void EdiSession::setEdiSessOnly()
{
    clearSessionControlFalgs();
    addFlag(edi_sess_only);
}

void EdiSession::setEdiSessLast()
{
    clearSessionControlFalgs();
    addFlag(edi_sess_last);
}

void EdiSession::setEdiSessContinue()
{
    clearSessionControlFalgs();
    addFlag(edi_sess_continue);
}

bool EdiSession::isEdiSessInDB() const
{
    return Flags&edi_sess_in_base;
}

static std::string sessidAsString(unsigned sessid, size_t len)
{
    std::string res = StrUtils::ToBase36Lpad(sessid, len);
    if(res.length() > len)
        res = res.substr(res.length() - len, len);
    return res;
}

void EdiSession::genOurRef(std::string base)
{
    const unsigned sessid = EdilibDbCallbacks::instance()->ediSessionNextEdiId();

    base = base.substr(0, EdiSessData::maxOurrefBase);
    setOurRef( base + sessidAsString(sessid, EdiSessData::maxOurrefLen - base.length()) );
}

void EdiSession::setPredp(const string &pp)
{
    pred_p = pp;
}

void EdiSession::setSearchKey(const EdiSessionSearchKey &sk)
{
    search_key = sk;
}

void EdiSession::setPult(const string &p)
{
    Pult = p;
}

const string &EdiSession::pult() const
{
    return Pult;
}

const string &EdiSession::ourRef() const
{
    return our_ref;
}

const string &EdiSession::ourCarf() const
{
    return our_carf;
}

const string &EdiSession::otherCarf() const
{
    return other_carf;
}

unsigned EdiSession::ourRefNum() const
{
    return our_ref_num;
}

unsigned EdiSession::otherRefNum() const
{
    return other_ref_num;
}

const string &EdiSession::otherRef() const
{
    return other_ref;
}

edi_act_stat EdiSession::status() const
{
    return Status;
}

const boost::optional<EdiSessionSearchKey> &EdiSession::searchKey() const
{
    return search_key;
}

void EdiSession::incOurRefNum()
{
    our_ref_num ++;
}

void EdiSession::setOurRefNum(unsigned n)
{
    our_ref_num = n;
}

int EdiSession::dbFlags() const
{
    return DbFlags;
}

void EdiSession::addDbFlag(int flg)
{
    DbFlags |= flg;
}

void EdiSession::setOtherRef(const string &r)
{
    other_ref = r;
}

void EdiSession::setOtherRefNum(unsigned n)
{
    other_ref_num = n;
}

void EdiSession::setOtherCarf(const string &c)
{
    other_carf = c;
    ASSERT(our_carf.size() + other_carf.size() < EdiSessCfg::MaxCarfSize);
}


Dates::DateTime_t EdiSession::timeout() const
{
    return Timeout;
}

void EdiSession::setTimeout(const Dates::DateTime_t &to)
{
    Timeout = to;
}

void EdiSession::mark2delete()
{
    if(isEdiSessInDB())
    {
        addFlag(edi_sess_need_del);
        rmFlag(edi_sess_new);
        ProgTrace(TRACE3,"Mark edifact session for deletion");
    }else {
        ProgTrace(TRACE3,"Edifact session is not found in the base");
        rmFlag(edi_sess_new);
    }
}

EdiSessionId_t EdiSession::ida() const
{
    return Ida;
}

const string &EdiSession::predp() const
{
    return pred_p;
}

void EdiSession::setOurRef(const string &oref)
{
    our_ref = oref;
}

void EdiSession::setOurCarf(const string &carf)
{
    our_carf = carf;
    ASSERT(our_carf.size() + other_carf.size() < EdiSessCfg::MaxCarfSize);
}

void EdiSession::setStatus(edi_act_stat st)
{
    Status = st;
}

void EdiSession::setMsgId(const tlgnum_t &msgid)
{
    MsgId = msgid;
}

void EdiSession::addFlag(unsigned flg)
{
    Flags |= flg;
}

void EdiSession::rmFlag(unsigned flg)
{
    Flags &= ~flg;
}

void EdiSession::setIda(EdiSessionId_t id)
{
    Ida = id;
}

void EdiSession::CommitEdiSession()
{
    if(mustBeDeleted())
    {
        ProgTrace(TRACE1,
                  "Deletion edifact session on %s, other_ref=%s, our_ref=%s ida=%d",
                  pred_p.c_str(),
                  other_ref.c_str(),
                  our_ref.c_str(),
                  Ida.get());
        EdilibDbCallbacks::instance()->ediSessionDeleteDb(this->ida());
        EdilibDbCallbacks::instance()->ediSessionToDeleteDb(this->ida());
    }else if(isEdiSessInDB()){
        EdilibDbCallbacks::instance()->ediSessionUpdateDb(*this);
    }else if(isEdiSessNew()){
        EdilibDbCallbacks::instance()->ediSessionWriteDb(*this);
    }else {
        ProgTrace(TRACE1,"No action whith edisession needed");
    }
}

/*
 Не обновлять/создавать/удалять сессию
 по окончании обработки
 */
void EdiSession::EdiSessNotUpdate()
{
    rmFlag(edi_sess_need_del);
    rmFlag(edi_sess_in_base);
    rmFlag(edi_sess_new);
}

void EdiSession::clearSessionControlFalgs()
{
    rmFlag(edi_sess_first|edi_sess_only|edi_sess_last|edi_sess_continue);
}

std::string EdiSession::OurRefByFull(const std::string &full, int *num)
{
    if(full.size()){
        if(full.size() > 4){
            char *p;
            int i = strtol(full.c_str()+full.size()-4, &p, 10);
            if(p == full.c_str() + full.size()){ /*Есть номер в our_ref*/
                tst();
                if(num){
                    *num=i;
                }
                return std::string (full, 0, full.size()-4);
            }
        }
        if(num)
            *num=0;
        return full;
    }else {
        throw EdiSessFatal(STDLOG, "Zero reference length");
    }
}

// Делит CARF на части
CarfPair_t EdiSession::splitCarf(const string &carf)
{
    if(carf.size() > EdiSessCfg::MaxCarfSize) {
        throw EdiSessFatal(STDLOG,"Too long Common Access Reference (or its part)");
    }
    string part1;
    string part2;
    string::size_type pos = carf.find('/');

    part1 = carf.substr(0, pos);
    if(pos != string::npos)
    {
	   // Что-то нашли
        part2 = carf.substr(pos+1);
    }

    return make_pair(part1, part2);
}


EdiSession EdiSession::readByIda(edilib::EdiSessionId_t sess_ida, bool update)
{
    return EdilibDbCallbacks::instance()->ediSessionReadByIda(sess_ida, update);
}

template<typename T> inline void swap(std::pair<T,T> &p)
{
    T f = p.first;
    p.first = p.second;
    p.second = f;
}

// first  == our_carf
// second == other_carf
CarfPair_t EdiSession::splitCarf(const std::string &carf, int msg_type_req)
{
    CarfPair_t carfpair = splitCarf(carf);
    if(msg_type_req==QUERY){
        swap(carfpair);
    } else if(msg_type_req!=RESPONSE){
        throw EdiSessFatal(STDLOG, "Unknown message type");
    }
    ProgTrace(TRACE3,"our_carf=%s, their_carf=%s",
          carfpair.first.c_str(),
          carfpair.second.c_str());
    return carfpair;
}

edi_act_stat Qri5Flg::toAssoc(const char qri5)
{
    switch(Qri5Flg::flag(qri5))
    {
        case Qri5Flg::first:
            return edi_act_s;
        case Qri5Flg::last:
            return edi_act_t;
        case Qri5Flg::only:
            return edi_act_o;
        case Qri5Flg::middle:
            return edi_act_e;
        default:
            throw EdiSessFatal(STDLOG,"Unknown Qri5");
    }
}

char Qri5Flg::toQri5(edi_act_stat assoc_code)
{
    char qri5 = Qri5Flg::base;

    if(assoc_code == 0)
    {
        throw EdiSessFatal(STDLOG,
                           "Can't retrieve QRI5 from assoc_code.");
    } else {
        int act;
        switch(assoc_code){
            case edi_act_s:
                act = Qri5Flg::first;
                break;
            case edi_act_o:
                act = Qri5Flg::only;
                break;
            case edi_act_t:
            case edi_act_i:
                act = Qri5Flg::last;
                break;
            case edi_act_e:
                act = Qri5Flg::middle;
                break;
            default:
                throw EdiSessFatal(STDLOG,
                                   (string("Unknown value of assoc_code ") +
                                           string(1,assoc_code)).c_str());
        }
        qri5 |= act;
    }
    return qri5;
}


void EdiSessData::MakeHth()
{
    ProgTrace(TRACE3,"Make HTH");

    if(edih()->msg_type_req == QUERY){
        hth()->type = hth::Request;
    } else {
        hth()->type = hth::Response;
    }

    if (sndrHthAddr().empty() || rcvrHthAddr().empty())
    {
        throw EdiSessFatal(STDLOG, "No hth sndr or rcvr");
    }

#define COPY_ADDR(dest, src) {\
    if (src.length() > hth::AddrLength)  { \
        strncpy(dest, src.c_str(), hth::AddrLength); \
        dest[hth::AddrLength] = 0; \
    } \
    else {\
        strcpy(dest, src.c_str()); \
    } \
}

    // sender & receiver меняем местами
    if (edih()->msg_type_req == QUERY)
    {
        COPY_ADDR(hth()->sender, sndrHthAddr())
        COPY_ADDR(hth()->receiver, rcvrHthAddr())
    }
    else
    {
        COPY_ADDR(hth()->sender, rcvrHthAddr())
        COPY_ADDR(hth()->receiver, sndrHthAddr())
    }

    if (hthTpr().length() > hth::TprLength)
        strncpy(hth()->tpr, hthTpr().c_str(), hth::TprLength);
    else
        strcpy(hth()->tpr, hthTpr().c_str());

    if(hth()->type == (int)hth::Request || !hth()->qri5)
    {
        // В случае интерактивного EDIFACT берём QRI5/6 из исходной телеграммы
        hth()->qri5 = Qri5Flg::toQri5(ediSession() ? ediSession()->status() : static_cast<edi_act_stat>(edih()->assoc_code[0]));
        hth()->qri6 = 'A' + ((edih()->mes_num - 1) % ('Z' - 'A' + 1));
    }


    hth::trace(TRACE2, *hth());
}

namespace{
    void checkEdiSessionType(const std::string &sessionType) {
        static const std::string PossibleSessionTypes[] = {
            "TR1", "TR2",
            "AV1", "SM1",
            "ETK1","CSP1",
            "INF1","INF2",
            "FTV1","FTV2"};
        static const size_t PossibleSessionTypesSize =
                sizeof(PossibleSessionTypes)/sizeof(PossibleSessionTypes[0]);

        size_t i = 0;
        for(; i < PossibleSessionTypesSize; i++)
            if(PossibleSessionTypes[i] == sessionType)
                break;
        if(i == PossibleSessionTypesSize)
            throw EdiSessFatal(STDLOG, "Invalid edifact session type");

    }
}

EdiSessionSearchKey::EdiSessionSearchKey(unsigned systemId,
                                         const string &key,
                                         const string &systemType,
                                         const string &sessionType)
    :systemId(systemId),
      key(key),
      systemType(systemType),
      sessionType(sessionType)
{

    if(systemType.length() != 3)
        throw EdiSessFatal(STDLOG, "Invalid length of system type");

    checkEdiSessionType(sessionType);
}

std::ostream& operator << (ostream &os, const EdiSessionSearchKey &sk)
{
    os << sk.systemType << ":" << sk.systemId << " "
       << sk.sessionType << " "<< sk.key;
    return os;
}


} // namespace edilib
