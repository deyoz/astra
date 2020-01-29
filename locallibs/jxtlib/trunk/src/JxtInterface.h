#ifndef JXTINTERFACE_H_
#define JXTINTERFACE_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <libxml/tree.h>

#include "jxt_xml_cont.h"
#include "jxt_sys_reqs.h"
#include "jxt_handle.h"
#include "xml_tools.h"


/* --------------------------  */
/*           JxtEvent          */
/* --------------------------  */
struct JxtEvent
{
	std::string event;
	bool blockPult;
	bool withReg;
	explicit JxtEvent(const std::string &ev) : event(ev), blockPult(true), withReg(true)
	{}
	JxtEvent(const std::string &ev, bool bp, bool reg = true) : event(ev), blockPult(bp), withReg(reg)
	{}
	bool operator < (const JxtEvent &ev) const
	{
		return event < ev.event;
	}
	bool operator < (const std::string &ev) const
	{
		return event < ev;
	}
	bool operator == (const JxtEvent &ev) const
	{
		return event == ev.event;
	}
	bool operator == (const std::string &ev) const
	{
		return event == ev;
	}
};

class BaseIface;
class JxtInterface;

#include "Accessible.h"
/* --------------------------  */
/*           Handler           */
/* --------------------------  */
class Handler
    : public IAccessible
{
public:
    Handler(Accessible* acc = NULL)
        : IAccessible(acc)
    {}
	virtual ~Handler()
	{}
    void call(JxtInterface *bi, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	virtual void run(JxtInterface *bi, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) = 0;
};


/* --------------------------  */
/*        JxtInterface         */
/* --------------------------  */
class JxtInterface
    : public IAccessible
{
public:
	JxtInterface(const std::string &nm_, const std::string &iface_, Accessible* acc = NULL);
	virtual ~JxtInterface()
	{}
	void OnEvent(const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
	{	call(ev, ctxt, reqNode, resNode);	}

    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual xmlNodePtr createDataNode(xmlNodePtr resNode);

    const std::string &GetName() const
	{	return name;	}
	const std::string &GetIfaceName() const
	{	return ifaceName;	}

	void AddEvent(const JxtEvent &ev, Handler *bh);
	void AddEvent(const std::string &ev, Handler *bh);

    virtual void OpenNewWindow(xmlNodePtr resNode) const;
    virtual void OpenNewModalWindow(xmlNodePtr resNode) const;
    virtual void OpenWindow(xmlNodePtr resNode) const;
    virtual void OpenModalWindow(xmlNodePtr resNode) const;
    virtual void CloseWindow(xmlNodePtr resNode) const;
    virtual void setWindowTitle(xmlNodePtr resNode, const std::string &title) const;
    virtual void setWindowTitle(xmlNodePtr resNode, const char *title) const;
    virtual xmlNodePtr getDataNode(xmlNodePtr resNode) const;
    virtual xmlNodePtr getPropNode(xmlNodePtr resNode) const;
    virtual void findHandleByIface() const;
    virtual const char* Icon() const;
protected:
    friend class JxtInterfaceMng;
    const JxtEvent &findEvent(const std::string &evStr) const;
private:
	std::string name;
	std::string ifaceName;
	std::map<JxtEvent, Handler*> events;
    boost::shared_ptr<Accessible> m_acc;
    //xmlNodePtr ResNode;
    //xmlNodePtr ReqNode;

	void call(const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

/* --------------------------  */
/*         JxtHandler          */
/* --------------------------  */
template<class T>
class JxtHandler : public Handler
{
public:
	typedef void (T::* xmlRequestProc)(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	virtual void run(JxtInterface *bi, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
	{
		(((T*)bi)->*req)(ctxt, reqNode, resNode);
	}
	static Handler *CreateHandler(xmlRequestProc rq_, Accessible* acc = NULL)
	{
		return new JxtHandler(rq_, acc);
	}
private:
	JxtHandler(xmlRequestProc rq_, Accessible* acc = NULL) 
        : Handler(acc), req(rq_)
	{}
	xmlRequestProc req;
};

/* --------------------------  */
/*         TxtHandler          */
/* --------------------------  */
template<class T>
class TxtHandler : public Handler
{
public:
	typedef void (T::* xmlRequestProc)(XMLRequestCtxt *ctxt);
	virtual void run(JxtInterface *bi, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
	{
		(((T*)bi)->*req)(ctxt);
	}
	static Handler *CreateHandler(xmlRequestProc rq_)
	{
		return new TxtHandler(rq_);
	}
private:
	TxtHandler(xmlRequestProc rq_) : req(rq_)
	{}
	xmlRequestProc req;
};

/* --------------------------  */
/*       JxtInterfaceMng       */
/* --------------------------  */
class JxtInterfaceMng
{
public:
	typedef std::map<std::string, JxtInterface*> IfaceMap;
	typedef IfaceMap::iterator IfaceMapIterator;

	static JxtInterfaceMng *Instance();

	void AddInterface(JxtInterface *iface);
	JxtInterface *GetInterface(const std::string &iface);
	void OnEvent(const std::string &iface, const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void OnEventWithHook(const std::string &iface, const std::string &ev, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	void InitInterfaces();
	void Force2Register(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
	bool AlreadyLogged();
	void BlockPult();
#ifndef TEST_150
private:
#endif
	IfaceMap interfaces;

	JxtInterfaceMng()
	{}
    void setHook(const JxtEvent &ev);
    void OnEventInner(const std::string &iface, const std::string &ev, XMLRequestCtxt *ctxt,
                      xmlNodePtr reqNode, xmlNodePtr resNode, bool hook);
};

/**
 * JXT_HANDLER is used to create callback function haldler
 * example:
 *      Handler *eventHandler = JXT_HANDLER(ClassName, ClassMethodName);
 */
#define JXT_HANDLER(x, y) JxtHandler<x>::CreateHandler(&x::y)
/**
 * JXT_HANDLER2 is used to create callback function haldler with checking access
 * example:
 *      Handler *eventHandler = JXT_HANDLER2(ClassName, ClassMethodName, new SomeAccessible());
 */
#define JXT_HANDLER2(x, y, z) JxtHandler<x>::CreateHandler(&x::y, z)


#endif /*JXTINTERFACE_H_*/

