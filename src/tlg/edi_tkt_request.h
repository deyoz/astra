#ifndef _EDI_TKT_REQUEST_H_
#define _EDI_TKT_REQUEST_H_
#include <string>
#include "astra_ticket.h"
#include "EdifactRequest.h"

class edi_common_data
{
    Ticketing::OrigOfRequest Org;
    std::string ediSessCtxt;
    edifact::KickInfo m_kickInfo;
public:
    edi_common_data(const Ticketing::OrigOfRequest &org,
                    const std::string &ctxt,
                    const edifact::KickInfo &kickInfo)
        :Org(org), ediSessCtxt(ctxt), m_kickInfo(kickInfo) {};
    const Ticketing::OrigOfRequest &org() const { return Org; }
    const std::string & context() const { return ediSessCtxt; }
    const edifact::KickInfo &kickInfo() const { return m_kickInfo; }
    virtual ~edi_common_data(){}
};

#endif /* _EDI_TKT_REQUEST_H_ */
