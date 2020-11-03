#pragma once

#include <string>
#include <vector>
#include "cursctl.h"
    extern "C" {
#include <oci.h>
#include "oci_err.h"
    }
namespace OciCpp
{

bool checkerr8(const char *nick, const char *file, int line, sword status, OCIError *errhp, const std::string& connectString);

class OciStatementHandle
{
  public:
    OCIStmt *stmthp;
    explicit OciStatementHandle(OCIStmt *st) : stmthp(st) {}
    ~OciStatementHandle();
};

/**
 * open OCI8 session
 * */
void Oci8Init(const std::string& str);

class AqEventCallback
{
public:
    virtual void onEvent() = 0;
    virtual ~AqEventCallback() {}
};

class Oci8Session
{
    /* Declare OCI Handles to be used */
    OCIServer     *srvhp; // OCI Server context
    OCISession    *usrhp; // OCI user context

    OciSession* converted_session;
    int converted_mode;
    oracle_connection_param connection_param;

  public:
    Oci8Session(const char* nick, const char* file, int line, const std::string& connStr);
    Oci8Session(const char* nick, const char* file, int line, OciSession& conv_sess);
    Oci8Session(OciSession& conv_sess);
    ~Oci8Session();

    bool operator==(const Oci8Session& s) const {  return usrhp and usrhp == s.usrhp;  }

    OCIEnv        *envhp; // OCI Environment
    OCISvcCtx     *svchp; // OCI Service context
    OCIError      *errhp; // OCI Error handler
    sword status; // operation result

    static Oci8Session& instance(const char *nick, const char *file, int line);
    [[deprecated("use instance(STDLOG) instead")]] static Oci8Session& instance(); // must not be used in Sirena!
    bool checkerr(const char *nick, const char *file, int line);
    bool checkerr(const char *nick, const char *file, int line, sword _status);
    bool commit();
    bool rollback();
    int err() const;
    bool isErr() const
    {
        return status != OCI_SUCCESS;
    }
    OciStatementHandle getStHandle();

    void setAqCallback(const std::string& name, OciCpp::AqEventCallback* cb);
    std::string getConnectString() const;
    void setClientInfo(const std::string &clientInfo);
};


OCILobLocator* createLob(Oci8Session& os, const std::string& data, ub4 dtype);
OCILobLocator* createLob(Oci8Session& os, const std::vector<char>& data, ub4 dtype);
OCILobLocator* createLob(Oci8Session& os, const std::vector<uint8_t>& data, ub4 dtype);
OCILobLocator* createLobLocator(Oci8Session& os);

} // namespace OciCpp
