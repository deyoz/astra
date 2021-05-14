#ifndef DBO_H
#define DBO_H

#include <variant>
#include <optional>
#include <map>
#include <set>

#include "exceptions.h"
#include "astra_consts.h"
#include "date_time.h"
#include "PgOraConfig.h"

#include <serverlib/dates_io.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "FELIX"
#include <serverlib/slogger_nonick.h>
#include <serverlib/test.h>

namespace DbCpp {
    class Session;
    class CursCtl;
}

namespace dbo
{

enum CurrentDb {
    Config,
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

struct QueryOps {
    std::string select;
    std::string where;
    std::string from;
    std::string order_by;
    std::string fetch_first;
    bool        for_update = false;
};

DbCpp::Session* getSession(CurrentDb db, const std::shared_ptr<MappingInfo>& mapInfo,
                           bool for_update, const std::string& from);
std::string buildQuery(const std::shared_ptr<MappingInfo> &mapInfo, const QueryOps & ops,
                       bool isOracle);

typedef std::variant<bool, int, short, long, long long, float, double, std::string,
                     Dates::DateTime_t, Dates::Date_t> bindedTypes;

static std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

enum Flag {
    NotNull = 0x1
};

template<typename T>
constexpr bool isSimpleType =
    std::is_same_v<T, bool> ||
    std::is_same_v<T, Dates::DateTime_t> ||
    std::is_same_v<T, Dates::Date_t> ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, long> ||
    std::is_same_v<T, long long> ||
    std::is_same_v<T, short> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, float>;

template<typename T>
constexpr T nullValue(const T& val)
{
    //    if constexpr (std::is_same_v<V, bool>) {
    //        return false;
    //    }
    if constexpr (std::is_same_v<T, Dates::DateTime_t>) {
        return Dates::not_a_date_time;
    }
    else if constexpr (std::is_same_v<T, Dates::Date_t>) {
        return Dates::date{};
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return std::string("");
    }
    else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same_v<T, double>) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same_v<T, float>) {
        return ASTRA::NoExists;
    }
    else if constexpr (std::is_same_v<T, short>) {
        return -1;
    }
    else if constexpr (std::is_same_v<T, BASIC::date_time::TDateTime>) {
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

template<typename T>
T coalesce(T const a, T const b)
{
    return dbo::isNotNull(a) ? a : b;
}

template<typename T1, typename... T>
T1 coalesce(T1 const a, T... args)
{
    return coalesce(a, coalesce(args...));
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
        //LogTrace5 << __func__ << " name :" << name_ << " value: " << value;
        if(isNullable()) {
//            LogTrace5 << (isNotNull(value) ? " NOT NULL" : "NULL");
//            LogTrace5 << __func__ << name_ << " value: " << (isNotNull(value) ? value : nullValue(value_));
            cur.bind(":"+name_, value, isNotNull(value) ? &nnull : &null);
        } else {
            //LogTrace5 << __func__ << " name :" << name_ << " value: " << value;
            cur.bind(":"+name_, value);
        }
    }

    void def(DbCpp::CursCtl &cur) const
    {
        if(isNullable()) {
            //LogTrace5 << __func__ << " nullable: " << name_ <<  " value: " << nullValue(value_);
            cur.defNull(value_, nullValue(value_));
        } else {
            //LogTrace5 << __func__ << "NOT nullable: " << name_;
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

class Transaction
{
public:
    enum class Policy{
        AutoCommit,
        Managed
    };

    explicit Transaction(DbCpp::Session& s, Policy tr = Policy::AutoCommit)
        :session(s), policy(tr)
    {
    }

    bool isManaged() const {
        return policy==Policy::Managed;
    }

    void make_savepoint()
    {
        make_db_curs(savepointQuery, session).exec();
    }
    void rollback_savepoint()
    {
        make_db_curs(rollbackQuery, session).exec();
    }
    void release_savepoint()
    {
        make_db_curs(releaseQuery, session).exec();
    }
private:
    DbCpp::Session& session;
    Policy policy;
    static constexpr const char* savepointQuery = "SAVEPOINT SP_EXEC_SQL";
    static constexpr const char* rollbackQuery = "ROLLBACK TO SAVEPOINT SP_EXEC_SQL";
    static constexpr const char* releaseQuery = "RELEASE SAVEPOINT SP_EXEC_SQL";
};

class Cursor
{
    DbCpp::CursCtl cur_;
    std::set<DbCpp::ResultCode> ignore_errors;
    Transaction transaction;
public:
    Cursor(DbCpp::CursCtl&& cur, Transaction tr)
        : cur_(std::move(cur)), transaction(tr)
    {
        cur_.unstb();
    }

    virtual ~Cursor() = default;

    Cursor& bind(const std::map<std::string, dbo::bindedTypes>& bindedVars)
    {
        short null = -1, nnull = 0;
        for(const auto & [name, value] : bindedVars) {
            std::visit([&](const auto &v) {
                cur_.bind(str_tolower(name), v, isNotNull(v) ? &nnull : &null);
            }, value);
        }
        return *this;
    }
    //Переменные биндятся по значению
    Cursor& stb()
    {
        cur_.stb();
        return *this;
    }
    //Переменные биндятся по адресу, сохраняется адрес объекта, нельзя использовать с временными переменными(rvalue ref)
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
        if(transaction.isManaged()) {
            transaction.make_savepoint();
            cur_.exec();
            if(ignore_errors.find(cur_.err()) != ignore_errors.end()) {
                transaction.rollback_savepoint();
                LogTrace5 << __func__ << " rollback. transaction error: " << cur_.err();
            } else {
                transaction.release_savepoint();
            }
        } else {
            cur_.exec();
        }
        return *this;
    }

    Cursor& noThrowError(DbCpp::ResultCode err)
    {
        cur_.noThrowError(err);
        return *this;
    }

    bool fen()
    {
        DbCpp::ResultCode res = cur_.fen();
        //LogTrace5 << " fen returned: " << res;
        return res != DbCpp::ResultCode::Ok;
    }

    template<typename Container>
    void ignoreErrors(Container errors)
    {
        ignore_errors.insert(begin(errors), end(errors));
        for(const auto& err: ignore_errors) {
            noThrowError(err);
        }
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
public:
    explicit Session(dbo::CurrentDb db = dbo::Config): _currentDb(db) {}
    Session(const Session &s) = delete;
    Session& operator=(const Session &s) = delete;
    Session(Session &&) = delete;
    Session& operator=(Session &&) = delete;

    template<typename Object>
    void insert(Object && obj, const std::string& tblName = "")
    {
        std::string tableName = tblName;
        if(tableName.empty()) {
            tableName = Mapper::getInstance().getMapping<Object>()->tableName();
        }
        std::shared_ptr<MappingInfo> mapInfo = Mapper::getInstance().getMapping(tableName);
        if(!mapInfo) {
            LogTrace5 << __func__ << "Unknown table: " << tableName;
            throw EXCEPTIONS::Exception("Unknown table: " + tableName);
        }
        std::string query = mapInfo->insertColumns();
        DbCpp::Session& session = PgOra::getRWSession(mapInfo->tableName());
        Cursor cur(session.createCursor(STDLOG, query), transactPolicy(session));
        cur.ignoreErrors(moveErrors());
        cur.bindAll(obj).exec();
    }

    std::string dump(const std::string& db, const std::string& tableName,
                     const std::vector<std::string>& tokens, const std::string& query);

    template <class Result>
    Query<Result> query(std::string selectQuery = "")
    {
        return Query<Result>(this, selectQuery);
    }

    Session & noThrowError(DbCpp::ResultCode err)
    {
        _ignoreErrors.push_back(err);
        return *this;
    }

    dbo::CurrentDb currentDb() const
    {
        return _currentDb;
    }
    Transaction transactPolicy(DbCpp::Session& session);
    std::vector<DbCpp::ResultCode> moveErrors();
private:
    dbo::CurrentDb _currentDb;
    std::vector<DbCpp::ResultCode> _ignoreErrors;
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
        : m_sess(sess), m_ops{selectQuery}
    {
    }

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

    Query<Result> & from(const std::string& condition)
    {
        m_ops.from = condition;
        return *this;
    }

    Query<Result> & where(const std::string& condition)
    {
        m_ops.where = condition;
        return *this;
    }

    Query<Result> & order_by(const std::string& condition)
    {
        m_ops.order_by = condition;
        return *this;
    }

    Query<Result> & for_update(bool condition = true)
    {
        m_ops.for_update = condition;
        return *this;
    }

    Query<Result> & fetch_first(const std::string& condition)
    {
        m_ops.fetch_first = condition;
        return *this;
    }

    Query<Result> & setBind(const std::map<std::string, dbo::bindedTypes> & bindedVars)
    {
        bindVars = bindedVars;
        return *this;
    }

private:
    Session* m_sess;
    QueryOps m_ops;
    std::map<std::string, dbo::bindedTypes> bindVars{};

    std::string createQuery(const std::shared_ptr<MappingInfo>& mapInfo, bool isOracle)
    {
        return buildQuery(mapInfo, m_ops, isOracle);
    }

    Cursor getCursor(std::shared_ptr<MappingInfo> map_info)
    {
        DbCpp::Session* session = getSession(m_sess->currentDb(), map_info, m_ops.for_update, m_ops.from);
        ASSERT(session);
        std::string query = createQuery(map_info, session->isOracle());
        LogTrace(TRACE6) << " query: " << query;
        return Cursor(session->createCursor(STDLOG, query), Transaction(m_sess->transactPolicy(*session)));
    }

    std::vector<Result> resultList()
    {
        const auto& map_info = Mapper::getInstance().getMapping<Result>();
        Cursor cur = getCursor(map_info);
        cur.ignoreErrors(m_sess->moveErrors());
        Result r;
        cur.bind(bindVars);
        cur.defAll(r);
        cur.exec();

        std::vector<Result> res;
        while(!cur.fen()) {
            res.push_back(r);
        }
        //LogTrace5 << " result vector size: " << res.size();
        return res;
    }
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
