//
// C++ Interface: tlg_source
//
// Description: Исходный текст телеграмм
// чтение/запись в базу
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
 * @class TlgSource - класс c исходным текстом телеграммы
 */
class TlgSource
{
public:
    enum HandMadeKind
    {
        Net = 0,    // Тлг из сети
        Human = 1,  // Тлг создана оператором
        Bot = 2,    // Тлг создана ботом (например, препроцессором)
    };
private:
    std::string Text;
    std::string FromRot;
    std::string ToRot;
    boost::posix_time::ptime ReceiveDate; // AutoGen
    boost::posix_time::ptime ProcessDate; // AutoGen
    tlgnum_t TlgNum; // AutoGen
    std::string ErrorText;
    tlgnum_t AnswerTlgNum; // Номер ответной телеграммы
    bool Postponed;
    int HandMade;

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
     * Запись в базу
     * Устанавливает номер телеграммы и дату
     * Поэтому-то и принемает не константную сцылку
     * @param tlg TlgSource
     */
    static void writeToDb(TlgSource &tlg);

    /**
     * Раскладывает текст телеграммы на строки (нужно для функции diff)
     * @param spliter - символ разделитель
     * @return list of tlg lines
     */
    virtual std::list<std::string> splitText(const std::string &spliter) const;

    /**
     * символ разделитель строк в телеграмме
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
     * Текст телеграммы
     * @return string
     */
    virtual const std::string &text() const { return Text; }
    /**
     * Роутер источник
     * @return canon_name
     */
    virtual const std::string& fromRot() const { return FromRot; }

    /**
     * Роутер назначения
     * @return canon_name
     */
    virtual const std::string& toRot() const { return ToRot; }

    /**
     * Дата приема телеграммы
     * @return boost::date_time::ptime &
     */
    virtual const boost::posix_time::ptime &receiveDate() const { return ReceiveDate; }

    /**
     * Номер телеграммы
     * @return tlgnum_t
     */
    virtual const tlgnum_t &tlgNum() const { return TlgNum; }

    /**
     * Если телеграмма обработана или принята из сети с ошибкой,
     * то ее текст тут
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
     * @return ссылка, чтобы не сломать unstb binds
     */
    int handMade() const { return HandMade; }


    /**
     * Установить текст ошибки
     * @param txt Текст ошибки
     */
    virtual void setErrorText(const std::string &txt) { ErrorText = txt; }

    /**
     * Обновить запись в БД (результаты обработки)
     * Прописать номер ответной телеграммы,
     * Выставить дату обработки тлг (системное время)
     * проставить компанию и тип телеграммы
     */
    virtual void writeHandlingResulst() const;

    /**
     * Установить роутер назначения
     * @param r router num
     */
    virtual void setToRot(const std::string& r)
    {
        ToRot = r;
    }

    /**
     * Установить роутер источникы
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
     * Номер ответной телеграммы
     */
    virtual const tlgnum_t& answerTlgNum() const
    {
        return AnswerTlgNum;
    }

    /**
     * Дата обработки
     * @return boost::date_time::ptime &
     */
    virtual const boost::posix_time::ptime &processDate() const { return ProcessDate; }

    /**
     * Указать номер ответной телеграммы
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
     * Установить компанию
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

    /**
     * @brief hand made flag
     * @param val
     */
    virtual void setHandMade(int val) { HandMade = val; }

    /**
     * Чтение из базы по номеру
     * @return TlgSource
     */
    static TlgSource readFromDb(const tlgnum_t& num);

    /**
     * Генерировать след номер телеграммы
     * @return tlgnum_t
     */
    static tlgnum_t genNextTlgNum();

    /**
     * Читает из базы дату получения тлг по ее номеру
     * @param  tlgnum_t - номер
     * @return boost::posix_time::ptime - дата получения
     */
    static boost::posix_time::ptime receiveDate(const tlgnum_t& tnum);

    /**
     * Запись в базу текста ошибки
     * @param tnum Номер тлг
     * @param err Текст ошибки
     */
     static void setDbErrorText(const tlgnum_t& tnum, const std::string &err);

     /**
      * @brief Разница между текстами телеграмм в формате близком к GNU diff
      * @param tsrc телеграмма для сравнения
      * @param skeepfl skeep first lines
      * @param skeepll skeep last lines
      * @return diff str
      */
     std::string diff(const TlgSource &tsrc, unsigned skeepfl=0, unsigned skeepll=0, bool ignore_xxx=true) const;

    /**
     * Запись в базу
     */
    virtual void write() { TlgSource::writeToDb(*this); };
};

/// @class TlgSourceTypified базовый класс типизированной тлг
class TlgSourceTypified : public TlgSource
{
public:
    TlgSourceTypified(const TlgSource &src);

    /**
     * Имя типа тлг для отображения
     * @return const char *
     */
    virtual ~TlgSourceTypified()
    {
    }

    virtual const char *name() const = 0;

    /**
     * Тип телеграммы числом
     * @return tlg_type_t. see tlg_types.h
     */
    virtual tlg_type_t type() const = 0;

    /**
     * Можно ли редактировать этот тип телеграмм в ручную
     * default: false
     * @return bool
     */
    virtual bool editable() const { return true; }

    /**
     * Текст для отображения на экране терминала
     * @return return string
     */
    virtual const std::string &text2view() const { return text(); }

    /**
     * @brief Error highlighted html based message text.
     * @return
     */
    virtual std::string text2viewHtml() const { return text2view(); }

    /**
     * Установить подтип телеграммы
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
 * Засунуть тлг в поток
 * @param os поток
 * @param  tlg - класс телеграммы
 */
std::ostream & operator << (std::ostream& os, const TlgSource &tlg);
std::ostream & operator << (std::ostream& os, const TlgSourceTypified &tlg);
}
