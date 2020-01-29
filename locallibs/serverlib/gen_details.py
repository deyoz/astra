#!/usr/bin/python

import sys

print """#include "makename_defs.h"
#include "json_packer.h"
#include "json_spirit.h"

namespace json_spirit {
namespace details {
template<typename T>
struct DescHelper
{
    boost::function<mValue (const T&)> packInt;
    boost::function< boost::optional<T> (const mValue&) > unpackInt;
    boost::function<mValue (const T&)> packExt;
    boost::function< boost::optional<T> (const mValue&) > unpackExt;

    template<typename T2> DescHelper(const T2& desc)
    {
        packInt = boost::bind(&T2::packInt, desc, _1);
        unpackInt = boost::bind(&T2::unpackInt, desc, _1);
        packExt = boost::bind(&T2::packExt, desc, _1);
        unpackExt = boost::bind(&T2::unpackExt, desc, _1);
    }
};

template<int Int>
struct DefaultValue
{
    template<typename T> static boost::optional<T> value(const boost::optional<T>& def) { return def; }
};

template<>
struct DefaultValue<1>
{
    template<typename T> static boost::optional<T> value(const boost::optional<T>& def) {
        return def ? def : T();
    }
};

template<typename Type, typename T>
struct FieldDesc
{
    FieldDesc(const std::string& n,
            const boost::function<T (const Type&)>& getter_,
            const boost::optional<T>& def = boost::optional<T>()
            )
        : name(n),
        defaultVal(DefaultValue< Traits<T>::allowEmptyConstructor >::value(def)),
        get(getter_)
    {
    }
    std::string name;
    boost::optional<T> defaultVal;
    boost::function<T (const Type&)> get;
};

template<typename Type, typename T>
static boost::optional<T> unpackIntHelper(const mObject& o, const FieldDesc<Type, T>& fd)
{
    const mObject::const_iterator i(o.find(fd.name));
    if (i == o.end()) {
        return fd.defaultVal;
    }
    return Traits<T>::unpackInt(i->second);
}

template<typename Type, typename T>
static boost::optional<T> unpackExtHelper(const mObject& o, const FieldDesc<Type, T>& fd)
{
    const mObject::const_iterator i(o.find(fd.name));
    if (i == o.end()) {
        return fd.defaultVal;
    }
    return Traits<T>::unpackExt(i->second);
}

"""

def templateStr(n):
    s = "template<typename Type"
    for i in range(0, n):
        s += ", typename T" + str(i)
    s += ">"
    return s

def typesStr(n):
    s = "Type"
    for i in range(0, n):
        s += ", T" + str(i)
    return s

def varsStr(n):
    s = ""
    for i in range(0, n):
        s += ", *t" + str(i)
    if len(s) == 0:
        return s
    return s[2:]

def fieldsStr(n):
    s = ""
    for i in range(0, n):
        s += ", f" + str(i)
    if len(s) == 0:
        return s
    return s[2:]

def structName(n):
    return "Constructor" + str(n)

