#ifndef __QUERY_RUNNER_H_
#define __QUERY_RUNNER_H_

#include <string>
#include <vector>
#include <array>
#include <list>
#include <stdint.h>

#include "tclmon.h"
#include "mespro_crypt.h"
#include "monitor_ctl.h"
#include "sirena_queue.h"
#include "exception.h"
#include "EdiHelpManager.h"
#include "http_parser.h"

namespace ServerFramework
{

namespace Fcgi { class Request; class Response; }

class Obrzapnik;

class TclFuncRegister{
    struct fdesc
    {
        int (*fptr)(ClientData, Tcl_Interp *, int objc, Tcl_Obj *CONST objv[]);
        ClientData data;
        char const *name;
    };
    typedef std::list<TclFuncRegister::fdesc> flist_t;
    flist_t flist;
public:
    static TclFuncRegister &getInstance()
    {
        static TclFuncRegister instance;
        return instance;
    }

    void Register(char const *name,
            int (*fptr)(ClientData, Tcl_Interp *, int objc, Tcl_Obj *CONST objv[]),
            ClientData data )
    {
        flist.push_back({fptr, data, name});
    }

    void init(Tcl_Interp *interp)
    {
        for (flist_t::iterator i=flist.begin();i!=flist.end();++i){
            if(!Tcl_CreateObjCommand(interp,i->name,i->fptr,i->data,0)){
                fprintf(stderr,"%s\n",Tcl_GetString(Tcl_GetObjResult(interp)));
                abort();
            }
        }
    }
};

class ApplicationCallbacks
{
  protected:
    ApplicationCallbacks() {}
    virtual ~ApplicationCallbacks(){}
    friend class Obrzapnik;
  public:
    virtual void init();
    typedef std::array<uint8_t,205> Grp2Head;
    virtual std::tuple<Grp2Head, std::vector<uint8_t>> internet_proc(const Grp2Head& head, const std::vector<uint8_t>& body);
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len); //function to processing jxt
    virtual void fcgi_responder(Fcgi::Response&, const Fcgi::Request&);

    virtual void http_handle(HTTP::reply& rep, const HTTP::request& req);

    // function to processing text terminal
    // *len2 is the size of the buffer mes2 points to
    virtual int text_proc(const char *mes1, char *mes2, size_t len1, int *len2, int *err_code);

    virtual int message_control(int type /* 0 - request, 1 - answer */,
                                const char *head, int hlen,
                                const char *body, int blen);
    // cryptography
    virtual int read_pub_key(char const* /*who*/, char* /*key*/, int* /*key_len*/)
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual int read_pub_key(int /*who*/, char* /*key*/, int* /*key_len*/)
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual int read_sym_key(const char* /*who*/, unsigned char* /*key*/, size_t /*key_len*/, int* /*sym_key_id*/)
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual int read_sym_key(int /*who*/, unsigned char* /*key*/, size_t /*key_len*/, int* /*sym_key_id*/)
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual int getIClientID(const Grp2Head& /*s*/) const
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual int sym_key_id(const Grp2Head& /*head 205*/) const
    {
      throw comtech::Exception("Crypting not implemented");
    }
    virtual size_t form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error);
#ifdef USE_MESPRO
    virtual void getMesProParams(const char* /*head*/, int /*hlen*/, int* /*error*/, MPCryptParams &/*params*/)
    {
      throw comtech::Exception("MesPro crypting not implemented");
    }
#endif // USE_MESPRO
    virtual void connect_db(); //function to connect to database
    virtual int commit_db(); //commint
    virtual int rollback_db(); //rollabck
    virtual int tcl_init(Tcl_Interp *interp); // init tcl - tcl function declarartion etc
    virtual int tcl_start(Tcl_Interp *) // some usefull function calls, in watch just before loop
    {
        return TCL_OK;
    }

    virtual void on_exit() // what to do on exit from main process
    {
    }
    virtual void levC_app_init(void) // what to do to init obrzap
    {
    }
    virtual int nosir_proc(int argc,char **argv); //what to do in nosir mode
    virtual void help_nosir(); // help for nosir mode
    int run (int argc,char **argv);
};


class QueryRunner ;
QueryRunner const & getQueryRunner(void );
void setQueryRunner(QueryRunner const &);

class Obrzapnik
{
    std::vector<NAME2F> v;
    bool finished_;
    ApplicationCallbacks *ac_;

    friend QueryRunner const & getQueryRunner(void);
    friend void setQueryRunner(QueryRunner const &);
    friend void clearQueryRunner();
    template <class A> friend void setApplicationCallbacks();
    friend ApplicationCallbacks* applicationCallbacks();

    QueryRunner const *query_runner;
    QueryRunner const *empty_query_runner;
    Obrzapnik();
    ~Obrzapnik();

  public:
    static Obrzapnik *getInstance();

    ApplicationCallbacks *getApplicationCallbacks()
    {
        return ac_;
    }

    template <class A>
    void setApplicationCallbacks()
    {
        ApplicationCallbacks* ac = new A;
        ApplicationCallbacks* t_ac_ = ac_; // to avoid memleaks
        ac_ = ac;
        delete t_ac_;
    }

    Obrzapnik *add(const char* name_, const char* log_group_, ProcessHandlerType *pf_, short fl = 0)
    {
        if(finished_)
        {
            throw comtech::Exception("add process after getProcTable");
        }

        NAME2F rec = {name_, log_group_ ? log_group_ : std::string(), pf_, fl};

        v.push_back(rec);

        return this;
    }

    const NAME2F* getProcTable(int* len)
    {
        finished_=true;
        *len=v.size();

        return &v[0];
    }
};

inline ApplicationCallbacks* applicationCallbacks()
{
    return Obrzapnik::getInstance()->getApplicationCallbacks();
}

template <class A> void setApplicationCallbacks()
{
    Obrzapnik::getInstance()->setApplicationCallbacks<A>();
}

enum Tgroup { tg_unspecified=0, tg_text, tg_inet, tg_jxt, tg_http };

class QueryRunner
{
public:
    typedef std::shared_ptr<EdiHelpManager> EdiHelpManagerPtr;
    QueryRunner(EdiHelpManagerPtr e = EdiHelpManagerPtr());
    void setPult(std::string const &p) const;
    std::string pult() const;
    void setEdiHelpManager(EdiHelpManagerPtr e);
    EdiHelpManager& getEdiHelpManager() const;
    virtual ~QueryRunner();
    virtual Tgroup environment() const;
private:
    EdiHelpManagerPtr em;
    mutable std::string pult_;
};

QueryRunner EmptyQueryRunner();
QueryRunner TextQueryRunner();
QueryRunner InternetQueryRunner();
void setQueryRunner(QueryRunner const &q);
void clearQueryRunner(void);
QueryRunner const & getQueryRunner(void);

void registerTclCommand(char const *name,
            int (*fptr)(ClientData, Tcl_Interp *, int objc, Tcl_Obj *CONST objv[]),
            ClientData data );
} // namespace ServerFramework

#endif
