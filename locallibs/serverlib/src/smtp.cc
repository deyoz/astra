#include <iostream>
#include <list>
#include <string>
#include <exception>
#include <cstdlib>
#include <tcl.h>
#include "tclmon.h"
#include "tcl_utils.h"
#include "smtp.h"
#include "dates.h"
#include "cursctl.h"
#include "helpcpp.h"
#include "ourtime.h"
#include "new_daemon.h"
#include "daemon_event.h"
#include "smtp_dbora_callbacks.h"
#define NICKNAME "SYSTEM"
#include "slogger.h"

namespace TclCpp {

    void TclExecCmd(std::string const &s)
    {
        TclObjHolder set_cmd(Tcl_NewStringObj(s.c_str(),s.length()));
        int ret=Tcl_EvalObjEx(getTclInterpretator(),set_cmd.obj,TCL_EVAL_GLOBAL);
        if(ret!=TCL_OK){
            int len;
            char* str = Tcl_GetStringFromObj(Tcl_GetObjResult(getTclInterpretator()), &len);
            throw Tcl_Exception(std::string(str,len));
        }
    }
        
    TclCmd::TclCmd()
    {
        cmd.reserve(20);
    }
    
    void TclCmd::add(TclObjHolder* o)
    {
        cmd.push_back(o->obj);
    }
    
    void TclCmd::exec()
    {
        int ret=Tcl_EvalObjv(getTclInterpretator(),cmd.size(),&cmd[0],TCL_EVAL_GLOBAL);
        if(ret!=TCL_OK)
        {
            int len;
            char* str = Tcl_GetStringFromObj(Tcl_GetObjResult(getTclInterpretator()), &len);
            throw Tcl_Exception(std::string(str,len));
        }
    }
}

using namespace TclCpp;

namespace SMTP {



    std::string EmailMsg::createMsgText ()
    {
        TclObjHolder parts(Tcl_NewObj());
        std::string bytes;
        Tcl_Interp* tcl = getTclInterpretator();
        int len;
        for (std::list<EmailPart>::iterator li=partList.begin();li!=partList.end();++li){
            TclCmd c;

            bytes = "sirena::mail::create_part";
            len = bytes.length();
            TclObjHolder com(Tcl_NewStringObj(bytes.c_str(), len));
            c.add(&com);

            len = li->contents.size();
            TclObjHolder cont(Tcl_NewByteArrayObj(reinterpret_cast<unsigned const char*>(li->contents.data()), len));
            c.add(&cont);

            bytes = li->type;
            TclObjHolder tp(Tcl_NewStringObj(bytes.c_str(), bytes.length()));
            c.add(&tp);

            bytes = li->encoding;
            TclObjHolder encd(Tcl_NewStringObj(bytes.c_str(), bytes.length()));
            c.add(&encd);

            TclObjHolder bin(Tcl_NewIntObj((int)(li->binary)));
            TclObjHolder att(Tcl_NewIntObj((int)(li->attachment)));
            c.add(&bin);
            c.add(&att);

            bytes = li->name;
            TclObjHolder nm(Tcl_NewStringObj(bytes.c_str(), bytes.length()));
            c.add(&nm);
            c.exec();
            Tcl_ListObjAppendElement(tcl, parts.obj, Tcl_GetObjResult(tcl));
        }

        TclObjHolder addrlist(Tcl_NewObj());
        for(std::list<Address>::iterator i = addressList.begin(); i!=addressList.end(); ++i){
            TclObjHolder pair(Tcl_NewObj());
            TclObjHolder name(Tcl_NewStringObj(i->getName().c_str(), i->getName().length()));
            TclObjHolder addr(Tcl_NewStringObj(i->getAddress().c_str(), i->getAddress().length()));
            Tcl_ListObjAppendElement(tcl, pair.obj, name.obj);
            Tcl_ListObjAppendElement(tcl, pair.obj, addr.obj);
            Tcl_ListObjAppendElement(tcl, addrlist.obj, pair.obj);
        }

        TclCmd c;

        bytes = "sirena::mail::create_msg";
        len = bytes.length();
        TclObjHolder str(Tcl_NewStringObj(bytes.c_str(), len));
        TclObjHolder sbj(Tcl_NewStringObj(subject.c_str(), subject.length()));
        TclObjHolder empty(Tcl_NewStringObj("", 0));
        c.add(&str);
        c.add(&sbj);
        c.add(&empty);
        c.add(&addrlist);
        c.add(&parts);
        c.exec();
        //We need to free memory consumed by addrlist's sublists (aka pairs)
       
        Tcl_Obj* tmp;
        int index = 0, result;
        do{    
            //next pair in the list. Index() doesnt't increment refCount of object
            result = Tcl_ListObjIndex(tcl, addrlist.obj, index++, &tmp); 
            //actual deletion of name and addr tcl objects
            if(!tmp)
                break;
            Tcl_ListObjReplace(tcl, tmp, 0, 2, 0, NULL);
        }while(result == TCL_OK);
    
        std::string ResultMsg(Tcl_GetStringResult(tcl));
        return ResultMsg;
    }

