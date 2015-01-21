/*
*  C++ Implementation: tlg_source
*
* Description: Исходный текст телеграмм
* чтение/запись в базу
*
*/


#include "astra_dates.h"
#include "astra_dates_oci.h"
#include "tlg_source.h"
#include "exceptions.h"
//#include "exceptions.h"
//#include "tlg_msg.h"
//#include "TicketBaseTypesOci.h"

#include <serverlib/dates.h>
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
using namespace OciCpp;
using namespace std;
//using namespace Ticketing::TickExceptions;

TlgSource TlgSource::readFromDb(const tlgnum_t& tlg_num)
{
//    std::string full_text, errtext;
//    Ticketing::RouterId_t from_rot, to_rot;
//    oracle_datetime tlg_date, proc_date;
//    tlgnum_t answ_tnum;
//    Ticketing::Airline_t airline;
//    std::string type,subtype;
//    int postp, hmade;

//    short multi;
//    CursCtl tcur = make_curs("SELECT "
//            "TEXT, MULTY_PART, FROM_ADDR, TO_ADDR, DATE1, ERRTEXT, "
//            "ANSWER_MSG_ID, PROCESS_DATE, AIRLINE, TYPE, SUBTYPE, "
//            "POSTPONED, HAND_MADE "
//            "FROM TEXT_TLG_NEW WHERE MSG_ID = :ID");
//    tcur.
//            bind(":ID",tlg_num.num).
//            def(full_text).
//            def(multi).
//            defNull(from_rot, Ticketing::RouterId_t()).
//            defNull(to_rot, Ticketing::RouterId_t()).
//            def(tlg_date).
//            defNull(errtext, "").
//            defNull(answ_tnum.num, tlgnum_t().num).
//            defNull(proc_date, oracle_datetime()).
//            defNull(airline, Ticketing::Airline_t()).
//            defNull(type,"").
//            defNull(subtype,"").
//            defNull(postp, 0).
//            defNull(hmade, 0).
//            EXfet();

//    if(tcur.err() == NO_DATA_FOUND)
//    {
//        throw tick_soft_except(STDLOG, TlgErr::NoSuchTlg, "No such tlg: %s", HelpCpp::string_cast(tlg_num).c_str());
//    }

//    if(from_rot && from_rot.get() == 0)
//    {
//        from_rot = Ticketing::RouterId_t();
//    }
//    if(to_rot && to_rot.get() == 0)
//    {
//        to_rot = Ticketing::RouterId_t();
//    }

//    if(multi)
//    {
//        std::string cur_frag;
//        CursCtl pcur = make_curs(
//                "SELECT text FROM text_tlg_part WHERE msg_id=:id ORDER BY part"
//                                );
//        pcur.
//                bind(":id", tlg_num.num).
//                def(cur_frag).
//                exec();
//        unsigned num = 1;
//        while(!pcur.fen())
//        {
//            num++;
//            full_text += cur_frag;
//        }
//        LogTrace(TRACE3) << "TlgSource::readFromDb(): fetched " << num <<
//                " parts, total length is " << full_text.size();

//    }

//    LogTrace(TRACE2) << "Tlg " << tlg_num <<
//            " has been successfully read from DB."
//            " Length is " << full_text.size();

//    TlgSource tsrc( full_text, from_rot, to_rot );
//    tsrc.setReceiveDate(OciCpp::from_oracle_time(tlg_date));
//    tsrc.setProcessDate(OciCpp::from_oracle_time(proc_date));
//    tsrc.setTlgNum(tlg_num);
//    tsrc.setErrorText(errtext);
//    tsrc.setAnswerTlgNum(answ_tnum);
//    tsrc.setAirline(airline);
//    tsrc.setTypeStr(type);
//    tsrc.setSubtypeStr(subtype);
//    tsrc.setPostponed(postp!=0);
//    tsrc.setHandMade(hmade);

//    Ticketing::RouterId_t rot = tsrc.fromRot()?tsrc.fromRot():tsrc.toRot();

//    return tsrc;

    throw EXCEPTIONS::Exception("Unimplimented method called!");
}

