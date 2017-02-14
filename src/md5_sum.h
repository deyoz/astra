#ifndef _MD5_SUM_H_
#define _MD5_SUM_H_

#include <openssl/md5.h>
#include <string>

struct TMD5Sum {
    MD5_CTX c;
    u_char digest[16];
    double data_size;

    void init();
    int update(const void *data, size_t len);
    int Final();
    std::string str();
    static TMD5Sum *Instance();
};

std::string md5_sum(const std::string &val);

#endif
