#if HAVE_CONFIG_H
#endif
#ifdef WITH_ZLIB

#include <zlib.h>
#include "zlib_employment.h"

#include <stdexcept>
#include <algorithm>

#define NICKNAME "MIKHAIL"
#include "test.h"

namespace Zlib {

int decompress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, CompressionType ctype)
{
    return decompress(in.data(), in.size(), out, ctype);
}

int decompress(const void* in_ptr, size_t in_size, std::vector<uint8_t>& out, CompressionType ctype)
{
    z_stream d_stream = {};

    d_stream.next_in  = static_cast<Bytef*>(const_cast<void*>(in_ptr));
    d_stream.avail_in = in_size;

    out.clear();

    int err = ( ctype==GZip ? inflateInit2(&d_stream,16+MAX_WBITS) : inflateInit(&d_stream) );
    if(err != Z_OK)
        return err;

    uint8_t buf[10000];

    do
    {
        d_stream.next_out = buf;
        d_stream.avail_out = sizeof(buf);
        err = inflate(&d_stream, Z_NO_FLUSH);
        if(err == Z_STREAM_END || err == Z_OK)
            out.insert(out.end(), buf, buf+sizeof(buf)-d_stream.avail_out);
        if(d_stream.total_in == in_size)
            err = Z_STREAM_END;
    } while(err == Z_OK);

    int ierr = inflateEnd(&d_stream);
    if(err != Z_STREAM_END or ierr != Z_OK)
    {
        out.clear();
        return err;
    }

    return Z_OK; // == 0
}

int compress(void const* in_ptr, size_t in_size, std::vector<uint8_t>& out)
{
    out.resize(in_size + 100);
    uLongf out_len = out.size();

    if(int err = ::compress(out.data(), &out_len, static_cast<const Bytef*>(in_ptr), in_size))
    {
        out.clear();
        return err;
    }

    out.resize(out_len);
    out.shrink_to_fit();
    return Z_OK;
}

int compress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    return compress(in.data(), in.size(), out);
}

std::vector<uint8_t> compressStr(const std::string & in)
{
    static_assert(sizeof(char) == sizeof(uint8_t), "sizeof(char) != sizeof(uint8_t)");

    std::vector<uint8_t> out;

    if(compress(reinterpret_cast<const uint8_t *>(in.data()), in.length(), out) != Z_OK)
        throw std::runtime_error("ZLib::compress error");

    return out;
}

std::string decompressStr(const std::vector<uint8_t> & in)
{
    static_assert(sizeof(char) == sizeof(uint8_t), "sizeof(char) != sizeof(uint8_t)");

    std::vector<uint8_t> unzipped;
    if(Zlib::decompress(in, unzipped) != Z_OK)
        throw std::runtime_error("ZLib::decompress error");

    return !unzipped.empty() ? std::string(reinterpret_cast<const char *>(unzipped.data()), unzipped.size()) : std::string();
}

} // namespace Zlib

#endif // WITH_ZLIB
