#ifndef _EDI_TLG_H_
#define _EDI_TLG_H_
#include "edilib/edi_session.h"
#include "edilib/edi_session_cb.h"
#include "astra_ticket.h"
#include "serverlib/monitor_ctl.h"
#include "libtlg/hth.h"
#include "tlg/edi_tkt_request.h"

namespace Ticketing {
namespace RemoteSystemContext {
    class SystemContext;
}//RemoteSystemContext
}//namespace Ticketing

void set_edi_addrs( const std::pair<std::string,std::string> &addrs );
std::string get_edi_addr();
std::string get_edi_own_addr();

std::string get_last_session_ref();

struct EdiMess
{
    static const std::string Display;
    static const std::string ChangeStat;
    static const std::string EmdDisplay;
};

class AstraEdiSessWR : public edilib::EdiSessWrData
{
    edilib::EdiSession EdiSess;
    std::string Pult;
    edi_mes_head *EdiHead;
public:
    AstraEdiSessWR(const std::string &pult)
    : Pult(pult)
    {
        static const edi_mes_head zero_head = {};
        EdiHead = new edi_mes_head(zero_head);
    }

    AstraEdiSessWR(const std::string &pult, edi_mes_head *mhead)
        : Pult(pult), EdiHead(mhead)
    {
    }

    virtual edilib::EdiSession *ediSession() { return &EdiSess; }

    virtual hth::HthInfo *hth() { return 0; };
    virtual std::string sndrHthAddr() const { return ""; };
    virtual std::string rcvrHthAddr() const { return ""; };
    virtual std::string hthTpr() const { return ""; };

    // В СИРЕНЕ это recloc/ или our_name из sirena.cfg
    // Идентификатор сессии
    virtual std::string baseOurrefName() const { return "ASTRA"; };
    virtual edi_mes_head *edih() { return EdiHead; };
    virtual const edi_mes_head *edih() const { return EdiHead; };
    virtual std::string pult() const { return Pult; };
    // Аттрибуты сообщения
    virtual std::string ourUnbAddr() const { return get_edi_own_addr(); }
    virtual std::string unbAddr() const { return get_edi_addr(); }
};

// new edifact
class NewAstraEdiSessWR: public AstraEdiSessWR
{
    const Ticketing::RemoteSystemContext::SystemContext* SysCtxt;
public:
    NewAstraEdiSessWR(const std::string &pult, edi_mes_head *mhead,
                      const Ticketing::RemoteSystemContext::SystemContext* sysctxt)
        : AstraEdiSessWR(pult, mhead), SysCtxt(sysctxt)
    {}

    const Ticketing::RemoteSystemContext::SystemContext *sysCont() const { return SysCtxt; }

    // Аттрибуты сообщения
    virtual std::string ourUnbAddr() const;
    virtual std::string unbAddr() const;
};

class AstraEdiSessRD : public edilib::EdiSessRdData
{
    edi_mes_head Head;
    hth::HthInfo* H2H;
    bool isH2H;
    std::string rcvr;
    std::string sndr;
    public:
        AstraEdiSessRD(const hth::HthInfo * H2H_, const edi_mes_head &Head_)
            : Head(Head_),
              H2H( H2H_?(new hth::HthInfo(*H2H_)):0),
              isH2H(H2H?true:false)
        {
        }

        AstraEdiSessRD()
            : H2H(0),
              isH2H(false)
        {
            memset(&Head, 0, sizeof(Head));
        }

        virtual hth::HthInfo *hth() { return H2H; };
        virtual std::string sndrHthAddr() const { return ""; };
        virtual std::string rcvrHthAddr() const { return ""; };
        virtual std::string hthTpr() const { return ""; };

        virtual std::string baseOurrefName() const
        {
            return "ASTRA";
        }

        void setMesHead(const edi_mes_head &head) { Head = head; }
        virtual const edi_mes_head *edih() const { return &Head; };
        virtual edi_mes_head *edih() { return &Head; };
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


enum TickDispType_t {
    TickDispByTickNo=0,
};

class TickDisp : public edi_common_data
{
    TickDispType_t DispType;
public:
    TickDisp(const Ticketing::OrigOfRequest &org,
             const std::string &ctxt,
             const edifact::KickInfo &kickInfo,
             TickDispType_t dt)
    :edi_common_data(org, ctxt, kickInfo), DispType(dt)
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
                  const edifact::KickInfo &kickInfo,
                  const std::string &ticknum)
    :   TickDisp(org, ctxt, kickInfo, TickDispByTickNo),
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
                 const edifact::KickInfo &kickInfo,
                 const std::list<Ticketing::Ticket> &lt,
                 const Ticketing::Itin *itin_ = NULL)
    :edi_common_data(org, ctxt, kickInfo), lTick(lt)
    {
        if(itin_){
            Itin_ = Ticketing::Itin::SharedPtr(new Ticketing::Itin(*itin_));
        }
    }
    const std::list<Ticketing::Ticket> & ltick() const { return lTick; }
    bool isGlobItin() const
    {
        return Itin_.get();
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
void proc_new_edifact(const std::string &tlg);

Ticketing::Pnr readPnr(const std::string &tlg_text);
void SearchEMDsByTickNo(const std::set<Ticketing::TicketNum_t> &emds,
                        const edifact::KickInfo& kickInfo,
                        const Ticketing::OrigOfRequest &org,
                        const Ticketing::FlightNum_t &flNum);

#endif /*_EDI_TLG_H_*/
