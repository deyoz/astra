#ifndef _TRIPINFO_H_
#define _TRIPINFO_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "oralib.h"
#include "JxtInterface.h"

struct TVar {
  std::string name;
  otFieldType type;
  std::string value;
  TVar( std::string &aname, otFieldType atype, std::string &avalue ) {
    name = aname;
    type = atype;
    value = avalue;
  }
};

class TSQLParams {
  private:
    std::vector<TVar> vars;
  public:
    std::string sqlfrom;
    void addVariable( TVar &var );
    void addVariable( std::string aname, otFieldType atype, std::string avalue );
    void clearVariables( );
    void setVariables( TQuery &Qry );
};

class TSQL
{
private:
  std::map<std::string,TSQLParams> sqltrips;
  static TSQL *Instance();
  void createSQLTrips( );
public:
  TSQL();
  static void setSQLTrips( TQuery &Qry, const std::string &screen );
};

void readTripCounters( int point_id, xmlNodePtr dataNode );
void viewPNL( int point_id, xmlNodePtr dataNode );


class TripsInterface : public JxtInterface
{
private:
public:
  TripsInterface() : JxtInterface("","trips")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripList);
     AddEvent("GetTripList",evHandle);
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripInfo);
     AddEvent("GetTripInfo",evHandle);
  };
  void GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif /*_TRIPINFO_H_*/

