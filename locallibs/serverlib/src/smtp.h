#ifndef _SMTP_H_
#define _SMTP_H_
#include <ctime>
#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include "exception.h"
#include "daemon_task.h"
#include "smtp_db_callbacks.h"

#if 0
Example
local_after.tcl
    set_local SMTP(server) mail.sirena-travel.ru
    set_local SMTP(username) myaddr@sirena2000.ru
    set_local SMTP(password) "XXXXX"
    set_local SMTP(from) [ list "MyName" myaddr@sirena2000.ru ]

    #include <smtp.h>
    using namespace SMTP;
    ifstream f("test1.txt");

    EmailMsg m("big",Address("asm@sirena2000.ru"),
            SMTP::MainPart(
                string (
                    (istreambuf_iterator<char>(f.rdbuf())),
                    istreambuf_iterator<char>() 
                   ) 
                )
            );
    ifstream f2("data.tgz");
    vector<char> v11(istreambuf_iterator<char>(f2.rdbuf()),istreambuf_iterator<char>());
    m.addPart(EmailMsg::binaryAttachment(v11,"data.tgz"));

    EmailHandler::saveMsg(m.createMsgText(),"big",true);
#endif    

namespace TclCpp {

    class Tcl_Exception : public comtech::Exception {
        public:
          Tcl_Exception(std::string const &s) : comtech::Exception(s) {}
            virtual ~Tcl_Exception() throw() {}

    };
    
    void TclExecCmd(std::string const &s);
    
    class TclCmd {
          std::vector <Tcl_Obj *> cmd;
        public:
          TclCmd();
          void add(TclObjHolder* o);
          void exec();
    };  
} // namespace TclCpp

namespace SMTP {
    enum ContentType
    {
      Text=0,
      Html
    };

    class EmailMsg;


    class Charset {
        std::string name_;
        public:
        std::string name() const
        {
            return name_;
        }
        Charset(std::string const &n):name_(n) { }
        public:
        static Charset UTF8()
        {
            return Charset("utf-8");
        }
        static Charset  CP1251()
        {
            return Charset("cp1251");
        }
        static Charset KOI8()
        {
            return Charset("koi8-r");
        }
        static Charset CP866()
        {
            return Charset("cp866");
        }
        bool operator != (const Charset &a) const { return name_!=a.name(); }
    };
    class MainPart;
    class EmailPart {
        friend class EmailMsg;
        friend class MainPart;
        std::vector<char> contents;
        std::string encoding;
        std::string type;
        bool attachment;
        bool binary;
        std::string name;

        EmailPart (std::vector<char> contents_, std::string const & encoding_ , std::string const & type_,
                bool attachment_ , bool binary_, std::string name_):
            contents(contents_),encoding(encoding_),type(type_),attachment(attachment_),binary(binary_),
            name(name_){}


    } ;

    class MainPart : public EmailPart {
        friend class EmailMsg;
        public:
        MainPart (std::string const &contents, ContentType ctype=Text )
            : EmailPart(std::vector<char>(contents.begin(),contents.end())  ,Charset::UTF8().name(),ctype==Text?"text/plain":"text/html",false,false,"") { }
    };
    class Address {
        std::string name;
        std::string address;
        public:
        Address(std::string const & n, std::string const &a):name(n),address(a){}
        Address(std::string const &a):name(""),address(a){}
        std::string getName()const
        {
            return name;
        }
        std::string getAddress()const
        {
            return address;
        }
    };   
    class EmailMsg {
        std::string subject;
        std::list<Address> addressList;
        std::list<EmailPart> partList;
        public:
        EmailMsg (std::string const & subject_, Address const &a,MainPart const &mainp):subject(subject_)
        {
            addressList.push_back(a);
            partList.push_back(mainp);
        }
        EmailMsg & additionalAddress(Address const &a)
        {
            addressList.push_back(a);
            return *this;
        }    
        EmailMsg & addPart(EmailPart const &e)
        {
            partList.push_back(e);
            return *this;
        }    
        std::string createMsgText ();
        static EmailPart binaryAttachment(std::vector<char> const &contents,std::string const &name)
        {
            return EmailPart(contents,"","application/octet-stream",true,true,name);
        }
        static EmailPart pdfAttachment(std::vector<char> const &contents,std::string const &name)
        {
            return EmailPart(contents,"","application/pdf",true,true,name);
        }
        static EmailPart textAttachment(std::vector<char> const &contents,Charset const &charset, std::string const &name)
        {
            return EmailPart(contents,charset.name(),"text/plain",true,false,name);
        }
        static EmailPart htmlAttachment(std::vector<char> const &contents,Charset const &charset, std::string const &name)
        {
            return EmailPart(contents,charset.name(),"text/html",true,true,name);
        }
        static EmailPart textHtmlAttachment(std::vector<char> const &contents, Charset const &charset, std::string const &name)
        {
            return EmailPart(contents, charset.name(), "text/html", true, false /*set binary flag to false, when it's true we have no charset*/, name);
        }

        static EmailPart xmlAttachment(std::vector<char> const &contents, Charset const &charset, std::string const &name)
        {
            return EmailPart(contents, charset.name(), "text/xml", true, false /*set binary flag to false, when it's true we have no charset*/, name);
        }
    
    };
    

    class EmailHandler
    {
        private:
            EmailHandler();
            static EmailMsgDbCallbacks *_callbacks;
        public:
          static bool callbacksSet() { return _callbacks != 0; }
          static void setCallbacks(EmailMsgDbCallbacks *callbacks);
          static EmailMsgDbCallbacks *callbacks();
          static void  send(std::vector<char> const &s, std::string const & server, std::string const &user,
            std::string const &password);
          static void  send(std::vector<char> const &s)
          {
              return send(s, "","","");
          }
          static bool loop (std::time_t,const size_t loop_max_count);
          static void markForSend(std::string const &id);
          static void deleteDelayed(std::string const &id);
          static std::string saveMsg(std::string const &txt,std::string const & type, bool send_now);
    };

    bool process_enable();

}

class SendMail :public ServerFramework::DaemonTask {
        virtual int run(const boost::posix_time::ptime&);
        virtual bool doNeedRepeat();
        bool needRepeat_;        
        virtual void monitorRequest();
    public:
        SendMail();
};

#endif
