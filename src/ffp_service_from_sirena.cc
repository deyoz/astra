#include "ffp_service_from_sirena.h"
#include "sirena_service.h"
#include <boost/crc.hpp>
#include "astra_context.h"
#include "httpClient.h"
#include "points.h"
#include "basic.h"
#include "astra_misc.h"
#include "misc.h"
#include "emdoc.h"
#include "baggage.h"
#include "astra_locale.h"
#include <boost/asio.hpp>
#include <serverlib/xml_stuff.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include "xml_unit.h"

#define NICKNAME "ROMAN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace SirenaExchange;
using namespace EXCEPTIONS;
using namespace AstraLocale;


/*
std::string SendAzimutRequestToSirena()
{ using namespace BASIC;
  using namespace EXCEPTIONS;
  using namespace std;
  using namespace AstraLocale;
  string hst = "test.sirena-travel.ru";
  try{
  SirenaExchange::TAvailabilityReq request; 
  RequestInfo requestInfo; ResponseInfo responseInfo;
  try{
  requestInfo.host = hst; 
  requestInfo.port = 9000;
  requestInfo.path = "/astra";
  std::string str;
    std::ifstream ifs("/home/roman/text.xml");
    ifs >> std::noskipws;
    std::copy(std::istream_iterator<char>(ifs), std::istream_iterator<char>(), std::back_inserter(str));
  requestInfo.content = str;
  requestInfo.using_ssl = false;
  requestInfo.timeout = SIRENA_REQ_TIMEOUT();
  }
  catch(Exception& e)
  { 	std::cout<<"Error while init: \n";
	return e.what();
  }	
  catch(...)
  {return "Unrecognized error while init";}

  try{  int request_count = 3;
  for(int pass=0; pass<request_count; pass++)
  {
    httpClient_main(requestInfo, responseInfo);
    if (!(!responseInfo.completed && responseInfo.error_code==boost::asio::error::eof && responseInfo.error_operation==toStatus))
      break;
  };
  if (!responseInfo.completed ) {
         cout<<"!responseInfo.completed\n";		
	return "Err\n";
	//throw Exception("%s: responseInfo.completed()=false", __FUNCTION__);
   }
  }
   catch(Exception& e)
  { 	std::cout<<"Error while connect: \n";
	return e.what();
  }

  catch(...)
  { return "Cant connect";

  }
  try{
  xmlDocPtr doc=TextToXMLTree(responseInfo.content);
  if(doc == NULL) return "Service returned null data";
   
  xmlNodePtr rootNode=xmlDocGetRootElement( doc );
  if(!rootNode) std::cout<<"rootNode == NULL\n";
  xmlNodePtr node;
  try{ 
	node  = NodeAsNode("/answer/ffp_info/info", doc);
  }
  catch(...){node = NULL;}  
  
  if(!node){  
        xmlNodePtr err_node = NodeAsNode("/answer/ffp_info/error", doc);
        if(err_node)
        {  return NodeAsString("@message", err_node);      
        }
	return "The data for this card is not available."; 
  } 
  
  
  string status=NodeAsString("@status", node);
  return string("Got status: ") + status;
  }
   catch(Exception& e)
  {     std::cout<<"Error while parsing: \n";
	return e.what();
  }

  catch(...){return "Unrecognized error while parsing response\n";}
	
  }
 catch(...)
 {

 }
 return "Some unrecognized error\n";
}

*/


std::string SirenaExchange::send_ffp_request(std::string airline, std::string card_num)
{	
        TFFPExchangeReq req(airline, card_num);
	TFFPExchangeRes res;
	try{
		SendRequest(req, res);
		if(res.get_status().empty()) 
			throw EXCEPTIONS::Exception("%s: strange situation: field status empty in response", __FUNCTION__);	
		return std::string("Got status: ") + res.get_status();
	}
        catch(Exception e)
	{   if(!res.error() || res.error_code.size() != 1) throw e; //если код ошибки не входит в список документированных тоже перекидываем текущее исключение
            switch(res.error_code[0])		
            {	case '1': throw e;
		case '2': throw e;
		case '3': throw UserException("MSG.FFPService.WrongAirline");  //"Для данной авиакомпании данные по ЧПСЖ отсутствуют")
		case '4': throw UserException("MSG.FFPService.WrongCard"); //"Карта с таким номером не зарегистрирована"
		case '5': throw UserException("MSG.FFPService.3rdPartyTimeOut"); //"Таймаут взаимодействия с ЧПСЖ-системой"
                case '6': throw UserException("MSG.FFPService.3rdPartyServerError"); //"Ошибка взаимодействия с системой ЧПСЖ"
		default: throw e;
	    }			
	}
}


std::string SendAzimutRequestToSirena()
{	std::string str, airline, card;
	std::ifstream ifs("./ffp_test_options");
	ifs>>airline>>card;      
	try {return send_ffp_request(airline, card);}
	catch(Exception& e)
        { return std::string("Got err: ") +  e.what();
        }
}




void TFFPExchangeReq::toXML(xmlNodePtr node) const
{       xmlNodePtr ffp = NewTextChild(node, "ffp");
	SetProp(ffp, "company", company);
	NodeSetContent(ffp, card_number);	        
}



void TFFPExchangeRes::fromXML(xmlNodePtr node)
{      status=NodeAsString("@status", NodeAsNode("info", node));
}


bool TFFPExchangeReq::isRequest() const 
{ return true; 
}

const std::string TFFPExchangeRes::id="ffp_info";


bool TFFPExchangeRes::isRequest() const 
{ return false; 
}

const std::string TFFPExchangeReq::id="ffp_info";

std::string TFFPExchangeRes::get_status() 
{ return status;
}


std::string TFFPExchangeReq::exchangeId() const
{	return id;
}

std::string TFFPExchangeRes::exchangeId() const
{       return id;
}


