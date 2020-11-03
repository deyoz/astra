#ifndef _EHELPSIG_H_
#define _EHELPSIG_H_
#include "posthooks.h"

/*this class is int its own file for xp_tests*/
#include <netinet/in.h>

namespace ServerFramework {  class InternalMsgId;  }

class EdiHelpSignal:public Posthooks::BaseHook
{
    static const size_t max_buf_size = 1000;
    virtual bool less2( const BaseHook *p) const noexcept;
    int msg1[4];
    char sigtext[max_buf_size];
    char ADDR[60];
public:
    virtual void run();
    EdiHelpSignal(const ServerFramework::InternalMsgId& msg,const std::string &adr,const std::string &txt);
    virtual EdiHelpSignal* clone() const;
};
#endif
