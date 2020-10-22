#ifndef DBO_H
#define DBO_H

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <any>
#include <utility>
#include <optional>
#include <cctype>
#include <optional>
#include <memory>

#include "astra_types.h"
#include "astra_dates.h"
#include "astra_consts.h"
#include "exceptions.h"
#include "string_view"
#include "pg_session.h"
#include "date_time.h"
#include "stat/stat_agent.h"

#include <serverlib/rip_oci.h>
#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>
#include <serverlib/cursctl.h>
#include <serverlib/algo.h>
#include <serverlib/str_utils.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include "serverlib/oci_rowid.h"


#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
//#include <serverlib/slogger.h>
#include <serverlib/slogger_nonick.h>

using std::string;
using std::vector;
using std::shared_ptr;
using std::type_info;
using std::endl;

using namespace Dates;

namespace dbo
{

enum CurrentDb{
    Oracle,
    Postgres
};


class Session;
template<typename Result>
class Query;
class InitSchema;
class MappingInfo;
class CursorInterface;
template<typename Cur> class DefineClass;
template<typename Cur> class Binder;



typedef  std::variant<int, long long, float, double, DateTime_t, Date_t ,std::string,
    bool, PointId_t, GrpId_t, MoveId_t, PaxId_t, PointIdTlg_t, PnrId_t > bindedTypes;

static std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); }
    );
    return s;
}

enum Flag {
    NotNull = 0x1
};

template<typename T>
constexpr bool isSimpleType =
    std::is_same<T, bool>::value ||
    std::is_same<T, DateTime_t>::value ||
    std::is_same<T, Date_t>::value ||
    std::is_same<T, std::string>::value ||
    std::is_same<T, int>::value ||
    std::is_same<T, short>::value ||
    std::is_same<T, double>::value ||
    std::is_same<T, float>::value;

template<typename V>
constexpr V nullValue(const V& val)
{
    //    if constexpr (std::is_same<V, bool>::value) {
    //        return false;
    //    }
    if constexpr (std::is_same<V, DateTime_t>::value) {
        return not_a_date_time;
    }
    else if constexpr (std::is_same<V, Date_t>::value) {
        return not_a_date_time;
    }
    else if constexpr (std::is_same<V, std::string>::value) {
        return std::string("");
    }
    else if constexpr (std::is_same<V, int>::value) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same<V, double>::value) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same<V, float>::value) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same<V, short>::value) {
        return -1;
    }
    else if constexpr (std::is_same<V, BASIC::date_time::TDateTime>::value) {
        return ASTRA::NoExists;
    }
    else return val;
}

template<typename C>
bool isNull(const C& val)
{
    return val == nullValue(val);
}

template<typename C>
bool isNotNull(const C& val)
{
    return val != nullValue(val);
}

template <typename V>
class FieldRef
{
public:
    FieldRef(V& value, const std::string& name, int flags = 0)
        : value_(value),
          name_(str_tolower(name)),
          flags_(flags)
    {}

    int flags() const
    {
        return flags_;
    }

    const std::string& name() const
    {
        return name_;
    }

    bool isNullable() const
    {
        return  !(flags_ & Flag::NotNull);
    }

    template<typename Cur, typename T>
    void bind(Cur &cur, T&& value) const
    {
        if(isNotNull(value)) {
            cur.bind(":"+name_, std::forward<T>(value));
        } else {
            short null = -1;
            cur.bind(":"+name_, std::forward<T>(value), &null); //bind NULL
        }
    }

    template<typename Cur>
    void define(Cur &cur) const
    {
        if(isNullable()) {
            cur.defNull(value_, nullValue(value_));
        } else {
            cur.def(value_);
        }
    }

    const std::type_info & type() const
    {
        return typeid(V);
    }
    const V& value() const { return value_; }
    void setValue(const V& value) const { value_ = value; }

private:
    V& value_;
    std::string name_;
    int flags_;
};

template <typename Action, typename V>
void field(Action & action, V&& value, const std::string& name, int flags = 0)
{
    action.act(FieldRef<V>(value, name, flags));
}


template<typename C, typename Action>
void persist(C& object, Action & a)
{
    object.persist(a);
}


class FieldInfo
{
public:
    FieldInfo(const std::string& name, const std::type_info& type, int flags = 0) :
        name_(name),
        type_(type)
    {
    }
    const std::string& name() const
    {
        return name_;
    }
    const std::type_info & type() const
    {
        return type_;
    }

private:
    std::string name_;
    const std::type_info& type_;
};

class MappingInfo
{
public:
    MappingInfo(const std::string& tableName, const std::type_info & type) : m_type(type), m_tableName(tableName)
    {}
    template<typename C>
    void init();
    std::string columnsStr(const string &table = "") const;
    inline int columnsCount() const
    {
        return m_fields.size();
    }
    std::string stringColumns(const vector<string> &m_fields = {}) const;
    std::string insertColumns() const;
    const type_info & getType() const
    {
        return m_type;
    }
    void addFld(FieldInfo && fld) {
        m_fields.push_back(fld);
    }
    const std::string tableName() const {
        return m_tableName;
    }
private:
    const type_info & m_type;
    std::string m_tableName;
    std::vector<FieldInfo> m_fields;
    bool m_initialized = false;