void TlgSource::writeToDb(TlgSource & tlg)
{
//    oracle_datetime insDate;
//    unsigned num_part = tlg.text().size()/DbPartLen +
//            ((tlg.text().size()%DbPartLen)>0?1:0);

//    short null=-1, nnull=0;

//    LogTrace(TRACE2) << "tlg_len=" << tlg.text().size() << ", num_part=" << num_part;
//    if(tlg.text().empty())
//        throw EXCEPTIONS::Exception("tlg text is null");

//    if((tlg.fromRot() && tlg.toRot()) || (!tlg.fromRot() && !tlg.toRot()))
//    {
//        LogError(STDLOG) << "tlg.fromRot()=" << tlg.fromRot() <<
//                ", tlg.toRot()=" << tlg.toRot();
//        throw EXCEPTIONS::Exception("Invalid from/to router values");
//    }

//    unsigned tlgnumtry = 0;
//    while(tlgnumtry < 10)
//    {
//        tlg.setTlgNum(genNextTlgNum());

//        LogTrace(TRACE2) << "New tlg_num was generated: " << tlg.tlgNum();

//        CursCtl ins_tlg =
//                make_curs(
//                          "begin\n"
//                "INSERT INTO text_tlg_new"
//                "(msg_id, text, date1, multy_part, from_addr, to_addr, postponed, airline, hand_made) "
//                "VALUES(:tlg_id, :cur_frag, :local_time, :multi, :from_rot, :to_rot, :pp, :airl, :hm) "
//                "returning date1 into :d1;\n"
//                "end;");
//        ins_tlg.stb().
//                noThrowError(CERR_DUPK).
//                bind(":tlg_id", tlg.tlgNum().num).
//                bind(":cur_frag",num_part>1?string(tlg.text(),0,DbPartLen):tlg.text()).
//                bind(":local_time", OciCpp::to_oracle_datetime(Dates::second_clock::local_time())).
//                bind(":multi", num_part>1?1:0).
//                bind(":from_rot", tlg.fromRot(), tlg.fromRot()?&nnull:&null).
//                bind(":to_rot", tlg.toRot(),     tlg.toRot()?&nnull:&null).
//                bind(":pp", tlg.postponed()?1:0).
//                bind(":airl", tlg.airline(),     tlg.airline()?&nnull:&null).
//                bind(":hm",   tlg.handMade()).
//                bindOut(":d1", insDate).
//                exec();
//        if(ins_tlg.err() == CERR_DUPK)
//        {
//            LogWarning(STDLOG) << "tlgnumtry = " << tlgnumtry << " tlgnum " << tlg.tlgNum() <<
//                    " already in use";
//            tlgnumtry ++;
//            continue;
//        }
//        break;
//    }
//    if(tlgnumtry >= 10)
//    {
//        throw tick_fatal_except(STDLOG, Ticketing::EtErr::ProgErr, "uniq tlg num");
//    }

//    tlg.setReceiveDate(OciCpp::from_oracle_time(insDate));

//    if ( num_part>1 )
//    {
//        for (unsigned i=2;i<=num_part;i++)
//        {
//            tst();
//            CursCtl ins_part = make_curs(
//                    "INSERT INTO text_tlg_part(msg_id, text, part) "
//                    "VALUES(:tlg_id, :cur_frag, :frag_part)");
//            ins_part.
//                    bind(":tlg_id", tlg.tlgNum().num).
//                    bind(":cur_frag",string(tlg.text(),((i-1)*DbPartLen), DbPartLen)).
//                    bind(":frag_part",i).
//                    exec();
//        }
//    }

//    LogTrace(TRACE2) << "Tlg "<< tlg.tlgNum() <<
//                        " fragmented in " << num_part << " parts";

    throw EXCEPTIONS::Exception("Unimplimented method called!");
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
    : Text(txt), FromRot(fRot), ToRot(tRot), Postponed(false), HandMade(Net)
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
