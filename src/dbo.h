#ifndef DBO_H
#define DBO_H

#include <variant>

#include "stat/stat_agent.h"
#include <serverlib/dates_io.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/dbcpp_cursctl.h>
#include "hooked_session.h"

#define NICKNAME "FELIX"
#include <serverlib/slogger_nonick.h>


namespace DbCpp {
    class Session;
    class CursCtl;
}

namespace dbo
{

enum CurrentDb {
    Oracle,
    Postgres
};

template<typename Result>
class Query;
class InitSchema;
class MappingInfo;
class CursorInterface;
class DefineClass;
class Binder;



typedef std::variant<bool, int, long long, float, double, std::string,
                     Dates::DateTime_t, Dates::Date_t> bindedTypes;

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
    std::is_same<T, Dates::DateTime_t>::value ||
    std::is_same<T, Dates::Date_t>::value ||
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
    if constexpr (std::is_same<V, Dates::DateTime_t>::value) {
        return Dates::not_a_date_time;
    }
    else if constexpr (std::is_same<V, Dates::Date_t>::value) {
        return Dates::date{};
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

const short null = -1;
const short nnull = 0;

template<typename C>
bool isNull(const C& val)
{
    return val == nullValue(val);
}

template<typename C>
bool isNotNull(const C& val)
{
    return !isNull(val);
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

    template<typename T>
    void bind(DbCpp::CursCtl &cur, const T& value) const
    {
        cur.bind(":"+name_, value, isNotNull(value) ? &nnull : &null);
    }

    void def(DbCpp::CursCtl &cur) const
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
    std::string columnsStr(const std::string &table = "") const;
    inline size_t columnsCount() const
    {
        return m_fields.size();
    }
    std::string stringColumns(const std::vector<std::string> &m_fields = {}) const;
    std::string insertColumns() const;
    const std::type_info& getType() const
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
    const std::type_info& m_type;
    std::string m_tableName;
    std::vector<FieldInfo> m_fields;
    bool m_initialized = false;

    std::vector<std::string> columns() const;
};

class DefineClass
{
public:
    DefineClass(DbCpp::CursCtl & cur) : m_cur(cur)  {}

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
        field.def(m_cur);
    }
private:
    DbCpp::CursCtl & m_cur;
};

class Binder
{
public:
    Binder(DbCpp::CursCtl & cur) : m_cur(cur) {}

    template<typename C>
    void visit(C& obj) {
        persist<C>(obj, *this);
    }
    template <typename V>
    void act(const FieldRef<V> & field) {
        field.bind(m_cur, field.value());
    }
private:
    DbCpp::CursCtl & m_cur;
};

typedef const std::type_info * const_typeinfo_ptr;
struct typecomp {
    bool operator() (const const_typeinfo_ptr& lhs, const const_typeinfo_ptr& rhs) const
    {
        return lhs->before(*rhs);
    }
};

//�㭪�� ��᫥���騩 �� ��।���� ������ � �ᯮ����騩 �� ������� ()
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

class Cursor
{
    DbCpp::CursCtl cur_;

public:
    Cursor(DbCpp::CursCtl&& cur)
        : cur_(std::move(cur))
    {
        cur_.unstb();
    }

    virtual ~Cursor() = default;

    Cursor& bind(const std::map<std::string, dbo::bindedTypes>& bindedVars)
    {
        short null = -1, nnull = 0;
        for(const auto & [name, value] : bindedVars) {
            std::visit([&](const auto &v) {
                cur_.bind(name, v, isNotNull(v) ? &nnull : &null);
            }, value);
        }
        return *this;
    }
    //��६���� �������� �� ���祭��
    Cursor& stb()
    {
        cur_.stb();
        return *this;
    }
    //��६���� �������� �� �����, ��࠭���� ���� ��ꥪ�, ����� �ᯮ�짮���� � �६���묨 ��६���묨(rvalue ref)
    Cursor& unstb()
    {
        cur_.unstb();
        return *this;
    }
    template<typename Object>
    Cursor& bindAll(Object & obj)
    {
        Binder b(cur_);
        b.visit(obj);
        return *this;
    }

    std::string dump(size_t fields_size);

    template<typename Result>
    Cursor& defAll(Result & r)
    {
        DefineClass d(cur_);
        d.visit(r);
        return *this;
    }

    Cursor& exec()
    {
        cur_.exec();
        return *this;
    }

    Cursor& noThrowError(DbCpp::ResultCode err)
    {
        cur_.noThrowError(err);
        return *this;
    }

    bool fen()
    {
        return cur_.fen() != DbCpp::ResultCode::Ok;
    }

};


class Mapper
{
private:
    Mapper() {}

public:
    static Mapper& getInstance()
    {
        static Mapper inst;
        return inst;
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
            throw EXCEPTIONS::Exception("Unable to allocate memory");
        }
        mapInfo->init<Class>();
        classRegistry[&typeid(Class)] = mapInfo;
        if(tableRegistry.find(tblName) != tableRegistry.end()) {
            return;
        }
        tableRegistry[tblName] = mapInfo;
    }

