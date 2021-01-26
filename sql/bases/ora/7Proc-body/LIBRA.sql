create or replace package body LIBRA
as

function GET_OR_GEN_AHM_DICT_ID(AIRLINE_IN in AIRLINES.ID%type,
                                CATEGORY_IN in varchar2,
                                BORT_NUM_IN in varchar2) return AHM_DICT.ID%type;

function GET_OR_GEN_AHM_DICT_ID_BY_A(AIRLINE_IN in AIRLINES.ID%type) return AHM_DICT.ID%type;
function GEN_AHM_DICT_ID_BY_A(AIRLINE_IN in AIRLINES.ID%type) return AHM_DICT.ID%type;

function GET_OR_GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN in AIRLINES.ID%type,
                                      CATEGORY_IN in varchar2) return AHM_DICT.ID%type;
function GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN in AIRLINES.ID%type,
                               CATEGORY_IN in varchar2) return AHM_DICT.ID%type;

function GET_OR_GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN in AIRLINES.ID%type,
                                      BORT_NUM_IN in varchar2) return AHM_DICT.ID%type;
function GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN in AIRLINES.ID%type,
                               BORT_NUM_IN in varchar2) return AHM_DICT.ID%type;

function GET_OR_GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN in AIRLINES.ID%type,
                                       CATEGORY_IN in varchar2,
                                       BORT_NUM_IN in varchar2) return AHM_DICT.ID%type;
function GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN in AIRLINES.ID%type,
                                CATEGORY_IN in varchar2,
                                BORT_NUM_IN in varchar2) return AHM_DICT.ID%type;


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


