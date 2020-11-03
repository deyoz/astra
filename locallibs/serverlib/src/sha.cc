#if HAVE_CONFIG_H
#endif

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
#include <openssl/sha.h>

#include <cstdio>
#include "serverlib_sha.h"

std::string sha256(const std::string& pass)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, pass.c_str(), pass.length());
    SHA256_Final(hash, &sha256);
    char outputBuffer[65] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    return std::string(outputBuffer);
}
