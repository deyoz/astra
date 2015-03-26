//
// C++ Interface: RemoteResults
//
// Description: remote request results
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _REMOTERESULTS_H_
#define _REMOTERESULTS_H_
#include <list>
#include <iosfwd>
#include <edilib/EdiSessionId_t.h>
#include <serverlib/base_code_set.h>
#include <boost/shared_ptr.hpp>
#include "astra_dates.h"
#include "CheckinBaseTypes.h"

namespace edifact
{

class RemoteStatusElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
    public:
        static const char *ElemName;
        RemoteStatusElem(int codeI, const char *code,
                        const char *ldesc,
                        const char *rdesc) throw()
        :CodeListData_t(codeI, code, code, rdesc, ldesc)
        {
        }
        virtual ~RemoteStatusElem(){}
};

class RemoteStatus : public BaseTypeElemHolder<RemoteStatusElem>
{
    RemoteStatus():TypeElemHolder(){}
public:
    typedef BaseTypeElemHolder<RemoteStatusElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap RemoteStatusMap;

    enum Status_t
    {
        Timeout,
        Contrl,
        CommonError,
        Success,
        RequestSent,
    };

    RemoteStatus(Status_t t):TypeElemHolder(t){}
    RemoteStatus(const std::string &status):TypeElemHolder(status){};

};

class RemoteResults;
typedef boost::shared_ptr<RemoteResults> pRemoteResults;
class RemoteResults
{
    RemoteResults():Status(RemoteStatus::RequestSent){}
    //void removeOld() const;
private:
    RemoteResults(const std::string &msgId,
                  const std::string &pult,
                  const edilib::EdiSessionId_t &edisess,
                  const Ticketing::SystemAddrs_t &remoteId);
public:
    /**
     * @brief add record
     * @param pult
     * @param edisess
     * @param remoteId
     */
    static void add(const std::string &msgId,
                    const std::string &pult,
                    const edilib::EdiSessionId_t &edisess,
                    const Ticketing::SystemAddrs_t &remoteId);    

    /**
     * @brief to whom
     * @return
     */
    const std::string &msgId() const { return MsgId; }

    const std::string &pult() const { return Pult; }

    const RemoteStatus &status() const { return Status; }

    /**
     * @brief error code returned on 0 level in error report answer
     * @return
     */
    const std::string &ediErrCode() const { return EdiErrCode; }

    /**
     * @brief remark from remote system
     * @return
     */
    const std::string &remark() const { return Remark; }

    /**
     * @brief tlg source
     * @return
     */
    const std::string &tlgSource() const { return TlgSource; }

    /**
     * @brief edifact session id
     * @return
     */
    const edilib::EdiSessionId_t &ediSession() const { return EdiSession; }

    /**
     * @brief remote system settings id
     * @return
     */
    const Ticketing::SystemAddrs_t &remoteSystem() const { return RemoteSystem; }

    /**
     * @brief creation date
     * @return
     */
    const Dates::DateTime_t &dateCr() const { return DateCr; }

    /**
     * @brief reads all results by levb msg Id
     * @param lres
     */
    static void readDb(std::list<RemoteResults> &lres);

    static pRemoteResults readSingle();

    /**
     * @brief reads by edifact session Id
     * @param Id
     * @return
     */
    static pRemoteResults readDb(const edilib::EdiSessionId_t &Id);

    /**
     * @brief write data
     */
    void writeDb();

    void updateDb() const;    

    void deleteDb(const edilib::EdiSessionId_t &Id);

    static void cleanOldRecords(const int min_ago);

    void setEdiErrCode(const std::string &err)
    {
        EdiErrCode = err.substr(0,3);
    }
    void setRemark(const std::string &rem)
    {
        Remark = rem.substr(0,140);
    }
    void setTlgSource(const std::string &tlg)
    {
        TlgSource = tlg;
    }
    void setStatus(const RemoteStatus &st)
    {
        Status = st;
    }
private:
    friend struct defsupport;
    std::string MsgId;
    std::string Pult;
    std::string EdiErrCode;
    std::string Remark;
    std::string TlgSource;
    edilib::EdiSessionId_t EdiSession;
    Dates::DateTime_t DateCr;
    RemoteStatus Status;
    Ticketing::SystemAddrs_t RemoteSystem;
};

std::ostream & operator << (std::ostream& os, const RemoteResults &r);

}

#endif /*_REMOTERESULTS_H_*/
