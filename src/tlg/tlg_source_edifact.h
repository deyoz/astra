//
// C++ Interface: tlg_source_edifact
//
// Description: tlg_source: Специфика EDIFACT телеграмм
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
     * символ разделитель строк в телеграмме
     * Вообще-то в edifact телеграмме разделитель зависит от charset'а и маскирующего символа
     * но для тестов и так сойдет
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

    virtual const char *name() const { return "EDIFACT"; }
    virtual tlg_type_t type() const { return TlgType::edifact; }
    void readH2H();
    /**
     * Запись в базу (переопределение)
     */
    virtual void write();

    const hth::HthInfo *h2h() const { return H2h.get(); }

    void setH2h(const hth::HthInfo &h2h_) {
        H2h.reset(new hth::HthInfo(h2h_));
    }

    static bool isItYours(const std::string &txt);

    /**
     * Установить подтип телеграммы
     * Reimplement
     * @param stype  type
     */
    virtual void setTlgSubtype(const std::string &stype);

    virtual const std::string& text2view() const;
    /**
     * @brief Сравнение телеграмм (вместе с h2h!)
     * @param t
     * @return
     */
    virtual bool operator == (const TlgSourceEdifact &t) const;
};

/**
 * @brief Печать телеграммы вместе с h2h
 * @param os
 * @param tlg
 * @return
 */
std::ostream & operator << (std::ostream& os, const TlgSourceEdifact &tlg);
}
