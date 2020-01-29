#pragma once

#include <arpa/inet.h>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstring>
#include <stdexcept>

struct memcached_st;

namespace memcache
{

struct UninitialisedCallbacks : public std::logic_error
{
    UninitialisedCallbacks() : std::logic_error("Memcached callbacks not initialized!") {}
};

struct HostPortPair
{
    std::string m_host;
    unsigned    m_port;

    HostPortPair(const std::string& host, unsigned port)
        : m_host(host), m_port(port)
    {}
};
typedef std::vector<HostPortPair> HostPortPairs_t;

//---------------------------------------------------------------------------------------

size_t MemcachedMaxKey();

/**
 *  Обёртка над libmemcahed
**/
class MCache
{
public:
    MCache(const std::string& hostSetting, const std::string& portSetting,
           const std::string& relatedInstancesSetting,
           const std::string& expirationSettings);
    MCache(const std::string& host, unsigned port);
    ~MCache();

    bool get(std::vector<char>& res, const std::string& key);
    bool get(std::string& res, const std::string& key);
    bool get(char*& value, size_t& len, const std::string& key);
    bool get(const std::string& key);

    template<class T> std::enable_if_t<std::is_pod<T>::value, bool> get(T& res, const std::string& key)
    {
        std::vector<char> vec;
        bool ret = get(vec, key);
        if(ret && !vec.empty())
        {
            memcpy(&res, &vec[0], vec.size());
            return true;
        }
        return false;
    }

    bool mget(const std::vector<std::string>& keys);
    bool fetch(std::vector<char>& res, std::string& key);
    bool fetch(char*& value, size_t& len, std::string& key);

    template<class T> std::enable_if_t<std::is_pod<T>::value, bool> fetch(T& res, std::string& key)
    {
        std::vector<char> vec;
        bool ret = fetch(vec, key);
        if(ret && !vec.empty())
        {
            memcpy(&res, &vec[0], vec.size());
            return true;
        }
        return false;
    }

    bool set(const std::string& key, const std::vector<char>& value,
             time_t expiration = 0, uint32_t flags = 0);
    bool set(const std::string& key, const std::string& value,
             time_t expiration = 0, uint32_t flags = 0);
    bool set(const std::string& key, const char* value, size_t len,
             time_t expiration = 0, uint32_t flags = 0);

    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        set(const std::string& key, const T& value,
             time_t expiration = 0, uint32_t flags = 0)
    {
        std::vector<char> vec(sizeof(T));
        memcpy(&vec[ 0 ], &value, sizeof(T));
        return set(key, vec, expiration);
    }

    bool add(const std::string& key, const std::vector<char>& value,
             time_t expiration = 0, uint32_t flags = 0);
    bool add(const std::string& key, const std::string& value,
             time_t expiration = 0, uint32_t flags = 0);

    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        add(const std::string& key, const T& value,
             time_t expiration = 0, uint32_t flags = 0)
    {
        std::vector<char> vec(sizeof(T));
        memcpy(&vec[ 0 ], &value, sizeof(T));
        return add(key, vec);
    }

    bool del(const std::string& key);

    bool flush(time_t expiration = 0);

    const HostPortPairs_t& relatedHostsAndPorts() const { return m_relatedHostsAndPorts; }

    const std::string& host() const { return m_host; }
    unsigned port() const { return m_port; }

    bool isValid() const { return m_valid; }

    time_t getExpirationTime() const { return m_expiration; }

protected:
    bool init(const std::string& host, unsigned port);
    bool addServer(const std::string& hostname, in_port_t port);
    bool trueFlush(time_t expiration);

private:
    memcached_st*   m_memc;
    HostPortPairs_t m_relatedHostsAndPorts;
    time_t          m_expiration;
    std::string     m_host;
    unsigned        m_port;
    bool            m_valid;
};

//---------------------------------------------------------------------------------------

/**
 *  Тег кэша
*/
struct MCacheTag
{
    static const unsigned MaxTagKeyLen = 35;
    char key[MaxTagKeyLen + 1];
    uint64_t version;

    void setVersion(const uint64_t& v) { version = v; }
    uint64_t getVersion() const { return version; }

    void setKey(const std::string& k);
    std::string getKey() const;
    bool isValid() const;
};

//---------------------------------------------------------------------------------------

/**
 *  Метаданные объекта кэша
**/
struct MCacheMetaData
{
    static const unsigned MaxTagsCount = 4;
    unsigned m_tagsCount;
    MCacheTag m_tags[MaxTagsCount];

