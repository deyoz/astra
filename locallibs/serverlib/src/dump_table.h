#ifndef SERVERLIB_DUMP_TABLE_H
#define SERVERLIB_DUMP_TABLE_H

#include <list>
#include <string>

namespace OciCpp
{
class OciSession;
class DumpTableOut;

class DumpTable
{
public:
    DumpTable(OciSession&, const std::string& table);
    DumpTable(const std::string& table);
    DumpTable& addFld(const std::string& fld);
    DumpTable& where(const std::string& wh);
    DumpTable& order(const std::string& ord);
    void exec(int loglevel, const char* nick, const char* file, int line);
    void exec(std::string& out);
private:
    OciSession& sess_;
    std::string table_;
    std::list<std::string> fields_;
    std::string where_;
    std::string order_;
    void exec(DumpTableOut const& out);    
};

} // OciCpp

#endif /* SERVERLIB_DUMP_TABLE_H */

