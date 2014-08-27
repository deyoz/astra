#include "EdifactProfile.h"

#include <serverlib/str_utils.h>
#include <serverlib/cursctl.h>

#include <boost/lexical_cast.hpp>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>

using std::string;

namespace edifact
{

/**
 * BadEdifactProfile
 * */

BadEdifactProfile::BadEdifactProfile(const char *nick, const char *file, int line, const std::string& msg)
    : comtech::Exception(nick, file, line, "", msg)
{
}

BadEdifactProfile::~BadEdifactProfile() throw ()
{
}

BadEdifactMsgProfile::BadEdifactMsgProfile(const char *nick, const char *file, int line,
                                           const std::string &msgType, const std::string &msg)
    :BadEdifactProfile(nick, file, line, msg), MsgType(msgType)
{
}

namespace
{
    const int NONE_ID = -1;
}

/**
 * EdifactProfile
 * */

EdifactProfile::MessageOptions::MessageOptions()
    : majorVersion(0), minorVersion(0)
{}

EdifactProfile::MessageOptions::MessageOptions(const std::string& name_, const std::string& majVer_, const std::string& minVer_, const std::string& agn_)
    : name(name_), agency(agn_)
{
    const size_t max_name_length = 6;
    const size_t max_ver_length = 2;
    const size_t max_agn_length = 2;
    try
    {
        if (majVer_.length() > max_ver_length || !majVer_.length())
            throw BadEdifactProfile(STDLOG, "Тип сообщений профиля Edifact: неверная версия");
        if (minVer_.length() > max_ver_length || !minVer_.length())
            throw BadEdifactProfile(STDLOG, "Тип сообщений профиля Edifact: неверная подверсия");
        majorVersion = boost::lexical_cast<int>(majVer_);
        minorVersion = boost::lexical_cast<int>(minVer_);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        throw BadEdifactProfile(STDLOG, "Тип сообщений профиля Edifact: неверно заполнены версии");
    }
    if (name.length() > max_name_length || !name.length())
        throw BadEdifactProfile(STDLOG, "Тип сообщений профиля Edifact: неверно заполнено название");
    if (agency.length() > max_agn_length || !agency.length())
        throw BadEdifactProfile(STDLOG, "Тип сообщений профиля Edifact: неверно заполнено агентство");
}

EdifactProfile::EdifactProfile(const string& name)
    : m_id(NONE_ID), m_name(name), m_batch(false)
{
}

EdifactProfile::~EdifactProfile()
{
}

EdifactProfile::id_t EdifactProfile::idByName(const std::string & name)
{
    id_t id;
    // TODO
//    OciCpp::CursCtl cur = make_curs("select ida from edifact_profiles where name = :n");
//    cur.
//            bind(":n", name).
//            def(id).
//            EXfet();

//    if(cur.err() == NO_DATA_FOUND)
//    {
//        LogTrace(TRACE1) << "not found by name = " << name;
//        throw BadEdifactProfile(STDLOG,
//                                std::string("edifact_profile not found by name = ") + name);
//    }

    return id;
}

EdifactProfile EdifactProfile::load(id_t id)
{
    string name, synName;
      // TODO
//    int synVer, isBatch;
//    LogTrace(TRACE3) << "EdifactProfile::load, id = " << id;
//    OciCpp::CursCtl cr = make_curs("select name, syntax_name, syntax_ver, batch from edifact_profiles where ida = :id");
//    cr.bind(":id", id);
//    cr.def(name).def(synName).def(synVer).def(isBatch);
//    cr.EXfet();
//    if (cr.err() == NO_DATA_FOUND)
//        throw BadEdifactProfile(STDLOG, "load by id failed");
    EdifactProfile ep(name);
//    ep.m_id = id;
//    ep.setSyntax(synName, synVer);
//    ep.setBatch(isBatch);

//    MessageOptions mo;
//    OciCpp::CursCtl crMsgTypes = make_curs("select name, major_version, minor_version, agency from edifact_profile_msg_types where ida_ep = :id");
//    crMsgTypes.bind(":id", id);
//    crMsgTypes.def(mo.name).def(mo.majorVersion).def(mo.minorVersion).def(mo.agency);
//    crMsgTypes.exec();
//    while (!crMsgTypes.fen())
//        ep.m_opts.push_back(mo);
    return ep;
}

void EdifactProfile::save()
{
    // TODO
//    make_curs(
//        "begin\n"
//        "insert into edifact_profiles (ida, name, syntax_name, syntax_ver, batch) "
//        "values (edifact_profiles_seq.nextval, :name, :sn, :sv, :b) "
//        "returning ida into :id;\n"
//        "end;")
//        .bind(":name", m_name)
//        .bind(":sn", m_syntax.name)
//        .bind(":sv", m_syntax.version)
//        .bind(":b", (int)m_batch)
//        .bindOut(":id", m_id)
//        .exec();
//    if (m_opts.empty())
//        return;
//    OciCpp::CursCtl cr = make_curs(
//        "insert into edifact_profile_msg_types (ida_ep, name, major_version, minor_version, agency) "
//        "values (:ep, :name, :majv, :minv, :ag)");
//    cr.bind(":ep", m_id);
//    for (MessageOptionsCIter_t it = m_opts.begin(); it != m_opts.end(); ++it)
//    {
//        cr.bind(":name", it->name).bind(":majv", it->majorVersion)
//            .bind(":minv", it->minorVersion).bind(":ag", it->agency)
//            .exec();
//    }
}

EdifactProfile::id_t EdifactProfile::id() const
{
    return m_id;
}

const string& EdifactProfile::name() const
{
    return m_name;
}

const EdifactProfile::Syntax& EdifactProfile::syntax() const
{
    return m_syntax;
}

bool EdifactProfile::isBatch() const
{
    return m_batch;
}

void EdifactProfile::setBatch(bool batch)
{
    m_batch = batch;
}

void EdifactProfile::setSyntax(const string& name, int ver)
{
    Syntax s;
    s.name = name;
    s.version = ver;
    m_syntax = s;
}

const EdifactProfile::MessageOptionsList_t& EdifactProfile::opts() const
{
    return m_opts;
}

const EdifactProfile::MessageOptions & EdifactProfile::opts(const std::string & ediMsgName) const
{
    for (EdifactProfile::MessageOptionsCIter_t it = opts().begin(); it != opts().end(); ++it)
    {
        if (it->name == ediMsgName)
        {
            return *it;
        }
    }

    throw BadEdifactMsgProfile(STDLOG, ediMsgName, "load by edifact msg failed. msgType = " +
                ediMsgName + " ida_ep = " + HelpCpp::string_cast(id().get()));
}

void EdifactProfile::addMessageOption(const string& name, int majVer, int minVer, const string& agn)
{
    MessageOptions mo;
    mo.name = name;
    mo.majorVersion = majVer;
    mo.minorVersion = minVer;
    mo.agency = agn;
    m_opts.push_back(mo);
}

/**
 * MsgEdifactProfile
 * */

MsgEdifactProfile::MsgEdifactProfile(const string& name)
     : m_id(NONE_ID), m_name(name), m_batch(false)
{
}

MsgEdifactProfile::~MsgEdifactProfile()
{
}

MsgEdifactProfile MsgEdifactProfile::load(id_t id, const std::string& msgType)
{
    string name, synName;
    int synVer = 0, isBatch = 0;
    EdifactProfile::MessageOptions mo;

    // TODO
//    LogTrace(TRACE3) << "EdifactProfile::load id = " << id <<
//            "; msgType = " << msgType;

//    OciCpp::CursCtl cr = make_curs(
//        "select ep.name, ep.syntax_name, ep.syntax_ver, ep.batch, "
//        "ept.name, ept.major_version, ept.minor_version, ept.agency "
//        "from edifact_profiles ep, edifact_profile_msg_types ept "
//        "where ep.ida = ept.ida_ep and ida = :id and ept.name = :msgType");
//    cr.bind(":id", id).bind(":msgType", msgType);
//    cr.def(name).def(synName).def(synVer).def(isBatch);
//    cr.def(mo.name).def(mo.majorVersion).def(mo.minorVersion).def(mo.agency);
//    cr.EXfet();
//    if (cr.err() == NO_DATA_FOUND)
//        throw BadEdifactMsgProfile(STDLOG, msgType, "load by id failed. msgType=" +
//                msgType + " ida_ep = " + HelpCpp::string_cast(id.get()));

    MsgEdifactProfile ep(name);
    ep.m_id = id;
    ep.m_syntax.name = synName;
    ep.m_syntax.version = synVer;
    ep.m_batch = (isBatch == 0);
    ep.m_opts = mo;
    return ep;
}

MsgEdifactProfile::id_t MsgEdifactProfile::id() const
{
    return m_id;
}

const string& MsgEdifactProfile::name() const
{
    return m_name;
}

const EdifactProfile::Syntax& MsgEdifactProfile::syntax() const
{
    return m_syntax;
}

bool MsgEdifactProfile::isBatch() const
{
    return m_batch;
}

const EdifactProfile::MessageOptions& MsgEdifactProfile::opts() const
{
    return m_opts;
}

std::string EdifactProfile::MessageOptions::majorVersionStr() const
{
    return StrUtils::lpad(boost::lexical_cast<std::string>(majorVersion),2,'0');
}

std::string EdifactProfile::MessageOptions::minorVersionStr() const
{
    return boost::lexical_cast<std::string>(minorVersion);
}


} // edifact

