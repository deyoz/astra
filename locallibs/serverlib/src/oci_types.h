#pragma once

#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

namespace OciCpp
{
template <unsigned L> struct OciVcs
{
    unsigned short len;
    char arr[L];
    OciVcs() : len(0) { memset(arr, 0, L); }
    OciVcs(OciVcs const &v) {
        memcpy(arr, v.arr, L);
        len = v.len;
    }
    OciVcs(const char *s, unsigned l = L) {
        strncpy(arr, s, l);
        if (l > 0 and arr[l-1] == 0) {
            len = strlen(arr);
        } else {
            len = l;
        }
    }
    unsigned char *uarr() {
        return reinterpret_cast<unsigned char*>(arr);
    }
    const unsigned char *uarr() const {
        return reinterpret_cast<const unsigned char*>(arr);
    }
    bool operator==(const OciVcs& v) const {
        return len == v.len and memcmp(arr, v.arr, len) == 0;
    }
    int operator < (const OciVcs& v) const {
        return memcmp(arr, v.arr, len) < 0;
    }
    bool operator!=(const OciVcs& v) const {
        return !operator==(v);
    }
    OciVcs& operator=(const OciVcs& v) {
        if (this != &v) {
            len = v.len;
            memcpy(arr, v.arr, L);
        }
        return *this;
    }
    std::string toHumanReadableStr() const
    {
        if (len == 0)
            return "NIL";

        std::ostringstream out;
        out << "[";
        for (unsigned i = 0; i < len; ++i) {
            const int byte = static_cast<unsigned char>(arr[i]);
            out << " " << std::hex << std::setfill('0') << std::setw(2) << byte;
        }
        out << " ]";
        return out.str();
    }
};

template <unsigned L>
inline std::ostream & operator << (std::ostream& os, const OciVcs<L> &vcs)
{
    return os << std::string(vcs.arr, vcs.len);
}

} // namespace OciCpp
