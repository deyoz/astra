#ifndef JXTGLOBALINTERFACE_H_
#define JXTGLOBALINTERFACE_H_

#include "JxtInterface.h"

class JxtGlobalInterface : public JxtInterface
{
public:
	JxtGlobalInterface();
    virtual ~JxtGlobalInterface();
	
	virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void RefreshMenu(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void Window(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void Close(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void Update(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void Quit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void UpdateXml(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void Logout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void StartTask(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif /*JXTGLOBALINTERFACE_H_*/
