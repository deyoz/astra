#ifndef __ZLIB_EMPLOYMENT_H_
#define __ZLIB_EMPLOYMENT_H_

#ifdef WITH_ZLIB

#include <vector>
#include <string>
#include <stdint.h>
#include <stddef.h>

namespace Zlib {

enum CompressionType
{
  Zlib,
  GZip,
};

int decompress(const void* in_ptr, size_t in_size, std::vector<uint8_t>& out, CompressionType ctype=Zlib);
int decompress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, CompressionType ctype=Zlib);
int compress(void const* in_ptr, size_t in_size, std::vector<uint8_t>& out);
int compress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

std::vector<uint8_t> compressStr(const std::string & in);
std::string decompressStr(const std::vector<uint8_t> & in);
}

#endif // WITH_ZLIB

#endif // __ZLIB_EMPLOYMENT_H_