    bool addTag(const MCacheTag& tag);
    std::vector<std::string> tagsKeys() const;
    uint64_t getTagVersion(const std::string& tagKey) const;
};

//---------------------------------------------------------------------------------------

/**
 *  Объект проверки актуальности кэша
**/
class MCacheCheck
{
public:
    static bool check(const std::string& key,
                      const MCacheMetaData& metaData,
                      MCache* mcache);
    static uint64_t setTagVersion(MCacheTag& tag, MCache* mcache, time_t expiration);
    static uint64_t getTagVersion(const MCacheTag& tag, MCache* mcache);
};

//---------------------------------------------------------------------------------------

size_t MaxUserBuffSize();

//---------------------------------------------------------------------------------------

/**
 *  Объект синхронизации доступа к кэшу
**/
class MCacheLock
{
public:
    MCacheLock(const std::string& key, MCache* mcache, bool unlockOnDestroy = true);
    ~MCacheLock();
    bool lock();
    bool unlock();
    bool isLocked() const;

protected:
    std::string lockKey() const;
    std::string lockVal() const;
    time_t lockExpiration() const;

private:
    std::string m_key;
    MCache* m_mcache;
    bool m_locked;
    bool m_unlockOnDestroy;
};

//---------------------------------------------------------------------------------------

class MCacheObject
{
public:
    MCacheObject(const std::string& key, MCache* mcache);

    MCacheObject(const std::string& key, const char* data, size_t len, MCache* mcache,
                 const MCacheTag* tags = 0, unsigned tagsCount = 0);

    const std::string& key() const { return m_key; }

    const MCacheMetaData& metaData() const { return m_metaData; }
    MCacheMetaData& metaData() { return m_metaData; }

    const char* data() const { return m_data; }
    size_t dataLen() const { return m_dataLen; }

    bool isActual() const { return m_isActual; }

    const char* rawData(size_t& len) const;

    bool writeToCache(time_t expiration, bool manualLock);
    bool readFromCache();

    bool isValid() const { return m_isValid; }

protected:
    bool write_(time_t expiration);

    bool checkTagsVersions() const;

private:
    std::string m_key;
    const char* m_data;
    size_t m_dataLen;
    MCache* m_mcache;
    MCacheMetaData m_metaData;
    bool m_isActual;
    bool m_isValid;
};

//---------------------------------------------------------------------------------------

/* api для работы с кэшом, использующим memcached */

/**
 * @brief сконструировать ключ кэша
 * @param in recName - имя объекта, по которому создается тег. Обычно - имя таблицы
 * @param in recId - ключ по объекту, по которому создается тег. Обычно - ida
 * @return ключ тега
 */
std::string createTagKey(const std::string& recName, int recId);

/**
 * @brief сконструировать тег кэша
 * @param in recName - имя объекта, по которому создается тег. Обычно - имя таблицы
 * @param in recId - ключ по объекту, по которому создается тег. Обычно - ida
 * @param in version - версия тега
 * @return объекта тега кэша
 */
MCacheTag createTag(const std::string& recName, int recId, uint64_t version = 0);

/**
 * @brief сконструировать тег кэша
 * @param in tagKey - ключа тега
 * @param in version - версия тега
 * @return объекта тега кэша
 */
MCacheTag createTag(const std::string& tagKey, uint64_t version = 0);

//---------------------------------------------------------------------------------------


/**
 * @class класс-клиент конкретного instance memcached
 */
class UPV;

class Instance
{
public:
    /**
     * @brief construct
     */
    Instance(const std::string& id,
             const std::string& hostSetting, const std::string& portSetting,
             const std::string& relatedInstancesSetting = "",
             const std::string& expirationSettings = "");

    /**
     * @brief destruct
     */
    ~Instance();

    /*
        вспомогательные функции кэша
    */

    /**
     * @brief корректный ли это memcached?
     * @return true - да, false - нет
     */
    bool isValid() const;

    /**
     * @brief сбросить весь кэш
     * @return true - успех, false - ошибка
     */
    bool flushAll() const;

    /**
     * @brief узнать, заблокирован ли для записи кэш по ключу
     * @param in key - ключ, по которому расположен проверяемый на блокировку кэш
     * @return true - заблокирован, false - иначе
     */
    bool isCacheLocked(const std::string& key) const;

    /**
     * @brief установить блокировку для ключа в кэше
     * @param in key - ключ, по которому расположен блокируемый кэш
     * @return true - успех, false - ошибка
     */
    bool lockCache(const std::string& key) const;