    std::shared_ptr<MappingInfo> getMapping(const std::string& tableName) const
    {
        std::string tblName = str_tolower(tableName);
        auto it = tableRegistry.find(tblName);
        if(it != tableRegistry.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    template<typename Class>
    std::shared_ptr<MappingInfo> getMapping() const
    {
        if constexpr(isSimpleType<Class>)
        {
            //LogTrace1 << " RETURN nullptr for : "  << std::string(typeid(Class).name());
            return nullptr;
        }
        auto it = classRegistry.find(&typeid(Class));
        if (it != classRegistry.end()) {
            return it->second;
        } else {
            throw EXCEPTIONS::Exception("Unknown type: " + std::string(typeid(Class).name()));
        }
    }

private:
    std::map< std::string, std::shared_ptr<MappingInfo> > tableRegistry;
    std::map< const_typeinfo_ptr, std::shared_ptr<MappingInfo>, typecomp > classRegistry;
};


class Session
{
private:
    Session() {}

protected:
    Session(DbCpp::Session* oraSess,
            DbCpp::Session* pgSessRo, DbCpp::Session* pgSessRw);

public:
    Session(const Session &s) = delete;
    Session& operator=(const Session &s) = delete;
    Session(Session &&) = delete;
    Session& operator=(Session &&) = delete;

    static Session& getMainInstance();
    static Session& getArxInstance();

    void connectPostgres()
    {
        db = Postgres;
    }
    void connectOracle()
    {
        db = Oracle;
    }

    Cursor getReadCursor(const std::string& sql)
    {
        if(db == CurrentDb::Postgres) {
            return Cursor(pg_dbcpp_session_ro->createCursor(STDLOG, sql));
        } else {
            return Cursor(ora_dbcpp_session->createCursor(STDLOG, sql));
        }
    }

    Cursor getWriteCursor(const std::string& sql)
    {
        if(db == CurrentDb::Postgres) {
            return Cursor(pg_dbcpp_session_rw->createCursor(STDLOG, sql));
        } else {
            return Cursor(ora_dbcpp_session->createCursor(STDLOG, sql));
        }
    }

    template<typename Object>
    void insert(Object && obj, const std::string& tblName = "")
    {
        std::string tableName = tblName;
        if(tableName.empty()) {
            tableName = Mapper::getInstance().getMapping<Object>()->tableName();
        }
        std::shared_ptr<MappingInfo> mapInfo = Mapper::getInstance().getMapping(tableName);
        if(!mapInfo) {
            throw EXCEPTIONS::Exception("Unknown table: " + tableName);
        }
        std::string query = mapInfo->insertColumns();

        LogTrace1 << "query: " << query;

        Cursor cur = getWriteCursor(query);
        for(const auto & err : ignoreErrors) {
            cur.noThrowError(err);
        }
        cur.bindAll(obj).exec();
        ignoreErrors.clear();
    }

    std::string dump(const std::string& tableName, const std::vector<std::string>& tokens, const std::string& query);

    template <class Result>
    Query<Result> query(std::string selectQuery = "")
    {
        return Query<Result>(this, selectQuery);
    }

    Session & noThrowError(DbCpp::ResultCode err)
    {
        ignoreErrors.push_back(err);
        return *this;
    }

    void clearIgnoreErrors();

private:
    CurrentDb db;
    std::vector<DbCpp::ResultCode> ignoreErrors;

    // DbCpp session for reading/writing from/to Oracle
    DbCpp::Session* ora_dbcpp_session;
    // DbCpp sessions for reading/writing from/to Postgresql
    DbCpp::Session* pg_dbcpp_session_ro;
    DbCpp::Session* pg_dbcpp_session_rw;
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
    Query(Session* sess, const std::string& selectQuery)
        : m_sess(sess),
          m_select(selectQuery)
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
        auto mapInfo = Mapper::getInstance().getMapping<Result>();
        std::string query = createQuery(mapInfo);
        //LogTrace5 << __FUNCTION__ << " query: " << query;
        Cursor cur = m_sess->getReadCursor(query);

        Result r;
        cur.bind(bindVars)
           .defAll(r)
           .exec();

        std::vector<Result> res;
        while(!cur.fen()) {
            res.push_back(r);
        }
        m_sess->clearIgnoreErrors();
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

    Query<Result> & setBind(const std::map<std::string, dbo::bindedTypes> & bindedVars)
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
    Session*    m_sess;
    std::string m_select;
    std::string m_where;
    std::string m_from;
    std::map<std::string, dbo::bindedTypes> bindVars{};
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