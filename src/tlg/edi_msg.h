#pragma once

#include "astra_msg.h"

#include <etick/exceptions.h>


#define REGERR(x) static const Ticketing::ErrMsg_t x

namespace edifact
{

typedef Ticketing::TickExceptions::tick_exception edi_exception;
typedef Ticketing::TickExceptions::tick_fatal_except edi_fatal_except;
typedef Ticketing::TickExceptions::tick_soft_except edi_soft_except;

//-----------------------------------------------------------------------------

class EdiErrMsgDataElem
{
    typedef std::map<Ticketing::ErrMsg_t, std::string> ErrMsgMap_t;

    ErrMsgMap_t *m_errMsgMap;
    int m_dataElem;
    std::string m_defCode;

public:
    EdiErrMsgDataElem(int de, const std::string& defCode);
    void addElement(const Ticketing::ErrMsg_t& innerErr, const std::string& ediErr);
    const std::string& getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr);
    Ticketing::ErrMsg_t getInnerErrByEdi(const std::string& ediErr);
};

//-----------------------------------------------------------------------------

class EdiErrMsg
{
    typedef std::map<int, EdiErrMsgDataElem*> EdiErrMsgMap_t;

    static EdiErrMsgMap_t* m_ediErrMsgMap;

    static EdiErrMsgMap_t* ediErrMsgMap();
    static void init();

protected:
    static EdiErrMsgDataElem* getEdiErrMapByDataElem(int de);

public:
    static const Ticketing::ErrMsg_t DefaultInnerErr;
};

//-----------------------------------------------------------------------------

class EdiErrMsgERC: public EdiErrMsg
{
public:
    static const std::string DefaultEdiErr;
    static std::string getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr);
    static Ticketing::ErrMsg_t getInnerErrByEdi(const std::string& ediErr);
};

//-------------------------------------------------------------------

class EdiErrMsgERD: public EdiErrMsg
{
public:
    static const std::string DefaultEdiErr;
    static std::string getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr);
    static Ticketing::ErrMsg_t getInnerErrByEdi(const std::string& ediErr);
};

//-----------------------------------------------------------------------------

std::string getErdErrByInner(const Ticketing::ErrMsg_t& innerErr);
Ticketing::ErrMsg_t getInnerErrByErd(const std::string& erdErr);

}//namespace edifact

#undef REGERR
