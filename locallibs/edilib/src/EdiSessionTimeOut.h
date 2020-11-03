//
// C++ Interface: EdiSessionTimeOut
//
// Description:
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _EDILIB_EDISESSIONTIMEOUT_H_
#define _EDILIB_EDISESSIONTIMEOUT_H_
#include <list>
#include "serverlib/int_parameters.h"
#include "edilib/edi_types.h"
#include "edilib/EdiSessionId_t.h"
#include "edilib/edi_session.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace edilib
{

boost::posix_time::ptime getTimeOut(int to);

int expiredSecs(const boost::posix_time::ptime &d);

class EdiSessionTimeOut
{
    EdiSessionId_t EdiSessionId;
    edi_msg_types_t MsgType;
    std::string FuncCode;
    int Timeout;
public:
    EdiSessionTimeOut(edi_msg_types_t msg_type,
                      const std::string &func_code,
                      EdiSessionId_t,
                      int timeout);

    /**
     * @brief adds record to the DB
     * @param msg_type
     * @param func_code
     * @param
     * @param timeout
     */
    static void add(edi_msg_types_t msg_type,
                    const std::string &func_code,
                    EdiSessionId_t,
                    int timeout);

    /**
     * @brief delete record from DB by edisession id
     * @param
     */
    static void deleteDb(EdiSessionId_t);

    /**
     * @brief save all to the db
     */
    void writeDb();

    /**
     * @brief Delete edisession record
     */
    void deleteEdiSession() const;

    /**
     * @brief edifact session
     * @return
     */
    EdiSessionId_t ediSessionId() const { return EdiSessionId; }
    /**
     * @brief message type
     * @return
     */
    edi_msg_types_t msgType() const {return MsgType; }

    /**
     * @brief msg type as string
     * @return
     */
    std::string msgTypeStr() const;
    /**
     * @brief edifact message func code
     * @return
     */
    const std::string &funcCode() const { return FuncCode; }
    /**
     * @brief Answer type
     * @return
     */
    edi_msg_types_t answerMsgType() const;

    /**
     * @brief time out in seconds
     * @return
     */
    int timeout() const { return Timeout; }

    /**
     * @brief A list of expired sessions
     */
    static void readExpiredSessions(std::list<EdiSessionTimeOut> &lExpiredList);

    /**
     * @brief reads by ID
     * @param
     */
    static EdiSessionTimeOut readById(EdiSessionId_t Id);

    /**
     * @brief locks record
     * @return returns true if locked or false if not found or locked by another process
     */
    EdiSession::ReadStatus lock() const;

    static bool exists(EdiSessionId_t Id);
};

std::ostream  &operator << (std::ostream &s, const EdiSessionTimeOut &edisess);

}

#endif /*_EDILIB_EDISESSIONTIMEOUT_H_*/
