#pragma once
#include <string>
#include <vector>
#include <set>
#include <list>

namespace JxtContext {
    class JxtContRow;
}

namespace jxtlib
{
struct IfaceLinks
{
    std::string id;
    std::string type;
    std::string lang;
    long ver;
    IfaceLinks(const std::string &id_, const std::string &type_,
               const std::string &lang_=std::string(), bool need_ver=true);
};

struct RelIdTabName
{
    std::string root;
    std::string tabname;
    short undeletable;
};

struct RelTabcolTab {
    std::string tabcol;
    std::string tagname;
    int key;
};

typedef std::vector<std::string> tab_cols_t;
struct TabColsValues {
    tab_cols_t tabcols;
    std::vector<short> ind;
    int close;
};

class JxtlibDbCallbacks
{
    static JxtlibDbCallbacks *Instance;
public:
    virtual long getXmlDataVer(const std::string &type, const std::string &id,
                               bool no_iparts) = 0;
    virtual void insertXmlStuff(const std::string &type, const std::string &id,
                                const std::string &data) = 0;
    virtual void deleteIfaceLinks(const std::string &id) = 0;
    virtual bool ifaceLinkExists(const std::string &iface_id, long ver) = 0;
    virtual void insertIfaceLinks(const std::string &id,
                                  const std::vector<IfaceLinks> &ilinks,
                                  int version=0) = 0;
    virtual std::vector<IfaceLinks> getIfaceLinks(const std::string &iface_id,
                                                  const std::string &lang,
                                                  long ver) = 0;
    virtual std::string getXmlData(const std::string &type,
                                   const std::string &id, long ver) = 0;
    virtual std::string getCachedIfaceWoIparts(const std::string &id, long answer_ver) = 0;
    virtual void setCachedIfaceWoIparts(const std::string &iface_str, const std::string &id,
                                        long ver) = 0;
    virtual long getDataVer(const std::string &data_id) = 0;
    virtual std::list<std::string> getIparts(const std::string &iface) = 0;

    virtual RelIdTabName getRelIdTab(const std::string &id) = 0;
    virtual std::vector<RelTabcolTab> getRelTabcolTab(const std::string &id) = 0;
    virtual std::vector<TabColsValues> getTabValues(const RelIdTabName &relid,
                                             const std::vector<RelTabcolTab> &rel_tabcols,
                                             bool reset_ind, long term_ver) = 0;

    virtual bool jxtContIsSavedCtxt(int handle, const std::string &session_id) const = 0;
    virtual void jxtContDeleteSavedCtxt(int handle, const std::string &session_id) const = 0;
    virtual int jxtContGetNumbersOfSaved(std::vector<int> &contexts_vec, const std::string &term) const = 0;
    virtual bool jxtContIsSavedRow(const std::string &name, const std::string &session_id) const = 0;
    virtual std::string jxtContReadSavedRow(const std::string &name, const std::string &session_id) const = 0;
    virtual void jxtContRemoveLikeL(const std::string &key, const std::string &session_id) const = 0;
    virtual void jxtContAddRow(const JxtContext::JxtContRow *row, const std::string &session_id, int page_size) const = 0;
    virtual void jxtContDeleteRow(const JxtContext::JxtContRow *row, const std::string &session_id) const = 0;
    virtual std::set<std::string> jxtContReadAllKeys(const std::string &session_id) const = 0;

    virtual void commit() = 0;
    virtual void rollback() = 0;

    static JxtlibDbCallbacks *instance();
    static void setJxtlibDbCallbacks(JxtlibDbCallbacks *cb);

    virtual ~JxtlibDbCallbacks();
};

}
