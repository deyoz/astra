#include "recloc.h"

#include <serverlib/cursctl.h>
#include <serverlib/pg_cursctl.h>
#include "pg_session.h"

#include <string>
#include <math.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Recloc {

static const char rltab[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const unsigned int rtablen = sizeof(rltab)-1;
static const unsigned int ReclocLength = 6;

typedef unsigned long int pnr_seq_t;

inline std::string GenerateRecloc_(pnr_seq_t seq)
{
    std::string recloc;
    recloc.reserve(ReclocLength);
    for(int i = ReclocLength - 1; i>=0; i--)
    {
        recloc.push_back(rltab[static_cast<unsigned int>(seq/pow(rtablen,i))]);
        seq=seq%static_cast<pnr_seq_t>(pow(rtablen,i));
    }
    LogTrace(TRACE1) << "New recloc was generated: " << recloc;

    return recloc;
}

std::string GenerateRecloc()
{
    pnr_seq_t seq;
    PgCpp::CursCtl pnrCur = get_pg_curs("select nextval('RL_SEQ')");
    pnrCur.def(seq).EXfet();
    return GenerateRecloc_(seq);
}

}//namespace Recloc
