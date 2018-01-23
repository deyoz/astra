#ifndef _HOTEL_ACMD_H_
#define _HOTEL_ACMD_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"
#include "astra_locale_adv.h"
#include "oralib.h"

struct TPaxListItem {
    int pax_id;
    int reg_no;
    ASTRA::TPerson pers_type;
    std::string full_name;
    TPaxListItem():
        pax_id(ASTRA::NoExists),
        reg_no(ASTRA::NoExists),
        pers_type(ASTRA::NoPerson)
    {}
};

struct TPaxList {
    std::map<int, TPaxListItem> items;
    void fromDB(int point_id);
};

struct THotelAcmdPaxItem {
    int idx;
    int point_id;
    int pax_id;
    std::string full_name;
    ASTRA::TPerson pers_type;
    int hotel_id;
    int room_type;
    int breakfast;
    int dinner;
    int supper;
    THotelAcmdPaxItem():
        idx(ASTRA::NoExists),
        pax_id(ASTRA::NoExists),
        pers_type(ASTRA::NoPerson),
        hotel_id(ASTRA::NoExists),
        room_type(ASTRA::NoExists),
        breakfast(ASTRA::NoExists),
        dinner(ASTRA::NoExists),
        supper(ASTRA::NoExists)
    {}
};

struct THotelAcmdPax
{
    private:
        int GetVar(TQuery &Qry, int idx);
        void SetVar(TQuery &Qry, const std::string &name, int val);
        void SetBoolLogParam(LEvntPrms &params, const std::string &name, int val);
    public:
        int point_id;
        THotelAcmdPax(): point_id(ASTRA::NoExists) {}
        std::map<int, THotelAcmdPaxItem> items;
        void fromDB(int point_id);
        void fromXML(xmlNodePtr reqNode);
        void toDB(std::list<std::pair<int, int> > &inserted_paxes);
};

class HotelAcmdInterface : public JxtInterface
{
    public:
        HotelAcmdInterface() : JxtInterface("","hotel_acmd")
    {
        Handler *evHandle;
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::Save);
        AddEvent("Save",evHandle);
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::View);
        AddEvent("View",evHandle);
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::HotelAcmdClaim);
        AddEvent("HotelAcmdClaim",evHandle);
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::Print);
        AddEvent("Print",evHandle);
    }

        void View(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void HotelAcmdClaim(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void Print(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
