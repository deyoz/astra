#ifndef _EDIHELPMANAGER_H_
#define _EDIHELPMANAGER_H_
#include <string>
#include "monitor_ctl.h"

namespace ServerFramework
{

#ifdef XP_TESTING
/*
 * clear stored redisplay message
 */
void clearRedisplay();
/*
 * start listening for redisplay messages
 */
void listenRedisplay();
/**
  * get last stored redisplay message
 */
std::string getRedisplay();
/**
  * store redisplay message. internal use only.
 */
void setRedisplay(const std::string& redisp);

class InternalMsgId;
void imitate_confirm_notify_oraside_for_bloody_httpsrv(const InternalMsgId& msgid, const std::string& signal);
#endif /*XP_TESTING*/

class InternalMsgId;
class QueryRunner;
class EdiHelpManager
{
    virtual std::string make_text(std::string const &s)
    {
        return s;
    }
    QueryRunner  const *my_query_runner;
    int flagPerespros;
    int saved_flag;
    int old_timeout;
    int max_timeout;
    bool one_pult_many_msgid;
public:
    static const int minimum_timeout   = 5;
    static const int timeout_difference= 3;
    void setQueryRunner( QueryRunner const *qr)
    {
        my_query_runner = qr;
    }

    explicit EdiHelpManager (int f1)
    : flagPerespros(f1), saved_flag(0), old_timeout(0), max_timeout(0),
      one_pult_many_msgid(false)
    {
    }

    virtual ~EdiHelpManager() {}
    void setDebug();
    bool mustWait() const;
    void removeMustWaitFlag();
    void configForPerespros(const char *nick, const char *file, int line,
                            const char * requestText, int session_id, int timeout);
    int maxTimeout() const { return max_timeout; }
    void multiMsgidMode( bool value ) { one_pult_many_msgid=value; }

    /**
     * @brief internal levB msg ID
     * @return
     */
    InternalMsgId msgId() const;

    /**
      * @brief Application 3 letter instance name. For sirena: CENTER_NAME
    */
    static const std::string &instanceName();

    static void confirm_notify(const char *pult, int session_id = 0);
    static void confirm_notify(const InternalMsgId& msgid, int session_id = 0);
    /**
      * В случае если нужно послать новый запрос во время обработки ответа на первый,
      * нужно клонировать запись в edi_help с новым значением edisession
    */
    static bool copyEdiHelpWithNewEdisession(const InternalMsgId& intmsgid, int session_id, int new_session_id);

    static void cleanOldRecords();
};
}
namespace comtech = ServerFramework;
#endif /*_EDIHELPMANAGER_H_*/
