create or replace package body LIBRA
as

procedure WRITE_BALANCE_LOG_MSG2(POINT_ID_IN in number,
                                 USER_DESCR_IN in varchar2,
                                 USER_DESK_IN in varchar2,
                                 MSG_RU_IN in varchar2,
                                 MSG_EN_IN in varchar2) is
begin
    insert into LIBRA_LOG_EVENTS(AIRLINE, BORT_NUM, CATEGORY, POINT_ID, EV_STATION, EV_USER, EV_ORDER, LOG_TIME, LOG_TYPE, RUS_MSG, LAT_MSG)
    values ('', '', '', POINT_ID_IN, USER_DESK_IN, USER_DESCR_IN, LIBRA_LOG_EVENTS__SEQ.nextval, CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE), 'BAL', MSG_RU_IN, MSG_EN_IN);
end WRITE_BALANCE_LOG_MSG2;


procedure WRITE_BALANCE_LOG_MSG(POINT_ID_IN in number,
                                USER_DESCR_IN in varchar2,
                                USER_DESK_IN in varchar2,
                                MSG_IN in varchar2) is
begin
    WRITE_BALANCE_LOG_MSG2(POINT_ID_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_BALANCE_LOG_MSG;


procedure WRITE_AHM_LOG_MSG2(AIRLINE_IN in varchar2,
                             CATEGORY_IN in varchar2,
                             BORT_NUM_IN in varchar2,
                             USER_DESCR_IN in varchar2,
                             USER_DESK_IN in varchar2,
                             MSG_RU_IN in varchar2,
                             MSG_EN_IN in varchar2) is
begin
    insert into LIBRA_LOG_EVENTS(AIRLINE, BORT_NUM, CATEGORY, POINT_ID, EV_STATION, EV_USER, EV_ORDER, LOG_TIME, LOG_TYPE, RUS_MSG, LAT_MSG)
    values (AIRLINE_IN, BORT_NUM_IN, CATEGORY_IN, '', USER_DESK_IN, USER_DESCR_IN, LIBRA_LOG_EVENTS__SEQ.nextval, CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE), 'AHM', MSG_RU_IN, MSG_EN_IN);
end WRITE_AHM_LOG_MSG2;


procedure WRITE_AHM_LOG_MSG(AIRLINE_IN in varchar2,
                            CATEGORY_IN in varchar2,
                            BORT_NUM_IN in varchar2,
                            USER_DESCR_IN in varchar2,
                            USER_DESK_IN in varchar2,
                            MSG_IN in varchar2) is
begin
    WRITE_AHM_LOG_MSG2(AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_AHM_LOG_MSG;

end LIBRA;
/
