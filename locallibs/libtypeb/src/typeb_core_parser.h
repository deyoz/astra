//
// C++ Interface: typeb_core_parser
//
// Description: �� typeb �����
// ����� ������ ���� ��騥 ������� ��� ��� ⨯�� type B ⥫��ࠬ�
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_CORE_PARSER_H_
#define _TYPEB_CORE_PARSER_H_
#include <vector>
#include "typeb_parse_exceptions.h"
#include "tb_elements.h"
#include "typeb_template.h"

#ifndef _TYPEB_CORE_PARSER_SECRET_
#error Do not include this file! Include typeb_message.h instead!
#endif

/**
 * @file  typeb_core_parser.h ��� typeb �����
 * ����� ������ ���� ��騥 ������� ��� ��� ⨯�� type B ⥫��ࠬ�
*/

namespace typeb_parser
{

struct TBCfg
{
    static const unsigned short MsgTypeLen = 3;
    static const unsigned short MaxLineLen = 69;
};

class TypeBMessage;
class AddrHead
{
    std::string Sender;
    std::list<std::string> Receiver;
    std::string MsgId;

    AddrHead(){}
    friend class TypeBMessage;
    static AddrHead parse(const std::string &first, const std::string &second);

    static const unsigned short AddrLen = 7;
    AddrHead(std::string from, std::list<std::string> to, std::string mId)
    :Sender(from), Receiver(to), MsgId(mId)
    {
    }
public:

    /**
     * ��ࠢ�⥫�
     * @return string
     */
    const std::string &sender() const { return Sender; }

    /**
     * �����⥫�
     * @return string
     */
    const std::list<std::string> &receiver() const { return Receiver; }

    /**
     * �����䨪��� ⥫��ࠬ��
     * @return std::string
     */
    const std::string &msgId() const { return MsgId; }
};

struct TypeBHeader
{
    TypeBHeader(const std::string& type_, const std::string& depPt_, const AddrHead& addr_, size_t part_, bool last_, const std::string& uid_)
        : type(type_), depPoint(depPt_), addr(addr_), part(part_), isLastPart(last_), uid(uid_)
    {}
    std::string type;
    std::string depPoint;
    AddrHead addr;
    size_t part;
    bool isLastPart;
    std::string uid;
};

// class typeb_template;
// class base_templ_group;
class TypeBMessage : public TbGroupElement
{
    std::string SrcMessage;
    std::string Type;
    AddrHead Addrs;
    TypeBMessage(std::string msg):SrcMessage(msg), Template(0){}

    struct MsgLines_t
    {
        std::vector <std::string> Msgl;
        std::string &operator[](unsigned i) { return Msgl[i];}
        const std::string &operator[](unsigned i) const { return Msgl[i];}
        unsigned CurrMsgLine;
        std::string::size_type CurrLinePos;
        unsigned CurrPrsLength;
        unsigned totalLines() const { return Msgl.size(); }

        MsgLines_t():CurrMsgLine(0),CurrLinePos(0),CurrPrsLength(0){}
    }Msg;

    /**
     * ��������� ⥫��ࠬ�� �� ��ப�
     */
    void split2lines();

    void check_message_general_rules();

    const typeb_template *Template;

    /**
     * �����ઠ ⥫� ⥫��ࠬ�� �� 蠡����
     * @param templ 蠡���
     * @param msgl ��ப� ᮮ�饭��
     * @return TbGroupElement::ptr_t
     */
    static TbGroupElement::ptr_t parse_group(const base_templ_group &templ,
                                             MsgLines_t &msgl,
                                             TbGroupElement *group_ptr=0);

    /**
     * ������ ������ ����� �� ��㯯�
     * @param templ ������
     * @param msgl ��ப� ��室���� ᮮ�饭��
     * @return TbListElement::ptr_t - ᯨ᮪ ࠧ��࠭��� �. ᮮ�饭��
     */
    static TbElement::ptr_t parse_switch_elem(const templ_switch_group &templ,
                                              MsgLines_t &msgl);

