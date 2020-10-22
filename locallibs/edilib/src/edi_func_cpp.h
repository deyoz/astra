#ifndef _EDI_FUNC_CPP_H_
#define _EDI_FUNC_CPP_H_

#ifdef __cplusplus
#include <boost/lexical_cast.hpp>
#include "edi_user_func.h"
#include "edi_func.h"

#include <string>
#include <stdarg.h>
#include "edi_except.h"

namespace edilib {
using namespace std;
typedef std::string EdiErr_t;

class Exception : public EdiExcept
{
    EdiErr_t ErrCode;
    public:
        Exception(const std::string &msg, const EdiErr_t &err = ""):
            EdiExcept(msg), ErrCode(err)
        {
        }
        virtual const EdiErr_t &errCode() const
        {
            return ErrCode;
        }
        virtual bool hasErrCode() const
        {
            return errCode().size()>0;
        }
        virtual ~Exception() throw(){}
};

enum { // Element types
    DataElementType=1,
    CompElementType,
    SegmElementType,
    SegGrElementType,
    FullDataElementType,
};

class BaseBadCast : public Exception
{
    public:
        BaseBadCast(const std::string &msg, const EdiErr_t &err = "")
            :Exception(msg,err)
        {
        }
        virtual const string & dataStr() const = 0;
        virtual const string & name () const = 0; //number
        virtual const string & path() const = 0;
        virtual int type () const = 0;

        virtual ~BaseBadCast() throw () {}
};

class BaseEdiElement {
    public:
        virtual ~BaseEdiElement() {};
        virtual int type() const =0;
        virtual const char *name() const = 0;
        virtual const string & path() const =0;
};

class BaseFilledDataElement : public BaseEdiElement{
    public:
        virtual ~BaseFilledDataElement() {}
        virtual const string &dataStr() const =0;
};

typedef BaseFilledDataElement BaseFDE;

class DataElement : public BaseEdiElement{
    int Data;
    int Num;
    int NumInSequence;
    mutable string Path;
    public:
    explicit DataElement(int data=0, int num=0, int NumSeq=-1):
        Data(data),Num(num),NumInSequence(NumSeq)
    {
    }

    const char *name () const { return "Data Element"; }
    int type () const { return DataElementType; }
    int data() const { return Data; }
    int num() const { return Num; }
    int numInSequence() const { return NumInSequence; }
    const string & path() const;
};

class FilledDataElement : public BaseFilledDataElement, public DataElement {
    string DataStr;
    public:
        FilledDataElement (int data, int num, const string & dataStr):
            DataElement(data, num), DataStr(dataStr) { }
        FilledDataElement (const DataElement &dataElem, const string & dataStr):
                DataElement(dataElem), DataStr(dataStr) { }

        virtual const string & dataStr() const { return DataStr; }
        virtual const string & path () const { return DataElement::path(); }
        virtual const char * name () const { return  DataElement::name(); }
        virtual int type () const { return DataElement::type(); }
        virtual ~FilledDataElement() { }
};

class CompElement : public BaseEdiElement{
    string Comp;
    int Num;
    mutable string Path;
    public:
        explicit CompElement(const string &comp="", int num=0): Comp(comp), Num(num), Path("") { }
        explicit CompElement(const char *comp, int num=0): Comp(comp?comp:""), Num(num), Path("") { }
        const char *name () const { return "Composite Element"; }
        int type () const { return CompElementType; }
        const string & comp() const { return Comp; }
        int num() const { return Num; }
        const string & path() const;
};

class SegmElement : public BaseEdiElement
{
    string Segm;
    int Num;
    mutable string Path;
    public:
    explicit SegmElement(const string &segm="", int num=0): Segm(segm), Num(num), Path("") { }
    explicit SegmElement(const char *segm, int num=0): Segm(segm?segm:""), Num(num), Path("") { }
    const char *name () const { return "Segment Element"; }
    int type () const { return SegmElementType; }
    const string & segm() const { return Segm; }
    int num() const { return Num; }
    const string & path() const;
};

class SegGrElement : public BaseEdiElement{
    int SegGr;
    int Num;
    mutable string Path;
    public:
        explicit SegGrElement(int segGr=0, int num=0): SegGr(segGr), Num(num), Path("") {}
        const char *name () const { return "Segment Group"; }
        int type () const { return SegGrElementType; }
        const string & path() const;
        int segGr() const { return SegGr; }
        int num() const { return Num; }
};

class FullDataElement : public BaseEdiElement{
    DataElement Data;
    CompElement Comp;
    SegmElement Segm;
    SegGrElement SegGr;

