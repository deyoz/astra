#if HAVE_CONFIG_H
#endif

#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include <string>
#include <serverlib/zlib_employment.h>
#include "zip.h"

#ifndef WITH_ZLIB
#warning "No WITH_ZLIB parameter - zip won't be working!"
#endif /* ! WITH_ZLIB */

using namespace std;

namespace Zip
{

ZipException::ZipException(const std::string &msg) : ZipException{"","",-1,"",msg} {}

ZipException::ZipException(const std::string &nick_, const std::string &fl_, int ln_, const std::string &msg)
    : ZipException{nick_.c_str(),fl_.c_str(),ln_,"",msg} {}

ZipException::ZipException(const char* n, const char* f, int l, const char* fn, const std::string &msg)
    : ServerFramework::Exception(n, f, l, fn, msg)
{
    ProgTrace(TRACE1,"ZipException thrown from %s:%s:%i: '%s'", n, f, l, msg.c_str());
}

//-----------------------------------------------------------------------

std::string decompress_(const std::string &str_in, Zlib::CompressionType ctype)
{
  if(str_in.empty())
    return std::string();

#ifdef WITH_ZLIB
  std::vector<uint8_t> res;
  if(int err = Zlib::decompress(std::vector<uint8_t>(str_in.begin(),str_in.end()), res, ctype))
      throw ZipException(STDLOG, __func__, "Zlib::decompress failed with "+std::to_string(err));

  return std::string(res.begin(),res.end());
#else
  return std::string();
#endif
}

std::string decompress(const std::string &str_in)
{
  return decompress_(str_in,Zlib::Zlib);
}

std::string gunzip(const std::string &str_in)
{
  return decompress_(str_in,Zlib::GZip);
}

std::string compress(const char* str_in, size_t str_size)
{
    if(not str_size)
        return {};
#ifdef WITH_ZLIB
    std::vector<uint8_t> res;
    if(int err = Zlib::compress(std::vector<uint8_t>(str_in, str_in+str_size), res))
        throw ZipException(STDLOG, __func__, "Zlib::compress failed with "+std::to_string(err));
    return std::string(res.begin(),res.end());
#else
    return {};
#endif
}

std::string compress(const std::string &str_in, bool perfom_test)
{
  if(str_in.empty())
    return std::string();

#ifdef WITH_ZLIB
  std::vector<uint8_t> res;
  if(int err = Zlib::compress(std::vector<uint8_t>(str_in.begin(),str_in.end()), res))
      throw ZipException(STDLOG, __func__, "Zlib::compress failed with "+std::to_string(err));

  return std::string(res.begin(),res.end());
#else
  return std::string();
#endif
}

} // namespace Zip
