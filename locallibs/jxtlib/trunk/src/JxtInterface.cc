#include "JxtInterface.h"
#include "JxtGlobalInterface.h"

#include <serverlib/posthooks.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>

#include "jxtlib.h"
#include "jxt_tools.h"
//#include "xml_msg.h" // addXmlMessageBoxFmt

#include "AccessException.h"

using namespace std;

namespace JxtInterfaceExceptions
{

class interface_exception : public jxtlib::jxtlib_exception
{
public:
  interface_exception(const std::string &msg, const std::string &iface_, const std::string &ev)
          : jxtlib::jxtlib_exception(msg + " [" + iface_ + "], " + "[" + ev + "]")
  {}
};

class no_global_event : public jxtlib::jxtlib_exception
{
public:
  no_global_event()
          : jxtlib::jxtlib_exception("event not found in global interface")
  {}
};

}

void Handler::call(JxtInterface *bi, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if (isAccessible())
        run(bi, ctxt, reqNode, resNode);
    else
        throw AccessException(STDLOG, "Действие недоступно");
}

JxtInterface::JxtInterface(const string &nm_, const string &iface_, Accessible* acc)
	: IAccessible(acc), name(nm_), ifaceName(iface_)
{
	JxtInterfaceMng::Instance()->AddInterface(this);

    Handler *evHandle = JXT_HANDLER(JxtInterface, Display);
	AddEvent(JxtEvent("display"), evHandle);
}

const char* JxtInterface::Icon() const
{
    return "icons/komtex_small.png";
}

void JxtInterface::call(const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	map<JxtEvent, Handler*>::iterator it = events.find(JxtEvent(ev));
	if (it == events.end())
		throw JxtInterfaceExceptions::interface_exception("Event not found here", GetIfaceName().c_str(), ev.c_str());

	Handler *rf = it->second;
    rf->call(this, ctxt, reqNode, resNode);
/*	if (it->first.withReg)
	{
		if (JxtInterfaceMng::Instance()->AlreadyLogged())
        {
            if (it->first.blockPult)
                JxtInterfaceMng::Instance()->BlockPult();
            rf->call(this, ctxt, reqNode, resNode);
        }
		else
		{
//			if (it->first.blockPult)
			JxtInterfaceMng::Instance()->Force2Register(ctxt, reqNode, resNode);
		}
	}
	else
        rf->call(this, ctxt, reqNode, resNode);*/
}


JxtInterfaceMng *JxtInterfaceMng::Instance()
{
	static JxtInterfaceMng *self = NULL;
	if (!self)
	{
		ProgTrace(TRACE3, "creating JxtInterfaceMng");
		self = new JxtInterfaceMng();
		self->InitInterfaces();
		return self;
	}
	else
		return self;
}

void JxtInterfaceMng::AddInterface(JxtInterface *iface)
{
	IfaceMapIterator it = interfaces.find(iface->GetIfaceName());
	if (it != interfaces.end())
	{
		ProgError(STDLOG, ":%s interface already added : %s", __FUNCTION__, iface->GetIfaceName().c_str());
		return;
	}
	interfaces.insert(make_pair(iface->GetIfaceName(), iface));
}

JxtInterface *JxtInterfaceMng::GetInterface(const std::string &iface)
{
	IfaceMapIterator it = interfaces.find(iface);
	if (it != interfaces.end())
		return it->second;

	ProgError(STDLOG, ":%s interface %s not found", __FUNCTION__, iface.c_str());
	return NULL;
}

void JxtInterfaceMng::OnEvent(const std::string &iface, const std::string &ev,
                              XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    OnEventInner(iface,ev,ctxt,reqNode,resNode,false);
}

void JxtInterfaceMng::OnEventWithHook(const std::string &iface, const std::string &ev,
                              XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    OnEventInner(iface,ev,ctxt,reqNode,resNode,true);
}

void JxtInterfaceMng::OnEventInner(const std::string &iface, const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, bool hook)
{
	JxtInterface *ifc = GetInterface(iface);

	//	если этого интерфейса не найдено, это наверное картотека
	if (!ifc)
	{
		ifc = GetInterface("");
		if (!ifc)
			throw jxtlib::jxtlib_exception(STDLOG, "Ошибка обработки запроса");
    }
    ProgTrace(TRACE5, "event [%s] from [%s]", ev.c_str(), ifc->GetName().c_str());

    try
    {
        const JxtEvent& je = ifc->findEvent(ev);
        if (je.withReg)
        {
            if (AlreadyLogged())
            {
                if (je.blockPult)
                    BlockPult();
            }
            else
            {
                Force2Register(ctxt, reqNode, resNode);
                return;
            }
        }

        if (!ifc->isAccessible())
            throw AccessException(STDLOG, "Интерфейс недоступен");
        if (hook)
        {
            setHook(je);
        }
        ifc->OnEvent(ev, ctxt, reqNode, resNode);
    }
    catch (const JxtInterfaceExceptions::interface_exception& ie)
    {
        OnEventInner("", ev, ctxt, reqNode, resNode, hook);
    }
    catch (const JxtInterfaceExceptions::no_global_event&)
    {
        ProgError(STDLOG, "event not found");
        throw;
    }
}

