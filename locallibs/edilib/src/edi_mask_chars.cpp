#include <stdlib.h>
#include <string.h>

#include <string>
#include <sstream>
#include <iostream>

#define NICKNAME "ROMAN"
#include "edi_test.h"
#include "edi_user_func.h"

//!!!!! It must be compiled without edisession support !!!!!!!!
// No oracle, no boost and others..
namespace edilib
{
std::string maskSpecialChars(const Edi_CharSet &Chars, const std::string &instr)
{
    std::string outstr;
    size_t instr_len = instr.size();
    for(size_t i = 0; i < instr_len; i++)
    {
        int str_flag = 0;
        if(instr[i] == Chars.EndData ||
           instr[i] == Chars.EndComp ||
           instr[i] == Chars.Release ||
           (instr_len - i >= (size_t)Chars.EndSegLen &&
           !memcmp(&instr[i], Chars.EndSegStr, Chars.EndSegLen) &&
           (str_flag = Chars.EndSegLen)))
        {
            outstr.push_back(Chars.Release);
        }

        if(str_flag)
        {
            outstr += std::string(&instr[i], str_flag);
            i += str_flag - 1;
        }
        else
        {
            outstr.push_back(instr[i]);
        }
    }

    return outstr;
}

std::string maskSpecialChars(const std::string &instr)
{
    Edi_CharSet chset;
    memset(&chset, 0, sizeof(chset));
    chset.EndSegStr[0] = '\"';
    chset.EndSegStr[1] = '\n';
    chset.EndSegLen = 2;
    chset.EndComp = '+';
    chset.EndData = ':';
    chset.Release = '?';
    return maskSpecialChars(chset, instr);
}

} // namespace edilib

int maskSpecialChars_capp(const Edi_CharSet *Chars, const char *instr, char **out)
{
    try
    {
        std::string out_ = edilib::maskSpecialChars(*Chars, instr);
        *out = (char *)malloc(out_.size() + 1);
        if(!*out)
        {
            EdiError(EDILOG, "failed to allocate %zdb", out_.size() + 1);
            return -1;
        }
        memcpy(*out, out_.c_str(), out_.size() + 1);
        return out_.size();
    }
    catch(std::exception &e)
    {
        EdiError(EDILOG, "%s", e.what());
        return -1;
    }
    catch(...)
    {
        EdiError(EDILOG, "... catched");
        return -1;
    }
}
