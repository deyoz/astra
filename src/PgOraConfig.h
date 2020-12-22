#pragma once

#include <string>

namespace DbCpp
{
    class Session;
}

namespace PgOra
{
    /* Postgres    Oracle
     * read write  read write parameter
     *             +    +     0
     *      +      +    +     1
     * +    +           +     2
     * +    +                 3
     */
    class Config
    {
    public:
        static std::string PgOra_config_OnlyPGparam() { return std::string("_3"); }

    public:
        Config(const std::string& tcl);
        bool readOracle() const { return mCfg == 0 || mCfg == 1; }
        bool readPostgres() const { return mCfg == 2 || mCfg == 3; }
        bool writeOracle() const { return mCfg < 3; }
        bool writePostgres() const { return mCfg > 0; }

    private:
        int mCfg;
    };

    DbCpp::Session& getROSession(const std::string& objectName);
    DbCpp::Session& getRWSession(const std::string& objectName);
    DbCpp::Session& getAutoSession(const std::string& objectName);
    bool supportsPg(const std::string& objectName);
    std::string getGroup(const std::string& objectName);

    std::string makeSeqNextVal(const std::string& sequenceName);
    long getSeqNextVal(const std::string& sequenceName);

}// namespace PgOra
