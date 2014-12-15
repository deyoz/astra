#ifndef ASTRA_LOCALE_ADV_H
#define ASTRA_LOCALE_ADV_H

#include "astra_elems.h"
#include "stages.h"
#include "xml_unit.h"

class LEvntPrm {
  public:
    LEvntPrm() {}
    virtual ~LEvntPrm() {}
    virtual std::string GetMsg (const std::string& lang) const = 0;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const = 0;
    virtual LEvntPrm* MakeCopy () const = 0;
    virtual void ParamToXML(xmlNodePtr paramNode) const = 0;
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
    void clearPrms () {
        for (std::deque<LEvntPrm*>::iterator iter=begin(); iter != end(); iter++)
            delete *iter;
        clear();
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
    AstraLocale::LParams GetParams (const std::string& lang) const;
    void toXML(xmlNodePtr eventNode) const;
    void fromXML(xmlNodePtr paramsNode);
};

template <typename T>
class PrmElem:public LEvntPrm {
  private:
    std::string name;
    TElemType elem_type;
    T id;
    TElemFmt fmt;
  public:
    PrmElem(const std::string& name, TElemType type, T id, TElemFmt fmt = efmtCodeNative):
        LEvntPrm(), name(name), elem_type(type), id(id), fmt(fmt) {}
    virtual ~PrmElem() {}
    virtual std::string GetMsg (const std::string& lang) const{
        return ElemIdToPrefferedElem(elem_type, id, fmt, lang);
    }
    virtual AstraLocale::LParam GetParam (const std::string& lang) const {
        return AstraLocale::LParam(name, GetMsg(lang));
    }
    virtual LEvntPrm* MakeCopy () const {
        return new PrmElem(*this);
    }
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        std::string type = std::string("PrmElem<") + typeid(T).name() + std::string(">");
        NewTextChild(paramNode, "type", type);
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "elem_type", elem_type);
        NewTextChild(paramNode, "id", id);
        NewTextChild(paramNode, "fmt", fmt);
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        std::string type = std::string("PrmSmpl<") + typeid(T).name() + std::string(">");
        NewTextChild(paramNode, "type", type);
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "value", param);
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmDate");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "date", date);
        NewTextChild(paramNode, "fmt", fmt);
    }
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmBool");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "val", val);
    }
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmEnum");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "separator", separator);
        prms.toXML(paramNode);
    }
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmFlight");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "airline", airline);
        NewTextChild(paramNode, "flt_no", flt_no);
        NewTextChild(paramNode, "suffix", suffix);
    }
};

class PrmLexema:public LEvntPrm {
  private:
    std::string name;
    std::string lexema_id;
  public:
    LEvntPrms prms;
    PrmLexema(const std::string& name, const std::string& lexema_id):
        LEvntPrm(), name(name), lexema_id(lexema_id), prms() {}
    PrmLexema(const std::string& name, const std::string& lexema_id, const LEvntPrms& prms):
        LEvntPrm(), name(name), lexema_id(lexema_id), prms(prms) {}
    PrmLexema(const PrmLexema& param):
        LEvntPrm(), name(param.name), lexema_id(param.lexema_id), prms(param.prms) {}
    virtual ~PrmLexema() {}
    virtual std::string GetMsg (const std::string& lang) const;
    virtual AstraLocale::LParam GetParam (const std::string& lang) const;
    virtual LEvntPrm* MakeCopy () const;
    inline void ChangeLexemaId (std::string new_id) {
        lexema_id = new_id;
    };
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmLexema");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "lexema_id", lexema_id);
        prms.toXML(paramNode);
    }
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
    virtual void ParamToXML(xmlNodePtr paramNode) const {
        NewTextChild(paramNode, "type", "PrmStage");
        NewTextChild(paramNode, "name", name);
        NewTextChild(paramNode, "stage", stage);
        NewTextChild(paramNode, "airp", airp);
    }
};

