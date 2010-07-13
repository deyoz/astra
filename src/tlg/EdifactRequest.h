//
// C++ Interface: EdifactRequest
//
// Description: Makes an EDIFACT request structure
//
//
// Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
//
//
#ifndef _EDIFACTREQUEST_H_
#define _EDIFACTREQUEST_H_
#include <string>
#include "edilib/edi_types.h"
#include "edilib/edi_request.h"

struct _edi_mes_head_;
struct _EDI_REAL_MES_STRUCT_;

namespace RemoteSystemContext{
    class SystemContext;
}

namespace TlgHandling{
    class TlgSourceEdifact{};
}

namespace edifact
{
    class AstraEdiSessWR;

/**
 * @class EdifactRequest
 * @brief Makes an EDIFACT request structure
 */
class EdifactRequest : public edilib::EdifactRequest
{
   TlgHandling::TlgSourceEdifact *TlgOut;
public:
    /**
     * @brief EdifactRequest
     * @param msg_type ⨯ ᮮ�饭��
     */
    EdifactRequest(const std::string &pult,
                   const RemoteSystemContext::SystemContext *SCont,
                   edi_msg_types_t msg_type);

    /**
     * @brief remote system context
     * @return
     */
    const RemoteSystemContext::SystemContext *sysCont();

    /**
     * @brief ��᫠�� ⥫��ࠬ��, �ᯮ���� ����� �� pMes()
     */
    virtual void sendTlg();

    /**
     * ��᫥���� ��᫠���� ⥫��ࠬ��
     * @return
     */
    const TlgHandling::TlgSourceEdifact *tlgOut() const;

    virtual ~EdifactRequest();
};
} // namespace edifact
#endif /*_EDIFACTREQUEST_H_*/
