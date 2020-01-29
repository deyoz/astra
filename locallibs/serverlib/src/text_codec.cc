#if HAVE_CONFIG_H
#endif


#include <cerrno>
#include <memory.h>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <iconv.h>

#include "text_codec.h"
#include "str_utils.h"
#include "helpcppheavy.h"
#include "helpcpp.h"

#define NICKNAME "DAG"
#include "slogger.h"

using std::string;

namespace
{
enum err_src { OPEN, ICONV };
string errno2str(err_src src)
{
    switch (src) {
    case OPEN:
        switch (errno) {
        case EMFILE :
            return "The process already has OPEN_MAX file descriptors open";
        case ENFILE :
            return "The system limit of open file is reached";
        case ENOMEM :
            return "Not enough memory to carry out the operation";
        case EINVAL :
            return "The conversion is not supported";
        default:
            ProgTrace(TRACE0, "unknown errno: %d", errno);
        };
        break;

    case ICONV:
        switch (errno) {
        case EILSEQ :
            return "Invalid byte sequence in the input";
            /* E2BIG not throwed */
        case EINVAL :
            return "Incomplete byte sequence at the end of the input buffer";
        case EBADF  :
            return "The convert descriptor argument is invalid";
        default:
            ProgTrace(TRACE0, "unknown errno: %d", errno);
        }
        break;
    }
    return "Unknown";
}

} // unnamed
//#############################################################################
namespace HelpCpp {

class TextCodec::TextCodecImpl
{
private:
    typedef HelpCpp::TextCodec TextCodec;
    typedef TextCodec::Charset Charset;

    iconv_t icvDesr_;
    static constexpr size_t convSize_ = 256;
    static constexpr unsigned convErrCtxShift = 32;

public:
    TextCodecImpl(Charset from, Charset to) : TextCodecImpl( charsetName(from), charsetName(to) ) {}

    TextCodecImpl(const string& from, const string& to)
    {
        icvDesr_ = iconv_open( to.c_str(), from.c_str() );
        if (icvDesr_ == (iconv_t)-1) {
            string msg("iconv_open(");
            msg += "\"" + from + "\"->";
            msg += "\"" + to + "\") error: " + errno2str(OPEN);
            throw ConvertException(msg);
        }
    }

    ~TextCodecImpl()
    {
        iconv_close(icvDesr_);
    }

    std::string convert(const char* _inBuf, size_t avail) const
    {
        if(not avail)
            return std::string();

        const size_t outSize = convSize_;
        const char* const _inBufStart = _inBuf;
        char outBuf[convSize_];

        string convStr;
        //sets intitial state conversion
        iconv( icvDesr_, 0, 0, 0, 0 );
        while (avail != 0) {
            char* _outBuf  = outBuf;
            size_t outLeft = outSize;

            // convert piece of src string
            size_t nconv = iconv( icvDesr_, const_cast<char**>(&_inBuf), &avail, &_outBuf, &outLeft );
            if( nconv == (size_t)-1 && errno != E2BIG ) {
                // get context around bad byte
                const auto contextBufStart = std::max(_inBuf - convErrCtxShift, _inBufStart);
                const auto contextBufSize = std::min<size_t>(convErrCtxShift * 2, _inBufStart + avail - contextBufStart);
                std::string contextStr(contextBufStart, contextBufSize);

                // replace all \n with "\\n"
                size_t pos = 0;
                const std::string rep = "\\n";
                while ((pos = contextStr.find('\n', pos)) != std::string::npos) {
                     contextStr.replace(pos, 1, rep);
                     pos += rep.size(); // size of /\n string
                }

                // calculate wrong byte pos in characters
                throw ConvertException( "iconv error: " + errno2str(ICONV) + "\n"
                                        "           context str: " + contextStr + "\n"
                                        "           convert failed at: " + std::to_string(_inBuf - _inBufStart + 1) + " byte\n"
                                        "           context mem:\n" + memdump(contextBufStart, contextBufSize, _inBuf - contextBufStart),
                                        _inBuf - _inBufStart );
            }
            convStr.append(outBuf, outSize - outLeft);
        }
        if(errno == E2BIG){
          errno = 0;
        }
        return convStr;
    }