    mutable string Path;
    public:
        explicit FullDataElement(const DataElement &data = DataElement(),
                        const CompElement &comp = CompElement(),
                        const SegmElement &segm = SegmElement(),
                        const SegGrElement &segGr = SegGrElement()):
            Data(data), Comp(comp), Segm(segm), SegGr(segGr), Path("")
            { }

            FullDataElement(int data=0, int dnum=0,
                            const string &comp="", int cnum=0,
                            const string &segm="", int snum=0,
                            int segGr=0, int sgnum=0)
            :
            Data(DataElement(data, dnum)),  Comp(CompElement(comp, cnum)),
            Segm(SegmElement(segm, snum)),  SegGr(SegGrElement(segGr, sgnum)), Path("")
            { }

            //Послед конструкторы защищают от NULL'a
            FullDataElement(int data, int dnum,
                            const char *comp, int cnum,
                            const char *segm, int snum=0,
                            int segGr=0, int sgnum=0)
    :
            Data(DataElement(data, dnum)),  Comp(CompElement(comp, cnum)),
            Segm(SegmElement(segm, snum)),  SegGr(SegGrElement(segGr, sgnum)), Path("")
            { }

            FullDataElement(int data, int dnum,
                            const char *comp, int cnum=0,
                            const string &segm="", int snum=0,
                            int segGr=0, int sgnum=0)
    :
            Data(DataElement(data, dnum)),  Comp(CompElement(comp, cnum)),
            Segm(SegmElement(segm, snum)),  SegGr(SegGrElement(segGr, sgnum)), Path("")
            { }

            const char *name () const { return "Data Element"; }
            int type () const { return FullDataElementType; }
            const string & path() const {
                if(!Path.size()){
                    Path =  SegGr.path() + (SegGr.path().size()?".":"") +
                            Segm.path() + (Segm.path().size()?".":"") +
                            Comp.path() + (Comp.path().size()?".":"") +
                            Data.path();
                }
                return Path;
            }
            const SegGrElement & segGr() const { return SegGr; }
            const SegmElement & segm() const { return Segm; }
            const CompElement & comp() const { return Comp; }
            const DataElement & data() const { return Data; }
};

class FilledFullDataElement : public FullDataElement, public BaseFilledDataElement {
    string DataStr;
    public:
        explicit FilledFullDataElement(const DataElement &data,
                              const CompElement &comp,
                              const SegmElement &segm,
                              const SegGrElement &segGr,
                              const string & dataStr):
            FullDataElement(data,comp,segm,segGr), DataStr(dataStr) { }

        virtual const string & dataStr() const { return DataStr; }
        virtual const string & path () const { return FullDataElement::path(); }
        virtual const char * name () const { return  FullDataElement::name(); }
        virtual int type () const { return FullDataElement::type(); }

        virtual ~FilledFullDataElement() { }
};

class BadCast : public BaseBadCast
{
    BaseFDE *Elem;
    std::string Name;
public:
    BadCast(const BaseFDE & elem, const std::string &name, const EdiErr_t &err) throw ();
    virtual const string & dataStr () const { return Elem->dataStr(); }
    virtual const string & name () const { return Name; }
    virtual const string & path () const { return Elem->path(); }
    virtual int type () const { return Elem->type(); }
    virtual const BaseFDE * dataElem () const { return Elem; }