procedure WRITE_BAL_LOG_MSG_(POINT_ID_IN in POINTS.POINT_ID%type,
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
end WRITE_BAL_LOG_MSG_;


procedure WRITE_AHM_LOG_MSG_(AHM_DICT_ID_IN in AHM_DICT.ID%type,
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
    values ('Äïå', null, LOG_TIME, LOG_EV_ORDER, 1, SCREEN_IN, USER_DESCR_IN, USER_DESK_IN, AHM_DICT_ID_IN, null, null, MSG_RU_IN, 'RU');

    insert into EVENTS_BILINGUAL(TYPE, SUB_TYPE, TIME, EV_ORDER, PART_NUM, SCREEN, EV_USER, STATION, ID1, ID2, ID3, MSG, LANG)
    values ('Äïå', null, LOG_TIME, LOG_EV_ORDER, 1, SCREEN_IN, USER_DESCR_IN, USER_DESK_IN, AHM_DICT_ID_IN, null, null, MSG_EN_IN, 'EN');
end WRITE_AHM_LOG_MSG_;


procedure WRITE_BALANCE_LOG_MSG2(POINT_ID_IN in POINTS.POINT_ID%type,
                                 USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                 USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                 MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                                 MSG_EN_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_BAL_LOG_MSG_(POINT_ID_IN, 'LIBRA_BAL.EXE', USER_DESCR_IN, USER_DESK_IN, MSG_RU_IN, MSG_EN_IN);
end WRITE_BALANCE_LOG_MSG2;


procedure WRITE_BALANCE_LOG_MSG(POINT_ID_IN in POINTS.POINT_ID%type,
                                USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                                USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                                MSG_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_BALANCE_LOG_MSG2(POINT_ID_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_BALANCE_LOG_MSG;


procedure WRITE_AHM_LOG_MSG2(AIRLINE_IN in AIRLINES.ID%type,
                             CATEGORY_IN in varchar2,
                             BORT_NUM_IN in varchar2,
                             USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                             USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                             MSG_RU_IN in EVENTS_BILINGUAL.MSG%type,
                             MSG_EN_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_AHM_LOG_MSG_(GET_OR_GEN_AHM_DICT_ID(AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN), 'LIBRA_AHM.EXE', USER_DESCR_IN, USER_DESK_IN, MSG_RU_IN, MSG_EN_IN);
end WRITE_AHM_LOG_MSG2;


procedure WRITE_AHM_LOG_MSG(AIRLINE_IN in AIRLINES.ID%type,
                            CATEGORY_IN in varchar2,
                            BORT_NUM_IN in varchar2,
                            USER_DESCR_IN in EVENTS_BILINGUAL.EV_USER%type,
                            USER_DESK_IN in EVENTS_BILINGUAL.STATION%type,
                            MSG_IN in EVENTS_BILINGUAL.MSG%type) is
begin
    WRITE_AHM_LOG_MSG2(AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN, USER_DESCR_IN, USER_DESK_IN, MSG_IN, MSG_IN);
end WRITE_AHM_LOG_MSG;


function GET_OR_GEN_AHM_DICT_ID(AIRLINE_IN in AIRLINES.ID%type,
                                CATEGORY_IN in varchar2,
                                BORT_NUM_IN in varchar2) return AHM_DICT.ID%type is
begin
    if AIRLINE_IN is null then
        raise value_error;
    end if;

    if CATEGORY_IN is not null and BORT_NUM_IN is not null then
        return GET_OR_GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN);
    end if;

    if CATEGORY_IN is not null then
        return GET_OR_GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN, CATEGORY_IN);
    end if;

    if BORT_NUM_IN is not null then
        return GET_OR_GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN, BORT_NUM_IN);
    end if;

    return GET_OR_GEN_AHM_DICT_ID_BY_A(AIRLINE_IN);
end GET_OR_GEN_AHM_DICT_ID;


function GET_OR_GEN_AHM_DICT_ID_BY_A(AIRLINE_IN in AIRLINES.ID%type) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID into AHM_DICT_ID from AHM_DICT
    where AIRLINE=AIRLINE_IN and CATEGORY is null and BORT_NUM is null;

    return AHM_DICT_ID;
exception
    when no_data_found then
        return GEN_AHM_DICT_ID_BY_A(AIRLINE_IN);
end GET_OR_GEN_AHM_DICT_ID_BY_A;


function GEN_AHM_DICT_ID_BY_A(AIRLINE_IN in AIRLINES.ID%type) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID__SEQ.nextval into AHM_DICT_ID from dual;
    insert into AHM_DICT(ID, AIRLINE) values(AHM_DICT_ID, AIRLINE_IN);
    return AHM_DICT_ID;
end GEN_AHM_DICT_ID_BY_A;


function GET_OR_GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN in AIRLINES.ID%type,
                                      CATEGORY_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID into AHM_DICT_ID from AHM_DICT
    where AIRLINE=AIRLINE_IN and CATEGORY=CATEGORY_IN and BORT_NUM is null;

    return AHM_DICT_ID;
exception
    when no_data_found then
        return GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN, CATEGORY_IN);
end GET_OR_GEN_AHM_DICT_ID_BY_AC;


function GEN_AHM_DICT_ID_BY_AC(AIRLINE_IN in AIRLINES.ID%type,
                               CATEGORY_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID__SEQ.nextval into AHM_DICT_ID from dual;
    insert into AHM_DICT(ID, AIRLINE, CATEGORY) values(AHM_DICT_ID, AIRLINE_IN, CATEGORY_IN);
    return AHM_DICT_ID;
end GEN_AHM_DICT_ID_BY_AC;


function GET_OR_GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN in AIRLINES.ID%type,
                                      BORT_NUM_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID into AHM_DICT_ID from AHM_DICT
    where AIRLINE=AIRLINE_IN and BORT_NUM=BORT_NUM_IN and CATEGORY is null;

    return AHM_DICT_ID;
exception
    when no_data_found then
        return GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN, BORT_NUM_IN);
end GET_OR_GEN_AHM_DICT_ID_BY_AB;


function GEN_AHM_DICT_ID_BY_AB(AIRLINE_IN in AIRLINES.ID%type,
                               BORT_NUM_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID__SEQ.nextval into AHM_DICT_ID from dual;
    insert into AHM_DICT(ID, AIRLINE, BORT_NUM) values(AHM_DICT_ID, AIRLINE_IN, BORT_NUM_IN);
    return AHM_DICT_ID;
end GEN_AHM_DICT_ID_BY_AB;


function GET_OR_GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN in AIRLINES.ID%type,
                                       CATEGORY_IN in varchar2,
                                       BORT_NUM_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID into AHM_DICT_ID from AHM_DICT
    where AIRLINE=AIRLINE_IN and CATEGORY=CATEGORY_IN and BORT_NUM=BORT_NUM_IN;
    return AHM_DICT_ID;
exception
    when no_data_found then
        return GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN);
end GET_OR_GEN_AHM_DICT_ID_BY_ACB;


function GEN_AHM_DICT_ID_BY_ACB(AIRLINE_IN in AIRLINES.ID%type,
                                CATEGORY_IN in varchar2,
                                BORT_NUM_IN in varchar2) return AHM_DICT.ID%type is
    AHM_DICT_ID AHM_DICT.ID%type;
begin
    select ID__SEQ.nextval into AHM_DICT_ID from dual;
    insert into AHM_DICT(ID, AIRLINE, CATEGORY, BORT_NUM) values(AHM_DICT_ID, AIRLINE_IN, CATEGORY_IN, BORT_NUM_IN);
    return AHM_DICT_ID;
end GEN_AHM_DICT_ID_BY_ACB;

end LIBRA;
/
