#pragma once

enum OciCppErrs
{
    CERR_OK=     0,     /* 2*/
    CERR_NULL=   1405,  /* 2*/
    CERR_TRUNC=  1406, /*  3*/
    CERR_NODAT=  1403, /* 4*/
    CERR_SELL=   1007, /*-303 */
    CERR_DUPK=   1,   /* -9*/
    CERR_BIND=   1006, /*6005*/
    CERR_EXACT=  1422,
    CERR_BUSY= 54,
    CERR_DEADLOCK= 60,
    CERR_U_CONSTRAINT= 1 ,
    CERR_I_CONSTRAINT= 2291,
    CERR_INVALID_IDENTIFIER = 904, // ORA-00904: "VERSION": invalid identifier
    CERR_TABLE_NOT_EXISTS=942,  // ORA-00942: table or view does not exist
    CERR_SNAPSHOT_TOO_OLD=1555, // ORA-01555: snapshot too old
    CERR_TOO_MANY_ROWS=2112,    // ORA-02112: PCC: SELECT..INTO returns too many rows
    CERR_INVALID_NUMBER=1722,   // ORA-1722: invalid number
    CERR_NOT_CONNECTED = 3114,
    CERR_NOT_LOGGED_ON = 1012,
    NO_DATA_FOUND=1403,
    REQUIRED_NEXT_PIECE= 3129,
    REQUIRED_NEXT_BUFFER= 3130,
    WAIT_TIMEOUT_EXPIRED = 30006,
};