    /**
     * @brief снять блокировку для ключа в кэше
     * @param in key - ключ, по которому расположен кэш, для которого снимается блокировка
     * @return true - успех, false - ошибка
     */
    bool unlockCache(const std::string& key) const;

    /**
     * @brief узнать, существует ли запись по ключу
     * @param in key - проверяемый ключ
     * @return true - существует, false - иначе
     */
    bool isCacheExists(const std::string& key) const;

    /**
     * @brief инвалидировать кэш по тегу
     * @param in recName - имя объекта, по которому использутеся тег. Обычно - имя таблицы
     * @param in recId - ключ по объекту, по которому используется тег. Обычно - ida
     * @return true - успех, false - ошибка
     */
    bool invalidateByTag(const std::string& recName, int recId) const;

    /**
     * @brief инвалидировать кэш по ключу
     * @param in key - ключ, по которому расположен инвалидируемый кэш
     * @return true - успех, false - ошибка
     */
    bool invalidateByKey(const std::string& key) const;

    /*
        функции записи кэша
    */

    /**
     * @brief записать объект POD-типа Т в кэш по ключу
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - объект, который следует разместить в кэше
     * @param in manualLock - флаг ручной блокировки
     * @return true - успех, false - ошибка
     */
    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        writeToCachePOD(const std::string& key, const T& value,
                         bool manualLock) const
    {
        return writeToCachePOD(key, value, manualLock, m_expiration);
    }

    /**
     * @brief записать объект POD-типа Т в кэш по ключу
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - объект, который следует разместить в кэше
     * @param in manualLock - флаг ручной блокировки
     * @param in expiration - время жизни кэша
     * @return true - успех, false - ошибка
     */
    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        writeToCachePOD(const std::string& key, const T& value,
                         bool manualLock, time_t expiration) const
    {
        MCacheObject cacheObject(key,
                                 reinterpret_cast<const char*>(&value),
                                 sizeof(value),
                                 m_mcache);
        return cacheObject.writeToCache(expiration, manualLock);
    }

    template<class T> std::enable_if_t<std::is_trivially_copyable<T>::value, bool>
        writeToCache(const std::string& key, const std::vector<T>& v,
                         bool manualLock, time_t expiration) const
    {
        MCacheObject cacheObject(key,
                                 reinterpret_cast<const char*>(v.data()),
                                 v.size() * sizeof(T),
                                 m_mcache);
        return cacheObject.writeToCache(expiration, manualLock);
    }

    /**
     * @brief записать блок данных в кэш по ключу
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in manualLock - флаг ручной блокировки
     * @return true - успех, false - ошибка
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      bool manualLock) const;
    /**
     * @brief записать блок данных в кэш по ключу
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in manualLock - флаг ручной блокировки
     * @param in expiration - время жизни кэша
     * @return true - успех, false - ошибка
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      bool manualLock, time_t expiration) const;

    /**
     * @brief записать блок данных в кэш по ключу с тегами для инвалидации
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in tags - массив тегов для инвалидации
     * @param in tagsCount - количество тегов в массиве
     * @param in manualLock - флаг ручной блокировки
     * @return true - успех, false - ошибка
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      const MCacheTag* tags, unsigned tagsCount,
                      bool manualLock) const;

    /**
     * @brief записать блок данных в кэш по ключу с тегами для инвалидации
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in tags - массив тегов для инвалидации
     * @param in tagsCount - количество тегов в массиве
     * @param in manualLock - флаг ручной блокировки
     * @param in expiration - время жизни кэша
     * @return true - успех, false - ошибка
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      const MCacheTag* tags, unsigned tagsCount,
                      bool manualLock, time_t expiration) const;

    /**
     * @brief записать объект POD-типа Т в кэш по ключу с тегами для инвалидации
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - объект, который следует разместить в кэше
     * @param in tags - массив тегов для инвалидации
     * @param in tagsCount - количество тегов в массиве
     * @param in manualLock - флаг ручной блокировки
     * @return true - успех, false - ошибка
     */
    // только для POD-типов!
    template<class T> typename std::enable_if<std::is_pod<T>::value, bool>::type
        writeToCachePOD(const std::string& key, const T& value,
                         const MCacheTag* tags, unsigned tagsCount,
                         bool manualLock) const
    {
        return writeToCachePOD(key, value, tags,  tagsCount, manualLock, m_expiration);
    }

