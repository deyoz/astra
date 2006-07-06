#ifndef _EDI_TLG_H_
#define _EDI_TLG_H_
#include "edilib/edi_session.h"
#include "edilib/edi_session_cb.h"
#include "astra_ticket.h"

class AstraEdiSessWR : public edilib::EdiSess::EdiSessWrData
{
    edilib::EdiSess::EdiSession EdiSess;
    edi_mes_head EdiHead;
public:
    AstraEdiSessWR()
    : EdiSess(edilib::EdiSess::CreateEdiSess())
    {
        memset(&EdiHead, 0, sizeof(EdiHead));
    }

    virtual edilib::EdiSess::EdiSession *ediSession() { return &EdiSess; }
    virtual edilib::EdiSess::H2host *h2h() { return 0; }

    virtual std::string sndrH2hAddr() const { return "";}
    virtual std::string rcvrH2hAddr() const { return "";}
    virtual std::string H2hTpr() const { return ""; }

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
    virtual std::string pult() const { return "ASTRA1"; };

    // Аттрибуты сообщения
/*    virtual std::string syntax() const { return "IATA"; }
    virtual unsigned syntaxVer() const { return 1; }
    virtual std::string ctrlAgency() const { return "IA"; }
    virtual std::string version() const { return "96"; }
    virtual std::string subVersion() const { return "2"; }*/
    virtual std::string ourUnbAddr() const { return "ASTRA"; }
    virtual std::string unbAddr() const { return "ETICK"; }
};

class edi_udata
{
    AstraEdiSessWR *SessData;
    std::string MsgId;
public:
    edi_udata(AstraEdiSessWR *sd, const std::string &msg_id)
    : SessData(sd), MsgId(msg_id){}

    AstraEdiSessWR *sessData() { return SessData; }
    const std::string &msgId() const { return MsgId; }

    ~edi_udata(){ delete SessData; }
};

//======================================
class edi_common_data
{
    OrigOfRequest Org;
public:
    const OrigOfRequest &org() const { return Org; }
    virtual ~edi_common_data(){}
};

enum TickDispType_t {
    TickDispByTickNo=0,
};

class TickDisp : public edi_common_data
{
    TickDispType_t DispType;
public:
    TickDisp(TickDispType_t dt)
    :DispType(dt)
    {
    }
    TickDispType_t dispType() { return DispType; }
};

class TickDispByNum : public TickDisp
{
    std::string TickNum;
public:
    TickDispByNum(const std::string &ticknum)
    :   TickDisp(TickDispByTickNo),
        TickNum(ticknum)
    {
    }

    const std::string tickNum() const { return TickNum; }
};


void SendEdiTlgTKCREQ_Disp(TickDisp &TDisp);

typedef void (* message_func_t)
        (edi_mes_head *pHead, edi_udata &udata, edi_common_data &data);
struct message_funcs_type{
    const char *msg_code;
    message_func_t parse;
    message_func_t proc;
    message_func_t collect_req;
    const char *func_name;
};
struct message_funcs_str{
    edi_msg_types_t msg_type;
    const char *mes_name;
    message_funcs_type *mes_f;
    size_t mes_f_len;
};

class EdiMesFuncs
{
    typedef std::map<std::string, message_funcs_type> types_map_t;
    typedef std::map<edi_msg_types_t, types_map_t> messages_map_t;
    static messages_map_t *messages_map;
    static messages_map_t *get_map() {
        if(!messages_map)
            messages_map = new messages_map_t;

        return messages_map;
    }
    EdiMesFuncs() {}
public:
    static void init_messages(message_funcs_str *mes_funcs, size_t count)
    {
        for(size_t i=0;i<count;i++){
            types_map_t tmap;
            for(size_t j=0;j<mes_funcs[i].mes_f_len;j++){
                tmap[mes_funcs[i].mes_f[j].msg_code] = mes_funcs[i].mes_f[j];
            }
            (*get_map())[mes_funcs[i].msg_type] = tmap;
        }
    }
    static const message_funcs_type &GetEdiFunc(edi_msg_types_t mes_type, const std::string &msg_code);
};

#endif /*_EDI_TLG_H_*/
