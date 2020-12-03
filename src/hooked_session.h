#pragma once

#include <serverlib/logger.h>


namespace DbCpp {
    class Session;
}

/////////////////////////////////////////////////////////////////////////////////////////

DbCpp::Session* get_main_ora_rw_sess(STDLOG_SIGNATURE);

DbCpp::Session* get_main_pg_ro_sess(STDLOG_SIGNATURE);
DbCpp::Session* get_main_pg_rw_sess(STDLOG_SIGNATURE);
DbCpp::Session* get_main_pg_au_sess(STDLOG_SIGNATURE);

//---------------------------------------------------------------------------------------

DbCpp::Session* get_arx_ora_rw_sess(STDLOG_SIGNATURE);

DbCpp::Session* get_arx_pg_ro_sess(STDLOG_SIGNATURE);
DbCpp::Session* get_arx_pg_rw_sess(STDLOG_SIGNATURE);