void JxtInterfaceMng::InitInterfaces()
{
	ProgTrace(TRACE3, "JxtInterfaceMng::InitInterfaces");

    jxtlib::JXTLib::Instance()->GetCallbacks()->CreateGlobalInterface();
//	JxtGlobalInterface *gIface = new JxtGlobalInterface();
    if (!JxtInterfaceMng::Instance()->GetInterface(""))
        throw jxtlib::jxtlib_exception(STDLOG, "Не найден глобальный интерфейс");

	jxtlib::JXTLib::Instance()->GetCallbacks()->InitInterfaces();
}

bool JxtInterfaceMng::AlreadyLogged()
{
	return jxtlib::JXTLib::Instance()->GetCallbacks()->AlreadyLogged();
}
void JxtInterfaceMng::BlockPult()
{
    return jxtlib::JXTLib::Instance()->GetCallbacks()->BlockPult();
}

void JxtInterfaceMng::Force2Register(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	return jxtlib::JXTLib::Instance()->GetCallbacks()->Force2Register();
}

void JxtInterface::AddEvent(const std::string & ev, Handler * bh)
{
    AddEvent(JxtEvent(ev), bh);
}

void JxtInterface::AddEvent(const JxtEvent & ev, Handler * bh)
{
    events[ev] = bh;
    LogTrace(TRACE3) << "AddEvent: " << ifaceName << "." << ev.event;
}

xmlNodePtr JxtInterface::getDataNode(xmlNodePtr resNode) const
{
    return getNode( resNode, "data" );
}

xmlNodePtr JxtInterface::getPropNode(xmlNodePtr resNode) const
{
    return getNode( resNode, "properties" );
}        

void JxtInterface::OpenNewWindow(xmlNodePtr resNode) const
{
    JxtHandles::createNewJxtHandle();
    OpenWindow(resNode);
}

void JxtInterface::OpenWindow(xmlNodePtr resNode) const
{
    if(!JxtHandles::getCurrJxtHandle())
    {
        JxtHandles::createNewJxtHandle();
    }
    iface(resNode,GetIfaceName());
    setElemProp(resNode,"win","icon", Icon());
}

void JxtInterface::CloseWindow(xmlNodePtr resNode) const
{
    closeWindow(resNode);
}

void JxtInterface::OpenNewModalWindow(xmlNodePtr resNode) const
{
    OpenNewWindow(resNode);
    markCurrWindowAsModal(resNode);
}

void JxtInterface::findHandleByIface() const
{
    getHandleByIface( GetIfaceName().c_str() );
}

void JxtInterface::Display(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    getHandleByIface(GetIfaceName().c_str());
    OpenWindow(resNode);
}

xmlNodePtr JxtInterface::createDataNode(xmlNodePtr resNode)
{
    return xmlNewChild(resNode, NULL, reinterpret_cast<const xmlChar*>("data"), NULL);
}

void JxtInterface::OpenModalWindow(xmlNodePtr resNode) const
{
    OpenWindow(resNode);
    markCurrWindowAsModal(resNode);
}

void JxtInterface::setWindowTitle(xmlNodePtr resNode, const std::string& title) const
{
    setWindowTitle(resNode, title.c_str());
}

void JxtInterface::setWindowTitle(xmlNodePtr resNode, const char* title) const
{
    setElemProp(resNode,"win","title", title);
}

const JxtEvent &JxtInterface::findEvent(const std::string& evStr) const
{
    map<JxtEvent, Handler*>::const_iterator it = events.find(JxtEvent(evStr));
    if (it == events.end())
    {
        if (GetIfaceName() == "")
        {
            throw JxtInterfaceExceptions::no_global_event();
        }
        else
        {
	    throw JxtInterfaceExceptions::interface_exception("Event not found here", GetIfaceName().c_str(), evStr.c_str());
        }
    }
    return it->first;
}

void JxtInterfaceMng::setHook(const JxtEvent& ev)
{
    registerHookBefore(ev.blockPult?XmlCtxtHook:XmlCtxtHookNoWrite);
}

