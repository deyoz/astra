#ifndef SERVERLIB_OCI_SEQ_H
#define SERVERLIB_OCI_SEQ_H

#include "cursctl.h"

namespace OciCpp
{
class Sequence
{
public:
    Sequence(OciSession& sess, const std::string& name)
        : name_(name), sess_(sess)
    {}
    Sequence(const std::string& name)
        : name_(name), sess_(mainSession())
    {}
    template<typename T>
    T nextval(const char* nick, const char* file, int line) {
        T t;
        sess_.createCursor(nick, file, line, "SELECT " + name_ + ".nextval FROM DUAL")
            .def(t)
            .EXfet();
        return t;
    }
private:
    std::string name_;
    OciSession& sess_;
};
} // OciCpp


#endif /* SERVERLIB_OCI_SEQ_H */

