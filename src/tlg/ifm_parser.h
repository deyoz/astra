#pragma once

#include <typeb/typeb_message.h>
#include <typeb/typeb_template.h>
#include <typeb/tb_elements.h>

#include <boost/date_time/gregorian/gregorian_types.hpp>

namespace typeb_parser {

// RcvFlightElem and RcvFlightTemplate

//     -P2452/24OCT TJMSVO SU
class RcvFlightElem : public TbElement
{
    std::string Airline; // Авиакомп
    unsigned FlightNum;  // Номер рейса

    boost::gregorian::date DepDate; // Дата вылета

    std::string DepPoint; // Город/порт вылета
    std::string ArrPoint; // Город/порт прилёта

    std::string HostIndication; // 
     
public:
    RcvFlightElem(const std::string &Airl, unsigned FlNum,
                  const boost::gregorian::date &depd, 
                  const std::string &depp, const std::string &arrp, 
                  const std::string &hostIndic)
        : Airline(Airl), FlightNum(FlNum),
          DepDate(depd), DepPoint(depp), ArrPoint(arrp), 
          HostIndication(hostIndic)
    {}

    const std::string& airline() const { return Airline; }
    unsigned flightNum() const { return  FlightNum; }
    const boost::gregorian::date& depDate() const { return DepDate; }
    const std::string& depPoint() const { return DepPoint; }
    const std::string& arrPoint() const { return ArrPoint; }
    const std::string& hostIndication() const { return HostIndication; }

    static RcvFlightElem* parse (const std::string &text);
};

//---------------------------------------------------------------------------------------

class RcvFlightTemplate :  public tb_descr_element
{
public:
    virtual const char* name() const { return "Receiving Flight element"; }
    virtual const char* lexeme() const { return ""; }
    virtual const char* accessName() const { return "RcvFlight"; }
    virtual TbElement* parse(const std::string &text) const;
};

//---------------------------------------------------------------------------------------

class ActionIndicatorElem: public TbElement
{
    std::string Act;

public:
    ActionIndicatorElem(const std::string& act)
        : Act(act)
    {}

    const std::string& act() const { return Act; }

    static ActionIndicatorElem* parse(const std::string &text);
};

//---------------------------------------------------------------------------------------

class ActionIndicatorTemplate : public tb_descr_element
{
public:
    virtual const char* name() const { return "Action indicator template"; }
    virtual const char* lexeme() const { return ""; }
    virtual const char* accessName() const { return "ActIndicator"; }
    virtual TbElement* parse(const std::string &text) const;
};

//---------------------------------------------------------------------------------------

class IFM_template: public typeb_template
{
public:
    IFM_template();
    virtual bool multipart() const { return false; }
};
    
}//namespace typeb_parser

/////////////////////////////////////////////////////////////////////////////////////////

namespace TypeB {

class HandleTypebIfm
{
public:
    static void handle(const typeb_parser::TypeBMessage& msg);
};

}//namespace TypeB
