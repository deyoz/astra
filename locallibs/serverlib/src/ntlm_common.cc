#include "ntlm_common.h"
#include "base64.h"

#include <openssl/hmac.h>
#include <openssl/md4.h>
#include <openssl/md5.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <sstream>

//-------------------------------------------------------------------

#define SHORTPAIR(x)   ((unsigned char)((x) & 0xff)), ((unsigned char)(((x) >> 8) & 0xff))
#define LONGQUARTET(x) ((unsigned char)((x) & 0xff)), ((unsigned char)(((x) >> 8) & 0xff)), ((unsigned char)(((x) >> 16) & 0xff)), ((unsigned char)(((x) >> 24) & 0xff))

#define NTLMv2_BLOB_LEN (44 - 16 + mdata.tinfo.size() + 4)

//-------------------------------------------------------------------

namespace {

uint32_t read16le(const Byte *buf);
uint32_t read32le(const Byte *buf);
void write32le(const int32_t value, Byte *buf);
void write64le(const int64_t value, Byte *buf);
void asc2utf(ByteVec &dest, const std::string &src, const std::size_t srclen);
void asc2utf(Byte *dest, const std::string &src, const std::size_t srclen);
void ascUpper2utf(ByteVec &dest, const std::string &src, const std::size_t srclen);
void asc2asc(ByteVec &dest, const std::string &str);
char ascToupper(char in);

uint32_t read16le(const Byte *buf)
{
    return ((unsigned int)buf[0]) | ((unsigned int)buf[1] << 8);
}

uint32_t read32le(const Byte *buf)
{
    return ((unsigned int)buf[0]) | ((unsigned int)buf[1] << 8) | ((unsigned int)buf[2] << 16) | ((unsigned int)buf[3] << 24);
}

void write32le(const int32_t value, Byte *buf)
{
    buf[0] = (char)(value & 0x000000FF);
    buf[1] = (char)((value & 0x0000FF00) >> 8);
    buf[2] = (char)((value & 0x00FF0000) >> 16);
    buf[3] = (char)((value & 0xFF000000) >> 24);
}

void write64le(const int64_t value, Byte *buf)
{
    write32le((int)value, buf);
    write32le((int)(value >> 32), buf + 4);
}

void asc2utf(ByteVec &dest, const std::string &str)
{
    for (size_t idx = 0; idx < str.size(); idx++) {
        dest[idx * 2] = (unsigned char)str.data()[idx];
        dest[idx * 2 + 1] = '\0';
    }
}

void asc2utf(ByteVec &dest, const std::string &src, const std::size_t srclen)
{
    for (size_t idx = 0; idx < srclen; idx++) {
        dest[idx * 2] = (unsigned char)src.data()[idx];
        dest[idx * 2 + 1] = '\0';
    }
}

void asc2utf(Byte *dest, const std::string &src, const std::size_t srclen)
{
    for (size_t idx = 0; idx < srclen; idx++) {
        dest[idx * 2] = (unsigned char)src.data()[idx];
        dest[idx * 2 + 1] = '\0';
    }
}

void ascUpper2utf(ByteVec &dest, const std::string &src, const std::size_t srclen)
{
    for (size_t idx = 0; idx < srclen; idx++) {
        dest[idx * 2] = (unsigned char)ascToupper(src.data()[idx]);
        dest[idx * 2 + 1] = '\0';
    }
}

void asc2asc(ByteVec &dest, const std::string &str)
{
    for (size_t idx = 0; idx < str.size(); idx++)
        dest[idx] = (unsigned char)str.data()[idx];
}

char ascToupper(char in)
{
    switch (in) {
    case 'a': return 'A';
    case 'b': return 'B';
    case 'c': return 'C';
    case 'd': return 'D';
    case 'e': return 'E';
    case 'f': return 'F';
    case 'g': return 'G';
    case 'h': return 'H';
    case 'i': return 'I';
    case 'j': return 'J';
    case 'k': return 'K';
    case 'l': return 'L';
    case 'm': return 'M';
    case 'n': return 'N';
    case 'o': return 'O';
    case 'p': return 'P';
    case 'q': return 'Q';
    case 'r': return 'R';
    case 's': return 'S';
    case 't': return 'T';
    case 'u': return 'U';
    case 'v': return 'V';
    case 'w': return 'W';
    case 'x': return 'X';
    case 'y': return 'Y';
    case 'z': return 'Z';
    }

    return in;
}

const ByteVec sign = {0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00};
const ByteVec typ1 = {0x01, 0x00, 0x00, 0x00};
const ByteVec typ2 = {0x02, 0x00, 0x00, 0x00};
const ByteVec typ3 = {0x03, 0x00, 0x00, 0x00};
const ByteVec blob = {0x01, 0x01, 0x00, 0x00};
const ByteVec zero = {0x00, 0x00, 0x00, 0x00};
unsigned MD5LEN = MD5_DIGEST_LENGTH;

/*
void print(const ByteVec &vec, const char *name)
{
    fprintf(stderr, "\n***\n");
    fprintf(stderr, "%s.size=%ld\n", name, vec.size());
    for (size_t idx = 0; idx < vec.size(); idx++)
        fprintf(stderr, "%2.2x ", vec[idx]);
    fprintf(stderr, "\n\n");
}
*/

void write(Byte *pd, uint32_t offset, const ByteVec &vec)
{
    for (size_t idx = 0; idx < vec.size(); idx++)
        pd[offset + idx] = vec[idx];
}

struct SecBuf {
    uint16_t length = 0;
    uint16_t allocated = 0;
    uint32_t offset = 0;

