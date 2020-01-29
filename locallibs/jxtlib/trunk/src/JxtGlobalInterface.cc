#include "JxtGlobalInterface.h"

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>

#include "xml_msg.h"
#include "jxt_stuff.h"
#include "jxtlib.h"

using namespace std;

JxtGlobalInterface::JxtGlobalInterface()
    : JxtInterface("", "")
{
    Handler *evHandle = JXT_HANDLER(JxtGlobalInterface, Display);
    AddEvent("init_main", evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, RefreshMenu);
    AddEvent("init_main_new", evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, Window);
    AddEvent("window", evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, Close);
    AddEvent("close", evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, Update);
    AddEvent(JxtEvent("data", false, false), evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, Quit);
    AddEvent(JxtEvent("quit"), evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, UpdateXml);
    AddEvent(JxtEvent("interface", false, false), evHandle);
    AddEvent(JxtEvent("type", false, false), evHandle);
    AddEvent(JxtEvent("ipart", false, false), evHandle);
    AddEvent(JxtEvent("ppart", false, false), evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, Logout);
    AddEvent("logout", evHandle);
    evHandle = JXT_HANDLER(JxtGlobalInterface, StartTask);
    AddEvent("start_task", evHandle);
}

JxtGlobalInterface::~JxtGlobalInterface()
{   }

void JxtGlobalInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->displayMain();
}

void JxtGlobalInterface::RefreshMenu(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->refreshMenu();
}

void JxtGlobalInterface::Window(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->mainWindow();
}

void JxtGlobalInterface::Close(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->closeHandle();
}

void JxtGlobalInterface::Update(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->updateData();
}

void JxtGlobalInterface::Quit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->quitWindow();
}

void JxtGlobalInterface::UpdateXml(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->updateXmlData();
}

void JxtGlobalInterface::Logout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->Logout();
}

void JxtGlobalInterface::StartTask(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	jxtlib::JXTLib::Instance()->GetCallbacks()->startTask();
}

