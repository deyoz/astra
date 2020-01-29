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
 *  ����⪠ ��� libmemcahed
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
 *  ��� ���
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
 *  ��⠤���� ��ꥪ� ���
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
 *  ��ꥪ� �஢�ન ���㠫쭮�� ���
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
 *  ��ꥪ� ᨭ�஭���樨 ����㯠 � ����
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

/* api ��� ࠡ��� � ��讬, �ᯮ����騬 memcached */

/**
 * @brief ᪮�����஢��� ���� ���
 * @param in recName - ��� ��ꥪ�, �� ���஬� ᮧ������ ⥣. ���筮 - ��� ⠡����
 * @param in recId - ���� �� ��ꥪ��, �� ���஬� ᮧ������ ⥣. ���筮 - ida
 * @return ���� ⥣�
 */
std::string createTagKey(const std::string& recName, int recId);

/**
 * @brief ᪮�����஢��� ⥣ ���
 * @param in recName - ��� ��ꥪ�, �� ���஬� ᮧ������ ⥣. ���筮 - ��� ⠡����
 * @param in recId - ���� �� ��ꥪ��, �� ���஬� ᮧ������ ⥣. ���筮 - ida
 * @param in version - ����� ⥣�
 * @return ��ꥪ� ⥣� ���
 */
MCacheTag createTag(const std::string& recName, int recId, uint64_t version = 0);

/**
 * @brief ᪮�����஢��� ⥣ ���
 * @param in tagKey - ���� ⥣�
 * @param in version - ����� ⥣�
 * @return ��ꥪ� ⥣� ���
 */
MCacheTag createTag(const std::string& tagKey, uint64_t version = 0);

//---------------------------------------------------------------------------------------