    void init(uint16_t le, uint16_t al, uint32_t of)
    {
        clear();
        length = le;
        allocated = al;
        offset = of;
    }

    void setup(const Byte *pd)
    {
        clear();
        length = read16le(pd);
        allocated = read16le(pd + 2);
        offset = read32le(pd + 4);
    }

    bool exist()
    {
        return length > 0;
    }

    void load(const Byte *pd, ByteVec &vec)
    {
        vec = ByteVec(pd + offset, pd + offset + length);
    }

    void save(Byte *pd, const ByteVec &vec, const uint16_t hoffset)
    {
        ByteVec head;
        head.resize(8, 0);

        head[0] = (int)((length) & 0xff);
        head[1] = (int)((length >> 8) & 0xff);

        head[2] = (int)((allocated) & 0xff);
        head[3] = (int)((allocated >> 8) & 0xff);

        head[4] = (int)((offset) & 0xff);
        head[5] = (int)((offset >> 8) & 0xff);
        head[6] = (int)((offset >> 16) & 0xff);
        head[7] = (int)((offset >> 24) & 0xff);

        write(pd, hoffset, head);
        write(pd, offset, vec);
    }

    void clear()
    {
        length = 0;
        allocated = 0;
        offset = 0;
    }
};

static ByteVec nthashFunc(MsgData &mdata)
{
    std::size_t len = mdata.ascPassword.size();

    ByteVec pvec;
    pvec.resize(len * 2, 0);
    asc2utf(pvec, mdata.ascPassword, len);

    ByteVec ovec;
    ovec.resize(24, 0);
    MD4(pvec.data(), len * 2, ovec.data());

    return ovec;
}

static ByteVec ntlmv2hashFunc(MsgData &mdata, const ByteVec &nthash)
{
    ByteVec uvec;
    uvec.resize(mdata.ascUsername.size() * 2 + mdata.ascDomain.size() * 2);
    ascUpper2utf(uvec, mdata.ascUsername, mdata.ascUsername.size());
    asc2utf(uvec.data() + mdata.ascUsername.size() * 2, mdata.ascDomain, mdata.ascDomain.size());

    ByteVec hvec;
    hvec.resize(24, 0);
    HMAC(EVP_md5(), nthash.data(), 16, uvec.data(), uvec.size(), hvec.data(), &MD5LEN);

    return hvec;
}

static ByteVec lmv2respFunc(const MsgData &mdata, const ByteVec &ntlmv2hash)
{
    ByteVec cvec;
    cvec.reserve(16);
    cvec.insert(cvec.end(), mdata.servChallenge.begin(), mdata.servChallenge.end());
    cvec.insert(cvec.end(), mdata.userChallenge.begin(), mdata.userChallenge.end());
    assert(cvec.size() == 16);

    ByteVec hvec;
    hvec.resize(16, 0);
    HMAC(EVP_md5(), ntlmv2hash.data(), 16, cvec.data(), 16, hvec.data(), &MD5LEN);

    ByteVec ovec;
    ovec.reserve(24);
    ovec.insert(ovec.end(), hvec.begin(), hvec.begin() + 16);
    ovec.insert(ovec.end(), mdata.userChallenge.begin(), mdata.userChallenge.end());
    assert(ovec.size() == 24);

    return ovec;
}

static ByteVec ntv2respFunc(const MsgData &mdata, const ByteVec &ntlmv2hash)
{
#ifdef XP_TESTING
    int64_t tw = 132313231000000000;
#else
    int64_t tw = (time(nullptr) + 11644473600) * 10000000;
#endif

    std::size_t len = MD5_DIGEST_LENGTH + NTLMv2_BLOB_LEN;

    ByteVec ovec;
    ovec.reserve(len);
    ovec.insert(ovec.end(), zero.begin(), zero.end()); // 0
    ovec.insert(ovec.end(), zero.begin(), zero.end());
    ovec.insert(ovec.end(), zero.begin(), zero.end()); // 8
    ovec.insert(ovec.end(), zero.begin(), zero.end());

    //
    ovec.insert(ovec.end(), blob.begin(), blob.end()); // 16
    ovec.insert(ovec.end(), zero.begin(), zero.end());

    //
    ovec.insert(ovec.end(), zero.begin(), zero.end()); // 24
    ovec.insert(ovec.end(), zero.begin(), zero.end());

    //
    ovec.insert(ovec.end(), mdata.userChallenge.begin(), mdata.userChallenge.end()); // 32
    ovec.insert(ovec.end(), zero.begin(), zero.end());
    ovec.insert(ovec.end(), mdata.tinfo.begin(), mdata.tinfo.end());
    ovec.insert(ovec.end(), zero.begin(), zero.end());

    assert(ovec.size() == len);
    assert(mdata.servChallenge.size() == 8);
    assert(mdata.userChallenge.size() == 8);

    std::memcpy(ovec.data() + 8, mdata.servChallenge.data(), mdata.servChallenge.size());
    write64le(tw, ovec.data() + 24);

    ByteVec hvec;
    hvec.resize(16, 0);
    HMAC(EVP_md5(), ntlmv2hash.data(), 16, ovec.data() + 8, NTLMv2_BLOB_LEN + 8, hvec.data(), &MD5LEN);

    std::memcpy(ovec.data(), hvec.data(), 16);

    return ovec;
}

} // namespace

