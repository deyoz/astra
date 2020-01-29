#ifndef JXTLIB_JXTEDIHELPMANAGER_H
#define JXTLIB_JXTEDIHELPMANAGER_H

#include <string>
#include <serverlib/EdiHelpManager.h>

namespace ServerFramework
{

class JxtEdiHelpManager : public ServerFramework::EdiHelpManager
{
private:
    virtual std::string make_text(std::string const &s);
public:
    JxtEdiHelpManager(int f1) : EdiHelpManager(f1) {}
    virtual ~JxtEdiHelpManager() {}
};

} // namespace ServerFramework


#endif /* JXTLIB_JXTEDIHELPMANAGER_H */

