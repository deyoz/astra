#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "iatci_settings.h"
#include "astra_msg.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "PgOraConfig.h"

#include <serverlib/testmode.h>
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

using namespace Ticketing;

IatciSettings::IatciSettings(const Ticketing::SystemAddrs_t& dcsId,
                             bool cki, bool ckx, bool cku,
                             bool bpr, bool plf, bool smf,
                             bool ifm)
    : DcsId(dcsId),
      Cki(cki), Ckx(ckx), Cku(cku),
      Bpr(bpr), Plf(plf), Smf(smf),
      Ifm(ifm)
{}

IatciSettings IatciSettings::defaultSettings(const Ticketing::SystemAddrs_t& dcsId)
{
    bool ifm = false;
#ifdef XP_TESTING
    if(inTestMode()) {
        LogWarning(STDLOG) << "Force enable IFM support in testmode";
        ifm = true;
    }
#endif/*XP_TESTING*/
    return IatciSettings(dcsId,
                         true /*cki*/,
                         true /*ckx*/,
                         true /*cku*/,
                         true /*bpr*/,
                         false/*plf*/,
                         true /*smf*/,
                         ifm);
}

//---------------------------------------------------------------------------------------

static void addIatciSettings(const IatciSettings& sett)
{
    LogTrace(TRACE3) << "add iatci settings for dcs_id: " << sett.DcsId;
    LogTrace(TRACE3) << "ifm:" << sett.Ifm;
    auto cur = make_db_curs(
"insert into IATCI_SETTINGS(DCS_ID, CKI, CKX, CKU, BPR, PLF, SMF, IFM) "
"values (:dcs_id, :cki, :ckx, :cku, :bpr, :plf, :smf, :ifm)",
                PgOra::getRWSession("IATCI_SETTINGS"));
    cur
            .stb()
            .bind(":dcs_id", sett.DcsId.get())
            .bind(":cki",    sett.Cki?1:0)
            .bind(":ckx",    sett.Ckx?1:0)
            .bind(":cku",    sett.Cku?1:0)
            .bind(":bpr",    sett.Bpr?1:0)
            .bind(":plf",    sett.Plf?1:0)
            .bind(":smf",    sett.Smf?1:0)
            .bind(":ifm",    sett.Ifm?1:0)
            .exec();
}

static void updateIatciSettings(const IatciSettings& sett)
{
    LogTrace(TRACE3) << "update iatci settings for dcs_id: " << sett.DcsId;
    auto cur = make_db_curs(
"update IATCI_SETTINGS set "
"CKI=:cki, CKX=:ckx, CKU=:cku, :BPR=:bpr, PLF=:plf, SMF=:smf, IFM=:ifm "
"where DCS_ID=:dcs_id",
                PgOra::getRWSession("IATCI_SETTINGS"));
    cur
            .stb()
            .bind(":dcs_id", sett.DcsId.get())
            .bind(":cki",    sett.Cki?1:0)
            .bind(":ckx",    sett.Ckx?1:0)
            .bind(":cku",    sett.Cku?1:0)
            .bind(":bpr",    sett.Bpr?1:0)
            .bind(":plf",    sett.Plf?1:0)
            .bind(":smf",    sett.Smf?1:0)
            .bind(":ifm",    sett.Ifm?1:0)
            .exec();
}

IatciSettings readIatciSettings(const Ticketing::SystemAddrs_t& dcsId, bool createDefault)
{ 
    auto cur = make_db_curs(
"select CKI, CKX, CKU, BPR, PLF, SMF, IFM "
"from IATCI_SETTINGS "
"where DCS_ID = :dcs_id",
                PgOra::getROSession("IATCI_SETTINGS"));

    IatciSettings sett = IatciSettings::defaultSettings(dcsId);
    sett.DcsId = dcsId;
    cur.stb()
       .bind(":dcs_id", dcsId.get())
       .def(sett.Cki)
       .def(sett.Ckx)
       .def(sett.Cku)
       .def(sett.Bpr)
       .def(sett.Plf)
       .def(sett.Smf)
       .def(sett.Ifm)
       .EXfet();

    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
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
    auto cur = make_db_curs(
"select 1 from IATCI_SETTINGS where DCS_ID = :dcs_id",
                PgOra::getROSession("IATCI_SETTINGS"));

    cur
            .bind(":dcs_id", dcsId.get())
            .exfet();

    return (cur.err() != DbCpp::ResultCode::NoDataFound);
}

}//namespace iatci