class Constructor(object):
    def __init__(self, n):
        self.n = n
        self.name = structName(n)
    def forwardDecl(self):
        return templateStr(self.n) + " struct " + self.name + ";"
    def packStr(self, mode):
        s = "\n  mValue pack" + mode + "(const Type& v) const {"
        s += "\n    mObject o;"
        for i in range(0, self.n):
            s += "\n    if (!f{0}.defaultVal || !(*f{0}.defaultVal == f{0}.get(v)))".format(str(i))
            s += " { "
            s += "o[f{0}.name] = Traits< T{0} >::pack{1}(f{0}.get(v));".format(str(i), mode)
            s += " }"
        s += "\n    return o;\n  }"
        return s
    def unpackStr(self, mode):
        s = "\n  boost::optional<Type> unpack" + mode + "(const mValue& v) const {"
        s += "\n    JSON_ASSERT_TYPE(Type, v, json_spirit::obj_type);"
        s += "\n    const mObject& o(v.get_obj());"
        for i in range(0, self.n):
            s += "\n    const boost::optional< T{0} > t{0}(json_spirit::details::unpack{1}Helper< Type, T{0} >(o, f{0}));".format(str(i), mode)
            s += "\n    if (!t{0}) { writeJsonLog(f{0}.name); return boost::optional<Type>(); }".replace("{0}", str(i))
        s += "\n    return Type(" + varsStr(self.n) + ");  \n  }"
        return s
    def structDecl(self):
        s = templateStr(self.n) + " struct " + self.name + "\n{"
        if self.n > 0:
            s += "\n "
        for i in range(0, self.n):
            s += " FieldDesc<Type, T{0}> f{0};".format(i)
        s += "\n  " + self.name + "("
        for i in range(0, self.n):
            s += "const FieldDesc<Type, T{0}>& f{0}_, ".format(i)
        if (self.n > 0):
            s = s[:-2]
        s += ") "
        if self.n > 0:
            s += ': '
        for i in range(0, self.n):
            s += "f{0}(f{0}_), ".format(i)
        if self.n > 0:
            s = s[:-2]
        s += " {}"
        s += "\n    template<typename {0}> {1}<{2}>\n    add_type(const std::string&, const boost::function<{0} (const Type&)>& get, const boost::optional<{0}>&);"\
                .format("T" + str(self.n), structName(self.n + 1), typesStr(self.n + 1))
        select_type = ""
        select_type += "\n    template<typename {0}>"
        select_type += "\n      boost::function<{1}< {2} > (const std::string&, const boost::function<{0} (const Type&)>&)>"
        s += (select_type + "\n      select_type(const {0}*);")\
                .format("T" + str(self.n), structName(self.n + 1), typesStr(self.n + 1))
        s += (select_type + "\n      select_type_def(const {0}*, const {0}&);")\
                .format("T" + str(self.n), structName(self.n + 1), typesStr(self.n + 1))
        s += "\n"
        if self.n > 0:
            s += self.packStr("Int")
            s += self.unpackStr("Int")
            s += self.packStr("Ext")
            s += self.unpackStr("Ext")
        
        s += "\n};"
        return s
    def add_type(self):
        s = templateStr(self.n) + "template<typename T{0}>".format(str(self.n))
        s += "\n" + structName(self.n + 1) + "<" + typesStr(self.n + 1) + ">"
        s += "\n  " + structName(self.n) + "<" + typesStr(self.n) + ">"
        s += "::add_type(const std::string& tag, const boost::function<T{0} (const Type&)>& get, const boost::optional<T{0}>& def) {"\
                .replace("{0}", str(self.n))
        fields = fieldsStr(self.n)
        if self.n > 0:
            fields += ", "
        s += "\n  return {0}<{1}>(-fields-FieldDesc<Type, T{2}>(tag, get, def));"\
                .format(structName(self.n + 1), typesStr(self.n + 1), self.n)\
                .replace("-fields-", fields)
        s += "\n}"
        return s
    def select_type(self, mode):
        s = templateStr(self.n) + "template<typename T{0}>".format(str(self.n))
        s += "\n      boost::function<{1}< {2} > (const std::string&, const boost::function<{0} (const Type&)>&)>"\
                .format("T" + str(self.n), structName(self.n + 1), typesStr(self.n + 1))
        decl = "\n  {1}< {2} >::select_type(const {0}*) "
        if mode == 'def':
            decl = "\n  {1}< {2} >::select_type_def(const {0}*, const {0}& def) "
        s += decl.format("T" + str(self.n), structName(self.n), typesStr(self.n))
        s += "\n{"
        defVal = "boost::optional<{0}>()".format("T" + str(self.n))
        if mode == 'def':
            defVal = 'def'
        s += "\n  return boost::bind(&{1}< {2} >::add_type<{0}>, this, _1, _2, {3});"\
                .format("T" + str(self.n), structName(self.n), typesStr(self.n), defVal)
        s += "\n}"
        return s
#template<typename Type, typename T0, typename T1>template<typename T2>
#boost::function<Constructor3<Type, T0, T1, T2> (const std::string& tag, const boost::function<T2 (const Type&)>& get)>
    #Constructor2<Type, T0, T1>::select_type_def(const T2*, const T2& def) {
        #return boost::bind(&Constructor2<Type, T0, T1>::add_type<T2>, this, _1, _2, boost::optional<T2>(def));
#}

n = 10
if len(sys.argv) == 2:
    n = int(sys.argv[1]) + 1

l = []
for i in range(0, n + 1):
    l.append(Constructor(i))

for c in l:
    print c.forwardDecl()

l = l[:-1]
print "\n"
for c in l:
    print '\n' + c.structDecl()

print "\n"
for c in l:
    print '\n' + c.add_type()
    print '\n' + c.select_type(None)
    print '\n' + c.select_type('def')


print """} // details
} // json_spirit
#undef JSON_UNPACK_OPTIONAL"""
