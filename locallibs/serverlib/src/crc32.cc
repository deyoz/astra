#include "crc32.h"
#include <zlib.h>

#if ZLIB_VERNUM < 0x1270
ZEXTERN uLong ZEXPORT crc32 OF((uLong crc, const Bytef *buf, uInt len));
#endif

unsigned long Crc32(const unsigned char* data, size_t size)
{
    const uLong seed = crc32(0, NULL, 0);
    return crc32(seed, (const Bytef*)data, (uInt)size);
}