    virtual ~BadCast() throw () { delete Elem; }
};


inline string GetFullStrDataPath(const DataElement &Data,
                                 const CompElement &Comp = CompElement(),
                                 const SegmElement &Segm = SegmElement(),
                                 const SegGrElement &SegGr = SegGrElement())
{
    FullDataElement Fdata(Data, Comp, Segm, SegGr);
    return Fdata.path();
}

class BaseNoSuchElement : public Exception
{
    public:
        BaseNoSuchElement(const EdiErr_t &err):
            Exception("", err)
        {
        }

        virtual const char * name() const =0;
        virtual const string & path () const =0;
        virtual int type () const =0;

        virtual ~BaseNoSuchElement() throw (){}
};

template <class Thing> class NoSuchElement : public BaseNoSuchElement
{
    Thing thing;
    public:
        NoSuchElement( const Thing &elem, const EdiErr_t &err ) :
            BaseNoSuchElement(err), thing(elem)
        {
            string str;
            str = "Missing " + string(thing.name()) + " [" +  thing.path() + "]";
            setMessage(str);
        }
        const Thing &element () const {return thing;}
        virtual const char *name () const { return thing.name(); }
        virtual const string & path () const { return thing.path(); }
        virtual int type () const { return thing.type(); }
        virtual ~NoSuchElement() throw () {}
};

typedef NoSuchElement <DataElement> noSuchData;
typedef NoSuchElement <CompElement> noSuchComp;
typedef NoSuchElement <SegmElement> noSuchSegm;
typedef NoSuchElement <SegGrElement> noSuchSegGr;
typedef NoSuchElement <FullDataElement> noSuchFullData;

// template <class FEdiElem, typename N> class BaseEdiCast {
//     FEdiElem Elem;
//     int Err;
//     public:
//         BaseEdiCast( const FEdiElem &elem, int err ) : Elem(elem), Err(err){}
//         virtual void cast ( N& ) = 0;
//         virtual const char * name () const = 0;
//
//         virtual const FEdiElem & element() const { return Elem; }
//         virtual const string & dataStr() const { return Elem.dataStr(); }
//         virtual const string & path() const { return Elem.path(); }
//         virtual int type () const { return Elem.type(); }
//         virtual int err () const { return Err; }
//         virtual void setErr(int err) { Err = err; }
//
//         virtual ~BaseEdiCast() {}
// };
// Облегчённый вариант...
template <class _RET> class BaseEdiCast {
    public:
        virtual _RET operator () (const BaseFDE &) = 0;
        virtual ~BaseEdiCast() {}
};

template <class _RET>  class EdiDigitCast : public BaseEdiCast <_RET>
{
    EdiErr_t Err;
    std::string defVal;
    public:
        EdiDigitCast (const EdiErr_t &err="", std::string dVal="0"):Err(err),defVal(dVal){}

        virtual _RET operator () (const BaseFDE &de) {
            try{
                return boost::lexical_cast<_RET>(de.dataStr().empty()?defVal:de.dataStr());
            }
            catch(boost::bad_lexical_cast &)
            {
                if(Err.size()){
                    throw BadCast(de,"Number",Err);
                } else {
                    return _RET();
                }
            }
        }
};

//EdiDigitCast<FullDataElement, int> a;

// typedef EdiDigitCast<FilledDataElement, int> EdiIntCast;
// typedef EdiDigitCast<FilledFullDataElement, int> EdiFIntCast;

inline void ResetEdiPointG(EDI_REAL_MES_STRUCT *pM){
    if(ResetEdiPoint_(pM)){
        throw Exception("Error: Reset edi point");
    }
}

inline void ResetEdiPointW(EDI_REAL_MES_STRUCT *pM){
    if(ResetEdiPointW_(pM)){
        throw Exception("Error (W): Reset edi wr_point");
    }
}

inline void ResetEdiWrPoint(EDI_REAL_MES_STRUCT *pM)
{
    if(ResetEdiWrPoint_(pM))
    {
        throw Exception("Error (W): Reset edi wr point");
    }
}

inline void ResetEdiStackPointG(EDI_REAL_MES_STRUCT *pM){
    if(ResetStackPoint_(pM)){
	throw Exception("Error : Reset edi point stack");
    }
}

inline void ResetEdiStackPointW(EDI_REAL_MES_STRUCT *pM){
    if(ResetStackPointW_(pM)){
	throw Exception("Error (W): Reset edi wr_point stack");
    }
}

inline void PushEdiPointG(EDI_REAL_MES_STRUCT *pM){
    if(PushEdiPoint_(pM)){
        throw Exception("Error: Push edi point");
    }
}

inline void PushEdiPointW(EDI_REAL_MES_STRUCT *pM){
    if(PushEdiPointW_(pM)){
        throw Exception("Error (W): Push edi wr_point");
    }
}

inline void PopEdiPointG(EDI_REAL_MES_STRUCT *pM){
    if(PopEdiPoint_(pM)){
        throw Exception("Error: Pop edi point");
    }
}

inline void PopEdiPointW(EDI_REAL_MES_STRUCT *pM){
    if(PopEdiPointW_(pM)){
        throw Exception("Error (W): Pop edi wr_point");
    }
}

inline void PopEdiPoint_wdG(EDI_REAL_MES_STRUCT *pM){
    if(PopEdiPoint_wd_(pM)){
        throw Exception("Error: Pop edi point (wd)");
    }
}

inline void PopEdiPoint_wdW(EDI_REAL_MES_STRUCT *pM){
    if(PopEdiPoint_wdW_(pM)){
        throw Exception("Error (W): Pop edi wr_point (wd)");
    }
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegGrW(EDI_REAL_MES_STRUCT *pM, int SegGr, int Num=0,
                                const EdiErr_t &err="")
{
    switch(SetEdiPointToSegGrW_(pM, SegGr, Num)){
        case 0:
            if(err.size())
                throw noSuchSegGr(SegGrElement(SegGr, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error (W): Set edi point to the Segment Group "+SegGrElement(SegGr, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegGrW(EDI_REAL_MES_STRUCT *pM, const SegGrElement &segGr,
                                const EdiErr_t & err="")
{
    return SetEdiPointToSegGrW(pM, segGr.segGr(), segGr.num(),err);
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegGrG(EDI_REAL_MES_STRUCT *pM, int SegGr, int Num=0,
                                const EdiErr_t &err="")
{
    switch(SetEdiPointToSegGr_(pM, SegGr, Num)){
        case 0:
            if(err.size())
                throw noSuchSegGr(SegGrElement(SegGr, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error: Set edi point to the Segment Group "+SegGrElement(SegGr, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegGrG(EDI_REAL_MES_STRUCT *pM, const SegGrElement &segGr,
                                const EdiErr_t &err="")
{
    return SetEdiPointToSegGrG(pM, segGr.segGr(), segGr.num(), err);
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegmentW(EDI_REAL_MES_STRUCT *pM, const char *Segm, int Num=0,
                                  const EdiErr_t &err="")
{
    switch(SetEdiPointToSegmentW_(pM, Segm, Num)){
        case 0:
            if(err.size())
                throw noSuchSegm(SegmElement(Segm, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error (W): Set edi point to the Segment Element "+SegmElement(Segm, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegmentW(EDI_REAL_MES_STRUCT *pM, const SegmElement &segm,
                                  const EdiErr_t & err="")
{
    return SetEdiPointToSegmentW(pM, segm.segm().c_str(), segm.num(), err);
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegmentG(EDI_REAL_MES_STRUCT *pM, const char *Segm, int Num=0,
                                  const EdiErr_t &err="")
{
    switch(SetEdiPointToSegment_(pM, Segm, Num)){
        case 0:
            if(err.size())
                throw noSuchSegm(SegmElement(Segm, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error: Set edi point to the Segment Element "+SegmElement(Segm, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToSegmentG(EDI_REAL_MES_STRUCT *pM, const SegmElement &segm,
                                  const EdiErr_t &err="")
{
    return SetEdiPointToSegmentG(pM, segm.segm().c_str(), segm.num(), err);
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToCompositeW(EDI_REAL_MES_STRUCT *pM, const char *Comp, int Num=0, const EdiErr_t &err="")
{
    switch(SetEdiPointToCompositeW_(pM, Comp, Num)){
        case 0:
            if(err.size())
                throw noSuchComp(CompElement(Comp, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error (W): Set edi point to the Composite Element "+CompElement(Comp, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToCompositeW(EDI_REAL_MES_STRUCT *pM, const CompElement &comp,
                                    const EdiErr_t & err="")
{
    return SetEdiPointToCompositeW(pM, comp.comp().c_str(), comp.num(), err);
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToCompositeG(EDI_REAL_MES_STRUCT *pM, const char *Comp,
                                    int Num=0, const EdiErr_t &err="")
{
    switch(SetEdiPointToComposite_(pM, Comp, Num)){
        case 0:
            if(err.size())
                throw noSuchComp(CompElement(Comp, Num), err);
            else
                return false;
        case -1:
            throw Exception("Error:  Set edi point to the Composite Element "+CompElement(Comp, Num).path());
    }
    return true;
}

// return true - Ok
// return false - Not found
inline bool SetEdiPointToCompositeG(EDI_REAL_MES_STRUCT *pM, const CompElement &comp,
                                    const EdiErr_t &err="")
{
    return SetEdiPointToCompositeG(pM, comp.comp().c_str(), comp.num(), err);
}

/***************************************************************/
inline const char * GetDBFName(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const EdiErr_t &err="",
                               const CompElement &Comp = CompElement(),
                               const SegmElement &Segm = SegmElement(),
                               const SegGrElement &SegGr = SegGrElement())
{
    const char *sd=GetDBFNSeq_(pM,
                            SegGr.segGr(),SegGr.num(),
                            Segm.segm().c_str(),Segm.num(),
                            Comp.comp().c_str(),Comp.num(),
                            Data.data(),Data.num(),Data.numInSequence());

    if(err.size()!=0 && sd==NULL){
        throw noSuchFullData(FullDataElement(Data, Comp, Segm, SegGr), err);
    }
    return sd?sd:"";
}

inline const char * GetDBFName(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const CompElement &Comp = CompElement(),
                               const SegmElement &Segm = SegmElement(),
                               const SegGrElement &SegGr = SegGrElement())
{
    const char *sd=GetDBFNSeq_(pM,
                            SegGr.segGr(),SegGr.num(),
                            Segm.segm().c_str(),Segm.num(),
                            Comp.comp().c_str(),Comp.num(),
                            Data.data(),Data.num(),Data.numInSequence());

    return sd?sd:"";
}

inline std::string GetDBFNameStr(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const CompElement &Comp = CompElement(),
                               const SegmElement &Segm = SegmElement(),
                               const SegGrElement &SegGr = SegGrElement())
{
    return std::string(GetDBFName(pM,Data,Comp,Segm,SegGr));
}

inline std::string GetDBFNameStr(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const EdiErr_t &err="",
                               const CompElement &Comp = CompElement(),
                               const SegmElement &Segm = SegmElement(),
                               const SegGrElement &SegGr = SegGrElement())
{
    return std::string (GetDBFName(pM,Data,err,Comp,Segm,SegGr));
}

inline const char * GetDBFName(EDI_REAL_MES_STRUCT *pM,
                           int Data  , int DataNum=0,
                           const EdiErr_t &err="",
                           const char *Comp=NULL, int CompNum=0,
                           const char *Segm=NULL, int SegmNum=0,
                           short SegGr=0, int SegGrNum=0)
{
    return GetDBFName(pM,
                      DataElement(Data, DataNum), err, CompElement(Comp, CompNum),
                      SegmElement(Segm, SegmNum), SegGrElement(SegGr, SegGrNum));
}

inline const char * GetDBFName(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const SegmElement &Segm,
                               const EdiErr_t &err="",
                               const SegGrElement &SegGr=SegGrElement())
{
    return GetDBFName(pM,
                      Data, err, CompElement(),
                      Segm, SegGr);
}

inline const char * GetDBFName(EDI_REAL_MES_STRUCT *pM,
                               const DataElement &Data,
                               const SegmElement &Segm,
                               const SegGrElement &SegGr,
                               const EdiErr_t &err="")
{
    return GetDBFName(pM,
                      Data, err, CompElement(),
                      Segm, SegGr);
}

/****************************************************************************************************/
/****/
template <class T, class Cast>
        inline T GetDBFNameCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                                const DataElement &Data,
                                const EdiErr_t &err_nodat,
                                const CompElement &Comp = CompElement(),
                                const SegmElement &Segm = SegmElement(),
                                const SegGrElement &SegGr = SegGrElement())
{
    const char *sd=GetDBFName(pM, Data, err_nodat, Comp, Segm, SegGr);
    return cast(FilledFullDataElement(Data, Comp, Segm, SegGr, sd));
}

template <class T, class Cast>
        inline T GetDBFNameCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                                const DataElement &Data,
                                const CompElement &Comp = CompElement(),
                                const SegmElement &Segm = SegmElement(),
                                const SegGrElement &SegGr = SegGrElement())
{
    const char *sd=GetDBFName(pM, Data, "", Comp, Segm, SegGr);
    return cast(FilledFullDataElement(Data, Comp, Segm, SegGr, sd));
}


/****/
/****/
template <class T, class Cast>
        inline T GetDBFNameCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                                int Data  , int DataNum=0,
                                const EdiErr_t & err_nodat="",
                                const char *Comp=NULL, int CompNum=0,
                                const char *Segm=NULL, int SegmNum=0,
                                short SegGr=0, int SegGrNum=0)
{
    return GetDBFNameCast<T,Cast>(cast, pM, DataElement(Data, DataNum),
                                        err_nodat,
                                        CompElement(Comp, CompNum),
                                        SegmElement(Segm, SegmNum),
                                        SegGrElement(SegGr, SegGrNum));
}

/****/
/****/
template <class T, class Cast>
        inline T GetDBFNameCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                                const DataElement &Data,
                                const SegmElement &Segm,
                                const EdiErr_t &err_nodat="",
                                const SegGrElement &SegGr = SegGrElement())
{
    return GetDBFNameCast<T,Cast>( cast,
                            Data, err_nodat, CompElement(), Segm, SegGr);
}

/****/
/****/
template <class T, class Cast>
        inline T GetDBFNameCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                                const DataElement &Data,
                                const SegmElement &Segm,
                                const SegGrElement &SegGr,
                                const EdiErr_t &err_nodat="")
{
    return GetDBFNameCast<T,Cast>(cast,
                                  Data, err_nodat, CompElement(), Segm, SegGr);
}

/****/
/*****************************************************************/
inline const char * GetDBNum(EDI_REAL_MES_STRUCT *pM, const DataElement &Data,
                             const EdiErr_t & err="")
{
    const char *c_str = GetDBNumSeq_(pM,Data.data(), Data.num(), Data.numInSequence());
    if(err.size()!=0 && c_str == NULL){
        throw noSuchData(Data, err);
    }
    return c_str?c_str:"";
}

inline const char * GetDBNum(EDI_REAL_MES_STRUCT *pM, int data, int num=0,
                             const EdiErr_t & err="")
{
    return GetDBNum(pM, DataElement(data, num), err);
}


template <class T, class Cast>
        inline T GetDBNumCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                              const DataElement &Data,
                              const EdiErr_t &err_nodat="")
{
    const char *sd=GetDBNum(pM,Data,err_nodat);
    return cast (FilledDataElement(Data, sd));
}

template <class T, class Cast>
        inline T GetDBNumCast(Cast cast, EDI_REAL_MES_STRUCT *pM,
                              int Data  , int DataNum=0,
                              const EdiErr_t &err_nodat="")
{
    return GetDBNumCast<T,Cast>(cast, pM, DataElement(Data, DataNum), err_nodat);
}


/**********************************************************************************/

//Кол-во сегментных групп с заданным именем
inline unsigned GetNumSegGr(EDI_REAL_MES_STRUCT *pMes, short SegGr,
                            const EdiErr_t & err = "")
{
    int num = GetNumSegGr_(pMes, SegGr);
    if (num < 0) {
        throw Exception("Error: Get number of the Segment Groups "+SegGrElement(SegGr, 0).path());
    } else if (num ==0 && err.size()) {
        throw noSuchSegGr(SegGrElement(SegGr, 0), err);
    }
    return num;
}

//Кол-во сегментов с заданным именем
inline unsigned GetNumSegment(EDI_REAL_MES_STRUCT *pMes, const char *Segm,
                              const EdiErr_t & err = "")
{
    int num = GetNumSegment_(pMes, Segm);
    if(num < 0){
        throw Exception("Error: Get number of the Segment Element "+SegmElement(Segm, 0).path());
    } else if(num == 0 && err.size()){
        throw noSuchSegm(SegmElement(Segm, 0), err);
    }
    return num;
}

//Кол-во композитов с заданным именем
inline unsigned GetNumComposite(EDI_REAL_MES_STRUCT *pMes, const char *Comp,
                                const EdiErr_t & err = "")
{
    int num = GetNumComposite_(pMes, Comp);
    if(num < 0){
        throw Exception("Error: Get number of the Composite Element "+CompElement(Comp, 0).path());
    } else if(num == 0 && err.size()){
        throw noSuchComp(CompElement(Comp, 0), err);
    }
    return num;
}

//Кол-во элементов данных с заданным именем
inline unsigned GetNumDataElem(EDI_REAL_MES_STRUCT *pMes, int Data,
                               const EdiErr_t &err = "")
{
    int num;
    if(GetDataByName_(pMes, Data, &num)<0){
        throw Exception("Error: Get number of the Data Element "+DataElement(Data, 0).path());
    } else if (num == 0 && err.size()){
        throw noSuchData(DataElement(Data, 0), err);
    }
    return num;
}

inline unsigned GetNumDataElem(EDI_REAL_MES_STRUCT *pMes, const DataElement& data,
                               const EdiErr_t &err = "")
{
    int num;
    if(GetDataByNum_(pMes, data.data(), data.numInSequence(), &num)<0){
        throw Exception("Error: Get number of the Data Element "+data.path());
    } else if (num == 0 && err.size()){
        throw noSuchData(data, err);
    }
    return num;
}

struct EdiPointHolder {
    EDI_REAL_MES_STRUCT *pMes;
    EdiPointHolder(EDI_REAL_MES_STRUCT *pMes):pMes(pMes) { PushEdiPointG(pMes); }

    void pop()
    {
        if(pMes)
            PopEdiPointG(pMes);
        pMes = 0;
    }

    void popNoDel() {
        if(pMes)
            PopEdiPoint_wdG(pMes);
    }

    ~EdiPointHolder() { pop(); }
};

inline string WriteEdiMessage(EDI_REAL_MES_STRUCT *pMes)
{
    char *tmp=NULL;
    int ret = WriteEdiMessage_(pMes, &tmp);
    string buf;

    if(ret<=0){
        throw Exception("Error: While writing message into buffer");
    }
    try{
        buf.assign(tmp);
    }
    catch(...){
        free(tmp);
        throw;
    }
    free(tmp);

    return buf;
}

inline void SetEdiFullSegment(EDI_REAL_MES_STRUCT *pMes,
                              const SegmElement &Seg, const string &str)
{
    int ret = SetEdiFullSegment_(pMes, Seg.segm().c_str(), Seg.num(), str.c_str());
    if(ret) {
        throw Exception("Error: While inserting segment element ["+Seg.path()+"]"+str+"  => ret=" + std::to_string(ret));
    }
}

inline void SetEdiFullSegment(EDI_REAL_MES_STRUCT *pMes,
                              const char *segm, int num, const string &str)
{
    SetEdiFullSegment(pMes, SegmElement(segm, num), str);
}

inline void SetEdiFullComposite(EDI_REAL_MES_STRUCT *pMes,
				const CompElement &comp, const string &str)
{
    int ret = SetEdiFullComposite_(pMes, comp.comp().c_str(), comp.num(), str.c_str());
    if(ret) {
	throw Exception("Error: While inserting composite element ["+comp.path()+"]"+str);
    }
}

inline void SetEdiFullComposite(EDI_REAL_MES_STRUCT *pMes,
				const char *comp, int num, const string &str)
{
    SetEdiFullComposite(pMes, CompElement(comp, num), str);
}

inline void SetEdiSegGr(EDI_REAL_MES_STRUCT *pMes, const SegGrElement &SegGr)
{
    int ret = SetEdiSegGr_(pMes, SegGr.segGr(), SegGr.num());
    if(ret){
	throw Exception("Error: While inserting segment group element ["+SegGr.path()+"]");
    }
}

inline void SetEdiSegGr(EDI_REAL_MES_STRUCT *pMes, int SegGr, int num=0)
{
    SetEdiSegGr(pMes, SegGrElement(SegGr, num));
}

inline void SetEdiSegment(EDI_REAL_MES_STRUCT *pMes,
			  const SegmElement &Segm)
{
    int ret = SetEdiSegment_(pMes, Segm.segm().c_str(), Segm.num());
    if (ret) {
	throw Exception("Error: While inserting segment element ["+Segm.path()+"]");
    }
}

inline void SetEdiSegment(EDI_REAL_MES_STRUCT *pMes, const char *segm, int num)
{
    SetEdiSegment(pMes, SegmElement(segm, num));
}

inline void SetEdiComposite(EDI_REAL_MES_STRUCT *pMes, const CompElement &comp)
{
    int ret = SetEdiComposite_(pMes, comp.comp().c_str(), comp.num());
    if (ret){
	throw Exception("Error: While inserting composite element ["+comp.path()+"]");
    }
}

inline void SetEdiComposite(EDI_REAL_MES_STRUCT *pMes, const char *comp, int num)
{
    SetEdiComposite(pMes, CompElement(comp, num));
}

inline void SetEdiDataElem(EDI_REAL_MES_STRUCT *pMes, const DataElement &de, const string &str)
{
    int ret = SetEdiDataElem__(pMes, de.data(), de.numInSequence(), de.num(), str.c_str(), str.size());
    if(ret) {
	throw Exception("Error: While inserting data element ["+de.path()+"]: "+str);
    }
}

inline void SetEdiDataElem(EDI_REAL_MES_STRUCT *pMes, int dataNum, int num, const string &str)
{
    SetEdiDataElem(pMes, DataElement(dataNum, num), str);
}

inline EDI_REAL_MES_STRUCT *ReadEdifactMessage(const char *msg)
{
	switch(ReadEdiMessage(msg))
	{
	case EDI_MES_STRUCT_ERR:
		throw Exception(std::string("Error in message structure: ") + EdiErrGetString());
	case EDI_MES_NOT_FND:
		throw  Exception(std::string("No message found in template: ") + EdiErrGetString());
	case EDI_MES_ERR:
		throw Exception("Program error in ReadEdiMes()");
	}
	return GetEdiMesStruct();
}

inline string GetEdiCharset(const string& sourceText)
{
    const char *chsname=GetCharSetNameFromText(sourceText.c_str(), sourceText.length(), 0);
    if(chsname)
    {
        return std::string(chsname, EDI_CHSET_LEN);
    }

    throw Exception("Error: While getting charset from source: "+sourceText.substr(0, 15));
}

inline string ChangeEdiCharset(const string& sourceText, const string& charSet)
{
    char *buf = 0;
    size_t bufLen;
    if(!ChangeEdiCharSet(sourceText.c_str(), sourceText.length(), charSet.c_str(), &buf, &bufLen) && buf)
    {
        string res = std::string(buf, bufLen);
        free(buf);
        return res;
    }

    throw Exception("Error: While changing charset to "+charSet);
}

/**
 * @brief Маскирует EDIFACT спец символы. +:?"\n
*/
std::string maskSpecialChars(const Edi_CharSet &Chars, const std::string &instr);
std::string maskSpecialChars(const std::string &instr);

} // namespace edilib

#endif /*__cplusplus*/

#endif /*_EDI_FUNC_CPP_H_*/
