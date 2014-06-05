#ifndef _EMD_DISP_REQUEST_H_
#define _EMD_DISP_REQUEST_H_

#include "edi_tlg.h"
#include "edi_tkt_request.h"

enum EmdDispType_e{
    emdDispByNum,
};

class EmdDispParams : public edi_common_data
{
    int reqCtxtId;
    EmdDispType_e DispType;
public:
    EmdDispParams(const Ticketing::OrigOfRequest &org,
             const std::string &ctxt,
             const int req_ctxt_id,
             EmdDispType_e dt)
    :edi_common_data(org, ctxt, req_ctxt_id), DispType(dt)
    {
    }
    EmdDispType_e dispType() { return DispType; }
};

class EmdDispByNum : public EmdDispParams
{
    std::string TickNum;
public:
    EmdDispByNum(const Ticketing::OrigOfRequest &org,
                  const std::string &ctxt,
                  const int req_ctxt_id,
                  const std::string &ticknum)
    :   EmdDispParams(org, ctxt, req_ctxt_id, emdDispByNum),
        TickNum(ticknum)
    {
    }

    const std::string tickNum() const { return TickNum; }
};

void CreateTKCREQEmdDisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void ProcTKCRESemdDisplay(edi_mes_head *pHead, edi_udata &udata,
                             edi_common_data *data);
void ParseTKCRESemdDisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);

void SendEdiTlgTKCREQ_Disp(TickDisp &TDisp);
#endif /* _EMD_DISP_REQUEST_H_ */