    /**
     * @brief записать объект POD-типа Т в кэш по ключу с тегами для инвалидации
     * @param in key - ключ, по которому следут разместить объект в кэше
     * @param in value - объект, который следует разместить в кэше
     * @param in tags - массив тегов для инвалидации
     * @param in tagsCount - количество тегов в массиве
     * @param in manualLock - флаг ручной блокировки
     * @param in expiration - время жизни кэша
     * @return true - успех, false - ошибка
     */
    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        writeToCachePOD(const std::string& key, const T& value,
                         const MCacheTag* tags, unsigned tagsCount,
                         bool manualLock, time_t expiration) const
    {
        MCacheObject cacheObject(key, (const char*)&value, sizeof(value), m_mcache,
                                 tags, tagsCount);
        return cacheObject.writeToCache(expiration, manualLock);
    }

    /**
     * @brief записать блок данных в кэш по ключу без версии, без тегов и без блокировки
     * @param in key - ключ, по которому следует разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in manualLock - флаг ручной блокировки
     * @return true - успех, false - ошибка
     */
    bool writeToCacheRaw(const std::string& key, const char* value, size_t len) const;

    /**
     * @brief записать блок данных в кэш по ключу без версии, без тегов и без блокировки
     * @param in key - ключ, по которому следует разместить объект в кэше
     * @param in value - указатель на блок данных, размещаемых в кэше
     * @param in len - размер блока данных
     * @param in manualLock - флаг ручной блокировки
     * @param in expiration - время жизни кэша
     * @return true - успех, false - ошибка
     */
    bool writeToCacheRaw(const std::string& key,
                         const char* value, size_t len,
                         time_t expiration) const;


    /*
        функции чтения кэша
    */

    /**
     * @brief прочитать актуальный блок данных из кэша
     * @param out value - указатель на блок данных, прочитанный из кэша. !!need free!!
     * @param out len - размер блока данных, прочитанного из кэша
     * @param in key - ключ, по которому читаются данные из кэша
     * @return true - успех, false - ошибка
     */
    // вызывающая подпрограмма должна вызвать free() для value
    bool readFromCache(char*& value, size_t& len, const std::string& key) const;

    /**
     * @brief прочитать актуальный вектор из кэша
     * @param out value - результирующий вектор
     * @param in key - ключ, по которому читаются данные из кэша
     * @return true - успех, false - ошибка
     */
    bool readFromCache(std::vector<char>& value, const std::string& key) const;

    /**
     * @brief прочитать актуальный объект POD-типа Т из кэша
     * @param out value - прочитанный объект POD-типа Т
     * @param in key - ключ, по которому читаются данные из кэша
     * @return true - успех, false - ошибка
     */
    template<class T> std::enable_if_t<std::is_trivially_copyable<T>::value, bool>
        readFromCachePOD(T& value, const std::string& key) const
    {
        MCacheObject cacheObject(key, m_mcache);
        if(!cacheObject.readFromCache())
            return false;
        if(!cacheObject.isActual())
            return false;
        if(cacheObject.dataLen() != sizeof(value))
            return false;
        memcpy(&value, cacheObject.data(), sizeof(value));
        return true;
    }

    template<class T> std::enable_if_t<std::is_trivially_copyable<T>::value, bool>
        readFromCacheVector(std::vector<T>& v, const std::string& key) const
    {
        MCacheObject cacheObject(key, m_mcache);
        if(!cacheObject.readFromCache())
            return false;
        if(!cacheObject.isActual())
            return false;
        if(cacheObject.dataLen() % sizeof(T))
            return false;
        v.resize(cacheObject.dataLen() / sizeof(T));
        auto t = reinterpret_cast<const T*>(cacheObject.data());
        std::copy(t, t+v.size(), v.begin());
        return true;
    }

    /**
     * @brief прочитать имена и версии тегов кэша с ключом key
     * @param out tagsVersions
     * @param in key
     */
    void readCacheTagsVersions(std::map<std::string, uint64_t>& tagsVersions,
                               const std::string& key) const;

    /**
     * @brief прочитать блок каких-то данных из кэша
     * @param out value - указатель на блок данных, прочитанный из кэша. !!need free!!
     * @param out len - размер блока данных, прочитанного из кэша
     * @param in key - ключ, по которому читаются данные из кэша
     * @return true - успех, false - ошибка
     */
    // вызывающая подпрограмма должна вызвать free() для value
    bool readFromCacheRaw(char*& value, size_t& len, const std::string& key) const;

    template <typename T> using UP = std::unique_ptr<T,decltype(&free)>;
    template <typename T> std::enable_if_t<std::is_pod<T>::value, UP<T>> readFromCacheRaw(const std::string& key) const
    {
        size_t len = 0;
        char* p = nullptr;
        if(not readFromCacheRaw(p, len, key))
            return UP<T>(nullptr, free);
        if(len != sizeof(T))
        {
            free(p);
            throw std::logic_error("sizeof(T) does not match len");
        }
        return UP<T>(reinterpret_cast<T*>(p), free);
    }

