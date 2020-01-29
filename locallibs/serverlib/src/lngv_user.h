#pragma once

#include <string>
#include <map>

#include "lngv.h"

class UserLanguage
{
public:
    explicit UserLanguage(const std::string& lang);

    static UserLanguage en_US();
    static UserLanguage ru_RU();
    static UserLanguage es_ES();

    typedef std::map<std::string, std::map<UserLanguage, std::string>> Dictionary;
    static std::string translate(const std::string& msg, const UserLanguage& l, const Dictionary& d);

    bool operator<(const UserLanguage& that) const;
    bool operator == (const UserLanguage &that) const;

    friend std::ostream& operator <<(std::ostream & os, const UserLanguage&);
    friend class UserLanguagePackerHelper;
private:
    std::string langCode_;
};

class UserLanguagePackerHelper
{
public:
    std::string pack(const UserLanguage& l);
    UserLanguage unpack(const std::string& s);
    UserLanguage unpack(Language l);
};

std::ostream& operator<<(std::ostream& os, const UserLanguage& l);

