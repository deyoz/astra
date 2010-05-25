#ifndef _EDI_TLG_H_
#define _EDI_TLG_H_
#include "edilib/edi_session.h"
#include "edilib/edi_session_cb.h"
#include "astra_ticket.h"
#include "serverlib/monitor_ctl.h"
#include "libtlg/hth.h"

bool get_et_addr_set( std::string airline, int flt_no, std::pair<std::string,std::string> &addrs );
void set_edi_addrs( const std::pair<std::string,std::string> &addrs );
std::string get_edi_addr();
std::string get_edi_own_addr();

std::string get_last_session_ref();

struct EdiMess
{
    static const std::string Display;
    static const std::string ChangeStat;
};

class AstraEdiSessWR : public edilib::EdiSessWrData
{
    edilib::EdiSession EdiSess;
    edi_mes_head EdiHead;
    std::string Pult;
public:
    AstraEdiSessWR(const std::string &pult)
    : Pult(pult)
    {
        memset(&EdiHead, 0, sizeof(EdiHead));
    }

    virtual edilib::EdiSession *ediSession() { return &EdiSess; }
    virtual hth::HthInfo *hth() { return 0; };

    virtual std::string sndrHthAddr() const { return ""; };
    virtual std::string rcvrHthAddr() const { return ""; };
    virtual std::string hthTpr() const { return ""; };

    // В СИРЕНЕ это recloc/ или our_name из sirena.cfg
    // Идентификатор сессии
    virtual std::string baseOurrefName() const { return "ASTRA"; };

    virtual edi_mes_head *edih() { return &EdiHead; };
    virtual const edi_mes_head *edih() const { return &EdiHead; };
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

class AstraEdiSessRD : public edilib::EdiSessRdData
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
        virtual const edi_mes_head *edih() const { return Head; };
        virtual edi_mes_head *edih() { return Head; };
};

class edi_udata
{
    edilib::EdiSessData *SessData;
//     ServerFramework::EdiHelpManager EdiHelpMng;
public:
    edi_udata(edilib::EdiSessData *sd)
    :SessData(sd)/*,EdiHelpMng(ServerFramework::EdiHelpManager(15))*/
    {
    }
    edilib::EdiSessData *sessData() {return SessData; }
//     ServerFramework::EdiHelpManager *ediHelp() { return &EdiHelpMng; }
    virtual ~edi_udata(){ delete SessData; }
};

class edi_udata_wr : public edi_udata
{
    std::string MsgId;
public:
    edi_udata_wr(AstraEdiSessWR *sd, const std::string &msg_id)
    : edi_udata(sd), MsgId(msg_id){}

    AstraEdiSessWR *sessDataWr()
    {
        return &dynamic_cast<AstraEdiSessWR &>(*sessData());
    }
    const std::string &msgId() const { return MsgId; }
};

class edi_udata_rd : public edi_udata
{
    const std::string &TlgText;
public:
    edi_udata_rd(AstraEdiSessRD *sd, const std::string &tlg)
    : edi_udata(sd), TlgText(tlg){}

    AstraEdiSessRD *sessDataRd()
    {
        return &dynamic_cast<AstraEdiSessRD &>(*sessData());
    }

    const std::string tlgText() const { return TlgText; }
};

//======================================
class edi_common_data
{
    Ticketing::OrigOfRequest Org;
    std::string ediSessCtxt;
    int reqCtxtId;
public:
    edi_common_data(const Ticketing::OrigOfRequest &org,
                    const std::string &ctxt,
                    const int req_ctxt_id)
        :Org(org), ediSessCtxt(ctxt)
    {
      reqCtxtId = req_ctxt_id;
    }
    const Ticketing::OrigOfRequest &org() const { return Org; }
    const std::string & context() const { return ediSessCtxt; }
    const int req_ctxt_id() const { return reqCtxtId; }
    virtual ~edi_common_data(){}
};

enum TickDispType_t {
    TickDispByTickNo=0,
};

class TickDisp : public edi_common_data
{
    TickDispType_t DispType;
    int reqCtxtId;
public:
    TickDisp(const Ticketing::OrigOfRequest &org,
             const std::string &ctxt,
             const int req_ctxt_id,
             TickDispType_t dt)
    :edi_common_data(org, ctxt, req_ctxt_id), DispType(dt)
    {
    }
    TickDispType_t dispType() { return DispType; }
};

class TickDispByNum : public TickDisp
{
    std::string TickNum;
public:
    TickDispByNum(const Ticketing::OrigOfRequest &org,
                  const std::string &ctxt,
                  const int req_ctxt_id,
                  const std::string &ticknum)
    :   TickDisp(org, ctxt, req_ctxt_id, TickDispByTickNo),
        TickNum(ticknum)
    {
    }

    const std::string tickNum() const { return TickNum; }
};

class ChngStatData : public edi_common_data
{
    std::list<Ticketing::Ticket> lTick;
    Ticketing::Itin::SharedPtr Itin_;
public:
    ChngStatData(const Ticketing::OrigOfRequest &org,
                 const std::string &ctxt,
                 const int req_ctxt_id,
                 const std::list<Ticketing::Ticket> &lt,
                 const Ticketing::Itin *itin_ = NULL)
    :edi_common_data(org, ctxt, req_ctxt_id), lTick(lt)
    {
        if(itin_){
            Itin_ = Ticketing::Itin::SharedPtr(new Ticketing::Itin(*itin_));
        }
    }
    const std::list<Ticketing::Ticket> & ltick() const { return lTick; }
    bool isGlobItin() const
    {
        return Itin_;
    }
    const Ticketing::Itin & itin() const
    {
        return *Itin_.get();
    }
};

// Запрос на смену статуса
void SendEdiTlgTKCREQ_ChangeStat(ChngStatData &TChange);
// Запрос на Display
void SendEdiTlgTKCREQ_Disp(TickDisp &TDisp);

typedef void (* message_func_t)
        (edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
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

// Обработка EDIFACT
void proc_edifact(const std::string &tlg);

#endif /*_EDI_TLG_H_*/
