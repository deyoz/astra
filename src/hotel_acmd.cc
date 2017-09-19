#include "hotel_acmd.h"
#include "tripinfo.h"
#include "brd.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;

void HotelAcmdInterface::ViewHotelPaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "point_id", reqNode );
    ProgTrace(TRACE5, "HotelAcmdInterface::ViewHotelPaxList, point_id=%d", point_id );
    //TReqInfo::Instance()->user.check_access( amRead );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    viewCRSList( point_id, dataNode );

    LogTrace(TRACE5) << "after viewCRSList: " << GetXMLDocText(resNode->doc);

    TQuery Qry(&OraSession);
    BrdInterface::GetPaxQuery(Qry, point_id, NoExists, NoExists, TReqInfo::Instance()->desk.lang, rtNOREC, "", stRegNo);
    Qry.Execute();
    if(not Qry.Eof) {
        LogTrace(TRACE5) << "norecs found";
        xmlNodePtr passengersNode = GetNode("tlg_trips/tlg_trip/passengers", dataNode);
        if(not passengersNode) {
            passengersNode = NewTextChild(NewTextChild(NodeAsNode("tlg_trips", dataNode), "tlg_trip"), "passengers");
        }
        for(; not Qry.Eof; Qry.Next()) {
            CheckIn::TSimplePaxItem pax;
            pax.fromDB(Qry);
            ostringstream buf;
            buf
                << transliter(pax.surname, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU) << " "
                << transliter(pax.name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
            xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
            NewTextChild(paxNode, "full_name", buf.str());
            NewTextChild(paxNode, "reg_no", pax.reg_no);
            NewTextChild(paxNode, "pax_id", pax.id);
            NewTextChild(paxNode, "pers_type", ElemIdToElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative, TReqInfo::Instance()->desk.lang));
        }
    }

    xmlNodePtr defaultsNode = GetNode("defaults", dataNode);
    if(not defaultsNode) defaultsNode = NewTextChild(dataNode, "defaults");

    NewTextChild(defaultsNode, "hotel_name", "‘®Ά¥βαª ο");
    NewTextChild(defaultsNode, "room_type", "single");
    NewTextChild(defaultsNode, "breakfast", 0);
    NewTextChild(defaultsNode, "dinner", 0);
    NewTextChild(defaultsNode, "supper", 0);
    NewTextChild(defaultsNode, "acmd", 0);

    LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}
