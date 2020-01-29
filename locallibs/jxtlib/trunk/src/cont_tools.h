#ifndef __CONT_TOOLS_H__
#define __CONT_TOOLS_H__

#ifdef __cplusplus
#include <string>
#include <serverlib/string_cast.h>

std::string readContext(const char *name);

std::string readContextNVL(const char *name, const char *NVL);

std::string readSysContext(const char *name);

std::string readSysContextNVL(const char *name, const char *NVL);

extern "C" {
#endif /* __cplusplus */

int writeContext(const char *name, const char *value);
int readContextInt(const char *name, int NVL);

int writeContextInt(const char *name, const int value);
int readWContextInt(int handle, const char *name, int NVL);

int readSysContextInt(const char *name, int NVL);

int writeSysContext(const char *name, const char *value);

int writeSysContextInt(const char *name, int i);

int setCurContext(const char *term, int handle);

void readcont(const char *name, char *buf, int bufsize);

#ifdef __cplusplus
}
int writeContext(const char* name, const std::string& val);
int writeContext(const char* name, bool val);
// write anything that can be converted to string
template<typename T>
int writeContext(const char* name, const T& val) {
    return writeContext(name, HelpCpp::string_cast(val));
}
void SaveContextsHook();
#endif /* __cplusplus */

#endif /* __CONT_TOOLS_H__ */
