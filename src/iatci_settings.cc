#include "iatci_settings.h"
#include "astra_msg.h"
#include "exceptions.h"
#include <serverlib/cursctl.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

using namespace Ticketing;

IatciSettings::IatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                             bool cki, bool ckx, bool cku,
                             bool bpr, bool plf, bool smf)
    : DcsId(dcsId),
      Cki(cki), Ckx(ckx), Cku(cku),
      Bpr(bpr), Plf(plf), Smf(smf)
{}

IatciSettings IatciSettings::defaultSettings(const Ticketing::SystemAddrs_t& dcsId)
{
    return IatciSettings(dcsId,
                         true/*cki*/, true/*ckx*/, true/*cku*/,
                         true/*bpr*/, true/*plf*/, true/*smf*/);
}

//---------------------------------------------------------------------------------------

static void addIatciSettings(const IatciSettings& sett)
{
    LogTrace(TRACE3) << "add iatci settings for dcs_id: " << sett.DcsId;
    make_curs(
"insert into IATCI_SETTINGS(DCS_ID, CKI, CKX, CKU, BPR, PLF, SMF) "
"values (:dcs_id, :cki, :ckx, :cku, :bpr, :plf, :smf)")
       .bind(":dcs_id", sett.DcsId.get())
       .bind(":cki",    sett.Cki)
       .bind(":ckx",    sett.Ckx)
       .bind(":cku",    sett.Cku)
       .bind(":bpr",    sett.Bpr)
       .bind(":plf",    sett.Plf)
       .bind(":smf",    sett.Smf)
       .exec();
}

static void updateIatciSettings(const IatciSettings& sett)
{
    LogTrace(TRACE3) << "update iatci settings for dcs_id: " << sett.DcsId;
    make_curs(
"update IATCI_SETTINGS set "
"CKI = :cki, CKX = :ckx, CKU = :cku, :BPR = :bpr, PLF = :plf, SMF = :smf "
"where DCS_ID = :dcs_id")
      .bind(":dcs_id", sett.DcsId.get())
      .bind(":cki",    sett.Cki)
      .bind(":ckx",    sett.Ckx)
      .bind(":cku",    sett.Cku)
      .bind(":bpr",    sett.Bpr)
      .bind(":plf",    sett.Plf)
      .bind(":smf",    sett.Smf)
      .exec();
}

IatciSettings readIatciSettings(const Ticketing::SystemAddrs_t& dcsId, bool createDefault)
{
    OciCpp::CursCtl cur = make_curs(
"select DCS_ID, CKI, CKX, CKU, BPR, PLF, SMF "
"from IATCI_SETTINGS "
"where DCS_ID = :dcs_id");

    IatciSettings sett = IatciSettings::defaultSettings(dcsId);
    cur.stb()
       .bind(":dcs_id", dcsId.get())
       .def(sett.Cki)
       .def(sett.Ckx)
       .def(sett.Cku)
       .def(sett.Bpr)
       .def(sett.Plf)
       .def(sett.Smf)
       .EXfet();

    if(cur.err() == NO_DATA_FOUND) {
        LogTrace(TRACE0) << "Iatci settings not found for dcs: " << dcsId;
        if(createDefault) {
            sett = IatciSettings::defaultSettings(dcsId);
            addIatciSettings(sett);
        } else {
            throw EXCEPTIONS::Exception("Iatci settings not found!");
        }
    }

    return sett;
}

void writeIatciSettings(const IatciSettings& sett)
{
    if(!checkExistance(sett.DcsId)) {
        addIatciSettings(sett);
    } else {
        updateIatciSettings(sett);
    }
}

bool checkExistance(const Ticketing::SystemAddrs_t& dcsId)
{
    OciCpp::CursCtl cur = make_curs(
"select 1 from IATCI_SETTINGS "
"where DCS_ID = :dcs_id");

    cur.stb()
       .bind(":dcs_id", dcsId.get())
       .exfet();

    return (cur.err() != NO_DATA_FOUND);
}

}//namespace iatci
