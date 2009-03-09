//
// C++ Interface: EdiSessionAstra
//
// Description: Makes an EDIFACT request structure
//
//
// Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
//
//
#ifndef _ASTRAEDISESSWR_H_
#define _ASTRAEDISESSWR_H_

std::string get_edi_addr();
std::string get_edi_own_addr();

namespace edifact
{

class AstraEdiSessWR : public edilib::EdiSess::EdiSessWrData
{
    edilib::EdiSess::EdiSession EdiSess;
    edi_mes_head EdiHead;
    std::string Pult;
public:
    AstraEdiSessWR(const std::string &pult)
    : Pult(pult)
    {
        memset(&EdiHead, 0, sizeof(EdiHead));
    }

    virtual edilib::EdiSess::EdiSession *ediSession() { return &EdiSess; }

    virtual hth::HthInfo *hth() { return 0; };
    virtual std::string sndrHthAddr() const { return ""; };
    virtual std::string rcvrHthAddr() const { return ""; };
    virtual std::string hthTpr() const { return ""; };

    // В СИРЕНЕ это recloc/ или our_name из sirena.cfg
    // Идентификатор сессии
    virtual std::string baseOurrefName() const
    {
        return "ASTRA";
    }
    virtual edi_mes_head *edih()
    {
        return &EdiHead;
    }
    // Внешняя ссылка на сессию
    // virtual int externalIda() const { return 0; }
    // Пульт
    virtual std::string pult() const { return Pult; };

    // Аттрибуты сообщения
/*    virtual std::string syntax() const { return "IATA"; }
    virtual unsigned syntaxVer() const { return 1; }
    virtual std::string ctrlAgency() const { return "IA"; }
    virtual std::string version() const { return "96"; }
    virtual std::string subVersion() const { return "2"; }*/
    virtual std::string ourUnbAddr() const { return get_edi_own_addr(); }
    virtual std::string unbAddr() const { return get_edi_addr(); }
};

class AstraEdiSessRD : public edilib::EdiSess::EdiSessRdData
{
    edi_mes_head *Head;
    //H2host H2H;
    bool isH2H;
    std::string rcvr;
    std::string sndr;
    public:
        AstraEdiSessRD():
        isH2H(false)
        {
        }

        virtual hth::HthInfo *hth() { return 0; };
        virtual std::string sndrHthAddr() const { return ""; };
        virtual std::string rcvrHthAddr() const { return ""; };
        virtual std::string hthTpr() const { return ""; };

        virtual std::string baseOurrefName() const
        {
            return "ASTRA";
        }

        void setMesHead(edi_mes_head &head)
        {
            Head = &head;
        }
        virtual edi_mes_head *edih()
        {
            return Head;
        }
};
} // namespace edifact
#endif /*_ASTRAEDISESSWR_H_*/
