#include <ostream>

#include "text_codec.h"
#include "str_utils.h"

#include "encstring.h"

#define NICKNAME "MIXA"
#include "slogger.h"

class Codec
{
public:
    virtual ~Codec();
    virtual std::string fromUtf(const std::string& str) const = 0;
    virtual std::string from866(const std::string& str) const = 0;
    virtual std::string toUtf(const std::string& str) const = 0;
    virtual std::string to866(const std::string& str) const = 0;
};

class Base866
    : public Codec
{
public:
    Base866();
    virtual ~Base866();
    virtual std::string fromUtf(const std::string& str) const;
    virtual std::string from866(const std::string& str) const;
    virtual std::string toUtf(const std::string& str) const;
    virtual std::string to866(const std::string& str) const;

private:
    const std::shared_ptr<HelpCpp::TextCodec> utfTo866_;
    const std::shared_ptr<HelpCpp::TextCodec> cp866ToUtf_;
};

class BaseUtf
    : public Codec
{
public:
    BaseUtf();
    virtual ~BaseUtf();
    virtual std::string fromUtf(const std::string& str) const;
    virtual std::string from866(const std::string& str) const;
    virtual std::string toUtf(const std::string& str) const;
    virtual std::string to866(const std::string& str) const;

private:
    const std::shared_ptr<HelpCpp::TextCodec> utfTo866_;
    const std::shared_ptr<HelpCpp::TextCodec> cp866ToUtf_;
};

Codec::~Codec()
{ }

Base866::Base866()
    : utfTo866_(new HelpCpp::TextCodec(HelpCpp::TextCodec::UTF8, HelpCpp::TextCodec::CP866)),
      cp866ToUtf_(new HelpCpp::TextCodec(HelpCpp::TextCodec::CP866, HelpCpp::TextCodec::UTF8))
{ }

Base866::~Base866()
{ }

std::string Base866::fromUtf(const std::string& str) const
{
    return utfTo866_->encode(str);
}

std::string Base866::from866(const std::string& str) const
{
    return str;
}

std::string Base866::toUtf(const std::string& str) const
{
    return cp866ToUtf_->encode(str);
}

std::string Base866::to866(const std::string& str) const
{
    return str;
}


BaseUtf::BaseUtf()
    : utfTo866_(new HelpCpp::TextCodec(HelpCpp::TextCodec::UTF8, HelpCpp::TextCodec::CP866)),
      cp866ToUtf_(new HelpCpp::TextCodec(HelpCpp::TextCodec::CP866, HelpCpp::TextCodec::UTF8))
{ }

BaseUtf::~BaseUtf()
{ }

std::string BaseUtf::fromUtf(const std::string& str) const
{
    return str;
}

std::string BaseUtf::from866(const std::string& str) const
{
    return cp866ToUtf_->encode(str);
}

std::string BaseUtf::toUtf(const std::string& str) const
{
    return str;
}

std::string BaseUtf::to866(const std::string& str) const
{
    return utfTo866_->encode(str);
}

std::shared_ptr<Codec> EncString::codec_(new Base866());

void EncString::setBaseEncoding(const ENCODING e)
{
    switch (e) {
    case UTF8:
        codec_.reset(new BaseUtf());
        break;

    case CP866:
        codec_.reset(new Base866());
        break;

    default:
        LogError(STDLOG) << "Unknown encoding: " << e;
        codec_.reset();
    }
}

EncString EncString::fromDb(const std::string& str)
{
    return EncString(str);
}

EncString EncString::fromUtf(const std::string& str)
{
    return EncString(codec_->fromUtf(str));
}

EncString EncString::from866(const std::string& str)
{
    return EncString(codec_->from866(str));
}

EncString::EncString()
    :val_()
{ }

EncString::EncString(const std::string& str)
    : val_(str)
{ }

std::string EncString::toDb() const
{
    return val_;
}

std::string EncString::toUtf() const
{
    return codec_->toUtf(val_);
}

std::string EncString::to866() const
{
    return codec_->to866(val_);
}

bool EncString::empty() const
{
    return val_.empty();
}

EncString EncString::trim() const
{
    // All bytes in multy-byte UTF8 characters always > 127.
    // http://en.wikipedia.org/wiki/UTF-8
    return EncString(StrUtils::trim(val_));
}

bool EncString::operator == (const EncString& other) const
{
    return other.val_ == this->val_;
}

bool EncString::operator != (const EncString& other) const
{
    return !(other.val_ == this->val_);
}

bool EncString::operator < (const EncString& other) const
{
    return this->val_ < other.val_;
}

bool EncString::operator > (const EncString& other) const
{
    return this->val_ > other.val_;
}

std::ostream& operator<<(std::ostream& os, const EncString& u)
{
    return os << u.val_;
}
