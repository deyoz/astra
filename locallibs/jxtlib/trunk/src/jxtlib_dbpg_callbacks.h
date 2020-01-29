#pragma once
#include <string>
#include "jxtlib_db_callbacks.h"

namespace PgCpp {
  namespace details {
    class SessionDescription;
  }
}

namespace jxtlib
{

class JxtlibDbPgCallbacks: public JxtlibDbCallbacks
{
    PgCpp::details::SessionDescription *sd;
public:
    JxtlibDbPgCallbacks(PgCpp::details::SessionDescription *sd) : sd(sd) {}
    virtual long getXmlDataVer(const std::string &type, const std::string &id,
                               bool no_iparts) override;
    virtual void insertXmlStuff(const std::string &type, const std::string &id,
                                const std::string &data) override;
    virtual void deleteIfaceLinks(const std::string &id) override;
    virtual bool ifaceLinkExists(const std::string &iface_id, long ver) override;
    virtual void insertIfaceLinks(const std::string &id,
                                  const std::vector<IfaceLinks> &ilinks,
                                  int version=0) override;
    virtual std::vector<IfaceLinks> getIfaceLinks(const std::string &iface_id,
                                                  const std::string &lang,
                                                  long ver) override;
    virtual std::string getXmlData(const std::string &type,
                                   const std::string &id, long ver) override;
    virtual std::list<std::string> getIparts(const std::string &iface) override;
    virtual std::string getCachedIfaceWoIparts(const std::string &id, long answer_ver) override;
    virtual void setCachedIfaceWoIparts(const std::string &iface_str, const std::string &id,
                                        long ver) override;
    virtual long getDataVer(const std::string &data_id) override;

    virtual RelIdTabName getRelIdTab(const std::string &id) override;

    virtual std::vector<TabColsValues> getTabValues(const RelIdTabName &relid,
                                             const std::vector<RelTabcolTab> &rel_tabcols,
                                             bool reset_ind, long term_ver) override;
    virtual std::vector<RelTabcolTab> getRelTabcolTab(const std::string &id) override;

    virtual bool jxtContIsSavedCtxt(int handle, const std::string &session_id) const override;
    virtual void jxtContDeleteSavedCtxt(int handle, const std::string &session_id) const override;
    virtual int jxtContGetNumbersOfSaved(std::vector<int> &contexts_vec, const std::string &term) const override;
    virtual bool jxtContIsSavedRow(const std::string &name, const std::string &session_id) const override;
    virtual std::string jxtContReadSavedRow(const std::string &name, const std::string &session_id) const override;
    virtual void jxtContRemoveLikeL(const std::string &key, const std::string &session_id) const override;
    virtual void jxtContAddRow(const JxtContext::JxtContRow *row, const std::string &sess_id, int page_size) const override;
    virtual void jxtContDeleteRow(const JxtContext::JxtContRow *row, const std::string &session_id) const override;
    virtual std::set<std::string> jxtContReadAllKeys(const std::string &session_id) const override;

    virtual void commit() override;
    virtual void rollback() override;

    virtual ~JxtlibDbPgCallbacks() {}
};
}
