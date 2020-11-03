#ifndef LIBTLG_TLGNUM_H
#define LIBTLG_TLGNUM_H

#include <serverlib/int_parameters.h>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>

/**
 * тип номера телеграмы
 * */
struct tlgnum_t
{
    static const unsigned TLG_NUM_LENGTH = 10;

    DECL_RIP_LENGTH(num_t, std::string, 1, TLG_NUM_LENGTH);

    static boost::optional<tlgnum_t> create(const std::string& v, bool express = false);

    explicit tlgnum_t(const std::string& v, bool express = false);

    inline bool operator<(const tlgnum_t& other) const {
        return num < other.num;
    }
    inline bool operator==(const tlgnum_t& other) const {
        return num == other.num;
    }
    inline bool operator!=(const tlgnum_t& other) const {
        return num != other.num;
    }

    num_t num;
    bool express;

    friend std::ostream& operator<< (std::ostream& os, const tlgnum_t& tlgNum);
};

#endif /* LIBTLG_TLGNUM_H */

