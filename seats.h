#ifndef _SEATS_H_
#define _SEATS_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

struct TRem {
  std::string rem;
  int pr_denial;
};

struct TPlace {
  int x, y;
  std::string elem_type;
  int isplace;
  int xprior, yprior;
  int agle;
  std::string clname;
  int pr_smoke;
  int not_good;
  std::string xname, yname;
  std::string status;
  int pr_free;
  bool block;
  std::vector<TRem> rems;
};

typedef struct {
  int num;
  std::vector<TPlace> places;
} TPlaceList;

struct TSalons {
  int trip_id;
  std::string ClName;
  std::vector<TPlaceList*> placelists;
  ~TSalons( ) {
    for ( std::vector<TPlaceList*>::iterator i = placelists.begin(); i != placelists.end(); i++ ) {
      delete *i;  	
    }
    placelists.clear();
  }
};


class SeatsInterface : public JxtInterface
{
private:
public:
  SeatsInterface() : JxtInterface("","seats")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SeatsInterface>::CreateHandler(&SeatsInterface::XMLReadSalons);
     AddEvent("XMLReadSalons",evHandle);     
  };

  void XMLReadSalons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void BuildSalons( TSalons *Salons, xmlNodePtr salonsNode );
  static void InternalReadSalons( TSalons *Salons );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_SEATS_H_*/

