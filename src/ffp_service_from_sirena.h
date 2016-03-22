#pragma once
#include "sirena_service.h"
#include <boost/crc.hpp>
#include "astra_context.h"
#include "httpClient.h"
#include "points.h"
#include "basic.h"
#include "astra_misc.h"
#include "misc.h"
#include "emdoc.h"
#include "astra_locale.h"
#include <boost/asio.hpp>
#include <serverlib/xml_stuff.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include "xml_unit.h"

//#define NICKNAME "ROMAN"
//#define NICKTRACE SYSTEM_TRACE
//#include "serverlib/test.h"





std::string SendAzimutRequestToSirena();

namespace SirenaExchange
{

class TFFPExchangeRes : public TExchange
{
    static const std::string id;	 
    std::string company, card_number; 
    std::string status;
			
public: 
    std::string exchangeId() const;
    virtual void fromXML(xmlNodePtr node);
    //bool error() const;
    //std::string traceError() const;
    virtual void clear(){}
    //TFFPExchangeRes(const std::string& _company, const std::string& _card_number) : company(_company), card_number(_card_number){}
    bool isRequest() const; 
    std::string get_status();
};

class TFFPExchangeReq : public TExchange
{   static const std::string id;
    std::string company, card_number;
public:
    std::string exchangeId() const;
    virtual void clear(){}
    virtual void toXML(xmlNodePtr node) const;	
    TFFPExchangeReq(const std::string& _company, const std::string& _card_number) : company(_company), card_number(_card_number){}
    bool isRequest() const;    
};

std::string send_ffp_request(std::string airline, std::string card_num);
}
