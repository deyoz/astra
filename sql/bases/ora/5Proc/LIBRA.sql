create or replace package LIBRA
as

function DEFERR_ASTRA_CALL(XML_IN in clob) return DEFERRED_ASTRA_CALLS.ID%type;

procedure DEFERR_ASTRA_CALL2(XML_IN in clob);

procedure WRITE_AHM_FLIGHT_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                                   USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                   USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                   MSG_IN in EVENTS_BILINGUAL.MSG%type);

procedure WRITE_AHM_FLIGHT_LOG_MSG2(POINT_ID_IN in POINTS.POINT_ID%type,
                                    USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                    USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                    MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                                    MSG_EN_IN in EVENTS_BILINGUAL.MSG%type);                                
                                                                      
procedure WRITE_BAL_FLIGHT_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                                   USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                   USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                   MSG_IN in EVENTS_BILINGUAL.MSG%type);
                                   
procedure WRITE_BAL_FLIGHT_LOG_MSG2(POINT_ID_IN in POINTS.POINT_ID%type,
                                    USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                    USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                    MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                                    MSG_EN_IN in EVENTS_BILINGUAL.MSG%type);
                               

end LIBRA;
/
