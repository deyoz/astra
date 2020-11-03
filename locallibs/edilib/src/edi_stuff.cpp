#include <string.h>
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"
#include "edi_user_func.h"
#include "edi_func_cpp.h"
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

namespace edilib
{

BadCast::BadCast(const BaseFDE & elem, const std::string &name, const EdiErr_t &err) throw () :
    BaseBadCast("", err), Name(name)
{
    if(elem.type() == FullDataElementType){
        FilledFullDataElement const * const ffde =
            dynamic_cast<FilledFullDataElement const * const> (&elem);
        if(!ffde){
            std::cout << "Invalid FilledFullDataElement! [" << elem.type() << "]" << std::endl;
            abort();
        }
        Elem = new FilledFullDataElement(*ffde);
    } else if(elem.type() == DataElementType){
        FilledDataElement const * const fde =
            dynamic_cast<FilledDataElement const*const> (&elem);
        if(!fde){
            std::cout << "Invalid FilledDataElement! [" << elem.type() << "]" << std::endl;
            abort();
        }
        Elem = new FilledDataElement(*fde);
    } else {
        std::cout << "Invalid data type! [" << elem.type() << "]" << std::endl;
        abort();
    }
    if(!Name.size()){
        Name = Elem->name();
    }
    string ErrStr = "Invalid "+ string(Name) + ": " + Elem->dataStr()+" ["+Elem->path()+"]";
    setMessage(ErrStr);
}

const string & DataElement::path() const
{
    if(Data && !Path.size()){
        ostringstream PathTmp;
        PathTmp << Data;
        if(Num)
            PathTmp << ":" << Num;
        Path = PathTmp.str();
    }
    return Path;
}

const string & CompElement::path() const
{
    if(Comp.size() && !Path.size()){
        ostringstream PathTmp;
        PathTmp << Comp;
        if(Num)
            PathTmp << ":" << Num;
        Path = PathTmp.str();
    }
    return Path;
}

const string & SegmElement::path() const
{
    if(Segm.size() && !Path.size()){
        ostringstream PathTmp;
        PathTmp << Segm;
        if(Num)
            PathTmp << ":" << Num;
        Path = PathTmp.str();
    }
    return Path;
}

const string & SegGrElement::path() const
{
    if(SegGr && !Path.size()){
        ostringstream PathTmp;
        PathTmp << "SegGr" << SegGr;
        if(Num)
            PathTmp << ":" << Num;
        Path = PathTmp.str();
    }
    return Path;
}

}

