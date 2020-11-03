#ifndef __VA_HOLDER_H_
#define __VA_HOLDER_H_

#include <cstdarg>

struct VaHolder
{
    va_list* ap;
    VaHolder(va_list& a) : ap(&a) {}
    ~VaHolder() {  va_end(*ap);  }
};

#define VA_HOLDER(ap_name, ap_param) \
  va_list ap_name; \
  va_start(ap_name, ap_param); \
  VaHolder va_holder_ ## _FILE_ ## _LINE_ (ap_name);

#endif // __VA_HOLDER_H_