    static Charset charsetId(const string& cp_name)
    {
        static struct {
            const char* name;
            Charset cp;
        } CpTable[] = {
            { "KOI8R",        TextCodec::KOI8R },
            { "KOI8-R",       TextCodec::KOI8R },
            { "KOI8",         TextCodec::KOI8R },
            { "KOI",          TextCodec::KOI8R },

            { "WIN1251",      TextCodec::CP1251 },
            { "WIN-1251",     TextCodec::CP1251 },
            { "CP1251",       TextCodec::CP1251 },
            { "CP-1251",      TextCodec::CP1251 },
            { "WIN",          TextCodec::CP1251 },
            { "WINDOWS",      TextCodec::CP1251 },
            { "WINDOWS1251",  TextCodec::CP1251 },
            { "WINDOWS-1251", TextCodec::CP1251 },

            { "CP866",        TextCodec::CP866 },
            { "CP-866",       TextCodec::CP866 },
            { "IBM-866",      TextCodec::CP866 },
            { "IBM866",       TextCodec::CP866 },
            { "DOS-866",      TextCodec::CP866 },
            { "DOS866",       TextCodec::CP866 },
            { "ALT",          TextCodec::CP866 },

            { "UTF8",         TextCodec::UTF8 },
            { "UTF-8",        TextCodec::UTF8 },
            { "UTF",          TextCodec::UTF8 },
        };

        string name = cp_name;
        StrUtils::ToUpper(name);

        for (size_t i = 0; i < sizeof(CpTable) / sizeof(CpTable[0]); ++i) {
            if ( name == CpTable[i].name ) {
                return CpTable[i].cp;
            }
        }
        return TextCodec::Unknown;
    }

private:

    static string charsetName(Charset chset)
    {
        //fake define for script that check available encodings
        #define CSNAME(x) x

        switch (chset) {
        case TextCodec::CP866:  return CSNAME("CP866");
        case TextCodec::CP1251: return CSNAME("CP1251");
        case TextCodec::KOI8R:  return CSNAME("KOI8-R");
        case TextCodec::UTF8:   return CSNAME("UTF-8");
        default:;
        }
        return string();

        #undef CSDEF
    }
};
//#############################################################################
TextCodec::TextCodec(Charset from, Charset to)
{
    core_ = new TextCodecImpl(from, to);
}

TextCodec::TextCodec(const string& from, const string& to)
{
    core_ = new TextCodecImpl(from, to);
}

TextCodec::~TextCodec()
{
    delete core_;
}

std::string TextCodec::encode(const char* s, const size_t len) const
{
    return core_->convert(s, len);
}

std::string TextCodec::encode(const std::string& str) const
{
    return core_->convert(str.data(), str.size());
}

TextCodec::Charset TextCodec::charsetId(const std::string& name)
{
    return TextCodecImpl::charsetId(name);
}

std::string::size_type utf8NextCharPosition(std::string const & str, std::string::size_type begin)
{
    for (;begin<str.size();++begin){
       if(utf8IsFirstByte(str[begin])){
            return begin;
       } 
    }
    return std::string::npos;
}
std::string::size_type utf8SkipNChars(std::string::size_type nchars, std::string const & str, std::string::size_type begin )
{
    if(begin<str.size()){
        assert(utf8IsFirstByte(str[begin]));
        if(nchars==0){
            return begin;
        }
        ++nchars;
        for (; begin<str.size();++begin){
            if(utf8IsFirstByte(str[begin])){
                --nchars;
                if(nchars==0){
                    return begin;
                }
            } 
        }
    }
    return std::string::npos;
}

std::string utf8Substr2(std::string const & str, std::string::size_type begin, std::string::size_type limit=std::string::npos)
{
    std::vector<std::string::size_type> arrOfPos=utf8CharBeginPositions(str,begin,limit);
    if(arrOfPos.empty()){
        return std::string();
    }
    std::string::size_type last=arrOfPos[arrOfPos.size()-1];
    return str.substr(arrOfPos[0],last==std::string::npos ? last : last-arrOfPos[0]);
} 
std::vector<std::string::size_type> utf8CharBeginPositions(std::string const &str,std::string::size_type begin, std::string::size_type limit)
{
    std::vector<std::string::size_type> res;
    assert(utf8IsFirstByte(str[begin]));
    if(limit!=std::string::npos) { 
        ++limit;
    }
    for(;begin<str.size();){
        res.push_back(begin);
        if(res.size()==limit){
            break;
        }
        begin=utf8NextCharPosition(str,begin+1);
    }

    if(res.size()<limit){
        res.push_back(std::string::npos);
    }
    return res;
}

bool isUTF8Encoded(const std::string& text)
{
    if (StrUtils::isASCII(text)) {
        return true;
    }

    // this is a no-op if the text is utf-8,
    // but will fail if it isn't
    static TextCodec codec(TextCodec::Charset::UTF8,
                           TextCodec::Charset::UTF8);

    try {
        codec.encode(text);
    } catch (const ConvertException&) {
        // text contains non-utf8 encoded characters
        return false;
    }

    return true;
}
}

