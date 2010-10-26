#ifndef EDIFACTPROFILE_H
#define EDIFACTPROFILE_H

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>

#include <serverlib/exception.h>
#include "CheckinBaseTypes.h"

namespace edifact
{
class MsgEdifactProfile;

class BadEdifactProfile
    : public comtech::Exception
{
public:
    BadEdifactProfile(const char *nick, const char *file, int line, const std::string& msg);
    virtual ~BadEdifactProfile() throw();
};

class BadEdifactMsgProfile : public BadEdifactProfile
{
    std::string MsgType;
public:
    BadEdifactMsgProfile(const char *nick, const char *file, int line,
                         const std::string &msgType, const std::string& msg);
    const std::string &msgType() const { return MsgType; }
    virtual ~BadEdifactMsgProfile() throw() {}
};

/**
 * @class MsgEdifactProfile
 * @brief Edifact profile for several message types
 * */
class EdifactProfile
{
    typedef ASTRA::EdifactProfile_t id_t;
public:
    struct Syntax
    {
        std::string name;
        int version;
        bool operator==(const Syntax& rv) const
        {
            return ((name == rv.name) && (version == rv.version));
        }
    };
    struct MessageOptions
    {
        MessageOptions();
        MessageOptions(const std::string& name, const std::string& majVer,
                       const std::string& minVer, const std::string& agn);

        std::string name;
        int majorVersion;
        int minorVersion;
        std::string agency;
        std::string majorVersionStr() const;
        std::string minorVersionStr() const;
        bool operator==(const MessageOptions& rv) const
        {
            return ((name == rv.name)
                && (majorVersion == rv.majorVersion)
                && (minorVersion == rv.minorVersion)
                && (agency == rv.agency));
        }
    };
    typedef std::list<MessageOptions> MessageOptionsList_t;
    typedef MessageOptionsList_t::const_iterator MessageOptionsCIter_t;
public:
    EdifactProfile(const std::string& name);
    virtual ~EdifactProfile();

    static EdifactProfile load(id_t id);
    /**
     * @brief get ida by name
     * @param
     * @return
     */
    static id_t idByName(const std::string &name);
    /**
     * @brief save data to the db
     */
    void save();

    /**
     * properties
     * */
    id_t id() const;
    const std::string& name() const;
    const Syntax& syntax() const;
    void setSyntax(const std::string& name, int ver);
    bool isBatch() const;
    void setBatch(bool batch);
    const MessageOptionsList_t & opts() const;
    const MessageOptions & opts(const std::string &ediMsgName) const;
    void addMessageOption(const std::string& name, int majVer, int minVer, const std::string& agn);
private:
    id_t m_id;
    std::string m_name;
    Syntax m_syntax;
    bool m_batch;
    MessageOptionsList_t m_opts;
};

typedef boost::shared_ptr<EdifactProfile> pEdifactProfile;

/**
 * @class MsgEdifactProfile
 * @brief Edifact profile for current message type
 * */
class MsgEdifactProfile
{
    typedef ASTRA::EdifactProfile_t id_t;
public:
    MsgEdifactProfile(const std::string& name);
    virtual ~MsgEdifactProfile();

    static MsgEdifactProfile load(id_t id, const std::string& msgType);

    /**
     * properties
     * */
    id_t id() const;
    const std::string& name() const;
    /**
     * @brief IATA:1
     * @return
     */
    const EdifactProfile::Syntax& syntax() const;
    bool isBatch() const;
    const EdifactProfile::MessageOptions& opts() const;
private:
    id_t m_id;
    std::string m_name;
    EdifactProfile::Syntax m_syntax;
    bool m_batch;
    EdifactProfile::MessageOptions m_opts;
};

} // edifact

#endif /* EDIFACTPROFILE_H */

