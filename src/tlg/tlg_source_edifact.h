//
// C++ Interface: tlg_source_edifact
//
// Description: tlg_source: ����䨪� EDIFACT ⥫��ࠬ�
//
#pragma once

#include "tlg_source.h"

#include <libtlg/hth.h>
#include <edilib/edi_session.h>
//#include <edilib/edi_tick_msg_types.h>

namespace TlgHandling
{

class TlgSourceEdifact : public TlgSourceTypified
{
    boost::shared_ptr<hth::HthInfo> H2h;

    mutable std::string Text2View;
protected:
    /**
     * ᨬ��� ࠧ����⥫� ��ப � ⥫��ࠬ��
     * �����-� � edifact ⥫��ࠬ�� ࠧ����⥫� ������ �� charset'� � ��᪨���饣� ᨬ����
     * �� ��� ��⮢ � ⠪ ᮩ���
     * @return
     */
    virtual std::string textSpliter() const { return "'"; }
public:
    TlgSourceEdifact(const char * tlgText):TlgSourceTypified(TlgSource(tlgText)){}
    TlgSourceEdifact(const std::string & tlgText):TlgSourceTypified(TlgSource(tlgText)){}
    TlgSourceEdifact(const TlgSource &src):TlgSourceTypified(src){ readH2H(); }
    TlgSourceEdifact(const TlgSource &src,
                     const hth::HthInfo *h2h_):
            TlgSourceTypified(src)
    {
        if (h2h_)
            H2h.reset(new hth::HthInfo(*h2h_));
    }

    virtual const char *name() const { return "TPA"; }
    virtual const char *nameToDB(bool out) const { return out?"OUTA":"INA"; }
    virtual tlg_type_t type() const { return TlgType::edifact; }
    void readH2H();
    /**
     * ������ � ����
     */
    virtual void write();

    const hth::HthInfo *h2h() const { return H2h.get(); }

    void setH2h(const hth::HthInfo &h2h_) {
        H2h.reset(new hth::HthInfo(h2h_));
    }

    static bool isItYours(const std::string &txt);

    virtual const std::string& text2view() const;
    /**
     * @brief �ࠢ����� ⥫��ࠬ� (����� � h2h!)
     * @param t
     * @return
     */
    virtual bool operator == (const TlgSourceEdifact &t) const;
};

/**
 * @brief ����� ⥫��ࠬ�� ����� � h2h
 * @param os
 * @param tlg
 * @return
 */
std::ostream & operator << (std::ostream& os, const TlgSourceEdifact &tlg);
}
