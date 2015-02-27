#ifndef _TYPEBHELPMNG_H
#define _TYPEBHELPMNG_H

namespace  TypeBHelpMng {
    void configForPerespros(int tlgs_id, int originator_id);
    bool notify(int typeb_in_id, int typeb_out_id);
    int getOriginatorId(int typeb_in_id);
    void clean_typeb_help();
};

#endif
