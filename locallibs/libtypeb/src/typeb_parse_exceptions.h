//
// C++ Interface: typeb_parse_exceptions
//
// Description: exceptions at time of parsing
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_PARSE_EXCEPTIONS_H_
#define _TYPEB_PARSE_EXCEPTIONS_H_
#include <serverlib/slogger_nonick.h>
#include <serverlib/exception.h>
#include <boost/format/format_fwd.hpp>
#include <boost/shared_ptr.hpp>
#include <etick/etick_localization.h>
#include <typeb/tb_elements.h>

namespace typeb_parser
{

class typeb_except : public comtech::Exception
{
public:
    typeb_except(const char *n, const char *f, unsigned l,
                 const std::string &text) throw ()
    :comtech::Exception(n,f,l,"",text)
    {
    }

    virtual ~typeb_except() throw () {}
};

class typeb_parse_except : public typeb_except
{
    unsigned ErrLine;
    std::string::size_type ErrPos;
    std::string::size_type ErrLength;
    std::string ErrCode;
    mutable boost::shared_ptr<Ticketing::EtickLocalization> pLocalize;

    static const std::string Localize(std::string ErrCode);
public:
    typeb_parse_except(const char *nickname, const char *file, int line,
                       const std::string &ErrC, const std::string &ErrT,
                       unsigned errl=0, unsigned errp=0, unsigned errlen = 0)
    :typeb_except(nickname, file, line, ErrT),
         ErrLine(errl),ErrPos(errp), ErrLength(errlen),
         ErrCode(ErrC)
    {
        pLocalize.reset(new Ticketing::EtickLocalization(ErrC, RUSSIAN));
    }

    typeb_parse_except(const char *nickname, const char *file, int line,
                       const std::string &ErrC,
                       unsigned errl=0, unsigned errp=0, unsigned errlen = 0)
    :typeb_except(nickname, file, line, ErrC),
        ErrLine(errl), ErrPos(errp), ErrLength(errlen),
        ErrCode(ErrC)
    {
        pLocalize.reset(new Ticketing::EtickLocalization(ErrC, RUSSIAN));
    }

    typeb_parse_except(const char *nickname, const char *file, int line,
                       const std::string &ErrC, const TbElement &tb)
    :typeb_except(nickname, file, line, ErrC),
        ErrLine(tb.line()), ErrPos(tb.position()), ErrLength(tb.length()),
        ErrCode(ErrC)
    {
        pLocalize.reset(new Ticketing::EtickLocalization(ErrC, RUSSIAN));
    }

    void setErrLine(unsigned el) { ErrLine = el;}
    void setErrPos(std::string::size_type ep)  { ErrPos  = ep;}
    void setErrLength(std::string::size_type elen)  { ErrLength = elen;}

    /**
     * @brief ��ப� � ᮮ�饭�� �� ������ ᯮ�몭㫨��
     * @return unsigned
     */
    unsigned errLine() const { return ErrLine; }
    /**
     * @brief ������ � ��ப� ᮮ�饭�� �� ������ ᯮ�몭㫨��
     * @return std::string::size_type
     */
    std::string::size_type errPos () const { return ErrPos;  }

    /**
     * @brief error area length
     * @return std::string::size_type
     */
    std::string::size_type errLength() const { return ErrLength; }

    /**
     * @brief ��� �訡��
     * @return ErrCode, smth like TBMsg::PROG_ERR
     */
    const std::string &errCode() const { return ErrCode; }

    /**
     * @brief ����� �訡��
     * @return ErrText, smth line '�訡�� � �ணࠬ��'
     */
    std::string errText() const;

    /**
     * @brief ��।��� ��ࠬ��� � ��������஢���� ⥪��
     * @param p ��ࠬ���
     * @return *this
     */
    typeb_parse_except & operator << (const std::string &p);
    /**
     * @brief ��।��� ��ࠬ��� � ��������஢���� ⥪��
     * @param p ��ࠬ���
     * @return *this
    */
    typeb_parse_except & operator << (const char *p);
    virtual ~typeb_parse_except() throw () {}
};

inline typeb_parse_except typeb_lz_parse_except(const char *nickname, const char *file,
         int line, const std::string &ErrC)
{
    return typeb_parse_except(nickname, file, line, ErrC);
}

inline typeb_parse_except typeb_lz_parse_except(const char *nickname, const char *file,
        int line, const std::string &ErrC, const std::string &ErrT)
{
    return typeb_parse_except(nickname, file, line, ErrC, ErrT);
}
} // namespace typeb_parser
#endif /*_TYPEB_PARSE_EXCEPTIONS_H_*/