//-------------------------------------------------------------------

std::string type1msgFunc(const MsgData &mdata)
{
    ByteVec full;
    full.resize(16);

    write(full.data(), 0, sign);
    write(full.data(), 8, typ1);
    write(full.data(), 12, ByteVec({LONGQUARTET(mdata.userFlags)}));

    std::string encoded;
    base64_encode(full.data(), full.size(), encoded);

    return encoded;
}

void type2msgFunc(const std::string &encoded, MsgData &mdata)
{
    ByteVec data;
    base64_decode(encoded.data(), encoded.size(), data);
    if (data.size() < 48)
        throw std::runtime_error("type2msg: size=" + std::to_string(data.size()));

    //
    const Byte *const pd = data.data();

    uint32_t type = read32le(pd + 8);
    if (type != 2)
        throw std::runtime_error("type2msg: type=" + std::to_string(type));

    //
    SecBuf secbuf;

    secbuf.setup(pd + 12);
    if (secbuf.exist())
        secbuf.load(pd, mdata.tname);

    secbuf.setup(pd + 40);
    if (secbuf.exist())
        secbuf.load(pd, mdata.tinfo);

    //
    mdata.servFlags = read32le(pd + 20);
    mdata.servChallenge = ByteVec(pd + 24, pd + 32);
    mdata.context = ByteVec(pd + 32, pd + 40);
}

std::string type3msgFunc(MsgData &mdata)
{
    ByteVec nthash = nthashFunc(mdata);
    ByteVec ntlmv2hash = ntlmv2hashFunc(mdata, nthash);
    ByteVec lmv2resp = lmv2respFunc(mdata, ntlmv2hash);
    ByteVec ntv2resp = ntv2respFunc(mdata, ntlmv2hash);

    //
    ByteVec full;
    full.resize(64 + lmv2resp.size() + ntv2resp.size() + mdata.ascDomain.size() + mdata.ascUsername.size() + mdata.ascHostname.size(), 0);

    write(full.data(), 0, sign);
    write(full.data(), 8, typ3);
    write(full.data(), 60, ByteVec({LONGQUARTET(mdata.servFlags)}));

    //
    SecBuf secbuf;
    uint16_t length = 0;
    uint32_t offset = 64;

    // LM
    length = (uint16_t)lmv2resp.size();
    secbuf.init(length, length, offset);
    secbuf.save(full.data(), lmv2resp, 12);
    offset += length;

    // NT
    length = (uint16_t)ntv2resp.size();
    secbuf.init(length, length, offset);
    secbuf.save(full.data(), ntv2resp, 20);
    offset += length;

    // TARGET
    ByteVec dvec;
    dvec.resize(mdata.ascDomain.size(), 0);
    asc2asc(dvec, mdata.ascDomain);
    length = (uint16_t)dvec.size();
    secbuf.init(length, length, offset);
    secbuf.save(full.data(), dvec, 28);
    offset += length;

    // USERNAME
    ByteVec uvec;
    uvec.resize(mdata.ascUsername.size(), 0);
    asc2asc(uvec, mdata.ascUsername);
    length = (uint16_t)uvec.size();
    secbuf.init(length, length, offset);
    secbuf.save(full.data(), uvec, 36);
    offset += length;

    // HOSTNAME
    ByteVec hvec;
    hvec.resize(mdata.ascHostname.size(), 0);
    asc2asc(hvec, mdata.ascHostname);
    length = (uint16_t)hvec.size();
    secbuf.init(length, length, offset);
    secbuf.save(full.data(), hvec, 44);

    //
    std::string encoded;
    base64_encode(full.data(), full.size(), encoded);

    return encoded;
}
