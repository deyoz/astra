/* pragma cplusplus */
#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <libxml/encoding.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlerror.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <algorithm>
#include "xml_stuff.h"
#define NICKNAME "MNICK"
#define NICKTRACE MNICK_TRACE
#include "slogger.h"
#include "text_codec.h"
#include "exception.h"
#include "xmllibcpp.h"
#include "xml_tools.h"


using namespace ServerFramework;
using HelpCpp::TextCodec;
using HelpCpp::ConvertException;

// CP866TOUTF8
// Перекодирует буфер из CP866 в UTF-8
//    out: Указатель на буфер в котором будет сохранен результат в UTF-8
//    outlen: Длина out
//    in: Указатель на буфер в кодировке CP866
//    inlen: Длина in
// Результат: Либо количество записаных байт,
//  либо если -1 то недостаточно памяти или если -2 то перекодировка не возможна
// Если возвращаемое значение больше нуля, то значение inlen - количество преобразованных байт,
// значение outlen - количество полученных байт.

template <TextCodec::Charset from, TextCodec::Charset to> static std::string _text_convert(const char* in, const size_t len)
{
    static_assert(from != to, "_text_convert(from == to) unsupported");
    static TextCodec codec(from, to);
    return codec.encode(in, len);
}

static int CP866TOUTF8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    if((out == NULL) || (outlen == NULL) || (inlen == NULL) || (in == NULL))
        return -1;

    try {
        auto&& outStr = _text_convert<TextCodec::CP866, TextCodec::UTF8>(reinterpret_cast<const char*>(in), *inlen);

        if (*outlen < (int)outStr.length() ) {
            return -1;
        }
        memcpy(out, outStr.c_str(), *outlen = outStr.length());
    } catch (const ConvertException&) {
        return -2;
    }
    return *outlen;
}

// UTF8TOCP866
// Перекодирует буфер из UTF-8 в CP866
//    out: Указатель на буфер в котором будет сохранен результат в CP866
//    outlen: Длина out
//    in: Указатель на буфер в кодировке UTF-8
//    inlen: Длина in
// Результат: Либо количество записаных байт,
//  либо если -1 то недостаточно памяти или если -2 то перекодировка не возможна
// Если возвращаемое значение больше нуля, то значение inlen - количество преобразованных байт,
// значение outlen - количество полученных байт.
static int UTF8TOCP866(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    if((out == NULL) || (outlen == NULL) || (inlen == NULL) || (in == NULL))
        return -1;

    try {
        auto&& outStr = _text_convert<TextCodec::UTF8, TextCodec::CP866>(reinterpret_cast<const char*>(in), *inlen);

        if (*outlen < (int)outStr.length() ) {
            return -1;
        }
        memcpy(out, outStr.c_str(), *outlen = outStr.length());
    } catch (const ConvertException&) {
        return -2;
    }
    return *outlen;
}

xmlCharEncodingHandlerPtr getEncodingHandler()
{
  xmlCharEncodingHandlerPtr converter = NULL;
#ifdef USE_ICONV
  converter = xmlFindCharEncodingHandler("CP866");
#else
  converter = xmlFindCharEncodingHandler("X-CP866");
  if(!converter) {
#if LIBXML_VERSION > 20600
    converter = xmlNewCharEncodingHandler("X-CP866", CP866TOUTF8, UTF8TOCP866);
#else
    xmlCharEncodingHandlerPtr handler = (xmlCharEncodingHandlerPtr)
              xmlMalloc(sizeof(xmlCharEncodingHandler));
    handler->name = "X-CP866";
    handler->input = CP866TOUTF8;
    handler->output = UTF8TOCP866;
    xmlRegisterCharEncodingHandler(handler);
    converter = xmlFindCharEncodingHandler("X-CP866");
#endif
  }
#endif
  return converter;
}

//-----------------------------------------------------------------------

std::vector<uint8_t> CP866toUTF8(const uint8_t* buf, const size_t bufsize)
{
    auto&& out = _text_convert<TextCodec::CP866, TextCodec::UTF8>(reinterpret_cast<const char*>(buf), bufsize);
    return std::vector<uint8_t>(out.begin(), out.end());
}

bool CP866toUTF8(const char* buf, const size_t bufsize, std::string& out)
{
    out = _text_convert<TextCodec::CP866, TextCodec::UTF8>(reinterpret_cast<const char*>(buf), bufsize);
    return not out.empty();
}

std::string CP866toUTF8(char c)
{
    std::string s;
    CP866toUTF8(&c, 1, s);
    return s;
}

