#pragma once

#include <string>


namespace Ticketing {
namespace RemoteSystemContext{
    class SystemContext;
}//namespace RemoteSystemContext
}//namespace Ticketing


namespace airimp {

class TpbMessage
{
    std::string Msg;
    std::string Header;
    const Ticketing::RemoteSystemContext::SystemContext* SysCont;

public:
    TpbMessage(const std::string& msg,
               const Ticketing::RemoteSystemContext::SystemContext* sysCont);
            
    void send();

    const Ticketing::RemoteSystemContext::SystemContext* sysCont() const;
    std::string fullMsg() const; /* ��������� + ᮮ�饭�� */

protected:
    std::string makeHeader();
};

}//namespace airimp
