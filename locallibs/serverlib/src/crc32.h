#ifndef __CRC32_H
#define __CRC32_H

#include <stddef.h>
#include <vector>
#include <string>

unsigned long Crc32(const unsigned char* data, size_t size);

inline unsigned long Crc32(const std::vector<unsigned char>& v)
{
    return Crc32(v.data(), v.size());
}

inline unsigned long Crc32(const std::string& s)
{
    return Crc32((const unsigned char*)s.data(), s.size());
}

#endif /* #ifndef __CRC32_H */
