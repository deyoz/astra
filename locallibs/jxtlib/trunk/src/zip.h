#ifndef __ZIP_H__
#define __ZIP_H__

#ifdef __cplusplus

#include <string>
#include <serverlib/exception.h>

namespace Zip
{

struct ZipException : public ServerFramework::Exception
{
    ZipException(const std::string &msg);
    ZipException(const std::string &nick_, const std::string &fl_, int ln_, const std::string &msg);
    ZipException(const char* n, const char* f, int l, const char* fn, const std::string &msg);
};

std::string gunzip(const std::string &str_in); // decompresses gzip w/o header
std::string decompress(const std::string &str_in);

std::string compress(const std::string &str_in, bool perfom_test=false /* deprecated */);
std::string compress(const char* str_in, size_t str_size);
} // namespace Zip

#endif /* __cplusplus */

#endif /* __ZIP_H__ */