    template <typename T> std::enable_if_t<std::is_pod<T>::value, UP<T>> readFromCache(const std::string& key) const
    {
        size_t len = 0;
        char* p = nullptr;
        if(not readFromCache(p, len, key))
            return UP<T>(nullptr, free);
        if(len != sizeof(T))
        {
            free(p);
            throw std::logic_error("sizeof(T) does not match len");
        }
        return UP<T>(reinterpret_cast<T*>(p), free);
    }

    UPV readFromCacheRaw(const std::string& key) const;
    UPV readFromCache(const std::string& key) const;
    /**
     * @brief подготовить данные для multi-read
     * @param in keys - ключи для multi-read
     * @return true - успех, false - ошибка
     */
    bool multiReadFromCache(const std::vector<std::string>& keys) const;

    /**
     * @brief считать следующий блок каких-то данных из кэша(после multiReadFromCacheRaw)
     * @param out value - указатель на блок данных, прочитанный из кэша. !!need free!!
     * @param out len - размер блока данных, прочитанного из кэша
     * @param out key - ключ, по которому считались данные из кэша
     */
    bool fetchNextRaw(char*& value, size_t& len, std::string& key) const;

    /**
     * @brief прочитать блок каких-то данных из кэша
     * @param in key - ключ, по которому читаются данные из кэша
     * @param out value - указатель на блок данных, прочитанного из кэша
     * @return true - успех, false - ошибка
     */
    bool readRaw(const std::string& key, std::string& value) const;

    /**
     * @brief получить id instance memcached
     * @return id instance memcached
     */
    const std::string& id() const { return m_id; }

    /**
     * @brief получить вектор инстансов, связанных с данным
     * @return вектор инстансов (возможно пустой)
     */
    std::vector<Instance*> relatedInstances() const;

    /**
     * @brief является ли инстанс related
     * @return true - является, false - нет
     */
    bool isRelated() const { return m_isRelated; }

    const std::string& host() const;
    unsigned port() const;

    bool testWriteRead() const;

protected:
    /*
     * @brief конструктор related инстанса
     */
    Instance(const std::string& id,
             const std::string& host, unsigned port, time_t expiration);

private:
    std::string m_id;
    bool        m_isRelated;
    MCache*     m_mcache;
    time_t      m_expiration;
};

class UPV
{
    Instance::UP<char> p;
    size_t l = 0;
    friend UPV Instance::readFromCache(const std::string& ) const;
    friend UPV Instance::readFromCacheRaw(const std::string& ) const;
    explicit UPV();
  public:
    const char* data() const { return p.get(); }
    size_t size() const { return l; }
    explicit operator bool() const { return p.get(); }
};

//---------------------------------------------------------------------------------------

/**
 * @class класс для управления несколькими instance memcached
 */
class InstanceManager
{
public:
    typedef std::map<std::string, Instance*> InstancesMap_t;
    typedef std::multimap<std::string, Instance*> InstancesMultiMap_t;
    typedef std::vector<Instance*> InstancesVector_t;

    static InstanceManager& singletone();

    void add(const std::string& id, Instance* inst);
    void addRelated(const std::string& id, Instance* inst);
    Instance* get(const std::string& id) const;
    InstancesVector_t getAllInstances() const;
    InstancesVector_t getRelatedInstances(const std::string& instanceId) const;

    bool invalidateRelatedInstancesByKey(const std::string& instanceId,
                                         const std::string& key) const;
    bool invalidateRelatedInstancesByTag(const std::string& instanceId,
                                         const std::string& recName, int recId);

protected:
    InstanceManager();

private:
    InstancesMap_t       m_instances;
    InstancesMultiMap_t  m_relatedInstances;
};

//---------------------------------------------------------------------------------------

class Callbacks
{
public:
    virtual ~Callbacks() {}

    virtual void flushAll() const;
    virtual bool isInstanceAvailable(const std::string& instanceId) const;
};

//---------------------------------------------------------------------------------------

bool callbacksInitialized();
Callbacks* callbacks();
void setCallbacks(Callbacks* cbs);

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING
bool isInstanceAvailable(const std::string& instanceId);
#endif //XP_TESTING

//---------------------------------------------------------------------------------------

std::string makeFullKey(const std::string& key_in);
std::vector<std::string> makeFullKeys(const std::vector<std::string>& keys_in);

}//namespace memcache
