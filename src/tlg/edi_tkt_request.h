#ifndef _EDI_TKT_REQUEST_H_
#define _EDI_TKT_REQUEST_H_
#include <string>
#include "astra_ticket.h"

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

#endif /* _EDI_TKT_REQUEST_H_ */
