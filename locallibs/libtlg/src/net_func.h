#ifndef NET_FUNC_H
#define NET_FUNC_H

#include <sys/types.h>

namespace telegrams
{

unsigned get_ip(const char *IPstr);
void setSocketOptions(int sock);
int initSenderLocalSocket(int portnum);

}  // namespace telegrams


#endif /* NET_FUNC_H */

