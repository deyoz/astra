#ifndef __GETTEXT_H__
#define __GETTEXT_H__

#include "lngv.h"

#ifdef __cplusplus
extern "C"
{
#endif
int getCurrLang();
#ifdef __cplusplus
}
#include <map>
#include <string>

const char *getLocalizedText(const char *key_str, ...);
const char *getLocalizedText(unsigned err_code, ...);
const char *getLocalizedText(int lang, const char *key_str, ...);
const char *getLocalizedTextL(unsigned err_code, int lang, ...);
typedef std::map<std::string, std::string> LocalizationMap;
LocalizationMap &getLocMap(const std::string &lang_code);

inline Language currLang()
{
    return Language(getCurrLang());
}

// Локализирует текст из псевдо языка (наприм. из "EtsErr::PROG_ERR")
std::string getLocalText(const std::string &key, int lang=getCurrLang());

#endif /* __cplusplus */
#endif /* __GETTEXT_H__ */
