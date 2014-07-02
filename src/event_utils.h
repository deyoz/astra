#ifndef EVENT_UTILS_H
#define EVENT_UTILS_H

#include <string>
#include <vector>
#include "astra_locale.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "stages.h"

class LEvntPrm {
  public:
    LEvntPrm() {}
    virtual ~LEvntPrm() {}
    virtual std::string GetMsg (const std::string& lang) const = 0;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const = 0;
    virtual LEvntPrm* MakeCopy () const = 0;
};

template <typename T> class PrmElem;
template <typename T> class PrmSmpl;
class PrmDate;
class PrmBool;
class PrmEnum;
class PrmFlight;
class PrmLexema;
class PrmStage;

class LEvntPrms: public std::deque<LEvntPrm*>
{
  public:
    LEvntPrms(): std::deque<LEvntPrm*>() {}
    LEvntPrms (const LEvntPrms& params) {
        for (std::deque<LEvntPrm*>::const_iterator iter=params.begin(); iter != params.end(); iter++)
            push_back((*iter)->MakeCopy());
    }
    virtual ~LEvntPrms() {
        for (std::deque<LEvntPrm*>::iterator iter=begin(); iter != end(); iter++)
            delete *iter;
    }
    LEvntPrms& operator = (const LEvntPrms& params);
    LEvntPrms& operator << (const PrmElem<int>&  prmelem);
    LEvntPrms& operator << (const PrmElem<std::string>&  prmelem);
    LEvntPrms& operator << (const PrmSmpl<int>&  prmsmpl);
    LEvntPrms& operator << (const PrmSmpl<double>&  prmsmpl);
    LEvntPrms& operator << (const PrmSmpl<std::string>&  prmsmpl);
    LEvntPrms& operator << (const PrmDate&  prmdate);
    LEvntPrms& operator << (const PrmBool&  prmbool);
    LEvntPrms& operator << (const PrmEnum&  prmenum);
    LEvntPrms& operator << (const PrmFlight&  prmflight);
    LEvntPrms& operator << (const PrmLexema&  prmlexema);
    LEvntPrms& operator << (const PrmStage&  prmstage);
    AstraLocale::LParams GetParams (const std::string& lang);
};

template <typename T>
class PrmElem:public LEvntPrm {
  private:
    std::string name;
    TElemType type;
    T id;
    TElemFmt fmt;
  public:
    PrmElem(const std::string& name, TElemType type, T id, TElemFmt fmt = efmtCodeNative):
        LEvntPrm(), name(name), type(type), id(id), fmt(fmt) {}
    virtual ~PrmElem() {}
    virtual std::string GetMsg (const std::string& lang) const{
        return ElemIdToPrefferedElem(type, id, fmt, lang);
    }
    virtual AstraLocale::LParam GetParam (const std::string& lang) const {
        return AstraLocale::LParam(name, GetMsg(lang));
    }
    virtual LEvntPrm* MakeCopy () const {
        return new PrmElem(*this);
    }
};

template <typename T>
class PrmSmpl:public LEvntPrm {
  private:
    std::string name;
    T param;
  public:
    PrmSmpl(const std::string& name, T param):
        LEvntPrm(), name(name), param(param) {}
    virtual ~PrmSmpl() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const {
        return AstraLocale::LParam(name, GetMsg(lang));
    }
    virtual LEvntPrm* MakeCopy () const {
        return new PrmSmpl(*this);
    }
};

class PrmDate:public LEvntPrm {
  private:
    std::string name;
    BASIC::TDateTime date;
    std::string fmt;
  public:
    PrmDate(const std::string& name, BASIC::TDateTime date, const std::string& fmt):
              LEvntPrm(), name(name), date(date), fmt(fmt) {}
    virtual ~PrmDate() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
};

class PrmBool:public LEvntPrm {
  private:
    std::string name;
    bool val;
  public:
    PrmBool(const std::string& name, bool val): LEvntPrm(), name(name), val(val) {}
    virtual ~PrmBool() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
};

class PrmEnum:public LEvntPrm {
  private:
    std::string name;
    std::string separator;
  public:
    LEvntPrms prms;
    PrmEnum(const std::string& name, const std::string& separator):
        LEvntPrm(), name(name), separator(separator), prms() {}

    PrmEnum(const PrmEnum& prmenum):
        LEvntPrm(), name(prmenum.name), separator(prmenum.separator), prms(prmenum.prms) {}

    virtual ~PrmEnum() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
};

class PrmFlight:public LEvntPrm {
  private:
    std::string name;
    std::string airline;
    int flt_no;
    std::string suffix;
  public:
    PrmFlight(const std::string& name, const std::string& airline, int flt_no, const std::string& suffix):
        LEvntPrm(), name(name), airline(airline),  flt_no(flt_no), suffix(suffix) {}
    virtual ~PrmFlight() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
};

class PrmLexema:public LEvntPrm {
  private:
    std::string name;
    std::string lexema_id;
  public:
    LEvntPrms prms;
    PrmLexema(const std::string& name, const std::string& lexema_id):
        LEvntPrm(), name(name), lexema_id(lexema_id), prms() {}
    PrmLexema(const PrmLexema& param):
        LEvntPrm(), name(param.name), lexema_id(param.lexema_id), prms(param.prms) {}
    virtual ~PrmLexema() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
    inline void ChangeLexemaId (std::string new_id) {
        lexema_id = new_id;
    };
};

class PrmStage:public LEvntPrm {
  private:
    std::string name;
    TStage stage;
    std::string airp;
  public:
    PrmStage(const std::string& name, TStage stage, const std::string& airp):
        LEvntPrm(), name(name), stage(stage), airp(airp) {}
    virtual ~PrmStage() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
};

struct TLogLocale {
  public:
    BASIC::TDateTime ev_time;
    int ev_order;
    ASTRA::TEventType ev_type;
    std::string lexema_id;
    LEvntPrms prms;
    int id1,id2,id3;
    std::vector<std::string> vlangs;
    TLogLocale(): ev_time(ASTRA::NoExists), ev_order(ASTRA::NoExists), ev_type(ASTRA::evtUnknown),
        lexema_id(""), prms(), id1(ASTRA::NoExists), id2(ASTRA::NoExists), id3(ASTRA::NoExists), vlangs() {
        vlangs.push_back(AstraLocale::LANG_RU);
        vlangs.push_back(AstraLocale::LANG_EN);
    }
};

int test_event_utils(int argc,char **argv);
int insert_locales(int argc,char **argv);
#endif // EVENT_UTILS_H
