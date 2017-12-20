#include "md5_sum.h"

#include <sstream>
#include <iomanip>

using namespace std;

void TMD5Sum::init()
{
    data_size = 0;
    MD5_Init(&c);
}

int TMD5Sum::update(const void *data, size_t len)
{
    data_size += len;
    return MD5_Update(&c, (unsigned char *)data, len);
}

int TMD5Sum::Final()
{
    return MD5_Final(digest, &c);
}

string TMD5Sum::str()
{
    ostringstream md5sum;
    for(size_t i = 0; i < sizeof(digest) / sizeof(u_char); i++)
        md5sum << hex << setw(2) << setfill('0') << (int)digest[i];
    return md5sum.str();
}

TMD5Sum *TMD5Sum::Instance()
{
    static TMD5Sum *instance_ = 0;
    if(not instance_)
        instance_ = new TMD5Sum();
    return instance_;
}

string md5_sum(const string &val)
{
    TMD5Sum::Instance()->init();
    TMD5Sum::Instance()->update(val.c_str(), val.size());
    TMD5Sum::Instance()->Final();
    return TMD5Sum::Instance()->str();
}

