#ifndef _BAGTYPES_H_
#define _BAGTYPES_H_

#include <map>
#include <string>
#include <vector>
#include "xml_unit.h"
#include "basic.h"
#include "astra_utils.h"


namespace BagPayment
{

struct rficsItem {
    std::string rfisc;
    std::string service_type;
    std::string rfic;
    std::string emd_type;
    std::string name;
    std::string name_lat;
    bool operator == (const rficsItem &obj) const // сравнение
    {
      return ( rfisc == obj.rfisc &&
               service_type == obj.service_type &&
               rfic == obj.rfic &&
               emd_type == obj.emd_type &&
               name == obj.name &&
               name_lat == obj.name_lat);
    }
    void fromXML( xmlNodePtr node );
};

class grpRFISC {   //rfisc
  private:
    std::map<std::string,rficsItem> RFISCList; //храним справочник
    std::string normilizeData( ); // нормализованное строковое предстваление для операции сравнения
  public:
    void fromDB( int list_id ); // загрузка данных по группе
    int toDB( ); // сохранение данных по группе
    bool fromXML( xmlNodePtr node ); //разбор
    bool operator == (const grpRFISC &obj) const { // сравнение
      return RFISCList == obj.RFISCList;
    }
    int getCRC32();
    bool empty() {
      return RFISCList.empty();
    }
};

class paxNormsPC{   //bag_norms_pc
  private:
    std::map<std::string,std::string> free_baggage_norm;
    const int page_size = 4000;
  public:
    void from_BD( int pax_id );
    void toDB( int pax_id );
    bool from_XML( xmlNodePtr node );
};

struct passenger {
  int id;
  std::string ticket;
  std::string category;
  BASIC::TDateTime birthdate;
  std::string sex;
  passenger() {
    birthdate = ASTRA::NoExists;
  }
};

struct segment {
  int id;
  std::string company;
  std::string flight;
  std::string departure;
  std::string arrival;
  BASIC::TDateTime departure_time;
  std::string equipment;
};

class requestRFISC {
  private:
    std::vector<passenger> passengers;
    std::vector<segment> segments;
  public:
    void fillTESTDATA();
    xmlDocPtr createRequest();
    std::string createRequestStr();
    void clear() {
      passengers.clear();
      segments.clear();
    }
    void addPassenger( passenger pass ) {
      passengers.push_back( pass );
    }
    void addSegment( segment seg ) {
      segments.push_back( seg );
    }
};

void sendRequestTESTRFISC();

}

#endif