std::string CP866toUTF8(const std::string& value)
{
    return _text_convert<TextCodec::CP866, TextCodec::UTF8>(value.data(), value.size());
}

//-----------------------------------------------------------------------

std::string UTF8toCP866(const std::string& value)
{
    return _text_convert<TextCodec::UTF8, TextCodec::CP866>(value.data(), value.size());
}

//-----------------------------------------------------------------------

template <TextCodec::Charset from, TextCodec::Charset to> void set_recoded_content(xmlNodePtr node)
{
    if(not node or node->type != XML_TEXT_NODE or not node->content)
        return;
    const size_t z = xmlStrlen(node->content);
    if(std::none_of(node->content, node->content+z, [](xmlChar c){ return c > 127; })) // а вот тут можно люто ускорить, накладывая маску
        return;

    auto text = _text_convert<from,to>(reinterpret_cast<const char*>(node->content), xmlStrlen(node->content));
    if(text.size() > z or text.size() + 8 < z)
    {
        if(node->content != reinterpret_cast<xmlChar*>(&node->properties))
            xmlFree(node->content);
        node->content = reinterpret_cast<xmlChar*>(xmlMalloc(text.size()+1));
    }
    std::copy(text.begin(), text.end(), node->content);
    node->content[ text.size() ] = '\0';
}

/* encode nodelist (only TEXT nodes) from CODEPAGE to utf8 */
template <TextCodec::Charset from, TextCodec::Charset to> void xml_encode_nodelist_recursive_destructive(xmlNodePtr node)
{
    while(node != nullptr)
    {
        set_recoded_content<from,to>(node);

        for(auto prop = node->properties; prop; prop = prop->next)
            set_recoded_content<from,to>(prop->children);

        xml_encode_nodelist_recursive_destructive<from,to>(node->children);
        node = node->next;
    }
}

template <TextCodec::Charset from, TextCodec::Charset to> int xml_recode_nodelist(xmlNodePtr node) try
{
    xml_encode_nodelist_recursive_destructive<from,to>(node);
    return 0;
}
catch(const HelpCpp::ConvertException& )
{
    return -1;
}

int xml_encode_nodelist(xmlNodePtr node)
{
    return xml_recode_nodelist<TextCodec::CP866, TextCodec::UTF8>(node);
}

int xml_decode_nodelist(xmlNodePtr node)
{
    return xml_recode_nodelist<TextCodec::UTF8, TextCodec::CP866>(node);
}

static void xml_err_handler(void *, const char *, ...)
{
  xmlError *err = xmlGetLastError();
  if(err)
  {
#if LIBXML_VERSION > 20700
    if(err->code != XML_TREE_NOT_UTF8)
#endif
    {
        LogWarning("SYSTEM", "XmlError",0) << "errCode: " << err->code << ". errText: " << err->message;
    }
  }
}

void xmlErrorToSirenaLog()
{
    xmlSetGenericErrorFunc(0, xml_err_handler);
}

std::string xml_node_dump(const xmlNodePtr pnode, int format)
{
    std::unique_ptr<xmlBuffer, decltype(xmlBufferFree)*> p(xmlBufferCreate(), xmlBufferFree);
    xmlNodeDump(p.get(), pnode->doc, pnode, 0, format);
    return std::string(reinterpret_cast<const char*>(xmlBufferContent(p.get())), xmlBufferLength(p.get()));
}

#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"

void init_encoding_tests()
{
}

