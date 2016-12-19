#pragma once

#include <string>


namespace edifact {

struct EdifactProfileData
{
    std::string m_profileName;
    std::string m_version;
    std::string m_subVersion;
    std::string m_ctrlAgency;
    std::string m_syntaxName;
    unsigned    m_syntaxVer;

    EdifactProfileData();
    EdifactProfileData(const std::string& profileName,
                       const std::string& version,
                       const std::string& subVersion,
                       const std::string& ctrlAgency,
                       const std::string& syntaxName,
                       unsigned syntaxVer);
};

//---------------------------------------------------------------------------------------

class EdifactProfile
{
public:
    const std::string& name() const;
    const std::string& version() const;
    const std::string& subVersion() const;
    const std::string& ctrlAgency() const;
    const std::string& syntaxName() const;
    unsigned           syntaxVer() const;

    static EdifactProfile readByName(const std::string& profileName);
    static EdifactProfile createDefault();

protected:
    EdifactProfile(const EdifactProfileData& data);

private:
    EdifactProfileData m_data;
};

}//namespace edifact
