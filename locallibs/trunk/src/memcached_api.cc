#ifdef HAVE_MEMCACHED
#include <libmemcached/memcached.h>
#endif // WITHOUT_MEMCACHED

#include "memcached_api.h"
#include "testmode.h"
#include "exception.h"
#include "str_utils.h"
#include "tcl_utils.h"
#include "algo.h"

#include <boost/random.hpp>
#define NICKNAME "ANTON"
#include "slogger.h"

#include <set>

namespace memcache
{

size_t MemcachedMaxKey()
{
#ifdef HAVE_MEMCACHED
    return MEMCACHED_MAX_KEY;
#else
    return 0;
#endif // HAVE_MEMCACHED
}

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING

static std::set<std::string> UsedKeys = {};

std::string makePidSuffix()
{
#ifdef HAVE_MEMCACHED
    std::ostringstream pid_stream;
    pid_stream << "_pid_" << getpid();
    return pid_stream.str();
#else
    return "";
#endif
}

std::string makePidKey(const std::string& key_in)
{
#ifdef HAVE_MEMCACHED
    const std::string pidSuffix = makePidSuffix();
    if(key_in.find(pidSuffix) != std::string::npos) {
        // ключ уже содержит привязку к pid
        return key_in;
    } else {
        // добавляем к ключу привязку к pid
        std::ostringstream s;
        s << key_in << pidSuffix;
        std::string key = s.str();
        ASSERT(key.length() <= MEMCACHED_MAX_KEY);
        UsedKeys.insert(key);
        return key;
    }
#else
    return key_in;
#endif// HAVE_MEMCACHED
}

std::vector<std::string> makePidKeys(const std::vector<std::string>& keys_in)
{
#ifdef HAVE_MEMCACHED
    return algo::transform(keys_in,
                           [](const std::string& key_in) { return makePidKey(key_in); });
#else
    return keys_in;
#endif
}
#endif //XP_TESTING

//---------------------------------------------------------------------------------------

#ifdef HAVE_MEMCACHED

namespace
{

// Базовый класс генератора псевдослучайных чисел
template<typename EngineT, typename IntValueT>
class RandomGenerator
{
    // typedef для равномерного распределения целых значений с типом ValueT
    typedef boost::uniform_int<IntValueT> UniformDistribution_t;

public:
    RandomGenerator(const IntValueT& a, const IntValueT& b)
        : m_gen(EngineT(static_cast<IntValueT>(time(0))),
                UniformDistribution_t(a, b))
    {}

public:
    IntValueT operator()() { return m_gen(); }

private:
    boost::variate_generator<EngineT, UniformDistribution_t> m_gen;
};

//---------------------------------------------------------------------------------------

// Генератор псевдослучайных чисел Вихрь Мерсенна
template<typename IntValueT>
class MersenneTwisterRandomGenerator: public RandomGenerator<boost::mt19937, IntValueT>
{
public:
    MersenneTwisterRandomGenerator(const IntValueT& a, const IntValueT& b)
        : RandomGenerator<boost::mt19937, IntValueT>(a, b)
    {}
};

//---------------------------------------------------------------------------------------

bool checkKey(const std::string& key_in)
{
    if(key_in.empty() || key_in.length() > MEMCACHED_MAX_KEY) {
        return false;
    }

    if(algo::contains(key_in, ' ')) {
        LogError(STDLOG) << "Memcached key '" << key_in << "' contains space character!";
        return false;
    }

    return true;
}


//---------------------------------------------------------------------------------------

static const size_t MaxUserBuffSize_ = 1024*1024 - sizeof(MCacheMetaData) - MEMCACHED_MAX_KEY;

const size_t MaxBuffSize = MaxUserBuffSize_ + sizeof(MCacheMetaData);

char* getBuff()
{
    static char Buff[MaxBuffSize];
    return Buff;
}

uint64_t makeUint64(uint32_t minor, uint32_t major)
{
    uint64_t ui64 = 0;
    ui64 |= major;
    ui64 <<= 32;
    ui64 |= minor;
    LogTrace(TRACE3) << "minor ui32[" << minor << "]; "
                     << "major ui32[" << major << "]; ui64[" << ui64 << "]";
    return ui64;
}

uint64_t createVersion()
{
    static MersenneTwisterRandomGenerator<uint32_t>* rg = 0;
    if(!rg) {
        rg = new MersenneTwisterRandomGenerator<uint32_t>(1, 4294967295u);
    }
    return makeUint64(static_cast<uint32_t>(time(0)), (*rg)());
}

// парсит строку вида: "127.0.0.1:11211;127.0.0.1:22122;10.1.9.128:11200"
// а также строку вида "127.0.0.1;127.0.0.1;10.1.9.128:11200"
HostPortPairs_t parseHostPortPairs(const std::string& hostPortPairsStr, unsigned defaultPort)
{
    HostPortPairs_t result;
    // сплитим по точке с запятой
    std::vector<std::string> splittedBySemicolon;
    StrUtils::split_string(splittedBySemicolon, StrUtils::trim(hostPortPairsStr), ';');
    if(splittedBySemicolon.empty()) {
        tst();
        return HostPortPairs_t();
    }
    // сплитим по двоеточию
    for(const std::string& s: splittedBySemicolon) {
        std::vector<std::string> splittedByColon;
        StrUtils::split_string(splittedByColon, StrUtils::trim(s), ':');
        if(splittedByColon.size() == 2) {
            const std::string host = splittedByColon[0];
            const unsigned port = std::stoi(splittedByColon[1]);
            LogTrace(TRACE1) << host << ":" << port;
            result.emplace_back(host, port);
        } else if(splittedByColon.size() == 1) {
            LogTrace(TRACE1) << s << ":" << defaultPort;
            result.emplace_back(s, defaultPort);
        } else {
            LogError(STDLOG) << "invalid host/port: " << s;
        }
    }

    return result;
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

size_t MaxUserBuffSize()
{
    return MaxUserBuffSize_;
}

//---------------------------------------------------------------------------------------

bool MCache::init(const std::string& host, unsigned port)
{
    m_host = host;
    m_port = port;

    LogTrace(TRACE0) << "add memcached server " << host << ":" << port;
    if(!addServer(host, port))
    {
        LogTrace(TRACE0) << "add memcached server "
                         << host << ":" << port << " failed!";
        return false;
    }

    LogTrace(TRACE3) << "MCache init successfull";

    if(getVariableStaticBool("USE_MEMCACHE_BINARY_PROTOCOL", NULL, 1) == 0)
    {
        LogTrace(TRACE0) << "USE_MEMCACHE_BINARY_PROTOCOL is unset";
    }
    else
    {
        LogTrace(TRACE0) << "USE_MEMCACHE_BINARY_PROTOCOL is set. Enable it...";
        if(memcached_behavior_set(m_memc,
                                  MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,
                                  1) == MEMCACHED_SUCCESS)
        {
            LogTrace(TRACE0) << "Binary protocol is enabled";
        }
        else
        {
            LogWarning(STDLOG) << "Unable to activate binary protocol";
        }
    }

    return true;
}

MCache::MCache(const std::string& hostSetting, const std::string& portSetting,
               const std::string& relatedInstancesSetting,
               const std::string& expirationSettings)
    : m_memc(memcached_create(NULL)), m_port(0), m_valid(false)
{
    if(getVariableStaticBool("USE_MEMCACHE", NULL, 1) == 0)
    {
        LogTrace(TRACE0) << "USE_MEMCACHE is unset";
        return;
    }

    const std::string InvalidHost = "";
    const unsigned InvalidPort = 0;
    std::string memcdHost = readStringFromTcl(hostSetting, InvalidHost);
    LogTrace(TRACE0) << "memcached host[" << hostSetting << "] is " << memcdHost;
    unsigned memcdPort = readIntFromTcl(portSetting, InvalidPort);
    LogTrace(TRACE0) << "memcached port[" << portSetting << "] is " << memcdPort;
    m_expiration = readIntFromTcl(expirationSettings, 0);
    LogTrace(TRACE0) << "expiration time[" << expirationSettings << "] "
                     << "is " << m_expiration;

    if(memcdHost == InvalidHost || memcdPort == InvalidPort)
    {
        LogTrace(TRACE0) << "Invalid memcache settings [" << memcdHost << ":" << memcdPort << "]";
#ifdef XP_TESTING
        if(inTestMode() and not (getenv("XP_NO_MEMCACHED") and strcmp(getenv("XP_NO_MEMCACHED"),"1")==0))
        {
            const auto memcdHost = readStringFromTcl("XP_TESTS_MEMCACHED_HOST","");
            if(not memcdHost.empty())
            {
                LogTrace(TRACE0) << __func__ << " : called for " << hostSetting << " but using " << memcdHost;
                m_valid = init(memcdHost,0);
                return;
            }
        }
#endif
        return;
    }

    std::string relatedInstancesStr = readStringFromTcl(relatedInstancesSetting, "");
    LogTrace(TRACE0) << "memcached related instances: " << relatedInstancesStr;
    m_relatedHostsAndPorts = parseHostPortPairs(relatedInstancesStr, memcdPort);

    m_valid = init(memcdHost, memcdPort);
}

MCache::MCache(const std::string& host, unsigned port)
    : m_memc(memcached_create(NULL)), m_port(0), m_valid(false)
{
    if(getVariableStaticBool("USE_MEMCACHE", NULL, 1) == 0)
    {
        LogTrace(TRACE0) << "USE_MEMCACHE is unset";
        return;
    }

    m_valid = init(host, port);
}

MCache::~MCache()
{
    memcached_free(m_memc);
}

bool MCache::addServer(const std::string& hostname, in_port_t port)
{
    memcached_return_t ret = port ? memcached_server_add(m_memc, hostname.c_str(), port)
                                  : memcached_server_add_unix_socket(m_memc, hostname.c_str());
    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::get(std::vector<char>& res, const std::string& key_in)
{
    memcached_return_t ret;
    uint32_t flags = 0;
    size_t len = 0;

    if(!checkKey(key_in)) {
        return false;
    }

    const std::string key = makeFullKey(key_in);

    char *value = memcached_get(m_memc, key.c_str(), key.length(),
                                &len, &flags, &ret);
    if(value)
    {
        res.reserve(len);
        res.assign(value, value + len);
        free(value);
        return true;
    }
    return false;
}

bool MCache::get(std::string& res, const std::string& key)
{
    std::vector<char> vec;
    bool ret = get(vec, key);
    res = std::string(vec.begin(), vec.end());
    return ret;
}

bool MCache::get(const std::string& key)
{
    std::vector<char> vec;
    return get(vec, key);
}

bool MCache::get(char*& value, size_t& len, const std::string& key_in)
{
    memcached_return_t ret;
    uint32_t flags = 0;

    if(!checkKey(key_in)) {
        return false;
    }

    const std::string key = makeFullKey(key_in);

    value = memcached_get(m_memc, key.c_str(), key.length(),
                          &len, &flags, &ret);
    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::mget(const std::vector<std::string>& keys_in)
{
    auto keys = makeFullKeys(keys_in);

    std::vector<const char*> realKeys;
    std::vector<size_t> realKeyLengths;

    realKeys.reserve(keys.size());
    realKeyLengths.reserve(keys.size());

    for(auto it = keys.begin(); it != keys.end(); ++it)
    {
        realKeys.push_back((*it).c_str());
        realKeyLengths.push_back((*it).length());
    }

    if(!realKeys.empty())
    {
        memcached_return_t ret = memcached_mget(m_memc,
                                                &realKeys[0], &realKeyLengths[0],
                                                realKeys.size());
        return (ret == MEMCACHED_SUCCESS);
    }
    return false;
}

bool MCache::fetch(std::vector<char>& res, std::string& key)
{
    char retKey[MEMCACHED_MAX_KEY] = {};
    size_t resLen = 0;
    size_t keyLen = 0;
    memcached_return_t ret;
    uint32_t flags = 0;

    char *value = memcached_fetch(m_memc, retKey, &keyLen,
                                  &resLen, &flags, &ret);
    if(value)
    {
        res.reserve(resLen);
        res.assign(value, value + resLen);
        key.assign(retKey, keyLen);
        free(value);
    }
    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::fetch(char*& value, size_t& len, std::string& key)
{
    char retKey[MEMCACHED_MAX_KEY] = {};
    size_t keyLen = 0;
    memcached_return_t ret;
    uint32_t flags = 0;

    value = memcached_fetch(m_memc, retKey, &keyLen,
                            &len, &flags, &ret);
    if(value) {
        key.assign(retKey, keyLen);
    }
    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::set(const std::string& key_in, const std::vector<char>& value,
                 time_t expiration, uint32_t flags)
{
    if(!checkKey(key_in)) {
        return false;
    }

    if(value.empty()) {
        tst();
        return false;
    }

    const std::string key = makeFullKey(key_in);

    memcached_return_t ret = memcached_set(m_memc,
                                           key.c_str(), key.length(),
                                           &value[0], value.size(),
                                           expiration, flags);

    if (ret != MEMCACHED_SUCCESS)
    {
        LogTrace(TRACE0) << "memcached_set error = " << memcached_strerror(m_memc, ret);
    }

    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::set(const std::string& key, const std::string& value,
                 time_t expiration, uint32_t flags)
{
    std::vector<char> val(value.begin(), value.end());
    return set(key, val, expiration, flags);
}

bool MCache::set(const std::string& key_in, const char* value, size_t len,
                 time_t expiration, uint32_t flags)
{
    if(!checkKey(key_in)) {
        return false;
    }

    const std::string key = makeFullKey(key_in);

    memcached_return_t ret = memcached_set(m_memc,
                                           key.c_str(), key.length(),
                                           value, len,
                                           expiration, flags);

    if (ret != MEMCACHED_SUCCESS)
    {
        LogTrace(TRACE0) << "memcached_set error = " << memcached_strerror(m_memc, ret);
    }

    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::add(const std::string& key_in, const std::vector<char>& value,
                 time_t expiration, uint32_t flags)
{
    if(!checkKey(key_in)) {
        return false;
    }

    if(value.empty()) {
        tst();
        return false;
    }


    const std::string key = makeFullKey(key_in);

    memcached_return_t ret = memcached_add(m_memc,
                                           key.c_str(), key.length(),
                                           &value[0], value.size(),
                                           expiration, flags);

    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::add(const std::string& key, const std::string& value,
                 time_t expiration, uint32_t flags)
{
    std::vector<char> val(value.begin(), value.end());
    return add(key, val, expiration, flags);
}

bool MCache::del(const std::string& key_in)
{
    if(!checkKey(key_in)) {
        return false;
    }

    const std::string key = makeFullKey(key_in);

    memcached_return_t ret = memcached_delete(m_memc,
                                              key.c_str(), key.length(),
                                              0);
    return (ret == MEMCACHED_SUCCESS || ret == MEMCACHED_NOTFOUND);
}

bool MCache::trueFlush(time_t expiration)
{
    memcached_return_t ret = memcached_flush(m_memc, expiration);
    return (ret == MEMCACHED_SUCCESS);
}

bool MCache::flush(time_t expiration)
{
#ifdef XP_TESTING
    if(!inTestMode()) {
        return trueFlush(expiration);
    } else {
        for(const auto& key: UsedKeys) {
            del(key);
        }
        UsedKeys.clear();
        return true;
    }
#else
    return trueFlush(expiration);
#endif// XP_TESTING
}

#else // HAVE_MEMCACHED

uint64_t createVersion()
{
    return 0;
}

size_t MaxUserBuffSize()
{
    return 0;
}

MCache::MCache(const std::string& hostSetting, const std::string& portSetting,
               const std::string& relatedInstancesSetting,
               const std::string& expirationSettings)
    : m_valid(false)
{
    LogTrace(TRACE0) << "trying to use memcached without libmemcached: "
                     << hostSetting << ':' << portSetting;
}

MCache::MCache(const std::string& host, unsigned port)
    : m_valid(false)
{
    LogTrace(TRACE0) << "trying to use memcached without libmemcached: "
                     << host << ':' << port;
}

MCache::~MCache()
{
}

bool MCache::addServer(const std::string& hostname, in_port_t port)
{
    return false;
}

bool MCache::get(std::vector<char>& res, const std::string& key)
{
    return false;
}

bool MCache::get(std::string& res, const std::string& key)
{
    return false;
}

bool MCache::get(const std::string& key)
{
    return false;
}

bool MCache::get(char*& value, size_t& len, const std::string& key)
{
    return false;
}

bool MCache::mget(const std::vector<std::string>& keys)
{
    return false;
}

bool MCache::fetch(std::vector<char>& res, std::string& key)
{
    return false;
}

bool MCache::fetch(char*& value, size_t& len, std::string& key)
{
    return false;
}

bool MCache::set(const std::string& key, const std::vector<char >& value,
                 time_t expiration, uint32_t flags)
{
    return false;
}

bool MCache::set(const std::string& key, const std::string& value,
                 time_t expiration, uint32_t flags)
{
    return false;
}

bool MCache::set(const std::string& key, const char* value, size_t len,
                 time_t expiration, uint32_t flags)
{
    return false;
}

bool MCache::add(const std::string& key, const std::vector<char>& value,
                 time_t expiration, uint32_t flags)
{
    return false;
}

bool MCache::add(const std::string& key, const std::string& value,
                 time_t expiration, uint32_t flags)
{
    return false;
}

bool MCache::del(const std::string& key)
{
    return false;
}

bool MCache::flush(time_t expiration)
{
    return false;
}

#endif // HAVE_MEMCACHED

//---------------------------------------------------------------------------------------

void MCacheTag::setKey(const std::string& k)
{
    if(k.length() > MaxTagKeyLen)
    {
        LogError(STDLOG) << "attempt to use invalid tag: " << k;
        key[0] = '\0';
    }
    strcpy(key, k.c_str());
}

std::string MCacheTag::getKey() const
{
    return makeFullKey(std::string(key, strlen(key)));
}

bool MCacheTag::isValid() const
{
    size_t keyLen = strlen(key);
    return (keyLen > 0 && keyLen <= MaxTagKeyLen);
}

//---------------------------------------------------------------------------------------

bool MCacheMetaData::addTag(const MCacheTag& tag)
{
    if((m_tagsCount <= MaxTagsCount - 1))
    {
        if(!tag.isValid()) {
            return false;
        }

        m_tags[m_tagsCount++] = tag;
        return true;
    }

    LogError(STDLOG) << "too many tags: " << m_tagsCount + 1;
    return false;
}

std::vector<std::string> MCacheMetaData::tagsKeys() const
{
    std::vector<std::string> res;
    for(unsigned i = 0; i < m_tagsCount; ++i) {
        res.push_back(m_tags[i].getKey());
    }
    return res;
}

uint64_t MCacheMetaData::getTagVersion(const std::string& tagKey) const
{
    for(unsigned i = 0; i < m_tagsCount; ++i)
    {
        if(m_tags[i].getKey() == tagKey) {
            return m_tags[i].getVersion();
        }
    }
    return 0;
}

//---------------------------------------------------------------------------------------

MCacheLock::MCacheLock(const std::string& key, MCache* mcache, bool unlockOnDestroy)
    : m_key(key), m_mcache(mcache), m_locked(false), m_unlockOnDestroy(unlockOnDestroy)
{
}

MCacheLock::~MCacheLock()
{
    if(m_locked && m_unlockOnDestroy) {
        unlock();
    }
}

bool MCacheLock::lock()
{
    m_locked = m_mcache->add(lockKey(), lockVal(), lockExpiration());
    return m_locked;
}

bool MCacheLock::unlock()
{
    m_locked = false;
    return m_mcache->del(lockKey());
}

bool MCacheLock::isLocked() const
{
    return m_mcache->get(lockKey());
}

std::string MCacheLock::lockKey() const
{
    return m_key + "_cache_lock";
}

std::string MCacheLock::lockVal() const
{
    return "lock";
}

time_t MCacheLock::lockExpiration() const
{
    return (time_t)10;  // 10 секунд
}

//---------------------------------------------------------------------------------------

bool MCacheCheck::check(const std::string& key,
                        const MCacheMetaData& metaData,
                        MCache* mcache)
{
#ifdef HAVE_MEMCACHED
    if(!checkKey(key))
    {
        LogTrace(TRACE1) << "check fail for key " << key;
        return false;
    }

    if(!metaData.m_tagsCount) {
        return true;
    }

    std::vector<std::string> tagsKeys = metaData.tagsKeys();

    if(!mcache->mget(tagsKeys))
    {
        LogTrace(TRACE0) << "multiget failed";
        return false;
    }

    std::string tagKey;
    uint64_t tagVersion;
    bool ret = true;
    unsigned tagsCount = 0;
    while(mcache->fetch(tagVersion, tagKey))
    {
        // break/return делать нельзя, т.к. надо сделать полный fetch
        uint64_t tagVersionFromMetadata = metaData.getTagVersion(tagKey);
        if(tagVersionFromMetadata != tagVersion)
        {
            LogTrace(TRACE1) << "tags versions are not equal (" << tagVersion << ","
                             << tagVersionFromMetadata << ") for key: " << tagKey;
            ret = false;
        }
        tagsCount++;
    }

    if(tagsCount != metaData.m_tagsCount)
    {
        LogTrace(TRACE1) << "tags counts are not equal (" << tagsCount << ","
                         << metaData.m_tagsCount << ") for key: " << tagKey;
        return false;
    }

    return ret;
#else
    return false;
#endif // HAVE_MEMCACHED
}

uint64_t MCacheCheck::setTagVersion(MCacheTag& tag, MCache* mcache, time_t expiration)
{
#ifdef HAVE_MEMCACHED
    uint64_t version = tag.getVersion();
    if(!version) {
        version = getTagVersion(tag, mcache);
    }
    if(!version)
    {
        tag.setVersion(createVersion());
        LogTrace(TRACE5) << "set tag key: " << tag.getKey()
                         << " with version: " << tag.getVersion();
        if(!mcache->set(tag.getKey(), tag.getVersion(), expiration))
        {
            LogTrace(TRACE0) << "unable to write cache tag '" << tag.getKey() << "'";
            return 0;
        }
    }
    else
    {
        tag.setVersion(version);
    }
    return tag.getVersion();
#else
    return 0;
#endif // HAVE_MEMCACHED
}

uint64_t MCacheCheck::getTagVersion(const MCacheTag& tag, MCache* mcache)
{
#ifdef HAVE_MEMCACHED
    uint64_t version = 0;
    if(!mcache->get(version, tag.getKey())) {
        return 0;
    }
    return version;
#else
    return 0;
#endif // HAVE_MEMCACHED
}

//---------------------------------------------------------------------------------------

MCacheObject::MCacheObject(const std::string& key, MCache* mcache)
    : m_key(key), m_data(0), m_dataLen(0),
      m_mcache(mcache), m_isActual(false), m_isValid(true)
{
    memset(&m_metaData, 0, sizeof(m_metaData));
}

MCacheObject::MCacheObject(const std::string& key,
                           const char* data,
                           size_t len,
                           MCache* mcache,
                           const MCacheTag* tags,
                           unsigned tagsCount)
    : m_key(key), m_data(data), m_dataLen(len),
      m_mcache(mcache), m_isActual(false), m_isValid(true)
{
    memset(&m_metaData, 0, sizeof(m_metaData));
    for(unsigned i = 0; i < tagsCount; ++i)
    {
        if(!m_metaData.addTag(tags[i]))
        {
            m_isValid = false;
            break;
        }
    }
}

const char* MCacheObject::rawData(size_t& len) const
{
#ifdef HAVE_MEMCACHED
    size_t metaDataLen = sizeof(m_metaData);
    const char* metaData = (const char*)&m_metaData;
    if((!metaData) || (metaDataLen > MaxBuffSize))
    {
        LogWarning(STDLOG) << "Invalid meta data of cache with key '" << key() << "'";
        return 0;
    }
    char* buff = getBuff();
    if(!buff)
    {
        LogWarning(STDLOG) << "Invalid buffer";
        return 0;
    }
    memcpy(buff, metaData, metaDataLen);

    if((!m_data) || (metaDataLen + m_dataLen > MaxBuffSize))
    {
        LogWarning(STDLOG) << "Invalid data of cache with key '" << key() << "'";
        return 0;
    }
    
    memcpy(buff + metaDataLen, m_data, m_dataLen);
    len = m_dataLen + metaDataLen;
    return buff;
#else
    return 0;
#endif // HAVE_MEMCACHED
}

bool MCacheObject::writeToCache(time_t expiration, bool manualLock)
{
#ifdef HAVE_MEMCACHED
    if(dataLen() > MaxUserBuffSize())
    {
        LogWarning(STDLOG) << "To long data to write " << dataLen() << ", maximum " 
                           << MaxUserBuffSize() << " bytes. Key: '" << key() << "'";    
        return false;
    }
    
    LogTrace(TRACE5) << "manual lock: " << std::boolalpha << manualLock;
    if(!isValid())
    {
        LogError(STDLOG) << "attempt to write invalid cache object";
        return false;
    }

    if(!manualLock)
    {
        MCacheLock lock(key(), m_mcache);
        if(lock.lock())
        {
            bool ret = write_(expiration);
            lock.unlock();
            return ret;
        }
        return false;
    }
    else
        return write_(expiration);
#else
    return false;
#endif // HAVE_MEMCACHED
}

bool MCacheObject::checkTagsVersions() const
{
    for(unsigned i = 0; i < metaData().m_tagsCount; ++i)
    {
        const MCacheTag& tag = metaData().m_tags[i];

        if(tag.getVersion())
        {
            uint64_t oldVersion = tag.getVersion();
            LogTrace(TRACE5) << "old version for tag '" << tag.getKey()
                             << "' is " << oldVersion;
            uint64_t curVersion = MCacheCheck::getTagVersion(tag, m_mcache);
            LogTrace(TRACE5) << "cur version for tag '" << tag.getKey()
                             << "' is " << curVersion;

            if(curVersion != oldVersion)
            {
                LogTrace(TRACE0) << "Tag version already has been modified!";
                return false;
            }
        }
    }

    return true;
}

bool MCacheObject::write_(time_t expiration)
{
    if(!checkTagsVersions()) {
        return false;
    }

    for(unsigned i = 0; i < metaData().m_tagsCount; ++i) {
        MCacheCheck::setTagVersion(metaData().m_tags[i], m_mcache, expiration);
    }

    size_t len = 0;
    const char* data = rawData(len);
    if(!data || !len)
    {
        LogWarning(STDLOG) << "No data to write for cache with key '" << key() << "'";
        return false;
    }

    LogTrace(TRACE5) << "write to memcached data with len=" << len;
    return m_mcache->set(key(), data, len, expiration);
}

bool MCacheObject::readFromCache()
{
#ifdef HAVE_MEMCACHED
    size_t len = 0;
    char* data = 0;
    if(!m_mcache->get(data, len, key())) {
        tst();
        return false;
    }

    if(!data || !len) {
        tst();
        return false;
    }

    if(len > MaxBuffSize || len < sizeof(MCacheMetaData))
    {
        LogWarning(STDLOG) << "Invalid data has been read for key '" << key() << "'";
        return false;
    }

    char* buff = getBuff();
    if(!buff)
    {
        LogWarning(STDLOG) << "Invalid buffer";
        return false;
    }

    memcpy(buff, data, len);
    free(data);

    m_metaData = *(MCacheMetaData*)buff;
    m_data = buff + sizeof(m_metaData);
    m_dataLen = len - sizeof(m_metaData);
    m_isActual = MCacheCheck::check(key(), m_metaData, m_mcache);

    return true;
#else
    return false;
#endif // HAVE_MEMCACHED
}

//---------------------------------------------------------------------------------------

std::string createTagKey(const std::string& recName, int recId)
{
    std::stringstream tagKey;
    tagKey << "tag_" << recName << "_" << recId;
    return tagKey.str();
}

MCacheTag createTag(const std::string& recName, int recId, uint64_t version)
{
    MCacheTag tag = {};
    tag.setKey(createTagKey(recName, recId));
    tag.setVersion(version);
    return tag;
}

MCacheTag createTag(const std::string& tagKey, uint64_t version)
{
    MCacheTag tag = {};
    tag.setKey(tagKey);
    tag.setVersion(version);
    return tag;
}

//---------------------------------------------------------------------------------------

Instance::Instance(const std::string& id,
                   const std::string& hostSetting,
                   const std::string& portSetting,
                   const std::string& relatedInstancesSetting,
                   const std::string& expirationSettings)
    : m_id(id), m_isRelated(false)
{
    m_mcache = new MCache(hostSetting, portSetting, relatedInstancesSetting, expirationSettings);
    m_expiration = m_mcache->getExpirationTime();

    for(const HostPortPair& relatedHostAndPort: m_mcache->relatedHostsAndPorts()) {
        InstanceManager::singletone().addRelated(id, new Instance(id,
                                                                  relatedHostAndPort.m_host,
                                                                  relatedHostAndPort.m_port,
                                                                  m_expiration));
    }
}

Instance::Instance(const std::string& id,
                   const std::string& host,
                   unsigned port,
                   time_t expiration)
    : m_id(id), m_isRelated(true), m_expiration(expiration)
{
    m_mcache = new MCache(host, port);
}

Instance::~Instance()
{
    delete m_mcache;
}

bool Instance::isValid() const
{
    return m_mcache->isValid();
}

const std::string& Instance::host() const
{
    return m_mcache->host();
}

unsigned Instance::port() const
{
    return m_mcache->port();
}

bool Instance::flushAll() const
{
    return m_mcache->flush(0);
}

bool Instance::isCacheLocked(const std::string& key) const
{
    return MCacheLock(key, m_mcache, false).isLocked();
}

bool Instance::lockCache(const std::string& key) const
{
    return MCacheLock(key, m_mcache, false).lock();
}

bool Instance::unlockCache(const std::string& key) const
{
    return MCacheLock(key, m_mcache, false).unlock();
}

bool Instance::isCacheExists(const std::string& key) const
{
    return m_mcache->get(key);
}

bool Instance::invalidateByTag(const std::string& recName, int recId) const
{
#ifdef HAVE_MEMCACHED
    LogTrace(TRACE1) << "invalidate by tag '" << recName << "["
                     << recId << "] at instance " << id();
    MCacheTag tag = createTag(recName, recId);
    uint64_t tagVersion = MCacheCheck::getTagVersion(tag, m_mcache);
    int i = 0;
    do {
        if(i++ > 10)
        {
            LogError(STDLOG) << "unable to create new tag version!";
            return false;
        }
        tag.setVersion(createVersion());
    } while(tagVersion == tag.getVersion());

    bool res = m_mcache->set(tag.getKey(), tag.getVersion());
    if(!res) {
        LogError(STDLOG) << "Failed to invalidate cache by tag ["
                         << recName << "/" << recId << "] at instance "
                         << (isRelated() ? "related to " : "")
                         << "'" << id() << "' "
                         << "[" << host() << ":" << port() << "]!";
    }
    if(!isRelated()) {
        tst();
        res &= InstanceManager::singletone().invalidateRelatedInstancesByTag(id(),
                                                                             recName,
                                                                             recId);
    }
    return res;
#else
    return false;
#endif // HAVE_MEMCACHED
}

bool Instance::invalidateByKey(const std::string& key) const
{
#ifdef HAVE_MEMCACHED
    LogTrace(TRACE1) << "invalidate by key '" << key << "' at instance " << id();
    bool res = m_mcache->del(key);
    if(!res) {
        LogError(STDLOG) << "Failed to invalidate cache by key [" << key << "] "
                         << "at instance "
                         << (isRelated() ? "related to " : "") << "'" << id() << "' "
                         << "[" << host() << ":" << port() << "]!";
    }
    if(!isRelated()) {
        tst();
        res &= InstanceManager::singletone().invalidateRelatedInstancesByKey(id(), key);
    }
    return res;
#else
    return false;
#endif // HAVE_MEMCACHED
}

bool Instance::writeToCache(const std::string& key, const char* value, size_t len,
                            bool manualLock) const
{
    return writeToCache(key, value, len, manualLock, m_expiration);
}

bool Instance::writeToCache(const std::string& key, const char* value, size_t len,
                            bool manualLock, time_t expiration) const
{
    MCacheObject cacheObject(key, value, len, m_mcache);
    return cacheObject.writeToCache(expiration, manualLock);
}

bool Instance::writeToCache(const std::string& key, const char* value, size_t len,
                            const MCacheTag* tags, unsigned tagsCount,
                            bool manualLock) const
{
    return writeToCache(key, value, len, tags, tagsCount, manualLock, m_expiration);
}

bool Instance::writeToCache(const std::string& key, const char* value, size_t len,
                            const MCacheTag* tags, unsigned tagsCount,
                            bool manualLock, time_t expiration) const
{
    MCacheObject cacheObject(key, value, len, m_mcache, tags, tagsCount);
    return cacheObject.writeToCache(expiration, manualLock);
}

bool Instance::writeToCacheRaw(const std::string& key,
                               const char* value, size_t len) const
{
    return writeToCacheRaw(key, value, len, m_expiration);
}

bool Instance::writeToCacheRaw(const std::string& key,
                               const char* value, size_t len,
                               time_t expiration) const
{
    return m_mcache->set(key, value, len, expiration);
}

bool Instance::readRaw(const std::string& key, std::string& value) const
{
    return m_mcache->get(value, key);
}

std::vector<Instance*> Instance::relatedInstances() const
{
    return InstanceManager::singletone().getRelatedInstances(m_id);
}

bool Instance::readFromCache(char*& value, size_t& len, const std::string& key) const
{
    MCacheObject cacheObject(key, m_mcache);
    if(!cacheObject.readFromCache()) {
        return false;
    }
    if(!cacheObject.isActual()) {
        return false;
    }
    len = cacheObject.dataLen();
    value = (char*)malloc(len);
    if(!value) {
        return false;
    }
    memcpy(value, cacheObject.data(), cacheObject.dataLen());
    return true;
}

UPV::UPV() : p(nullptr,free) {};

UPV Instance::readFromCache(const std::string& key) const
{
    UPV x;
    char* p = nullptr;
    if(readFromCache(p, x.l, key))
        x.p = UP<char>(p,free);
    return x;
}

UPV Instance::readFromCacheRaw(const std::string& key) const
{
    UPV x;
    char* p = nullptr;
    if(readFromCacheRaw(p, x.l, key))
        x.p = UP<char>(p,free);
    return x;
}

void Instance::readCacheTagsVersions(std::map<std::string, uint64_t>& tagsVersions,
                                     const std::string& key) const
{
    MCacheObject cacheObject(key, m_mcache);
    if(!cacheObject.readFromCache()) {
        return;
    }
    if(!m_mcache->mget(cacheObject.metaData().tagsKeys())) {
        return;
    }
    std::string tagKey;
    uint64_t tagVersion;
    while(m_mcache->fetch(tagVersion, tagKey)) {
        tagsVersions[tagKey] = tagVersion;
    }
}

bool Instance::readFromCacheRaw(char*& value, size_t& len, const std::string& key) const
{
    return m_mcache->get(value, len, key);
}

bool Instance::readFromCache(std::vector<char>& value, const std::string& key) const
{
    MCacheObject cacheObject(key, m_mcache);
    if(!cacheObject.readFromCache()) {
        return false;
    }
    if(!cacheObject.isActual()) {
        return false;
    }
    value.resize(cacheObject.dataLen());
    if(!value.empty()) {
        memcpy(&value[0], cacheObject.data(), cacheObject.dataLen());
    }
    return true;
}

bool Instance::multiReadFromCache(const std::vector<std::string>& keys) const
{
    return m_mcache->mget(keys);
}

bool Instance::fetchNextRaw(char*& value, size_t& len, std::string& key) const
{
    return m_mcache->fetch(value, len, key);
}

bool Instance::testWriteRead() const
{
    const std::string TestKey = "test_key";
    int testVal = 22112016;
    if(!writeToCachePOD(TestKey, testVal, false, 1)) {
        return false;
    }

    int newTestVal = 0;
    if(!readFromCachePOD(newTestVal, TestKey)) {
        return false;
    }

    return (testVal == newTestVal);
}

//---------------------------------------------------------------------------------------

InstanceManager& InstanceManager::singletone()
{
    static InstanceManager* inst = new InstanceManager();
    return *inst;
}

InstanceManager::InstanceManager()
{
    callbacks(); // check whether callbacks are initialized
}

void InstanceManager::add(const std::string& id, Instance* inst)
{
    m_instances.insert(std::make_pair(id, inst));
}

void InstanceManager::addRelated(const std::string& id, Instance* inst)
{
    m_relatedInstances.insert(std::make_pair(id, inst));
}

Instance* InstanceManager::get(const std::string& id) const
{
    InstancesMap_t::const_iterator it = m_instances.find(id);
    if(it != m_instances.end()) {
        return it->second;
    }

    LogError(STDLOG) << "memcached instance not found for id '" << id << "'";
    throw comtech::Exception("memcached instance not found!");
}

InstanceManager::InstancesVector_t
        InstanceManager::getAllInstances() const
{
    InstanceManager::InstancesVector_t allInstances;
    for(InstancesMap_t::const_iterator it = m_instances.begin();
            it != m_instances.end(); ++it)
    {
        allInstances.push_back(it->second);
    }
    return allInstances;
}

InstanceManager::InstancesVector_t
        InstanceManager::getRelatedInstances(const std::string& instanceId) const
{
    InstanceManager::InstancesVector_t relatedInstances;
    typedef InstancesMultiMap_t::const_iterator Imm_t;
    std::pair<Imm_t, Imm_t> mmRange = m_relatedInstances.equal_range(instanceId);
    for(Imm_t i = mmRange.first; i != mmRange.second; ++i) {
        relatedInstances.push_back(i->second);
    }
    return relatedInstances;
}

bool InstanceManager::invalidateRelatedInstancesByKey(const std::string& instanceId,
                                                      const std::string& key) const
{
    bool res = true;
    InstancesVector_t relatedInstances = getRelatedInstances(instanceId);
    for(auto instance: relatedInstances) {
        if(!instance->invalidateByKey(key)) {
            tst();
            res = false;
        }
    }
    return res;
}

bool InstanceManager::invalidateRelatedInstancesByTag(const std::string& instanceId,
                                                      const std::string& recName,
                                                      int recId)
{
    bool res = true;
    InstancesVector_t relatedInstances = getRelatedInstances(instanceId);
    for(auto instance: relatedInstances) {
        if(!instance->invalidateByTag(recName, recId)) {
            tst();
            res = false;
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

void Callbacks::flushAll() const
{
    auto allInstances = InstanceManager::singletone().getAllInstances();
    for(Instance* inst:  allInstances)
    {
        if(!inst)
        {
            LogTrace(TRACE0) << "memcached instance is invalid!";
            continue;
        }

        if(!inst->flushAll())
        {
            LogTrace(TRACE0) << "failed to flush_all for memcached instance "
                             << "[" << inst->id() << "]!";
            continue;
        }
    }
}

bool Callbacks::isInstanceAvailable(const std::string& instanceId) const
{
    auto inst = InstanceManager::singletone().get(instanceId);
    if(!inst) {
        return false;
    }

    return inst->testWriteRead();
}

//---------------------------------------------------------------------------------------

class Memcached
{
public:
    static Memcached* Instance();

    void setCallbacks(Callbacks* cbs);
    Callbacks* callbacks(bool throwIfNull);

protected:
    Memcached();

private:
    Callbacks* m_callbacks;
};

//

Memcached* Memcached::Instance()
{
    static Memcached* inst = NULL;
    if(!inst) {
        inst = new Memcached;
    }
    return inst;
}

Memcached::Memcached()
{
    m_callbacks = NULL;
}

void Memcached::setCallbacks(Callbacks* cbs)
{
    LogTrace(TRACE3) << "Memcached::setCallbacks";
    if(m_callbacks) {
        delete m_callbacks;
    }
    m_callbacks = cbs;
}

Callbacks* Memcached::callbacks(bool throwIfNull)
{
    if(!m_callbacks && throwIfNull) {
        throw UninitialisedCallbacks();
    }

    return m_callbacks;
}

//---------------------------------------------------------------------------------------

bool callbacksInitialized()
{
    return Memcached::Instance()->callbacks(false/*throwIfNull*/);
}

Callbacks* callbacks()
{
    return Memcached::Instance()->callbacks(true/*throwIfNull*/);
}

void setCallbacks(Callbacks* cbs)
{
    Memcached::Instance()->setCallbacks(cbs);
}

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING
bool isInstanceAvailable(const std::string& instanceId)
{
    return callbacks()->isInstanceAvailable(instanceId);
}
#endif //XP_TESTING

//---------------------------------------------------------------------------------------

std::string makeFullKey(const std::string& key_in)
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return makePidKey(key_in);
    } else {
        return key_in;
    }
#else
    return key_in;
#endif// XP_TESTING
}

std::vector<std::string> makeFullKeys(const std::vector<std::string>& keys_in)
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return makePidKeys(keys_in);
    } else {
        return keys_in;
    }
#else
    return keys_in;
#endif// XP_TESTING
}

}//namespace memcache

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef XP_TESTING
#include <iostream>
#include "xp_test_utils.h"
#include "checkunit.h"
#include "profiler.h"
#include "timer.h"

namespace memcache
{

Instance* testInstance()
{
    return InstanceManager::singletone().get("test");
}

}//namespace memcache

namespace
{

void start_tests()
{
    using namespace memcache;
    setTclVar("TEST_MEMCACHED_HOST",          "127.0.0.1");
    setTclVar("TEST_MEMCACHED_PORT",          "11211");
    setCallbacks(new Callbacks);
    InstanceManager::singletone().add("test", new Instance("test",
                                                           "TEST_MEMCACHED_HOST",
                                                           "TEST_MEMCACHED_PORT",
                                                           "TEST_MEMCACHED_RELATED"));
    callbacks()->flushAll();
}

void finish_tests()
{
    memcache::callbacks()->flushAll();
}

bool checkMemcached()
{
    return memcache::testInstance()->testWriteRead();
}

struct PodStruct
{
    int a;
    double b;
    char c;
};

bool operator==(const PodStruct& lhs, const PodStruct& rhs)
{
    if(lhs.a != rhs.a) return false;
    if(lhs.b != rhs.b) return false;
    if(lhs.c != rhs.c) return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, const PodStruct& ps)
{
    os << "a = " << ps.a << "; "
       << "b = " << ps.b << "; "
       << "c = " << ps.c << ". "
       << "SIZE = " << sizeof(ps);
    return os;
}

}//namespace

#define CHECK_MEMCACHED_AVAILABLE() \
    if(!checkMemcached()) \
    { \
        LogWarning(STDLOG) << "memcached is unavailable"; \
        return; \
    }


START_TEST(check_fixed_len_cache_wo_tags)
{
    CHECK_MEMCACHED_AVAILABLE();

    PodStruct ps = {};
    ps.a = 1;
    ps.b = 2.0;
    ps.c = '3';

    LogTrace(TRACE5) << "cache data: " << ps;

    bool ret = memcache::testInstance()->writeToCachePOD("key_1", ps, false);
    fail_if(!ret, "write to cache failed");

    PodStruct psNew = {};
    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    fail_unless(ps == psNew, "cache failed");
}
END_TEST

START_TEST(check_var_len_cache_wo_tags)
{
    CHECK_MEMCACHED_AVAILABLE();

    const char val[] = "Test cache data";
    size_t len = strlen(val);

    LogTrace(TRACE5) << "cache data: '" << val << "'; len:" << len;

    bool ret = memcache::testInstance()->writeToCache("key_2", val, len, false);
    fail_if(!ret, "write to cache failed");

    char* newVal = 0;
    size_t newLen = 0;

    ret = memcache::testInstance()->readFromCache(newVal, newLen, "key_2");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: '" << std::string(newVal, newLen)
                     << "'; len:" << newLen;

    fail_unless(len == newLen, "cache failed");
    fail_unless(!memcmp(val, newVal, len), "cache failed");

    free(newVal);
}
END_TEST

START_TEST(check_fixed_len_cache_with_tags)
{
    CHECK_MEMCACHED_AVAILABLE();

    PodStruct ps = {};
    ps.a = 1;
    ps.b = 2.0;
    ps.c = '3';

    LogTrace(TRACE5) << "cache data: " << ps;

    const unsigned TagsCount = 2;
    memcache::MCacheTag tags[TagsCount];
    tags[0] = memcache::createTag("a", 1);
    tags[1] = memcache::createTag("b", 2);

    bool ret = memcache::testInstance()->writeToCachePOD("key_3",
                                                         ps,
                                                         tags,
                                                         TagsCount,
                                                         false);
    fail_if(!ret, "write to cache failed");

    PodStruct psNew = {};
    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_3");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    fail_unless(ps == psNew, "cache failed");

    memcache::testInstance()->invalidateByTag("a", 1);
    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_3");
    fail_if(ret, "read from cache failed. ret = %d", ret);

    // again
    ret = memcache::testInstance()->writeToCachePOD("key_3",
                                                    ps,
                                                    tags,
                                                    TagsCount,
                                                    false);
    fail_if(!ret, "write to cache failed");

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_3");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    fail_unless(ps == psNew, "cache failed");
}
END_TEST

START_TEST(check_var_len_cache_with_tags)
{
    CHECK_MEMCACHED_AVAILABLE();

    const char val[] = "Test cache data";
    size_t len = strlen(val);

    LogTrace(TRACE5) << "cache data: '" << val << "'; len:" << len;

    const unsigned TagsCount = 4;
    memcache::MCacheTag tags[TagsCount];
    tags[0] = memcache::createTag("a", 1);
    tags[1] = memcache::createTag("b", 2);
    tags[2] = memcache::createTag("c", 3);
    tags[3] = memcache::createTag("d", 4);

    bool ret = memcache::testInstance()->writeToCache("key_4",
                                                      val,
                                                      len,
                                                      tags,
                                                      TagsCount,
                                                      false);
    fail_if(!ret, "write to cache failed");

    char* newVal = 0;
    size_t newLen = 0;

    ret = memcache::testInstance()->readFromCache(newVal, newLen, "key_4");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: '" << std::string(newVal, newLen)
                     << "'; len:" << newLen;

    fail_unless(len == newLen, "cache failed");
    fail_unless(!memcmp(val, newVal, len), "cache failed");

    free(newVal);

    memcache::testInstance()->invalidateByTag("b", 2);

    ret = memcache::testInstance()->readFromCache(newVal, newLen, "key_4");
    fail_if(ret, "read from cache failed");
}
END_TEST

START_TEST(check_cache_perf)
{
    CHECK_MEMCACHED_AVAILABLE();

    size_t len = 1000;
    char val[3000] = "";
    memset(val, '*', len);

    bool ret = memcache::testInstance()->writeToCache("key_check_performance",
                                                      val,
                                                      len,
                                                      0);
    fail_if(!ret, "write to cache failed");

    Timer::timer t1;
    size_t counter = 10000;
    start_profiling();
    for(size_t i = 0; i < counter; i++)
    {
        char* newVal = 0;
        size_t newLen = 0;
        ret = memcache::testInstance()->readFromCache(newVal,
                                                      newLen,
                                                      "key_check_performance");
        free(newVal);
    }
    stop_profiling();
    std::cout << t1 << " (" << t1.elapsed() / counter << ")" << std::endl;
}
END_TEST


START_TEST(get_vs_multiget_perf)
{
    CHECK_MEMCACHED_AVAILABLE();

    const size_t Len = 200;
    char val[Len] = "";
    memset(val, '*', Len);

    const size_t NumKeys = 15;

    bool ret = false;

    std::vector<std::string> keys;
    for(size_t j = 1; j <= NumKeys; j++)
    {
        std::ostringstream key;
        key << "key_get_vs_multiget_" << j;
        keys.push_back(key.str());
    }

    for(size_t j = 0; j < keys.size(); ++j)
    {
        ret = memcache::testInstance()->writeToCacheRaw(keys[j], val, Len, 0);
        fail_if(!ret, "write fail");
    }

    const size_t counter = 400;

    size_t dataCounter = 0;
    Timer::timer t1;
    for(size_t i = 0; i < counter; i++)
    {
        char* newVal = 0;
        size_t newLen = 0;
        for(size_t j = 0; j < keys.size(); ++j)
        {
            ret = memcache::testInstance()->readFromCacheRaw(newVal, newLen, keys[j]);
            fail_if(!ret, "read fail");
            fail_if(newLen != Len, "read fail");
            dataCounter++;
            free(newVal);
        }
    }
    std::cout << "Timer for get(" << dataCounter << ") : "
              << t1 << " (" << t1.elapsed() / counter << ")" << std::endl;


    dataCounter = 0;
    Timer::timer t2;

    for(size_t i = 0; i < counter; i++)
    {
        ret = memcache::testInstance()->multiReadFromCache(keys);
        fail_if(!ret, "multi-read fail");

        char* newVal = 0;
        size_t newLen = 0;
        std::string curKey;
        while(memcache::testInstance()->fetchNextRaw(newVal, newLen, curKey))
        {
            fail_if(newLen != Len, "fetch fail");
            dataCounter++;
            free(newVal);
        }
    }
    std::cout << "Timer for multi-get(" << dataCounter << "): "
              << t2 << " (" << t2.elapsed() / counter << ")" << std::endl;

}
END_TEST

START_TEST(check_memcached_lifetime)
{
    CHECK_MEMCACHED_AVAILABLE();

    PodStruct ps = {};
    ps.a = 1;
    ps.b = 2.0;
    ps.c = '3';

    LogTrace(TRACE5) << "cache data: " << ps;
    bool ret = memcache::testInstance()->writeToCachePOD("key_1", ps, false, 1);
    fail_if(!ret, "write to cache failed");

    PodStruct psNew = {};
    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    fail_unless(ps == psNew, "cache failed");

    sleep(2);

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(ret, "memcached lifetime failed");
}
END_TEST

START_TEST(check_memcached_lifetime_invalidate)
{
    CHECK_MEMCACHED_AVAILABLE();

    int a = 876;

    bool ret = memcache::testInstance()->writeToCachePOD("key_int", a, false, 2);
    fail_if(!ret, "write to cache failed");

    int b = 0;
    ret = memcache::testInstance()->readFromCachePOD(b, "key_int");
    fail_if(!ret, "read from cache failed");

    fail_unless(a == b, "cache failed");
}
END_TEST

START_TEST(check_memcached_lifetime_invalidate_complex)
{
    CHECK_MEMCACHED_AVAILABLE();

    PodStruct ps = {};
    ps.a = 1;
    ps.b = 2.0;
    ps.c = '3';

    LogTrace(TRACE5) << "cache data: " << ps;

    const unsigned TagsCount = 1;
    memcache::MCacheTag tags[TagsCount];
    tags[0] = memcache::createTag("a", 1);

    bool ret = memcache::testInstance()->writeToCachePOD("key_1",
                                                         ps,
                                                         tags,
                                                         TagsCount,
                                                         false,
                                                         3);
    fail_if(!ret, "write to cache failed");

    PodStruct psNew = {};
    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    sleep(1);

    ps.a = 2;
    ps.b = 3.0;
    ps.c = '4';

    ret = memcache::testInstance()->writeToCachePOD("key_2",
                                                    ps,
                                                    tags,
                                                    TagsCount,
                                                    false,
                                                    7);
    fail_if(!ret, "write to cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_2");
    fail_if(!ret, "read from cache failed");

    LogTrace(TRACE5) << "new cache data: " << psNew;

    sleep(3);

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(ret, "key lifetime failed");

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_2");
    fail_if(ret, "tag lifetime failed");

    //

    ret = memcache::testInstance()->writeToCachePOD("key_1",
                                                    ps,
                                                    tags,
                                                    TagsCount,
                                                    false,
                                                    1);
    fail_if(!ret, "write to cache failed");

    memcache::testInstance()->invalidateByTag("a", 1);

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(ret, "invalidate cache failed");

    ret = memcache::testInstance()->writeToCachePOD("key_2",
                                                    ps,
                                                    tags,
                                                    TagsCount,
                                                    false,
                                                    1);
    fail_if(!ret, "write to cache failed");

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(ret, "read from cache failed");

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_2");
    fail_if(!ret, "read from cache fastd::map<std::string, Instance*>iled");

    sleep(1);

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_1");
    fail_if(ret, "read from cache failed");

    ret = memcache::testInstance()->readFromCachePOD(psNew, "key_2");
    fail_if(ret, "read from cache failed");
}
END_TEST

START_TEST(check_mget_features)
{
    CHECK_MEMCACHED_AVAILABLE();

    const char val[] = "Hello, World";

    bool ret = memcache::testInstance()->writeToCacheRaw("key_1", val, sizeof(val));
    fail_if(!ret, "write to cache failed");

    ret = memcache::testInstance()->writeToCacheRaw("key_2", val, sizeof(val));
    fail_if(!ret, "write to cache failed");

    ret = memcache::testInstance()->writeToCacheRaw("key_3", val, sizeof(val));
    fail_if(!ret, "write to cache failed");

    char* newVal = 0;
    size_t newLen = 0;

    ret = memcache::testInstance()->readFromCacheRaw(newVal, newLen, "key_1");
    fail_if(!ret, "read from cache failed");
    fail_unless(!strcmp(newVal, val));
    fail_if(newLen != sizeof(val), "fetch fail");
    free(newVal);

    ret = memcache::testInstance()->readFromCacheRaw(newVal, newLen, "key_2");
    fail_if(!ret, "read from cache failed");
    fail_unless(!strcmp(newVal, val));
    fail_if(newLen != sizeof(val), "fetch fail");
    free(newVal);

    ret = memcache::testInstance()->readFromCacheRaw(newVal, newLen, "key_3");
    fail_if(!ret, "read from cache failed");
    fail_unless(!strcmp(newVal, val));
    fail_if(newLen != sizeof(val), "fetch fail");
    free(newVal);


    std::vector<std::string> keys;
    keys.push_back("key_1");
    keys.push_back("key_2");
    keys.push_back("key_3");

    ret = memcache::testInstance()->multiReadFromCache(keys);
    fail_if(!ret, "multi read failed");

    std::string curKey;
    int fetchCounter = 0;
    while(memcache::testInstance()->fetchNextRaw(newVal, newLen, curKey))
    {
        fail_if(newLen != sizeof(val), "fetch fail");
        free(newVal);
        fetchCounter++;
    }

    fail_unless(fetchCounter == 3);

    ret = memcache::testInstance()->invalidateByKey("key_1");
    fail_if(!ret, "del failed");


    ret = memcache::testInstance()->multiReadFromCache(keys);
    fail_if(!ret, "multi read failed");

    fetchCounter = 0;
    while(memcache::testInstance()->fetchNextRaw(newVal, newLen, curKey))
    {
        LogTrace(TRACE3) << "succesfull fetch for key " << curKey;
        fail_if(newLen != sizeof(val), "fetch fail");
        free(newVal);
        fetchCounter++;
    }

    fail_unless(fetchCounter == 2, "fetch counter = %d", fetchCounter);
}
END_TEST

START_TEST(check_manual_lock)
{
    CHECK_MEMCACHED_AVAILABLE();

    int val = 87;
    LogTrace(TRACE5) << "val: " << val;
    fail_unless(memcache::testInstance()->isCacheLocked("key_1") == false);

    if(memcache::testInstance()->lockCache("key_1"))
    {
        fail_unless(memcache::testInstance()->isCacheLocked("key_1") == true);
        bool ret = memcache::testInstance()->writeToCachePOD("key_1", val, true);
        fail_if(!ret, "write to cache failed");
        fail_unless(memcache::testInstance()->isCacheLocked("key_1") == true);
        memcache::testInstance()->unlockCache("key_1");
        fail_unless(memcache::testInstance()->isCacheLocked("key_1") == false);
    }
    else
    {
        fail_unless(0, "lock failed");
    }
}
END_TEST

START_TEST(check_big_data)
{
    CHECK_MEMCACHED_AVAILABLE();
    
    const size_t DataSize = memcache::MaxUserBuffSize();
    char buff[DataSize + 1000];
    memset(buff, '}', sizeof(buff));
            
    size_t len  = DataSize;
    bool ret = memcache::testInstance()->writeToCache("big_data", buff, DataSize, false);
    fail_if(!ret, "write to cache %zd bytes failed", DataSize);

    len = DataSize + 1;
    ret = memcache::testInstance()->writeToCache("big_data", buff, len, false);
    fail_if(ret, "unexpected return from write to cache %zd bytes", len);
    
    char* newBuff = 0;
    size_t newLen = 0;
    ret = memcache::testInstance()->readFromCache(newBuff, newLen, "big_data");
    free(newBuff);
    fail_if(!ret, "read from cache failed");
}
END_TEST

START_TEST(check_parseHostPortPairs)
{
#ifdef HAVE_MEMCACHED
    auto s = "127.0.0.1;127.0.0.2;127.0.0.3:11200;127.0.0.4";
    auto parsed = memcache::parseHostPortPairs(s, 22222);

    fail_unless(parsed.size() == 4);

    fail_unless(parsed[0].m_host == "127.0.0.1");
    fail_unless(parsed[0].m_port == 22222);

    fail_unless(parsed[1].m_host == "127.0.0.2");
    fail_unless(parsed[1].m_port == 22222);

    fail_unless(parsed[2].m_host == "127.0.0.3");
    fail_unless(parsed[2].m_port == 11200);

    fail_unless(parsed[3].m_host == "127.0.0.4");
    fail_unless(parsed[3].m_port == 22222);
#endif//HAVE_MEMCACHED
}
END_TEST


#undef CHECK_MEMCACHED_AVAILABLE


#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_parseHostPortPairs);
}
TCASEFINISH
#undef SUITENAME

#define SUITENAME "Serverlib"
TCASEREGISTER(start_tests, finish_tests)
{
    ADD_TEST(check_fixed_len_cache_wo_tags);
    ADD_TEST(check_var_len_cache_wo_tags);
    ADD_TEST(check_fixed_len_cache_with_tags);
    ADD_TEST(check_var_len_cache_with_tags);
    ADD_TEST(check_mget_features);
    ADD_TEST(check_manual_lock);
    ADD_TEST(check_big_data);
}
TCASEFINISH
#undef SUITENAME

#define SUITENAME "memcached_performance"
TCASEREGISTER(start_tests, finish_tests)
{
    ADD_TEST(check_cache_perf);
    ADD_TEST(get_vs_multiget_perf);
}
TCASEFINISH
#undef SUITENAME

#define SUITENAME "memcached_lifetime"
TCASEREGISTER(start_tests, finish_tests)
{
    ADD_TEST(check_memcached_lifetime);
    ADD_TEST(check_memcached_lifetime_invalidate);
    ADD_TEST(check_memcached_lifetime_invalidate_complex);
}
TCASEFINISH
#undef SUITENAME

#endif /*XP_TESTING*/