    std::vector<string> columns() const;
};

template<typename Cur>
class DefineClass
{
public:
    DefineClass(Cur & cur) : m_cur(cur)  {}

    template<typename C>
    void visit(C& obj)
    {
        if constexpr(isSimpleType<C>)
        {
            m_cur.def(obj);
        } else {
            persist<C>(obj, *this);
        }
    }

    template <typename V>
    void act(const FieldRef<V> & field) {
        field.define(m_cur);
    }
private:
    Cur & m_cur;
};

template<typename Cur>
class Binder
{
public:
    Binder(Cur & cur) : m_cur(cur) {}

    template<typename C>
    void visit(C& obj) {
        persist<C>(obj, *this);
    }
    template <typename V>
    void act(const FieldRef<V> & field) {
        field.bind(m_cur, field.value());
    }
private:
    Cur & m_cur;
};

typedef const std::type_info * const_typeinfo_ptr;
struct typecomp {
    bool operator() (const const_typeinfo_ptr& lhs, const const_typeinfo_ptr& rhs) const
    {
        return lhs->before(*rhs);
    }
};

//Функтор наследующий все переданные ламбды и использующий их операторы ()
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

class Cursor
{
    std::variant<PgCpp::CursCtl *, OciCpp::CursCtl *> cur_;

public:
    Cursor(PgCpp::CursCtl & cur) :  cur_(&cur) {}
    Cursor(OciCpp::CursCtl & cur) :  cur_(&cur) {}
    virtual ~Cursor() = default;
    Cursor & bind(std::map<std::string, dbo::bindedTypes> &bindedVars)
    {
        std::visit([&](auto &cur){
            for(auto & [name, value] : bindedVars) {
                std::visit([&](auto &v)
                {
                    cur->bind(name, v);
                }, value);
            }
        }, cur_);
        return *this;
    }

    template<typename Object>
    Cursor & bindAll(Object & obj)
    {
        std::visit([&](auto &cur){
            Binder b(*cur);
            b.visit(obj);
        }, cur_);
        return *this;
    }

    std::string dump(int fields_size)
    {
        std::stringstream res;
        std::vector<std::string> vals(fields_size);
        std::visit([&](auto &cur) {
            for(std::string& val:  vals) {
                cur->defNull(val, "NULL");
            }
            cur->exec();
            while(!cur->fen()) {
                for(const std::string& val:  vals) {
                    res << "[" << val << "] ";
                }
                res << endl;
            }
        }, cur_);
        return res.str();
    }

    template<typename Result>
    Cursor & define(Result & r)
    {
        std::visit([&](auto &cur){
            DefineClass d(*cur);
            d.visit(r);
        }, cur_);
        return *this;
    }

    Cursor & exec() {
        std::visit([&](auto &cur)
        {
            cur->exec();
        }, cur_);
        return *this;
    }



    template<typename Err>
    Cursor & noThrowError(Err err)
    {
        std::visit( overload {
             [&](PgCpp::CursCtl *cur) {cur->noThrowError(static_cast<PgCpp::ResultCode>(err));},
             [&](OciCpp::CursCtl *cur) {cur->noThrowError(static_cast<int>(err));}
             }, cur_);
        return *this;
    }

    bool fen() {
        bool res = false;
        std::visit([&](auto &cur)
        {
            res =  cur->fen();
        }, cur_);
        return res;
    }

};

class Session
{
private:
    Session(){}

public:
    Session(const Session &s) = delete;
    Session& operator=(const Session &s) = delete;
    Session(Session &&) = delete;
    Session & operator=(Session &&) = delete;
    CurrentDb db;

    static Session& getInstance() {
        static Session s;
        return s;
    }
    void connectPostgres()
    {
        db = Postgres;
    }
    void connectOracle()
    {
        db = Oracle;
    }

    Cursor createCursor(PgCpp::CursCtl& pg_cur, OciCpp::CursCtl & or_cur)
    {
        if(db == CurrentDb::Postgres)
        {
            return Cursor(pg_cur);
        } else {
            return Cursor(or_cur);
        }
    }

    template <typename Class>
    void MapClass(const std::string &tableName)
    {
        std::string tblName = str_tolower(tableName);
        if(classRegistry.find(&typeid(Class)) != classRegistry.end()) {
            return;
        }
        auto mapInfo = std::make_shared<MappingInfo>(tblName, typeid(Class));
        if(!mapInfo) {
            throw EXCEPTIONS::Exception(" Error allocate memory");
        }
        mapInfo->init<Class>();
        classRegistry[&typeid(Class)] = mapInfo;
        if(tableRegistry.find(tblName) != tableRegistry.end()) {
            return;
        }
        tableRegistry[tblName] = mapInfo;
    }