//#############################################################################
#ifdef XP_TESTING
#include <stdlib.h>
#include "checkunit.h"
#include "tcl_utils.h"
#include "memmap.h"

namespace {

using namespace HelpCpp;

static void convertString(const std::string& in)
{
    TextCodec::Charset sets[] = {
        TextCodec::UTF8,
        TextCodec::CP866,
        TextCodec::UTF8,
        TextCodec::CP1251,
        TextCodec::CP866,
        TextCodec::KOI8R,
        TextCodec::CP1251,
        TextCodec::UTF8,
        TextCodec::KOI8R,
        TextCodec::UTF8
    };

    const size_t count = sizeof(sets) / sizeof(sets[0]);

    std::string cStr = in;

    for (size_t i = 1; i < count; ++i ) {
        TextCodec codec(sets[i-1], sets[i]);
        std::string curStr = codec.encode(cStr);

        fail_unless(cStr != curStr, "must be difference");
        cStr.swap(curStr);
    }
    fail_unless( in == TextCodec(sets[count-1], sets[0]).encode(cStr), "invalid conversion");
}

START_TEST(conv)
{
    std::string data;
    readFile(readStringFromTcl("XP_TESTING_FILES_SERVERLIB","")+"/testConv.txt",data);
    fail_unless(data.size()==228);
    convertString(data);
}
END_TEST

static int checkMemory(size_t count)
{
    std::string data;
    readFile(readStringFromTcl("XP_TESTING_FILES_SERVERLIB","")+"/checkMemory.txt",data);
    fail_unless(data.size()==1444);

    std::string testStr;
    for (int mult = 10; mult > 0; --mult) {
        testStr += data;
    }

    int mem = static_cast<int>(size_of_allocated_mem());

    for (size_t i = 0; i < count; ++i) {
        convertString(testStr);
    }

    mem = static_cast<int>(size_of_allocated_mem()) - mem;
    ProgTrace(TRACE5, "%s [%zd cycles]; lost: %d KB", __FUNCTION__, count, mem);

    return mem;
}

START_TEST(memory_leak)
{
    struct {
        long diff;
        size_t count;
    } mems[] = {
        { 0, 1 }, { 0, 10 }, { 0, 100 }, {0, 500}, { 0, 1000 }
    };

    const size_t cnt = sizeof(mems) / sizeof(mems[0]);

    long averDiff = 0;
    for (size_t i = 0; i < cnt; ++i) {
        mems[i].diff = checkMemory(mems[i].count);
        if (i == 0) {
            continue;
        }
        if (i == 1) {
            averDiff = labs(mems[i].diff - mems[i-1].diff);
            continue;
        }

        if ( averDiff + 10 < labs(mems[i].diff - mems[i-1].diff) ) {
            fail_unless(0,"Memory leak [cycles = %zd]: %zd KB", mems[i].count, mems[i].diff);
        }
    }
}
END_TEST

START_TEST(testSubstr)
{

    std::ifstream fl((readStringFromTcl("XP_TESTING_FILES_SERVERLIB","")+"/testSubstr.txt").c_str());
    fail_unless(fl.good());
    std::string s;
    std::vector<string> arr1,arr2,arr3,arr4,*parr=&arr1;
    while(getline(fl,s)){
        if(boost::trim_copy(s).empty()){
            parr=(parr==&arr1) ?  &arr2 : (parr==&arr2 ? &arr3: &arr4);
            continue;
        }
        parr->push_back(s);
    }
    fail_unless(arr1.size()==arr2.size());
    fail_unless(arr1.size()==arr3.size());
    fail_unless(arr1.size()==arr4.size());
    for( size_t i=0 ;i<arr1.size();++i){
        std::stringstream is(arr3.at(i));
        int start,  len;
        is >>start>>len;
        fail_unless(utf8Substr(arr1.at(i),start,len==-1 ? std::string::npos : len) == arr2.at(i));
        fail_unless(utf8Substr2(arr1.at(i),start,len==-1 ? std::string::npos : len) == arr2.at(i));
    }
    for( size_t i=0 ;i<arr1.size();++i){
        std::stringstream is(arr4.at(i));
        int n, start,  len;
        is >>start>>n>>len;
        std::string::size_type res=utf8SkipNChars(n==-1 ? std::string::npos : n, arr1.at(i),start);

        fail_unless(res==(len==-1 ? std::string::npos : len));
    }
}
END_TEST

START_TEST(testConvertWrongUTF)
{
    std::string data;
    readFile(readStringFromTcl("XP_TESTING_FILES_SERVERLIB", "") + "/testConvertWrongUTF.txt", data);
    try {
        convertString(data);
        fail_if(1, "ConvertException expected");
    } catch (const ConvertException &e) {
        const size_t wrongBytePos = data.find("\xd0\xfe");
        fail_unless(e.wrongBytePos == wrongBytePos, e.what());
    }
}
END_TEST

START_TEST(testIsUtf8)
{
    struct {
        std::string text;
        bool isUtf8;
    } testcases[] = {
        {"", true},
        {"ascii text only", true},
        {"\xD0\x9D\xD0\xB5 \xD1\x82\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE", true},
        {"c\xD0\x9C""e\xD0\xA8""a\xD0\x9D""n\xD0\x9E""e", true},
        {"\x8D\xA5 \xE2\xAE\xAB\xEC\xAA\xAE", false}, // test case 3 in 866
        {"\xD0\x9D\xB5", false}, // invalid byte sequence
        {"\xFE\xFF", false}, // invalid byte sequence
        {"\xD0", false} // incomplete byte sequence
    };

    const size_t cnt = sizeof(testcases) / sizeof(testcases[0]);

    for (auto i = 0; i != cnt; ++i) {
        fail_unless(HelpCpp::isUTF8Encoded(testcases[i].text) == testcases[i].isUtf8,
                    "testcase %d failed", i);
        LogTrace(TRACE5) << HelpCpp::isUTF8Encoded(testcases[i].text);
    }
}
END_TEST

} //anonymous namespace

#define SUITENAME "textcodec"
TCASEREGISTER(0,0)
    ADD_TEST(conv);
    ADD_TEST(testSubstr);
    ADD_TEST(testConvertWrongUTF);
    ADD_TEST(testIsUtf8);
TCASEFINISH

TCASEREGISTER(0,0)
    SET_TIMEOUT( 300 );
    ADD_TEST(memory_leak);
TCASEFINISH

#endif //XP_TESTING
//#############################################################################
