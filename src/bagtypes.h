#ifndef _BAGTYPES_H_
#define _BAGTYPES_H_

#include <map>
#include <string>
#include "xml_unit.h"


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

}

#endif

