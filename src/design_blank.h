#ifndef _DESIGN_BLANK_H
#define _DESIGN_BLANK_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class DesignBlankInterface: public JxtInterface
{
    public:
        DesignBlankInterface(): JxtInterface("123", "design_blank")
        {
            Handler *evHandle;
            evHandle=JxtHandler<DesignBlankInterface>::CreateHandler(&DesignBlankInterface::GetBlanksList);
            AddEvent("GetBlanksList",evHandle);
            evHandle=JxtHandler<DesignBlankInterface>::CreateHandler(&DesignBlankInterface::Save);
            AddEvent("Save",evHandle);
            evHandle=JxtHandler<DesignBlankInterface>::CreateHandler(&DesignBlankInterface::PrevNext);
            AddEvent("PrevNext",evHandle);
        }

        void PrevNext(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetBlanksList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
