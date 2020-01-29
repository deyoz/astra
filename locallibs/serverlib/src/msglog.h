#ifndef __MSGLOG_H__
#define __MSGLOG_H__

#include "strong_types.h"

namespace msglog {

    DEFINE_STRING_WITH_LENGTH_VALIDATION(Id, 1, 16, "MSGLOG ID", "MSGLOG ID");
    DEFINE_STRING_WITH_LENGTH_VALIDATION(Domain, 1, 16, "DOMAIN", "DOMAIN");

    Id write(const Domain& domain, const std::string& text, bool is_autonomous = false);
    std::string read(const Id& id);
    bool remove(const Id& id);

} /* namespace msglog */

#endif /* __MSGLOG_H__ */