const std::string utf8("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
                  "\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13"
                  "\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D"
                  "\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27"
                  "\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31"
                  "\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B"
                  "\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45"
                  "\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
                  "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59"
                  "\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63"
                  "\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D"
                  "\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77"
                  "\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\xD0\x90"
                  "\xD0\x91\xD0\x92\xD0\x93\xD0\x94\xD0\x95"
                  "\xD0\x96\xD0\x97\xD0\x98\xD0\x99\xD0\x9A"
                  "\xD0\x9B\xD0\x9C\xD0\x9D\xD0\x9E\xD0\x9F"
                  "\xD0\xA0\xD0\xA1\xD0\xA2\xD0\xA3\xD0\xA4"
                  "\xD0\xA5\xD0\xA6\xD0\xA7\xD0\xA8\xD0\xA9"
                  "\xD0\xAA\xD0\xAB\xD0\xAC\xD0\xAD\xD0\xAE"
                  "\xD0\xAF\xD0\xB0\xD0\xB1\xD0\xB2\xD0\xB3"
                  "\xD0\xB4\xD0\xB5\xD0\xB6\xD0\xB7\xD0\xB8"
                  "\xD0\xB9\xD0\xBA\xD0\xBB\xD0\xBC\xD0\xBD"
                  "\xD0\xBE\xD0\xBF\xE2\x96\x91\xE2\x96\x92"
                  "\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2"
                  "\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95"
                  "\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97"
                  "\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2"
                  "\x94\x90\xE2\x94\x94\xE2\x94\xB4\xE2\x94"
                  "\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC"
                  "\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2"
                  "\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95"
                  "\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
                  "\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2"
                  "\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95"
                  "\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98"
                  "\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2"
                  "\x96\x8C\xE2\x96\x90\xE2\x96\x80\xD1\x80"
                  "\xD1\x81\xD1\x82\xD1\x83\xD1\x84\xD1\x85"
                  "\xD1\x86\xD1\x87\xD1\x88\xD1\x89\xD1\x8A"
                  "\xD1\x8B\xD1\x8C\xD1\x8D\xD1\x8E\xD1\x8F"
                  "\xD0\x81\xD1\x91\xD0\x84\xD1\x94\xD0\x87"
                  "\xD1\x97\xD0\x8E\xD1\x9E\xC2\xB0\xE2\x88"
                  "\x99\xC2\xB7\xE2\x88\x9A\xE2\x84\x96\xC2"
                  "\xA4\xE2\x96\xA0\xC2\xA0");

const std::string cp866("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
                   "\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13"
                   "\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D"
                   "\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27"
                   "\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31"
                   "\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B"
                   "\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45"
                   "\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
                   "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59"
                   "\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63"
                   "\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D"
                   "\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77"
                   "\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81"
                   "\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B"
                   "\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95"
                   "\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
                   "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9"
                   "\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3"
                   "\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD"
                   "\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7"
                   "\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1"
                   "\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB"
                   "\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5"
                   "\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
                   "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9"
                   "\xFA\xFB\xFC\xFD\xFE\xFF");

START_TEST(EncodingTest1)
{
  fail_unless(CP866toUTF8(cp866)==utf8, "Не работает перекодировка из CP866 в UTF8");
}
END_TEST

START_TEST(EncodingTest2)
{
  fail_unless(UTF8toCP866(utf8)==cp866, "Не работает перекодировка из UTF8 в CP866");
}
END_TEST

START_TEST(EncodingTest3)
{
 std::string cp866 = "В АВИАБИЛЕТЕ УКАЗАТЬ:НАПРАВЛЕНИЕ №,РЕГИОНАЛЬНОЕ ОТДЕЛЕНИЕ ФОНДА СОЦСТРАХА,ВЫДАВШЕЕ НАПРАВЛЕНИЕ И НОМЕР СНИЛС (УКАЗАННЫЙ В НАПРАВЛЕНИИ).";
 std::string tmp = cp866;
 std::string result = CP866toUTF8(tmp);
 ProgTrace(TRACE1, "Result %s|", result.c_str());
 result = UTF8toCP866( result ) ;
 ProgTrace(TRACE1, "Result %s|", result.c_str());
 fail_unless(result==cp866, "#Fail");  
}
END_TEST

START_TEST(EncodingTest4)
{
    xmlDocPtr xf = xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0"));
    xf->children = xmlNewDocNode(xf, NULL, reinterpret_cast<const xmlChar*>("Form1"), NULL);
    xmlNewTextChild(xf->children, NULL, "Msg1", "абвгдежзи");
    xmlNewTextChild(xf->children, NULL, "Msg2", "Как дела?)");
    xmlSetProp(xf->children, "Prop1", "Value1");
    xmlSetProp(xf->children, "Prop2", "Value2");
    xmlSetProp(xf->children, "Prop3", "Value3");
    std::string str=xml_node_dump(xf->children);
    ProgTrace(TRACE1, "Result\n%s", str.c_str());
    const std::string need_result = 
"<Form1 Prop1=\"Value1\" Prop2=\"Value2\" Prop3=\"Value3\">\n\
  <Msg1>абвгдежзи</Msg1>\n\
  <Msg2>Как дела?)</Msg2>\n\
</Form1>";
    fail_unless(str == need_result, "Fail test4");
    xmlFreeDoc(xf);
}
END_TEST

#define SUITENAME "Encoding"
TCASEREGISTER( 0, 0)
  ADD_TEST(EncodingTest1);
  ADD_TEST(EncodingTest2);
  ADD_TEST(EncodingTest3);
  ADD_TEST(EncodingTest4);
TCASEFINISH
#endif //XP_TESTING