    /**
     * ������ ���ᨢ� ����த��� ����⮢
     * @param templ ������
     * @param msgl ��ப� ��室���� ᮮ�饭��
     * @return TbListElement::ptr_t - ᯨ᮪ ࠧ��࠭��� �. ᮮ�饭��
     */
    static TbListElement::ptr_t parse_array(const typeb_template::const_elem_iter &iter_in,
                                            const typeb_template::const_elem_iter &enditer,
                                            const tb_descr_element &templ,
                                            MsgLines_t &msgl);

    /**
     * ������ ������ �����
     * @param templ ������
     * @param msgl ��ப� ��室���� ᮮ�饭��
     * @return TbElement::ptr_t
     */
    static TbElement::ptr_t
            parse_singleline_element(const typeb_template::const_elem_iter &iter_in,
                                     const typeb_template::const_elem_iter &enditer,
                                     const tb_descr_element & templ,
                                     MsgLines_t & msgl);

    static void addNextFewLines(const tb_descr_element & templ,
                                const base_templ_group::const_elem_iter &iter_in,
                                const base_templ_group::const_elem_iter &enditer,
                                MsgLines_t & msgl, std::string &str, std::string::size_type &end_pos);

    /**
     * ��⠭����� 蠡��� ��� ������� ᮮ�饭��
     * @param t ������
     */
    inline void setTemplate(const typeb_template *t) { Template = t; }

    static std::string::size_type
            GetNextMsgPosition(const typeb_template::const_elem_iter &iter_in,
                               const typeb_template::const_elem_iter &enditer,
                               const std::string &currline,
                               std::string::size_type st);

public:

    /**
     * ������� 蠡��� ��� ᮮ�饭��
     * @return ������ ��� ������� ᮮ�饭��
     */
    inline const typeb_template *templ() { return Template; }
    
    /**
     * ����� � ��ப� �� ������
     * @param i ����� ��ப�
     * @return ��ப� - string
     */
    const std::string &msgline(unsigned int i) const { return Msg[i]; }
    
    /**
     * @return �᫮ ��ப � ᮮ�饭��
     */
    unsigned totalLines() const { return Msg.Msgl.size(); }

    /**
     * �����ઠ ⥫��ࠬ��
     * @param message ��室�� ⥪�� type b ⥫��ࠬ��
     * @return class TypeBMessage
     */
    static TypeBMessage parse(const std::string &message);

    /**
     * �����ઠ ���ᮢ ⥫��ࠬ��
     * @param message ��室�� ⥪�� type b ⥫��ࠬ��
     * @return class AddrHead
     */
    static AddrHead parseAddr(const std::string &message);

    /**
     * �����ઠ ��������� ⥫��ࠬ��
     * @param message ��室�� ⥪�� type b ⥫��ࠬ��
     * @return class TypeBHeader
     */
    static TypeBHeader parseHeader(const AddrHead &addr, const std::string &message);

    /**
     * �஢�ઠ, ���� �� ⥫��ࠬ�� TypeB
     * @param tlg ����� ⫣
     * @return true/false
     */
    static bool is_it_typeb(const std::string &tlg);

    /**
     * ��室�� ⥪�� ��ࠡ�⠭��� ⫣
     * @return SrcMessage
     */
    const std::string &msgSource() const { return SrcMessage; }

    /**
     * ��������� ⥫��ࠬ�� - ���� ��ࠢ�⥫� � �����⥫�
     * @return AddrHead
     */
    const AddrHead &addrs() const { return Addrs; }

    /**
     * 3-� �㪢���� ⨯ ⥫��ࠬ��
     * @return string
     */
    const std::string &type() const { return Type; }
};

std::ostream & operator << (std::ostream& os, const TypeBMessage &msg);

std::ostream & operator << (std::ostream& os, const AddrHead &addr);

}

#endif /*_TYPEB_CORE_PARSER_H_*/
