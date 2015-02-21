#ifndef _TYPEBHELPMNG_H
#define _TYPEBHELPMNG_H

#include <string>
#include "astra_consts.h"

namespace  TypeBHelpMng {

    struct TypeBHelp {
        std::string addr, intmsgid, text;
        int tlgs_id;
        int timeout; // seconds
        TypeBHelp(
                const std::string &aaddr,
                const std::string &aintmsgid,
                const std::string &atext,
                int atlgs_id,
                int atimeout
                ):
            addr(aaddr),
            intmsgid(aintmsgid),
            text(atext),
            tlgs_id(atlgs_id),
            timeout(atimeout)
        {};
        TypeBHelp(int typeb_in_id):
            tlgs_id(ASTRA::NoExists)
        {
            fromDB(typeb_in_id);
        }
        void toDB();
        void fromDB(int typeb_in_id);
    };

    void configForPerespros(int tlgs_id);
    bool notify(int typeb_in_id, int typeb_out_id);
};

#endif
