#ifndef _ASTRA_SERVICE_H_
#define _ASTRA_SERVICE_H_

#include <libxml/tree.h>
#include <string>
#include <map>
#include "JxtInterface.h"

std::string MSG_OWN_CANON_NAME(); //!!!

void getFileParams( int id, std::map<std::string,std::string> &fileparams );
bool deleteFile( int id );
/*bool loadFile( int id );*/
bool errorFile( int id, std::string err, std::string msg );
bool sendFile( int id );
bool doneFile( int id );

class AstraServiceInterface : public JxtInterface
{
private:
public:
  AstraServiceInterface() : JxtInterface("","AstraService")
  {
     Handler *evHandle;
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::authorize);
     AddEvent("authorize",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::commitFileData);
     AddEvent("commitFileData",evHandle);          
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::errorFileData);
     AddEvent("errorFileData",evHandle);                    
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::createFileData);
     AddEvent("createFileData",evHandle);                              
  };

  void authorize( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void commitFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );  
  void errorFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void createFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_ASTRA_SERVICE_H_*/