    shared_ptr<MappingInfo> getMapping(const std::string& tableName) const
    {
        std::string tblName = str_tolower(tableName);
        auto it = tableRegistry.find(tblName);

        if (it != tableRegistry.end())
            return it->second;
        else
            return nullptr;
    }

    template<typename Class>
    shared_ptr<MappingInfo> getMapping() const
    {
        if constexpr(isSimpleType<Class>)
        {
            //LogTrace1 << " RETURN nullptr for : "  << std::string(typeid(Class).name());
            return nullptr;
        }
        auto it = classRegistry.find(&typeid(Class));
        if (it != classRegistry.end())
            return it->second;
        else
            throw EXCEPTIONS::Exception("Not known type: " + std::string(typeid(Class).name()));
    }


    template<typename Object>
    void insert(Object && obj, const std::string& tblName = "")
    {
        std::string tableName = tblName;
        if(tableName.empty()) {
            tableName = getMapping<Object>()->tableName();
        }
        shared_ptr<MappingInfo> mapInfo = getMapping(tableName);
        if(!mapInfo) {
            throw EXCEPTIONS::Exception("Not known table: " + tableName);
        }
        std::string query = mapInfo->insertColumns();

        PgCpp::CursCtl pg_cur = get_pg_curs(query);
        OciCpp::CursCtl or_cur = make_curs(query);

        Cursor cur = createCursor(pg_cur, or_cur);
        for(const auto & err : ignoreErrors) {
            cur.noThrowError(err);
        }
        cur.bindAll(obj).exec();
        ignoreErrors.clear();
    }

    std::string dump(const std::string& tableName, const vector<std::string>& tokens, const std::string& query);

    template <class Result>
    Query<Result> query(std::string selectQuery = "")
    {
        return Query<Result>(selectQuery);
    }

    Session & noThrowError(int err)
    {
        ignoreErrors.push_back(err);
        return *this;
    }
    std::vector<int> ignoreErrors;

private:
    std::map<std::string, shared_ptr<MappingInfo>> tableRegistry;
    std::map<const_typeinfo_ptr, shared_ptr<MappingInfo>, typecomp> classRegistry;
};

class InitSchema
{
public:
    InitSchema(MappingInfo &mapInfo) : mapInfo_(mapInfo)
    {}
    template<typename C>
    void visit(C& obj) {
        persist<C>(obj, *this);
    }

    template <typename V>
    void act(const FieldRef<V> & field) {
        mapInfo_.addFld(FieldInfo(str_tolower(field.name()), field.type()));
    }
private:
    MappingInfo & mapInfo_;
};

template<typename Result>
class Query
{
public:
    Query(const std::string& selectQuery) : m_select(selectQuery)
    {}

    operator std::optional<Result> ()
    {
        std::vector<Result> res = resultList();
        if(res.empty()) {
            LogTrace5 << " NO data found in query";
            return std::nullopt;
        }
        if(res.size() > 1) {
            throw EXCEPTIONS::Exception(" Returned more than one row ");
        }
        return res.front();
    }

    operator std::vector<Result> ()
    {
        return resultList();
    }

    std::vector<Result> resultList()
    {
        auto& sess = Session::getInstance();
        auto mapInfo = sess.getMapping<Result>();
        std::string query = createQuery(mapInfo);

        PgCpp::CursCtl pg_cur = get_pg_curs(query);
        OciCpp::CursCtl or_cur = make_curs(query);
        Cursor cur = sess.createCursor(pg_cur, or_cur);

        Result r;
        cur.bind(bindVars)
           .define(r)
           .exec();

        std::vector<Result> res;
        while(!cur.fen()) {
            res.push_back(r);
        }
        sess.ignoreErrors.clear();
        return res;
    }

    Query<Result> & from(std::string condition)
    {
        m_from = condition ;
        return *this;
    }

    Query<Result> & where(std::string condition)
    {
        m_where = condition ;
        return *this;
    }

    Query<Result> & setBind(const std::map<std::string, dbo::bindedTypes> &bindedVars)
    {
        bindVars = bindedVars;
        return *this;
    }

    std::string createQuery(const std::shared_ptr<MappingInfo>& mapInfo)
    {
        std::stringstream res;
        if(!m_select.empty()) {
            res << m_select;
        } else {
            if(!mapInfo) {
                throw EXCEPTIONS::Exception("Not correct select type or forgot select query");
            }
            res << "select " << mapInfo->columnsStr(mapInfo->tableName());
        }
        if(!m_from.empty()) {
            res << " from " << m_from;
        } else {
            if(!mapInfo) {
                throw EXCEPTIONS::Exception("Not correct select type");
            }
            res << " from " << mapInfo->tableName();
        }
        if(!m_where.empty()) {
            res <<  " where " << m_where;
        }
        return res.str();
    }

private:
    std::string m_select;
    std::string m_where;
    std::string m_from;
    std::map<std::string, bindedTypes> bindVars;
};

template<typename C>
void MappingInfo::init()
{
    if(!m_initialized) {
        m_initialized = true;
        C dummy;
        InitSchema action(*this);
        action.visit(dummy);
    }
}

}

#undef NICKNAME
#endif // DBO_H
