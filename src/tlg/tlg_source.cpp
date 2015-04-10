/*
*  C++ Implementation: tlg_source
*
* Description: Исходный текст телеграмм
* чтение/запись в базу
*
*/

#include "tlg_source.h"
#include "astra_dates.h"
#include "astra_dates_oci.h"
#include "exceptions.h"
#include "tlg.h"

#include <serverlib/dates.h>
#include <serverlib/dates_oci.h>
#include <serverlib/cursctl.h>
#include <serverlib/int_parameters_oci.h>

#include <etick/exceptions.h>
#include <etick/etick_msg.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace TlgHandling
{

TlgSource TlgSource::readFromDb(const tlgnum_t& tlg_num)
{
    std::string full_text = getTlgText2(tlg_num);
    std::string err_text;
    std::string from_rot, to_rot;
    std::string type;
    bool postponed = false;
    Dates::ptime date1;
    int gateway_num = 0;

    OciCpp::CursCtl tcur = make_curs("select "
            "TLG_NUM, SENDER, RECEIVER, ERROR, TYPE, TIME, POSTPONED "
            "from TLGS where ID = :ID");

    tcur.bind(":ID", tlg_num.num)
        .def(gateway_num)
        .def(from_rot)
        .def(to_rot)
        .defNull(err_text, "")
        .def(type)
        .def(date1)
        .defNull(postponed, false)
        .EXfet();

    if(tcur.err() == NO_DATA_FOUND)
    {
        throw EXCEPTIONS::Exception("No such tlg: %s", HelpCpp::string_cast(tlg_num).c_str());
    }

    LogTrace(TRACE2) << "Tlg " << tlg_num <<
            " has been successfully read from DB."
            " Length is " << full_text.size();

    TlgSource tsrc(full_text, from_rot, to_rot);
    tsrc.setErrorText(err_text);
    tsrc.setPostponed(postponed);
    tsrc.setTlgNum(tlg_num);
    tsrc.setGatewayNum(gateway_num);
    tsrc.setTypeStr(type);
    tsrc.setReceiveDate(date1);

    return tsrc;
}

void TlgSource::writeToDb(TlgSource & tlg)
{
    LogTrace(TRACE3) << __FUNCTION__ << " called!";
    saveTlg(tlg.fromRot().c_str(),
            tlg.toRot().c_str(),
            "TPA", // TODO
            tlg.text().c_str());

    tlg.setReceiveDate(Dates::second_clock::local_time());
}

std::ostream & operator <<(std::ostream & os, const TlgSource &tlg)
{
    os << tlg.text();
    return os;
}

std::ostream & operator <<(std::ostream & os, const TlgSourceTypified & tlg)
{
    os << "Type: " << tlg.name() << " (" << tlg.type() << "): "
          "PP: " << (tlg.postponed()?"YES":"NO") << "\n"
            << tlg.text2view();
    return os;
}

void TlgSource::swapRouters()
{
    std::swap(FromRot, ToRot);
}

TlgSource::TlgSource(const std::string& txt,
                     const std::string& fRot,
                     const std::string& tRot)
    : Text(txt), FromRot(fRot), ToRot(tRot), Postponed(false),
      HandMade(Net), GatewayNum(0)
{
}

void TlgSource::setTypeStr(const std::string & t)
{
    TypeStr = t;
}

tlgnum_t TlgSource::genNextTlgNum()
{
    tlgnum_t num;
    make_curs("SELECT TLGS_ID.NEXTVAL FROM DUAL").def(num.num).EXfet();
    return num;
}

boost::posix_time::ptime TlgSource::receiveDate(const tlgnum_t& tnum)
{
//    oracle_datetime date1;
//    CursCtl selCur = make_curs("SELECT DATE1 FROM TEXT_TLG_NEW WHERE MSG_ID = :ID");

//    selCur.
//            bind(":ID", tnum.num).
//            def(date1).
//            EXfet();

//    if(selCur.err())
//    {
//        throw tick_soft_except(STDLOG, TlgErr::NoSuchTlg, "No such tlg: %s", tnum.num.get().c_str());
//    }

//    return OciCpp::from_oracle_time(date1);

    throw EXCEPTIONS::Exception("Unimplimented method called!");
}

void TlgSource::setDbErrorText(const tlgnum_t& tnum, const std::string & err)
{
//    CursCtl upd = make_curs("UPDATE text_tlg_new SET ERRTEXT=:errtext "
//            "WHERE msg_id = :numtlg");

//    upd.
//            bind(":errtext",err).
//            bind(":numtlg",tnum.num).
//            exec();

//    if(upd.rowcount() == 0)
//    {
//        LogError(STDLOG) << "UPDATE text_tlg_new by tnum=" << tnum <<
//            ": no rows updated";
//    }

    throw EXCEPTIONS::Exception("Unimplimented method called!");
}

void TlgSource::writeHandlingResulst() const
{
//    LogTrace(TRACE3) << __FUNCTION__ << ". TypeStr:" << TypeStr <<
//            " SubtypeStr:" << SubtypeStr << " airline:" << Airline <<
//            " AnswerTlgNum:" << AnswerTlgNum;

//    CursCtl upd = make_curs(
//            "UPDATE text_tlg_new SET "
//            "ANSWER_MSG_ID = :ANS_ID, "
//            "PROCESS_DATE  = :local_time, "
//            "AIRLINE = :AIRL, "
//            "TYPE    = :TP, "
//            "SUBTYPE = :STP, "
//            "POSTPONED = :PP "
//            "WHERE msg_id = :ID");

//    short null=-1,nnull=0;
//    upd.
//            bind(":ANS_ID",AnswerTlgNum.num, AnswerTlgNum.num.valid() ? &nnull : &null).
//            bind(":local_time", OciCpp::to_oracle_datetime(Dates::second_clock::local_time())).
//            bind(":AIRL",  Airline,      Airline?&nnull:&null).
//            bind(":TP",    TypeStr).
//            bind(":STP",   SubtypeStr).
//            bind(":PP",    postponed()?1:0, postponed()?&nnull:&null).
//            bind(":ID",    TlgNum.num).
//            exec();

//    if(upd.rowcount() == 0)
//    {
//        LogError(STDLOG) << "UPDATE text_tlg_new set ANSWER_MSG_ID, "
//                "AIRLINE and etc by tnum=" << TlgNum << ": no rows updated";
//    }

    throw EXCEPTIONS::Exception("Unimplimented method called!");
}

void TlgSource::setAnswerTlgNumDb(const tlgnum_t& tnum_src, const tlgnum_t& tnum_answ)
{
//    LogTrace(TRACE3) << "Set AnswerTlgNum = " << tnum_answ << " for " << tnum_src;
//    if(!tnum_src.num.valid() || !tnum_answ.num.valid()) {
//        return;
//    }
//    make_curs("UPDATE text_tlg_new SET ANSWER_MSG_ID = :ANS_ID WHERE msg_id = :ID").
//            bind(":ID", tnum_src.num).
//            bind(":ANS_ID", tnum_answ.num).
//            exec();

    throw EXCEPTIONS::Exception("Unimplimented method called!");
}

std::list< std::string > TlgSource::splitText(const std::string & spliter) const
{
    std::list< std::string > splitedText;
    boost::split(splitedText, text(), boost::algorithm::is_any_of(spliter));

    return splitedText;
}

inline void out_put_diff(std::list< std::string >::const_iterator iter_beg,
                    std::list< std::string >::const_iterator iter_end,
                    const std::string &pref, std::string &cmp_results)
{
    while(iter_beg != iter_end)
    {
        cmp_results += pref;
        cmp_results += *iter_beg;
        cmp_results += "\n";
        iter_beg ++;
    }
}

bool eq_mask_xxx(const std::string &src, const std::string &dest, bool ignore_xxx)
{
    if(src.length() != dest.length())
        return false;

    if(src == dest)
        return true;

    for(size_t i = 0; i < src.length(); i++) {
        if(dest[i] != 'x' && src[i] != dest[i]) {
            return false;
        }
    }
    return true;
}

std::string TlgSource::diff(const TlgSource & tsrc, unsigned skeepfl, unsigned skeepll, bool ignore_xxx) const
{
    std::string cmp_results;
    std::list< std::string > splitedText1 = splitText(textSpliter());
    std::list< std::string > splitedText2 = tsrc.splitText(textSpliter());

    for(unsigned i=0;i<skeepfl;i++)
    {
        if(!splitedText1.empty())
            splitedText1.pop_front();
        if(!splitedText2.empty())
            splitedText2.pop_front();
    }
    for(unsigned i=0;i<skeepll;i++)
    {
        if(!splitedText1.empty())
            splitedText1.pop_back();
        if(!splitedText2.empty())
            splitedText2.pop_back();
    }

    std::list< std::string >::const_iterator iter2_end, iter22, iter2 = splitedText2.begin();
    std::list< std::string >::const_iterator iter1_end, iter11, iter1 = splitedText1.begin();
    iter22 = iter2;
    iter11 = iter1;
    iter1_end = splitedText1.end();
    iter2_end = splitedText2.end();
    for(; iter1 != iter1_end || iter2 != iter2_end;)
    {
        // если равны
        if((iter2 != iter2_end) && (iter1 != iter1_end) && eq_mask_xxx(*iter1, *iter2, ignore_xxx))
        {
            if(iter11 != iter1)
            {
                out_put_diff(iter11,iter1,"+ ",cmp_results);
            }
            if(iter22 != iter2)
            {
                out_put_diff(iter22,iter2,"- ",cmp_results);
            }
            iter2++;
            iter1++;
            iter22 = iter2;
            iter11 = iter1;
        }
        else
        {
            if(iter1 != iter1_end)
            {
                iter1 ++;
            }

            if(iter2 != iter2_end)
            {
                iter2 ++;
            }
        }
    }
    if(iter11 != iter1)
    {
        out_put_diff(iter11,iter1,"+ ",cmp_results);
    }
    if(iter22 != iter2)
    {
        out_put_diff(iter22,iter2,"- ",cmp_results);
    }

    return cmp_results;
}

TlgSourceTypified::TlgSourceTypified(const TlgSource & src)
        : TlgSource(src)
{
}

}//namespace TlgHandling
