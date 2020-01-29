#pragma once

#include <serverlib/message.h>
#include "mct.h"

namespace ssim { namespace mct {

struct ParseError
{
    enum Type { Syntax, Nsi, Logic };

    Type type;
    Message description;
};

class ImportHandler
{
protected:
    boost::optional<ContentIndicator> policy_;

    std::map<
        nsi::PointId,
        std::vector<std::pair<Record, boost::optional<ActionIndicator>>>
    > data_;

    virtual void removeRecords(nsi::PointId) = 0;
    virtual void removeRecord(const std::string&) = 0;
    virtual std::string insertRecord(const Record&) = 0;
    virtual std::vector<std::pair<std::string, Record>> getRecords(nsi::PointId) const = 0;
    virtual std::set<nsi::PointId> getMctPoints() const = 0;
    virtual void preProcess(nsi::PointId) = 0;
    virtual void postProcess(nsi::PointId) = 0;

public:
    virtual ~ImportHandler();

    void setProcessPolicy(ContentIndicator);
    void stash(const Record&, const boost::optional<ActionIndicator>&);
    void applyChanges();
    virtual bool handleError(const ParseError&) = 0;
};

Message parseMctDataSet(std::istream&, ImportHandler&);

} } //ssim::mct
