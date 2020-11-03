#ifndef SERVERLIG_USTRING_H
#define SERVERLIG_USTRING_H

#include <string>
#include <memory>

class Codec;
namespace HelpCpp { class TextCodec; }

class EncString
{
public:
    typedef std::string base_type;
    enum ENCODING {UTF8 = 0, CP866};

public:
    static void setBaseEncoding(const ENCODING e);

    EncString(); // empty string

    static EncString fromUtf(const std::string& str);
    static EncString from866(const std::string& str);
    std::string toUtf() const;
    std::string to866() const;

    bool empty() const;
    EncString trim() const;
    static EncString fromDb(const std::string& str);
    std::string toDb() const;

    bool operator == (const EncString& other) const;
    bool operator != (const EncString& other) const;
    bool operator < (const EncString& other) const;
    bool operator > (const EncString& other) const;
    friend std::ostream& operator<<(std::ostream& o, const EncString& u);

private:
    explicit EncString(const std::string& str);

private:
    static std::shared_ptr<Codec> codec_;
    base_type val_; // always in DB charset
};

#endif /* SERVERLIG_USTRING_H */