    EmailMsgDbCallbacks *EmailHandler::_callbacks = 0;
    EmailMsgDbCallbacks *EmailHandler::callbacks()
    {
        if(_callbacks == 0)
            _callbacks = new EmailMsgDbOraCallbacks(0,0,0);
        return _callbacks;
    }
    void EmailHandler::setCallbacks(EmailMsgDbCallbacks *callbacks)
    {
        if(_callbacks)
            delete _callbacks;
        _callbacks = callbacks;
    }

    std::string EmailHandler::saveMsg(const std::string& txt, const std::string& type, bool send_now)
    {
        return callbacks()->saveMsg(txt, type, send_now);
    }

    void EmailHandler::send(std::vector<char> const &s, std::string const & server,
                            std::string const &user, std::string const &password)
    {

        TclCmd c;
        std::string command("sirena::mail::send");
        TclObjHolder com(Tcl_NewStringObj(command.c_str(),command.length()));
        TclObjHolder str(Tcl_NewByteArrayObj(reinterpret_cast<unsigned const char*>(&s[0]), s.size()));
        TclObjHolder srv(Tcl_NewStringObj(server.c_str(),server.length()));
        TclObjHolder usr(Tcl_NewStringObj(user.c_str(), user.length()));
        TclObjHolder pswd(Tcl_NewStringObj(password.c_str(),password.length()));
        c.add(&com);
        c.add(&str);
        c.add(&srv);
        c.add(&usr);
        c.add(&pswd);
        c.exec();

    }
    void EmailHandler::markForSend(std::string const &id)
    {
        callbacks()->markMsgForSend(id);
        ProgTrace(TRACE1,"EmailHandler::markForSend: '%s'",id.c_str());
    }
    void EmailHandler::deleteDelayed(std::string const &id)
    {
        callbacks()->deleteDelayed(id);
    }

    // Returns FALSE if all done, TRUE if need repeat
    bool EmailHandler::loop(time_t interval,const size_t loop_max_count)
    {
        auto msg_id_list = callbacks()->msgListToSend(interval, loop_max_count);

        size_t loop_count=0;
        for(auto &&msgid: msg_id_list) {
            LogWarning(STDLOG) << "SMTP::EmailHandler::loop fetch" ;

            const auto data = callbacks()->readMsg(msgid.id, msgid.length);

            try {
                LogWarning(STDLOG) << "SMTP::EmailHandler::loop send msg_id: "<< msgid.id;
                send (data);
                callbacks()->markMsgSent(msgid.id);
                LogWarning(STDLOG) << "SMTP::EmailHandler::loop send ok msg_id: "<< msgid.id;
            }catch (const Tcl_Exception &e){
                const std::string err_text = std::string(e.what()).substr(0,1900);
                LogError (STDLOG) << "msg_id='" << msgid.id << "'"
                                  << " error: " << e.what();
                callbacks()->markMsgSentError(msgid.id, 1, err_text);
                LogWarning(STDLOG) << "SMTP::EmailHandler::loop send error msg_id: " << msgid.id;
            }
            callbacks()->commit();
            LogWarning(STDLOG) << "Commited";
            monitor_working_zapr_type(1,QUEPOT_NULL);
            ++loop_count;
        }
        LogWarning(STDLOG) << "loop_count="<<loop_count<< " loop_max_count="<<loop_max_count;
        return loop_count==loop_max_count;

    }

    bool process_enable()
    {
        return EmailHandler::callbacks()->processEnable();
    }
}

using namespace ServerFramework;
int SendMail::run(const boost::posix_time::ptime&)
{
    try {
      needRepeat_ = false;

      const size_t loop_max_count=2000;
      LogWarning(STDLOG) << "SMTP::EmailHandler::loop "<<loop_max_count;
      if (SMTP::EmailHandler::loop(72000,loop_max_count))
      {
        needRepeat_ = true;
      }
    } catch (std::exception &e ) {
        LogError(STDLOG) << e.what();
    } catch (...) {
        LogError(STDLOG) << "unknow exception";
    }
    return 0;
}

bool SendMail::doNeedRepeat() 
{
   return needRepeat_;
}

void SendMail::monitorRequest()
{
  monitor_idle();
}

SendMail::SendMail() : DaemonTask(DaemonTaskTraits::OracleAndHooks()) 
{
}

int main_smtp(int supervisorSocket, int argc, char **argv)
{
    NewDaemon d("SMTP");
    DaemonEventPtr timerEv(new TimerDaemonEvent(20));
    timerEv->addTask(DaemonTaskPtr(new SendMail));
    d.addEvent(timerEv);
    d.run();
    return 0;
}
