create or replace package body LIBRA
as

function DEFERR_ASTRA_CALL(XML_IN in clob) return DEFERRED_ASTRA_CALLS.ID%type is
    CALL_ID DEFERRED_ASTRA_CALLS.ID%type;
    
begin
    select DEFERRED_ASTRA_CALLS__SEQ.nextval into CALL_ID from dual;
    
    insert into DEFERRED_ASTRA_CALLS(ID, XML_REQ, TIME_CREATE, STATUS, SOURCE)
    values (CALL_ID, XML_IN, SYSTEM.UTCSYSDATE, 1, 'LIBRA');
    
    return CALL_ID;
end DEFERR_ASTRA_CALL;


procedure DEFERR_ASTRA_CALL2(XML_IN in clob) is
    CALL_ID DEFERRED_ASTRA_CALLS.ID%type;
    
begin    
    CALL_ID := DEFERR_ASTRA_CALL(XML_IN);
end DEFERR_ASTRA_CALL2;


procedure WRITE_FLIGHT_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                               SCREEN_IN in EVENTS_BILINGUAL.SCREEN%type,
                               USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                               USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                               MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                               MSG_EN_IN in EVENTS_BILINGUAL.MSG%type) is
    LOG_TIME     EVENTS_BILINGUAL.TIME%type;
    LOG_EV_ORDER EVENTS_BILINGUAL.EV_ORDER%type;
    
begin
    select SYSTEM.UTCSYSDATE, EVENTS__SEQ.nextval INTO LOG_TIME, LOG_EV_ORDER FROM dual;
    
    insert into EVENTS_BILINGUAL(TYPE, SUB_TYPE, TIME, EV_ORDER, PART_NUM, SCREEN, EV_USER, STATION, ID1, ID2, ID3, MSG, LANG)
    values ('êÖâ', null, LOG_TIME, LOG_EV_ORDER, 1, SCREEN_IN, USER_DESCR_IN, USER_DESK_IN, POINT_ID_IN, null, null, MSG_RU_IN, 'RU');
    
    insert into EVENTS_BILINGUAL(TYPE, SUB_TYPE, TIME, EV_ORDER, PART_NUM, SCREEN, EV_USER, STATION, ID1, ID2, ID3, MSG, LANG)
    values ('êÖâ', null, LOG_TIME, LOG_EV_ORDER, 1, SCREEN_IN, USER_DESCR_IN, USER_DESK_IN, POINT_ID_IN, null, null, MSG_EN_IN, 'EN');
end WRITE_FLIGHT_LOG_MSG;


procedure WRITE_AHM_FLIGHT_LOG_MSG2(POINT_ID_IN in POINTS.POINT_ID%type,
                                    USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                    USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                    MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                                    MSG_EN_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_FLIGHT_LOG_MSG(POINT_ID_IN, 'LIBRA_AHM.EXE', USER_DESCR_IN, USER_DESK_IN, MSG_RU_IN, MSG_EN_IN);    
end WRITE_AHM_FLIGHT_LOG_MSG2;


procedure WRITE_BAL_FLIGHT_LOG_MSG2(POINT_ID_IN in POINTS.POINT_ID%type,
                                    USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                    USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                    MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                                    MSG_EN_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_FLIGHT_LOG_MSG(POINT_ID_IN, 'LIBRA_BAL.EXE', USER_DESCR_IN, USER_DESK_IN, MSG_RU_IN, MSG_EN_IN);    
end WRITE_BAL_FLIGHT_LOG_MSG2;


procedure WRITE_AHM_FLIGHT_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                                   USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                   USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                   MSG_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_AHM_FLIGHT_LOG_MSG2(POINT_ID_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_AHM_FLIGHT_LOG_MSG;
                                   

procedure WRITE_BAL_FLIGHT_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                                   USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                   USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                   MSG_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_BAL_FLIGHT_LOG_MSG2(POINT_ID_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_BAL_FLIGHT_LOG_MSG;

end LIBRA;
/
