//
// C++ Interface: tlg_source
//
// Description: ��室�� ⥪�� ⥫��ࠬ�
// �⥭��/������ � ����
//
#pragma once

#include "tlg_types.h"
#include "CheckinBaseTypes.h"

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace TlgHandling
{

/**
 * @class TlgSource - ����� c ��室�� ⥪�⮬ ⥫��ࠬ��
 */
class TlgSource
{
public:
    enum HandMadeKind
    {
        Net = 0,    // ��� �� ��
        Human = 1,  // ��� ᮧ���� �����஬
        Bot = 2,    // ��� ᮧ���� ��⮬ (���ਬ��, �९����஬)
    };
private:
    std::string Text;
    std::string FromRot;
    std::string ToRot;
    boost::posix_time::ptime ReceiveDate; // AutoGen
    boost::posix_time::ptime ProcessDate; // AutoGen
    boost::optional<tlgnum_t> TlgNum; // AutoGen
    std::string ErrorText;
    boost::optional<tlgnum_t> AnswerTlgNum; // ����� �⢥⭮� ⥫��ࠬ��
    bool Postponed;
    int HandMade;
    int GatewayNum;

    std::string TypeStr;
    std::string SubtypeStr;
    std::string FuncCodeStr;

    void setReceiveDate(const boost::posix_time::ptime &date)
    {
        ReceiveDate = date;
    }
    void setProcessDate(const boost::posix_time::ptime &date)
    {
        ProcessDate = date;
    }
protected:
    void setTypeStr(const std::string &t);
    void setSubtypeStr(const std::string &st)
    {
        SubtypeStr = st;
    }
    void setFuncCodeStr(const std::string &fc)
    {
        FuncCodeStr = fc;
    }
protected:
    /**
     * ������ � ����
     * ��⠭�������� ����� ⥫��ࠬ�� � ����
     * ���⮬�-� � �ਭ����� �� ����⠭��� ��뫪�
     * @param tlg TlgSource
     */
    static void writeToDb(TlgSource &tlg);

    /**
     * ��᪫��뢠�� ⥪�� ⥫��ࠬ�� �� ��ப� (�㦭� ��� �㭪樨 diff)
     * @param spliter - ᨬ��� ࠧ����⥫�
     * @return list of tlg lines
     */
    virtual std::list<std::string> splitText(const std::string &spliter) const;

    /**
     * ᨬ��� ࠧ����⥫� ��ப � ⥫��ࠬ��
     * @return
     */
    virtual std::string textSpliter() const { return "\n"; }
public:
    static const unsigned DbPartLen = 2000;

    TlgSource(const std::string& txt,
              const std::string& fRot = "",
              const std::string& tRot = "");

    virtual ~TlgSource()
    {
    }

    /**
     * ����� ⥫��ࠬ��
     * @return string
     */
    virtual const std::string &text() const { return Text; }
    /**
     * ����� ���筨�
     * @return canon_name
     */
    virtual const std::string& fromRot() const { return FromRot; }

    /**
     * ����� �����祭��
     * @return canon_name
     */
    virtual const std::string& toRot() const { return ToRot; }

    /**
     * ��� �ਥ�� ⥫��ࠬ��
     * @return boost::date_time::ptime &
     */
    virtual const boost::posix_time::ptime &receiveDate() const { return ReceiveDate; }

    /**
     * ����� ⥫��ࠬ��
     * @return tlgnum_t
     */
    virtual const boost::optional<tlgnum_t> &tlgNum() const { return TlgNum; }

    /**
     * �᫨ ⥫��ࠬ�� ��ࠡ�⠭� ��� �ਭ�� �� �� � �訡���,
     * � �� ⥪�� ���
     * @return string
     */
    virtual const std::string &errorText() const { return ErrorText; }

    /**
     * @brief tlg has been postponed
     * @return
     */
    bool postponed() const { return Postponed; }

    /**
     * @brief hand made, no answer needed
     * @return ��뫪�, �⮡� �� ᫮���� unstb binds
     */
    int handMade() const { return HandMade; }

    int gatewayNum() const { return GatewayNum; }


    /**
     * ��⠭����� ⥪�� �訡��
     * @param txt ����� �訡��
     */
    virtual void setErrorText(const std::string &txt) { ErrorText = txt; }

    /**
     * �������� ������ � �� (१����� ��ࠡ�⪨)
     * �ய���� ����� �⢥⭮� ⥫��ࠬ��,
     * ���⠢��� ���� ��ࠡ�⪨ ⫣ (��⥬��� �६�)
     * ���⠢��� �������� � ⨯ ⥫��ࠬ��
     */
    virtual void writeHandlingResulst() const;

    /**
     * ��⠭����� ���� �����祭��
     * @param r router num
     */
    virtual void setToRot(const std::string& r)
    {
        ToRot = r;
    }

    /**
     * ��⠭����� ���� ���筨��
     * @param r router num
     */
    virtual void setFromRot(const std::string& r)
    {
        FromRot = r;
    }

    /**
     * @brief exchange fromRot and toRot between each others
     */
    virtual void swapRouters();

    /**
     * ����� �⢥⭮� ⥫��ࠬ��
     */
    virtual const boost::optional<tlgnum_t>& answerTlgNum() const
    {
        return AnswerTlgNum;
    }

    /**
     * ��� ��ࠡ�⪨
     * @return boost::date_time::ptime &
     */
    virtual const boost::posix_time::ptime &processDate() const { return ProcessDate; }

    /**
     * ������� ����� �⢥⭮� ⥫��ࠬ��
     * @param tnum
     */
    virtual void setAnswerTlgNum(const tlgnum_t& tnum)
    {
        AnswerTlgNum = tnum;
    }

    /**
     * @brief Update DB: set an answer msg_id
     * @param tnum_src
     * @param tnum_answ
     */
    static void setAnswerTlgNumDb(const tlgnum_t& tnum_src, const tlgnum_t& tnum_answ);

    /**
     * @brief sets postponed flag
     */
    void setPostponed(bool flg=true) { Postponed = flg; }

    /**
     * ��⠭����� ��������
     * @param  airl
     */
//    virtual void setAirline(const Ticketing::Airline_t &airl)
//    {
//        Airline = airl;
//    }

    void setTlgNum(const tlgnum_t& tnum)
    {
        TlgNum = tnum;
    }
    void setTlgNum(int tnum_deprecated);

    void setGatewayNum(int gwNum)
    {
        GatewayNum = gwNum;
    }

    /**
     * @brief hand made flag
     * @param val
     */
    virtual void setHandMade(int val) { HandMade = val; }

    /**
     * �⥭�� �� ���� �� ������
     * @return TlgSource
     */
    static TlgSource readFromDb(const tlgnum_t& num);

    /**
     * �����஢��� ᫥� ����� ⥫��ࠬ��
     * @return tlgnum_t
     */
    static tlgnum_t genNextTlgNum();

    /**
     * ��⠥� �� ���� ���� ����祭�� ⫣ �� �� ������
     * @param  tlgnum_t - �����
     * @return boost::posix_time::ptime - ��� ����祭��
     */
    static boost::posix_time::ptime receiveDate(const tlgnum_t& tnum);

    /**
     * ������ � ���� ⥪�� �訡��
     * @param tnum ����� ⫣
     * @param err ����� �訡��
     */
     static void setDbErrorText(const tlgnum_t& tnum, const std::string &err);

     /**
      * @brief ������ ����� ⥪�⠬� ⥫��ࠬ� � �ଠ� ������� � GNU diff
      * @param tsrc ⥫��ࠬ�� ��� �ࠢ�����
      * @param skeepfl skeep first lines
      * @param skeepll skeep last lines
      * @return diff str
      */
     std::string diff(const TlgSource &tsrc, unsigned skeepfl=0, unsigned skeepll=0, bool ignore_xxx=true) const;

    /**
     * ������ � ����
     */
    virtual void write() { TlgSource::writeToDb(*this); };
};

/// @class TlgSourceTypified ������ ����� ⨯���஢����� ⫣
class TlgSourceTypified : public TlgSource
{
public:
    TlgSourceTypified(const TlgSource &src);

    /**
     * ��� ⨯� ⫣ ��� �⮡ࠦ����
     * @return const char *
     */
    virtual ~TlgSourceTypified()
    {
    }

    virtual const char *name() const = 0;

    /**
     * ��� ⥫��ࠬ�� �᫮�
     * @return tlg_type_t. see tlg_types.h
     */
    virtual tlg_type_t type() const = 0;

    /**
     * ����� �� ।���஢��� ��� ⨯ ⥫��ࠬ� � �����
     * default: false
     * @return bool
     */
    virtual bool editable() const { return true; }

    /**
     * ����� ��� �⮡ࠦ���� �� �࠭� �ନ����
     * @return return string
     */
    virtual const std::string &text2view() const { return text(); }

    /**
     * @brief Error highlighted html based message text.
     * @return
     */
    virtual std::string text2viewHtml() const { return text2view(); }

    /**
     * ��⠭����� ���⨯ ⥫��ࠬ��
     * @param stype  type
     */
    virtual void setTlgSubtype(const std::string &stype) = 0;
};

typedef boost::shared_ptr <TlgSourceTypified> TlgSourceTypifiedPtr_t;

class TlgSourceUnknown : public TlgSourceTypified
{
public:
    TlgSourceUnknown (const TlgSource &src):TlgSourceTypified(src)
    {
    }
    virtual ~TlgSourceUnknown()
    {
    }

    virtual const char *name() const { return "UNKNOWN"; }
    virtual tlg_type_t type() const { return TlgType::unknown; }
    virtual void setTlgSubtype(const std::string &stype){}
};

/**
 * ������� ⫣ � ��⮪
 * @param os ��⮪
 * @param  tlg - ����� ⥫��ࠬ��
 */
std::ostream & operator << (std::ostream& os, const TlgSource &tlg);
std::ostream & operator << (std::ostream& os, const TlgSourceTypified &tlg);
}
