#ifndef _TYPEBHELPMNG_H
#define _TYPEBHELPMNG_H

#include <string>

namespace  TypeBHelpMng {
    void configForPerespros(int tlgs_id);

    bool notify(int typeb_in_id, int typeb_out_id); // deprecated! used in typeb_handler.cpp only
    // пока Антон не вернется из отпуска

    bool notify_ok(int typeb_in_id, int typeb_out_id);
    bool notify_msg(int typeb_in_id, const std::string &msg);
    void clean_typeb_help();
};

#endif
