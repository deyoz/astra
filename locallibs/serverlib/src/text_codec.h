#pragma once

#include <string>
#include "exception.h"

namespace HelpCpp {

inline bool utf8IsFirstByte( unsigned char c)
{
    return ((c&0x80) == 0) ||((c&0x40)!=0);
}
bool isUTF8Encoded(const std::string& text);
std::string::size_type utf8NextCharPosition(std::string const & str, std::string::size_type begin);
std::vector<std::string::size_type> utf8CharBeginPositions(std::string const &str,std::string::size_type begin=0, std::string::size_type limit=std::string::npos);
std::string::size_type utf8SkipNChars(std::string::size_type nchars, std::string const & str, std::string::size_type begin );

inline std::string utf8Substr(std::string const & str, std::string::size_type begin, std::string::size_type limit=std::string::npos)
{
    std::string::size_type last=utf8SkipNChars(limit,str,begin);
    return str.substr(begin,last==std::string::npos ? last : last-begin);
} 
class ConvertException : public comtech::Exception
{
public:
    ConvertException(const std::string& msg, size_t wrongBytePos_ = std::string::npos)
        :comtech::Exception(msg), wrongBytePos(wrongBytePos_)
    {   }

    size_t wrongBytePos;
};

class TextCodec
{
public:
    enum Charset {
        Unknown,
        KOI8R,
        CP1251,
        CP866,
        UTF8
    };

    TextCodec(Charset from, Charset to);
    TextCodec(const std::string& from, const std::string& to);
    ~TextCodec();

    std::string encode(const std::string& str) const;
    std::string encode(const char* s, const size_t len) const;
    static Charset charsetId(const std::string& name);
private:
    TextCodec(const TextCodec&);
    TextCodec& operator=(const TextCodec&);

    class TextCodecImpl;
    TextCodecImpl* core_;
};

} // namespace HelpCpp