/**
 * @class �����-������ �����⭮�� instance memcached
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
        �ᯮ����⥫�� �㭪樨 ���
    */

    /**
     * @brief ���४�� �� �� memcached?
     * @return true - ��, false - ���
     */
    bool isValid() const;

    /**
     * @brief ����� ���� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool flushAll() const;

    /**
     * @brief 㧭���, �������஢�� �� ��� ����� ��� �� �����
     * @param in key - ����, �� ���஬� �ᯮ����� �஢��塞� �� �����஢�� ���
     * @return true - �������஢��, false - ����
     */
    bool isCacheLocked(const std::string& key) const;

    /**
     * @brief ��⠭����� �����஢�� ��� ���� � ���
     * @param in key - ����, �� ���஬� �ᯮ����� ������㥬� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool lockCache(const std::string& key) const;

    /**
     * @brief ���� �����஢�� ��� ���� � ���
     * @param in key - ����, �� ���஬� �ᯮ����� ���, ��� ���ண� ᭨������ �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    bool unlockCache(const std::string& key) const;

    /**
     * @brief 㧭���, ������� �� ������ �� �����
     * @param in key - �஢��塞� ����
     * @return true - �������, false - ����
     */
    bool isCacheExists(const std::string& key) const;

    /**
     * @brief ��������஢��� ��� �� ⥣�
     * @param in recName - ��� ��ꥪ�, �� ���஬� �ᯮ������ ⥣. ���筮 - ��� ⠡����
     * @param in recId - ���� �� ��ꥪ��, �� ���஬� �ᯮ������ ⥣. ���筮 - ida
     * @return true - �ᯥ�, false - �訡��
     */
    bool invalidateByTag(const std::string& recName, int recId) const;

    /**
     * @brief ��������஢��� ��� �� �����
     * @param in key - ����, �� ���஬� �ᯮ����� ���������㥬� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool invalidateByKey(const std::string& key) const;

    /*
        �㭪樨 ����� ���
    */

    /**
     * @brief ������� ��ꥪ� POD-⨯� � � ��� �� �����
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - ��ꥪ�, ����� ᫥��� ࠧ������ � ���
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    template<class T> std::enable_if_t<std::is_pod<T>::value, bool>
        writeToCachePOD(const std::string& key, const T& value,
                         bool manualLock) const
    {
        return writeToCachePOD(key, value, manualLock, m_expiration);
    }

    /**
     * @brief ������� ��ꥪ� POD-⨯� � � ��� �� �����
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - ��ꥪ�, ����� ᫥��� ࠧ������ � ���
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @param in expiration - �६� ����� ���
     * @return true - �ᯥ�, false - �訡��
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
     * @brief ������� ���� ������ � ��� �� �����
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      bool manualLock) const;
    /**
     * @brief ������� ���� ������ � ��� �� �����
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @param in expiration - �६� ����� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      bool manualLock, time_t expiration) const;

    /**
     * @brief ������� ���� ������ � ��� �� ����� � ⥣��� ��� ��������樨
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in tags - ���ᨢ ⥣�� ��� ��������樨
     * @param in tagsCount - ������⢮ ⥣�� � ���ᨢ�
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      const MCacheTag* tags, unsigned tagsCount,
                      bool manualLock) const;

    /**
     * @brief ������� ���� ������ � ��� �� ����� � ⥣��� ��� ��������樨
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in tags - ���ᨢ ⥣�� ��� ��������樨
     * @param in tagsCount - ������⢮ ⥣�� � ���ᨢ�
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @param in expiration - �६� ����� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCache(const std::string& key, const char* value, size_t len,
                      const MCacheTag* tags, unsigned tagsCount,
                      bool manualLock, time_t expiration) const;

    /**
     * @brief ������� ��ꥪ� POD-⨯� � � ��� �� ����� � ⥣��� ��� ��������樨
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - ��ꥪ�, ����� ᫥��� ࠧ������ � ���
     * @param in tags - ���ᨢ ⥣�� ��� ��������樨
     * @param in tagsCount - ������⢮ ⥣�� � ���ᨢ�
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    // ⮫쪮 ��� POD-⨯��!
    template<class T> typename std::enable_if<std::is_pod<T>::value, bool>::type
        writeToCachePOD(const std::string& key, const T& value,
                         const MCacheTag* tags, unsigned tagsCount,
                         bool manualLock) const
    {
        return writeToCachePOD(key, value, tags,  tagsCount, manualLock, m_expiration);
    }

    /**
     * @brief ������� ��ꥪ� POD-⨯� � � ��� �� ����� � ⥣��� ��� ��������樨
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - ��ꥪ�, ����� ᫥��� ࠧ������ � ���
     * @param in tags - ���ᨢ ⥣�� ��� ��������樨
     * @param in tagsCount - ������⢮ ⥣�� � ���ᨢ�
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @param in expiration - �६� ����� ���
     * @return true - �ᯥ�, false - �訡��
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
     * @brief ������� ���� ������ � ��� �� ����� ��� ���ᨨ, ��� ⥣�� � ��� �����஢��
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCacheRaw(const std::string& key, const char* value, size_t len) const;

    /**
     * @brief ������� ���� ������ � ��� �� ����� ��� ���ᨨ, ��� ⥣�� � ��� �����஢��
     * @param in key - ����, �� ���஬� ᫥��� ࠧ������ ��ꥪ� � ���
     * @param in value - 㪠��⥫� �� ���� ������, ࠧ��頥��� � ���
     * @param in len - ࠧ��� ����� ������
     * @param in manualLock - 䫠� ��筮� �����஢��
     * @param in expiration - �६� ����� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool writeToCacheRaw(const std::string& key,
                         const char* value, size_t len,
                         time_t expiration) const;


    /*
        �㭪樨 �⥭�� ���
    */

    /**
     * @brief ������ ���㠫�� ���� ������ �� ���
     * @param out value - 㪠��⥫� �� ���� ������, ���⠭�� �� ���. !!need free!!
     * @param out len - ࠧ��� ����� ������, ���⠭���� �� ���
     * @param in key - ����, �� ���஬� ������ ����� �� ���
     * @return true - �ᯥ�, false - �訡��
     */
    // ��뢠��� ����ணࠬ�� ������ �맢��� free() ��� value
    bool readFromCache(char*& value, size_t& len, const std::string& key) const;

    /**
     * @brief ������ ���㠫�� ����� �� ���
     * @param out value - १������騩 �����
     * @param in key - ����, �� ���஬� ������ ����� �� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool readFromCache(std::vector<char>& value, const std::string& key) const;

    /**
     * @brief ������ ���㠫�� ��ꥪ� POD-⨯� � �� ���
     * @param out value - ���⠭�� ��ꥪ� POD-⨯� �
     * @param in key - ����, �� ���஬� ������ ����� �� ���
     * @return true - �ᯥ�, false - �訡��
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
     * @brief ������ ����� � ���ᨨ ⥣�� ��� � ���箬 key
     * @param out tagsVersions
     * @param in key
     */
    void readCacheTagsVersions(std::map<std::string, uint64_t>& tagsVersions,
                               const std::string& key) const;

    /**
     * @brief ������ ���� �����-� ������ �� ���
     * @param out value - 㪠��⥫� �� ���� ������, ���⠭�� �� ���. !!need free!!
     * @param out len - ࠧ��� ����� ������, ���⠭���� �� ���
     * @param in key - ����, �� ���஬� ������ ����� �� ���
     * @return true - �ᯥ�, false - �訡��
     */
    // ��뢠��� ����ணࠬ�� ������ �맢��� free() ��� value
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
     * @brief �����⮢��� ����� ��� multi-read
     * @param in keys - ���� ��� multi-read
     * @return true - �ᯥ�, false - �訡��
     */
    bool multiReadFromCache(const std::vector<std::string>& keys) const;

    /**
     * @brief ����� ᫥���騩 ���� �����-� ������ �� ���(��᫥ multiReadFromCacheRaw)
     * @param out value - 㪠��⥫� �� ���� ������, ���⠭�� �� ���. !!need free!!
     * @param out len - ࠧ��� ����� ������, ���⠭���� �� ���
     * @param out key - ����, �� ���஬� ��⠫��� ����� �� ���
     */
    bool fetchNextRaw(char*& value, size_t& len, std::string& key) const;

    /**
     * @brief ������ ���� �����-� ������ �� ���
     * @param in key - ����, �� ���஬� ������ ����� �� ���
     * @param out value - 㪠��⥫� �� ���� ������, ���⠭���� �� ���
     * @return true - �ᯥ�, false - �訡��
     */
    bool readRaw(const std::string& key, std::string& value) const;

    /**
     * @brief ������� id instance memcached
     * @return id instance memcached
     */
    const std::string& id() const { return m_id; }

    /**
     * @brief ������� ����� ���⠭ᮢ, �易���� � �����
     * @return ����� ���⠭ᮢ (�������� ���⮩)
     */
    std::vector<Instance*> relatedInstances() const;

    /**
     * @brief ���� �� ���⠭� related
     * @return true - ����, false - ���
     */
    bool isRelated() const { return m_isRelated; }

    const std::string& host() const;
    unsigned port() const;

    bool testWriteRead() const;

protected:
    /*
     * @brief ��������� related ���⠭�
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
 * @class ����� ��� �ࠢ����� ��᪮�쪨�� instance memcached
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
