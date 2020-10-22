#include "utf2cp866.h"
#include <iconv.h>
#include <errno.h>
#include <vector>

struct IconvHolder {
    iconv_t cd;
    explicit IconvHolder(iconv_t i) : cd(i) {}
    ~IconvHolder() {  iconv_close(cd);  }
};

std::string UTF8toCP866_no_throw(const std::string& s)
{
    iconv_t r = iconv_open("CP866","UTF-8");
    if(r == (iconv_t) -1)
    {
        switch(errno)
        {
            case EINVAL:  // debugmsgg to place here
                break;
            default:   // unexpected errno value
                break;
        }
        return {};
    }
    IconvHolder i(r);
    //int one = 1;
    //if(-1 == iconvctl(i.cd, ICONV_SET_DISCARD_ILSEQ, &one))
    //    ;
    char* inbuf = const_cast<char*>(s.data());
    size_t inbytesleft = s.size();
    std::vector<char> o(s.size());
    char* outbuf = o.data();
    size_t outbytesleft = o.size();

    while(inbytesleft)
    {
        size_t r = iconv(i.cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        if(r != (size_t) -1)
            continue;
        switch(errno)
        {
            case E2BIG:
            case EINVAL:
                inbytesleft = 0;
                break;
            case EILSEQ:
                inbuf++;
                if(inbytesleft > 0)
                {
                    inbytesleft--;
                }
        }
    }
    return std::string(o.data(), o.size()-outbytesleft);
}

#ifdef XP_TESTING
#include <serverlib/checkunit.h>

START_TEST(UTF8toCP866_no_throw)
{
    std::string s = UTF8toCP866_no_throw("Maria Prokopβ€™evna");
    fail_unless(s=="Maria Prokopevna", "Wrong UTF convert");

    std::string utf_txt = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                          "abcdefghijklmnopqrstuvwxyz\n"
                          "ΠΠ‘Π’Π“Π”Π•ΠΠ–Π—ΠΠ™ΠΠ›ΠΠΠΠΠ Π΅ΠΆΠ£Π¤Π¥Π¦Π§Π¨Π©ΠªΠ«Π¬Π­Π®Π―\n"
                          "Π°Π±Π²Π³Π΄ΠµΡ‘Π¶Π·ΠΈΠΉΠΊΠ»ΠΌΠ½ΠΎΠΏΡ€ΡΡ‚ΡƒΡ„Ρ…Ρ†Ρ‡ΡΡ‰ΡΡ‹ΡΡΡΡ\n"
                          "123456789,.:;'\"\\|/<>()!@#$%^&*-=_+";
    std::string cp_txt = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                         "abcdefghijklmnopqrstuvwxyz\n"
                         "€‚ƒ„…π†‡‰‹‘’“”•–—™›\n"
                         " ΅Ά£¤¥ρ¦§¨©ª«¬­®―ΰαβγδεζηθικλμνξο\n"
                         "123456789,.:;'\"\\|/<>()!@#$%^&*-=_+";
    s = UTF8toCP866(utf_txt);
    fail_unless(s==cp_txt, "Incorrect UTF->CP");

    s = UTF8toCP866_no_throw(utf_txt);
    fail_unless(s==cp_txt, "Incorrect UTF->CP no_throw");

    s = CP866toUTF8(cp_txt);
    fail_unless(s==utf_txt, "Incorrect CP->UTF");
}
END_TEST

#define SUITENAME "UTF8toCP866"
TCASEREGISTER(nullptr, nullptr)
{
    ADD_TEST(UTF8toCP866_no_throw)
}
TCASEFINISH
#endif // XP_TESTING
