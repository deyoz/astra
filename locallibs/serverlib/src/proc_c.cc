#include <vector>
#include <utility>
#include "proc_c.h"
#include "ssl_employment.h"
#include "zlib_employment.h"
#include "mespro_crypt.h"
#include "levc_callbacks.h"
#include "sirena_queue.h"
#include "exception.h"
#include "tclmon.h"

#define NICKNAME "MIKHAIL"
#include "test.h"

namespace ServerFramework {

namespace {

namespace utl {

struct SomethingFailed
{
    std::vector<uint8_t> head;
    std::string body;
    SomethingFailed(const std::vector<uint8_t>& h, const std::string& b) : head(h), body(b)
    {
        make_failed_head(head,body);
    }
};

typedef int (*FilterCore)(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

template<class BodyIn, class BodyOut> void filter(const void* head, size_t hlen, BodyIn& bin, BodyOut& bout, FilterCore cc)
{
    ProgTrace(TRACE7,"%s:: bin.size=%zu",__FUNCTION__,bin.size());

    if(int err = cc(head, hlen, bin, bout))
    {
        char _res[2000];
        size_t _res_len = levC_form_crypt_error(_res, sizeof(_res), static_cast<const char*>(head), hlen, err);
        throw SomethingFailed(std::vector<uint8_t>(_res,_res+hlen),std::string(_res+hlen,_res_len-hlen));
    }
    ProgTrace(TRACE7,"  => bout.size=%zu",bout.size());
}

//-----------------------------------------------------------------------

void crypt_filter(const void* head, size_t hlen, std::vector<uint8_t>& a_head, const std::vector<uint8_t>& ub, std::vector<uint8_t>& bu, int type)
{
    int sym_key_id = 0;

    int err = -1;
    if(false)
        {}
#ifdef HAVE_SSL
    else if(type == SYMMETRIC_CRYPTING)
        err = sym_encrypt(head, hlen, ub, bu, &sym_key_id);
    else if(type == RSA_CRYPTING)
        err = pub_encrypt(head, hlen, ub, bu);
#endif
#ifdef USE_MESPRO
    else if(type == MESPRO_CRYPTING)
        err = mespro_encrypt(head, hlen, ub, bu);
#endif
    else
        throw ServerFramework::Exception("invalid crypting type");

    if(err)
    {
        ProgTrace(TRACE1, "%s encrypt failed with %i", type == SYMMETRIC_CRYPTING ? "SYMMETRIC_CRYPTING"
                                                           : type == RSA_CRYPTING ? "RSA_CRYPTING"
                                                        : type == MESPRO_CRYPTING ? "MESPRO_CRYPTING" : "~???~", err);
        throw SomethingFailed(a_head, "");
    }
    else
    {
        ProgTrace(TRACE7,"%s(type=%i) : %zu bytes => %zu bytes",__FUNCTION__,type,ub.size(),bu.size());
        levC_adjust_crypt_header(reinterpret_cast<char*>(a_head.data()), bu.size(), type, sym_key_id);
    }
}

//-----------------------------------------------------------------------

#ifdef USE_MESPRO
bool is_mespro_crypted(const std::vector<uint8_t>& head)
{
    return levC_is_mespro_crypted(reinterpret_cast<const char*>(head.data()));
}
#endif

bool should_mespro_crypt(const std::vector<uint8_t>& head)
{
#ifdef USE_MESPRO
    return levC_should_mespro_crypt(reinterpret_cast<const char*>(head.data()), head.size());
#else
    return false;
#endif
}

//-----------------------------------------------------------------------

bool is_sym_crypted(const std::vector<uint8_t>& head)
{
#ifdef HAVE_SSL
    return levC_is_sym_crypted(reinterpret_cast<const char*>(head.data()));
#else
    return false;
#endif
}

bool is_pub_crypted(const std::vector<uint8_t>& head)
{
#ifdef HAVE_SSL
    return levC_is_pub_crypted(reinterpret_cast<const char*>(head.data()));
#else
    return false;
#endif
}

bool should_sym_crypt(const std::vector<uint8_t>& head)
{
#ifdef HAVE_SSL
    return levC_should_sym_crypt(reinterpret_cast<const char*>(head.data()), head.size());
#else
    return false;
#endif
}

bool should_pub_crypt(const std::vector<uint8_t>& head)
{
#ifdef HAVE_SSL
    return levC_should_pub_crypt(reinterpret_cast<const char*>(head.data()), head.size());
#else
    return false;
#endif
}

//-----------------------------------------------------------------------

#ifdef WITH_ZLIB
bool is_compressed(const std::vector<uint8_t>& head)
{
    return levC_is_compressed(reinterpret_cast<const char*>(head.data()));
}
bool should_compress(const std::vector<uint8_t>& head)
{
    return levC_should_compress(head.data(), head.size());
}

int decompress(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    if(static_cast<const uint8_t*>(head)[0] == 2)
        ProgTrace(TRACE7,"%s :: from client %u",__FUNCTION__,static_cast<const uint8_t*>(head)[45]*256+static_cast<const uint8_t*>(head)[46]);
    else
        ProgTrace(TRACE7,"%s",__FUNCTION__);

    if(int err = Zlib::decompress(in,out))
    {
        ProgTrace(TRACE1, "decompress failed with %i", err);
        return err; //6;//UNKNOWN_ERR;
    }
    return 0;
}

int compress(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    ProgTrace(TRACE7,"%s",__FUNCTION__);

    if(int err = Zlib::compress(in,out))
    {
        ProgTrace(TRACE1, "compress failed with %i", err);
        return err; //6;//UNKNOWN_ERR;
    }
    return 0;
}
#endif // WITH_ZLIB

} // namespace utl

//-----------------------------------------------------------------------

void demake_body(const std::vector<uint8_t>& reqHead, std::vector<uint8_t>& reqData, std::vector<uint8_t>& body)
{
    ProgTrace(TRACE7,"%s",__FUNCTION__);

    setPerespros(levC_is_perespros(reqHead));

    std::vector<uint8_t> bu;
    if(false)
        {}
#ifdef HAVE_SSL
    else if(utl::is_sym_crypted(reqHead))
        utl::filter(reqHead.data(), reqHead.size(), reqData, bu, sym_decrypt);
    else if(utl::is_pub_crypted(reqHead))
        utl::filter(reqHead.data(), reqHead.size(), reqData, bu, pub_decrypt);
#endif
#ifdef USE_MESPRO
    else if(utl::is_mespro_crypted(reqHead))
        utl::filter(reqHead.data(), reqHead.size(), reqData, bu, mespro_decrypt);
#endif
    if(bu.empty())
        bu.swap(reqData);
#ifdef WITH_ZLIB
    if(utl::is_compressed(reqHead))
        utl::filter(reqHead.data(), reqHead.size(), bu, body, utl::decompress);
    else
#endif
        body.swap(bu);
}

//-----------------------------------------------------------------------

void enmake_response(std::vector<uint8_t>& reqHead, std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData)
{
    ProgTrace(TRACE7,"%s",__FUNCTION__);
#ifdef WITH_ZLIB
    if(utl::should_compress(reqHead)) {
        ProgTrace(TRACE7,"%s :: should compress outgoung message",__FUNCTION__);
        std::vector<uint8_t> bu;
        utl::filter(reqHead.data(), reqHead.size(), ansData, bu, utl::compress);
        if(bu.size() < ansData.size())
        {
            ansData.swap(bu);
            levC_adjust_header(ansHead.data(), ansData.size());
        }
    }
#endif

    std::vector<uint8_t> bu;
    if(utl::should_sym_crypt(reqHead)) {
        ProgTrace(TRACE7,"%s :: should sym crypt outgoung message",__FUNCTION__);
        utl::crypt_filter(reqHead.data(), reqHead.size(), ansHead, ansData, bu, SYMMETRIC_CRYPTING);
    }
    else if(utl::should_pub_crypt(reqHead)) {
        ProgTrace(TRACE7,"%s :: should pub crypt outgoung message",__FUNCTION__);
        utl::crypt_filter(reqHead.data(), reqHead.size(), ansHead, ansData, bu, RSA_CRYPTING);
    }
    else if(utl::should_mespro_crypt(reqHead)) {
        ProgTrace(TRACE7,"%s :: should mespro crypt outgoung message",__FUNCTION__);
        utl::crypt_filter(reqHead.data(), reqHead.size(), ansHead, ansData, bu, MESPRO_CRYPTING);
    }
    if(not bu.empty())
        ansData.swap(bu);
}

//-----------------------------------------------------------------------

void levC_compose_wrapper(
        const std::vector<uint8_t>& reqHead,
        const std::vector<uint8_t>& body,
        std::vector<uint8_t>& ansHead,
        std::vector<uint8_t>& ansData)
{
    ProgTrace(TRACE7,"%s",__FUNCTION__);

    levC_compose(reinterpret_cast<const char*>(reqHead.data()), reqHead.size(), body, ansHead, ansData);

    ProgTrace(TRACE7,"%s:: levC_compose brought %zu byte data",__FUNCTION__,ansData.size());
}

} // anonymous namespace

//-----------------------------------------------------------------------
void process(std::vector<uint8_t>& reqHead, std::vector<uint8_t>& reqData,
             std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData) try
{
    ProgTrace(TRACE7,"%s",__FUNCTION__);

    std::vector<uint8_t> body;
    demake_body(reqHead, reqData, body);

    levC_compose_wrapper(reqHead, body, ansHead, ansData);

    enmake_response(reqHead, ansHead, ansData);

    ProgTrace(TRACE7,"%s -> successfull (=> %zu body)",__FUNCTION__, ansData.size());
}
catch(const utl::SomethingFailed& e)
{
    ProgTrace(TRACE0,"%s -> SomethingFailed",__FUNCTION__);
    ansHead.assign(e.head.begin(), e.head.end());
    ansData.assign(e.body.begin(), e.body.end());
}

//-----------------------------------------------------------------------

} // namespace ServerFramework