namespace AstraLocale {
class UserException:public EXCEPTIONS::Exception
{
    private:
        std::string lexema_id;
        LParams lparams;
        LEvntPrms advParams;
        int FCode;
        bool useAdvParams;
  protected:
    void setLexemaData( const LexemaData &data) {
        lexema_id = data.lexema_id;
        lparams = data.lparams;
    }
    public:
    int Code() { return FCode; }
    UserException( int code, const std::string &vlexema, const LParams &aparams):EXCEPTIONS::Exception(vlexema)
    {
        lparams = aparams;
        lexema_id = vlexema;
        FCode = code;
        useAdvParams=false;
    }
    UserException( const std::string &vlexema, const LParams &aparams):EXCEPTIONS::Exception(vlexema)
    {
        lparams = aparams;
        lexema_id = vlexema;
        FCode = 0;
        useAdvParams=false;
    }
    UserException( int code, const std::string &vlexema, const LEvntPrms &aparams):EXCEPTIONS::Exception(vlexema)
    {
        advParams = aparams;
        lexema_id = vlexema;
        FCode = code;
        useAdvParams=true;
    }
    UserException( const std::string &vlexema, const LEvntPrms &aparams):EXCEPTIONS::Exception(vlexema)
    {
        advParams = aparams;
        lexema_id = vlexema;
        FCode = 0;
        useAdvParams=true;
    }
    UserException( int code, const std::string &vlexema):EXCEPTIONS::Exception(vlexema)
    {
        lexema_id = vlexema;
        FCode = code;
    }
    UserException( const std::string &vlexema):EXCEPTIONS::Exception(vlexema)
    {
        lexema_id = vlexema;
        FCode = 0;
    }
    LexemaData getLexemaData( ) const {
        LexemaData data;
        data.lexema_id = lexema_id;
        if (useAdvParams)
          data.lparams = advParams.GetParams(LANG_RU);
        else
          data.lparams = lparams;
        return data;
    }
    void getAdvParams(std::string &lexema, LEvntPrms &aparams)
    {
        lexema = lexema_id;
        if (!useAdvParams) {
          for(std::map<std::string, boost::any>::iterator iter = lparams.begin(); iter != lparams.end(); iter++) {
            if ( boost::any_cast<std::string>(&(iter->second)))
              aparams << PrmSmpl<std::string>(iter->first, boost::any_cast<std::string>(iter->second));
            else if ((iter->second).type() != typeid(int))
              aparams << PrmSmpl<int>(iter->first, boost::any_cast<int>(iter->second));
            else if ((iter->second).type() != typeid(double))
              aparams << PrmSmpl<double>(iter->first, boost::any_cast<double>(iter->second));
          }
        }
        else
            aparams=advParams;
    }

    virtual const char* what() const throw()
    {
      LexemaData lexemeData=getLexemaData();
      if (lexemeData.lexema_id.empty()) return EXCEPTIONS::Exception::what();
      std::string text, master_lexema_id;
      try
      {
        buildMsg( LANG_EN, lexemeData, text, master_lexema_id );
        return text.c_str();
      }
      catch (...) {};
      try
      {
        buildMsg( LANG_RU, lexemeData, text, master_lexema_id );
        return text.c_str();
      }
      catch (...) {};
      return lexemeData.lexema_id.c_str();
    }
    virtual ~UserException() throw(){}
};
} // end namespace astraLocale

class UserException2:public AstraLocale::UserException
{
  public:
    UserException2(): UserException(""){}
};

int test_astra_locale_adv(int argc,char **argv);
int insert_locales(int argc,char **argv);
void LocaleToXML (const xmlNodePtr node, const std::string& lexema_id, const LEvntPrms& params);
void LocaleFromXML (const xmlNodePtr node, std::string& lexema_id, LEvntPrms& params);

#endif // ASTRA_LOCALE_ADV_H
