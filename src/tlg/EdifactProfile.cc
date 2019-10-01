#include "EdifactProfile.h"
#include "exceptions.h"

#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

EdifactProfileData::EdifactProfileData()
    : m_syntaxVer(0)
{}

EdifactProfileData::EdifactProfileData(const std::string& profileName,
                                       const std::string& version,
                                       const std::string& subVersion,
                                       const std::string& ctrlAgency,
                                       const std::string& syntaxName,
                                       unsigned syntaxVer)
    : m_profileName(profileName),
      m_version(version),
      m_subVersion(subVersion),
      m_ctrlAgency(ctrlAgency),
      m_syntaxName(syntaxName),
      m_syntaxVer(syntaxVer)
{}

//---------------------------------------------------------------------------------------

EdifactProfile::EdifactProfile(const EdifactProfileData& data)
    : m_data(data)
{}

const std::string& EdifactProfile::name() const       { return m_data.m_profileName; }
const std::string& EdifactProfile::subVersion() const { return m_data.m_subVersion;  }
const std::string& EdifactProfile::ctrlAgency() const { return m_data.m_ctrlAgency;  }
const std::string& EdifactProfile::syntaxName() const { return m_data.m_syntaxName;  }
unsigned           EdifactProfile::syntaxVer() const  { return m_data.m_syntaxVer;   }

std::string EdifactProfile::version() const
{
    std::string vers = m_data.m_version;
    if(vers.length() < 2 && StrUtils::isStrAllDigit(vers)) {
        vers = StrUtils::LPad(vers, 2, '0');
    }
    return vers;
}

EdifactProfile EdifactProfile::readByName(const std::string& profileName)
{
    OciCpp::CursCtl cur = make_curs(
"select NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER "
"from EDIFACT_PROFILES "
"where NAME=:profile_name");

    EdifactProfileData p;

    cur
            .bind(":profile_name", profileName)
            .def(p.m_profileName)
            .def(p.m_version)
            .def(p.m_subVersion)
            .def(p.m_ctrlAgency)
            .def(p.m_syntaxName)
            .def(p.m_syntaxVer)
            .EXfet();

    if(cur.err() == NO_DATA_FOUND) {
        throw EXCEPTIONS::Exception("No such EdifactProfile: %s", profileName.c_str());
    }

    return EdifactProfile(p);
}

EdifactProfile EdifactProfile::createDefault()
{
    EdifactProfileData p;
    p.m_profileName = "DEFAULT";
    p.m_version     = "96";
    p.m_subVersion  = "2";
    p.m_ctrlAgency  = "IA";
    p.m_syntaxName  = "IATA";
    p.m_syntaxVer   = 1;
    return EdifactProfile(p);
}

}//namespace edifact
